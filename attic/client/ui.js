((function () {
var SERVER_URL = "ws://127.0.0.1:8081";
var RECONNECT_INTERVAL = 1000;
var MAIN_LOG_SIZE = 50;
var PLAY_INTERVAL = 200;

var message_handlers = {};
var predicates = null;
var rules = null;
var mapnodes = {};
var mapedges = {}; // maps strings of tuples to edges
// we need this in order to avoid doing an animation with many facts
var last_sent = {}; // timestamps of source -> destination of sent tuples
var graph = new Graph();
var sock = null; // connection socket to server
var node_events = {}; // all events in a per node basis
var selected_node = null; // currently selected node in the graph
var connection_retry_number = 0; // number of attempts to server in this round
var playing_program = false; // if the play button was clicked and the program is playing by itself

function get_translated_id(id)
{
	return mapnodes[id].data.translated_id;
}

function send_msg(msg)
{
	sock.send($.toJSON(msg));
}

function update_graph()
{
	$('#graph').springyUpdate();
}

function clear_node_info()
{
	$('#node-log').empty();
	$('#node-info').empty();
	$('#node-window').hide();
}

function do_add_event_to_list(list_name, txt, cls)
{
	var content = $('<li class="clear' + (cls == null ? '' : (' ' + cls)) + '">' + txt + '</li>').hide();
	
	return $(list_name).prepend(content).children(':first');
}

function add_event_to_list(list_name, txt, cls)
{
	return do_add_event_to_list(list_name, txt, cls);
}

function limited_add_event_to_list(list_name, limit, txt, cls)
{
	// list_name could be for example '#main-log'
	var size = $(list_name + ' li').size();
	
	if(size > limit)
		$(list_name + ' li:last').remove();
		
	return do_add_event_to_list(list_name, txt, cls).fadeIn(200);
}

function add_to_main_log(txt, cls)
{
	limited_add_event_to_list('#main-log', MAIN_LOG_SIZE, txt, cls);
}

function add_events_to_list(list_name, ls)
{
	if(ls !== null) {
		$.each(ls, function (i, item) {
			var txt = item.text;
			var cls = item.cls;
			add_event_to_list(list_name, txt, cls);
		});
	}
	
	$(list_name + ' li').show();
}

function new_node_event(node, txt, cls)
{
	// add to node events
	var olds = node_events[node];
	var obj = {text: txt, cls: cls};

	if(olds == null)
		node_events[node] = [obj];
	else
		olds.push(obj);
		
	if(selected_node == node && selected_node !== null)
		add_event_to_list('#node-log', txt, cls).show();
}

function new_event(node, txt, show)
{
	new_node_event(node, txt);
	var str = '<span class="log-left">' + txt + '</span>';
	
	if(show == true)
		str = str + '<span class="log-right node-link">(node ' + get_translated_id(node) + ')</span>';

	add_to_main_log(str);
}

function new_global_event(txt, cls)
{
	add_to_main_log(txt, cls);
}

function handle_init(msg)
{
	var running = msg.running;

	new_ruler();
	
	if(running == null)
		new_global_event("No program running at the moment");
	else {
		activate_controls();
		new_global_event("Running program " + running);
		show_next_log_message();
	}
}

function handle_program_running(msg)
{
	var running = msg.running;

	new_ruler();
	new_global_event("Running program " + running);
	activate_controls();
}

function handle_program_stopped(msg)
{
	new_ruler();
	new_global_event("Program has terminated");
	clear_everything();
}

function add_node_to_screen(id, translated_id)
{
   mapnodes[id] = graph.newNode({label: translated_id, color: 'red', id: id, translated_id: translated_id});
}

function handle_database(msg)
{
	var db = msg.database;
	var nodes = db.nodes;

	// reset data
	graph.reset();
	node_events = {};

	for(var i = 0; i < nodes.length; i++) {
		var node = nodes[i];
		var id = node.id;
		var translated_id = node.translated_id;
      add_node_to_screen(id, translated_id);
	}

	new_global_event("Database initialized: " + msg.database.num_nodes + " nodes");
	update_graph();
}

function handle_program(msg)
{
	var program = msg.program;

	predicates = msg.program;
   rules = msg.rules;
}

function is_routing_pred(pred)
{
   return pred.route && !pred.reverse_route;
}

function hash_route_fact(node, tpl)
{
   return node + ' ' + tuple_to_string(tpl);
}

function check_for_routing(from, pred, tpl)
{
   if(is_routing_pred(pred)) {
      var dest = tpl.fields[0];
      var node_orig = mapnodes[from];
      var node_dest = mapnodes[dest];

      var label_str;

      if(tpl.fields.length > 1)
         label_str = field_to_string(tpl.fields[1], pred.fields[1]);
      else
         label_str = '';

		var edge = graph.newEdge(node_orig, node_dest, {color: '#00A0B0', label: label_str});
      mapedges[hash_route_fact(from, tpl)] = edge;
		update_graph();
   }
}

function handle_persistent_derivation(msg)
{
	var node = msg.node;
	var tpl = msg.tuple;
	var predid = tpl.predicate;
	var pred = predicates[predid];

   check_for_routing(node, pred, tpl);

	var tuple_str = tuple_to_string(tpl);

	new_event(node, "&darr; " + tuple_str);
}

function handle_linear_derivation(msg)
{
	var node = msg.node;
	var tpl = msg.tuple;
	var predid = tpl.predicate;
	var pred = predicates[predid];

   check_for_routing(node, pred, tpl);
	
	new_event(node, "&darr; " + tuple_to_string(tpl));
}

function do_tuple_send_animation(hash, from, to, pred, tpl)
{
   last_sent[hash] = Math.round((new Date()).getTime());

   var str = '@' + get_translated_id(from) + " &rarr; @" + get_translated_id(to) + " " + tuple_to_string(tpl);
   check_for_routing(to, pred, tpl);
   new_event(from, str);
   new_node_event(to, str);
   $('#graph').message(mapnodes[from], mapnodes[to], tuple_to_string(tpl));
   update_graph();
}

function handle_tuple_send(msg)
{
	var from = msg.from;
	var to = msg.to;
	var tpl = msg.tuple;
	var tpl_str = tuple_to_string(tpl);
   var predid = tpl.predicate;
   var pred = predicates[predid];
	
	if(from == to)
		new_event(from, "&harr; " + tuple_to_string(tpl));
	else {
      var hash_last_sent = from + ' ' + to;
      var now = Math.round((new Date()).getTime());
      var last = last_sent[hash_last_sent];
      if(typeof(last) == 'undefined') {
         do_tuple_send_animation(hash_last_sent, from, to, pred, tpl);
      } else if(now > last + PLAY_INTERVAL) {
         do_tuple_send_animation(hash_last_sent, from, to, pred, tpl);
      } else {
         setTimeout(function () { do_tuple_send_animation(hash_last_sent, from, to, pred, tpl); },
               PLAY_INTERVAL);
      }
	}
}

function handle_linear_consumption(msg)
{
	var node = msg.node;
	var tpl = msg.tuple;
	var predid = tpl.predicate;
	var pred = predicates[predid];

   if(is_routing_pred(pred)) {
      var hash = hash_route_fact(node, tpl);
      var edge = mapedges[hash];
      delete mapedges[hash];
      
      graph.removeEdge(edge);
		update_graph();
   }
	
	new_event(node, "&uarr; " + tuple_to_string(tpl));
}

function handle_step_start(msg)
{
	var node = msg.node;
	
	new_event(node, "&crarr; node " + get_translated_id(node) + " ready to execute", true);
}

function new_ruler(node)
{
	var txt = '<div class="hr"><hr /></div>';
	
	new_global_event(txt, 'hr');
	
	if(node !== null)
		new_node_event(node, txt, 'hr');
}

function show_next_log_message()
{
	$('#log-message').text('Click Next to proceed');
	$('#log-message-parent').show();
}

function handle_step_done(msg)
{
	var node = msg.node;
	
	new_ruler(node);
	
	//if(node == selected_node && selected_node !== null)
	if(selected_node !== null)
		request_node(selected_node);
	if(playing_program)
		setTimeout(request_next, PLAY_INTERVAL);
	else
		show_next_log_message();
}

function handle_node(msg)
{
	var info = msg.info;
	var db = info.database;
	var queue = info.queue;
	var node = msg.node;
	var empty_html = '<p class="italic">Empty</p>';
	
	var txt = '<h1>Node ' + get_translated_id(node) + '</h1><p>Database:</p>';
	
	$('#node-name').text('Node ' + get_translated_id(node));
	
	var node_db = $('#node-database');
	var next_column = 1;
	var db_empty = true;
	
	node_db.empty();
	
	$.each(db, function(predicate, tuples) {
		$.each(tuples, function(i, tpl) {
			db_empty = false;
			node_db.append('<div class="node-database-column node-database-column' + next_column + '">' + tuple_to_string(tpl) + '</div>');
			if(next_column == 4) {
				node_db.append('<br />');
				next_column = 1;
			} else {
				next_column++;
			}
		});
	});
	
	if(db_empty)
		node_db.html(empty_html);
	
	// restore queue
	var arrow_html = '<p class="arrow">&uarr;</p>';
	var node_queue = $('#node-queue');
	node_queue.empty();
	
	node_queue.append('<p class="node-queue-item">HEAD</p>');
	
	$.each(queue, function(i, stpl) {
		var tpl = stpl.tuple;
		var html = arrow_html + '<p class="node-queue-item';
		
		if(stpl.to_delete) {
			html = html + ' node-queue-item-delete';
		}
		html = html + '">' + tuple_to_string(tpl) + '</p>'
		node_queue.append(html);
	});
	
	node_queue.append(arrow_html);
	node_queue.append('<p class="node-queue-item">TAIL</p>');
	
	// restore events
	var node_log = $('#node-log');
	
	node_log.empty();
	if(typeof(node_events[node]) !== 'undefined') {
		add_events_to_list('#node-log', node_events[node]);
	} else {
		$('#node-log').html(empty_html);
	}
	
	$('#node-controls').show();
	$('#node-window').show();
}

function handle_changed_node(msg)
{
	new_global_event('Changed to node ' + msg.node);
	new_ruler();
}

function handle_program_termination(msg)
{
	$('#log-message').text('Click Terminate to Finish');
	$('#log-message-parent').show();
	$('#button-terminate').removeAttr('disabled');
	$('#button-next, #button-play, #button-stop, #button-jump').attr('disabled', 'disabled');
}

function handle_set_color(msg)
{
	var node_id = msg.node;
	var r = msg.r;
	var g = msg.g;
	var b = msg.b;
	var node = mapnodes[node_id];
	
	node.data.color = 'rgb(' + r + ',' + g + ',' + b + ')';
	update_graph();
}

function handle_set_edge_label(msg)
{
	var fromid = msg.from;
	var toid = msg.to;
	
	var from = mapnodes[fromid];
	var to = mapnodes[toid];
	
	var label = msg.label;
	
	var edges = graph.getEdges(from, to);
	
	$.each(edges, function (i, edge) {
		edge.data.label = label;
	});
	
	update_graph();
}

function handle_rule_applied(msg)
{
   var who = msg.node;
   var rule = msg.rule;

	//new_global_event('<span class="rule" title="' + rules[rule] + '">rule applied</span>');
}

function handle_rule_start(msg)
{
   var who = msg.node;
   var rule = msg.rule;

	new_event(who, '&gt;&gt; trying rule <span class="rule" title="' + rules[rule] + '">' + rule + '</span>');
}

function handle_new_node(msg)
{
   var who = msg.node;
   var translated = msg.translated;

   add_node_to_screen(who, translated);

   new_event(who, 'Node ' + translated + ' created');
}

function setup_message_handlers()
{
	message_handlers['init'] = handle_init;
	message_handlers['program_running'] = handle_program_running;
	message_handlers['program_stopped'] = handle_program_stopped;
	message_handlers['database'] = handle_database;
	message_handlers['program'] = handle_program;
	message_handlers['persistent_derivation'] = handle_persistent_derivation;
	message_handlers['linear_derivation'] = handle_linear_derivation;
	message_handlers['tuple_send'] = handle_tuple_send;
	message_handlers['linear_consumption'] = handle_linear_consumption;
	message_handlers['step_start'] = handle_step_start;
	message_handlers['step_done'] = handle_step_done;
	message_handlers['node'] = handle_node;
	message_handlers['changed_node'] = handle_changed_node;
	message_handlers['program_termination'] = handle_program_termination;
	message_handlers['set_color'] = handle_set_color;
	message_handlers['set_edge_label'] = handle_set_edge_label;
   message_handlers['rule_applied'] = handle_rule_applied;
   message_handlers['rule_start'] = handle_rule_start;
   message_handlers['new_node'] = handle_new_node;
}

function disable_controls()
{
	$('#button-next, #button-play, #button-stop, #button-terminate, #button-jump').attr('disabled', 'disabled');
}

function activate_controls()
{
	$('#button-next, #button-play, #button-stop, #button-jump').removeAttr('disabled');
}

function clear_everything()
{
	disable_controls();
	graph.reset();
	update_graph();
	clear_node_info();
}

function request_node(id)
{
	send_msg({msg: 'get_node', node: id});
	selected_node = id;
}

function node_selection(node)
{
	request_node(node.id);
}

function set_node(id)
{
	send_msg({msg: 'change_node', node: selected_node});
}

function node_dblclick(node)
{
	set_node(node.id);
}

function list_to_string(ls)
{
	var ret = '[';

	$.each(ls, function (i, x) {
			if(ret == '[')
				ret = ret + x;
			else
				ret = ret + ', ' + x;
		});

	return ret + ']';
}

function field_to_string(field_data, type)
{
	if(type == 'int')
		return field_data.toString();
	else if(type == 'float')
		return field_data.toFixed(2).toString();
	else if(type == 'node')
		return '@' + get_translated_id(field_data);
	else if(type == 'string')
		return '"' + field_data + '"';
	else if(type == 'list int')
		return list_to_string(field_data);
	else
		return 'PROBLEM';
}

function tuple_to_string(tpl)
{
	var pred_id = tpl.predicate;
	var pred = predicates[pred_id];
	var ret = '';
	
	if(pred.linear == null)
		ret = '!';
	
	ret = ret + pred.name + "(";

	for(var i = 0; i < pred.fields.length; ++i) {
		ret = ret + field_to_string(tpl.fields[i], pred.fields[i]);

		if(i < pred.fields.length - 1) {
			ret = ret + ", ";
		}
	}

	ret = ret + ")";

	return ret;
}

function init_connection ()
{
	connection_retry_number = connection_retry_number + 1;
	sock = new WebSocket(SERVER_URL);

	sock.onopen = function () {
		connection_retry_number = 0;
		new_global_event("Connected to " + SERVER_URL);
		return true;
	}
	sock.onclose = function () {
		if(connection_retry_number == 1)
			new_global_event("Server " + SERVER_URL + " disconnected.");
		
		clear_everything();
		setTimeout(function () { init_connection(); }, RECONNECT_INTERVAL);
		return true;
	};
	sock.onmessage = function (input) {
		var msg = $.evalJSON(input.data);
		var typ = msg.msg;

		var fun = message_handlers[typ];
		
		if(fun == null) {
			alert("Don't know how to handle messages of type " + typ);
		} else {
			fun(msg);
		}

		$('#version').text("Meld Virtual Machine " + msg.version);
	}
}

function request_next()
{
	send_msg({msg: 'next'});
	$('#log-message-parent').hide();
}

function request_jump()
{
   var many = $('#input-jump').val();
   send_msg({msg: 'jump', steps: parseInt(many)});
	$('#log-message-parent').hide();
}

$(document).ready(function () {

	setup_message_handlers();
	$('#graph').springy({ graph: graph }, node_selection, node_dblclick);
	
	new_global_event("Attempting connection with server at " + SERVER_URL);
	init_connection();
	
	disable_controls();
	clear_node_info();
	$('#node-controls #log-message-parent').hide();
	
	// adjust width of side-log
	var graph_width = $('#graph').width();
	var wrap_width = $('#wrap').width();
	var side_width = wrap_width - graph_width - 150;
	
	$('#side-log').width(side_width);

	$('#button-next').click(function () {
		request_next();
		return false;
	});

   $('#button-jump').click(function () {
      request_jump();
      return false;
   });
	
	$('#button-terminate').click(function () {
		send_msg({msg: 'terminate'});
      $('#log-message-parent').hide();
		playing_program = false;
		return false;
	});
	
	$('#button-play').click(function () {
		$('#button-next, #button-play, #button-jump').attr('disabled', 'disabled');
		$('#button-stop').removeAttr('disabled');
		request_next();
		playing_program = true;
		return false;
	});
	
	$('#button-stop').click(function () {
		$('#button-next, #button-play, #button-jump').removeAttr('disabled');
		$('#button-stop').attr('disabled', 'disabled');
		playing_program = false;
		return false;
	});
});
})());
