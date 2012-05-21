((function () {
var SERVER_URL = "ws://127.0.0.1:8080";
var RECONNECT_INTERVAL = 2000;
var predicates = null;
var mapnodes = {};
var graph = new Graph();
var sock = null;

function disable_controls()
{
	$('#button-next').attr('disabled', 'disabled');
}

function activate_controls()
{
	$('#button-next').removeAttr('disabled');
}

function clear_everything()
{
	var canvas = document.getElementById('graph');
	var context = canvas.getContext('2d');

	disable_controls();
	graph.reset();
	$('#log').text('');
	$('#database').text('');
}

function tuple_to_string(tpl)
{
	var pred_id = tpl.predicate;
	var pred = predicates[pred_id];
	var ret = pred.name + "(";

	for(var i = 0; i < pred.fields.length; ++i) {
		var type = pred.fields[i];

		if(type == "int") {
			ret = ret + tpl.fields[i].toString();
		} else if (type == "float") {
			ret = ret + tpl.fields[i].toString();
		} else if (type == "node") {
			ret = ret + tpl.fields[i].toString();
		} else {
			ret = ret + "PROBLEM";
		}

		if(i < pred.fields.length - 1) {
			ret = ret + ", ";
		}
	}

	ret = ret + ")";

	return ret;
}

function init_connection ()
{
	sock = new WebSocket(SERVER_URL);

	sock.onopen = function () { }
	sock.onclose = function () {
		$('#content').text("Server " + SERVER_URL + " disconnected.");
		clear_everything();
		setTimeout(function () { init_connection(); }, RECONNECT_INTERVAL);
		return true;
	};
	sock.onmessage = function (input) {
		var msg = $.evalJSON(input.data);
		var typ = msg.msg;

		if(typ == "init") {
			var running = msg.running;

			if(running == null) {
				$('#content').text("no program running at the moment");
			} else {
				$('#content').text("running " + running);
			}
		} else if(typ == "program_running") {
			var running = msg.running;

			$('#content').text("running " + running);
			activate_controls();
		} else if(typ == "program_stopped") {
			$('#content').text("no program running at the moment");
			disable_controls();
		} else if(typ == "database") {
			var db = msg.database;
			var nodes = db.nodes;

			graph.reset();

			for(var i = 0; i < nodes.length; i++) {
				var node = nodes[i];
				var id = node.id;
				var translated_id = node.translated_id;
				mapnodes[id] = graph.newNode({label: translated_id});
			}

			$('#database').text("number of nodes: " + msg.database.num_nodes);
		} else if(typ == 'program') {
			var program = msg.program;

			predicates = msg.program;
		} else if(typ == 'persistent_derivation') {
			var node = msg.node;
			var tpl = msg.tuple;
			var predid = tpl.predicate;
			var pred = predicates[predid];

			if(pred.route && !pred.reverse_route) {
				var dest = tpl.fields[0];
				var node_orig = mapnodes[node];
				var node_dest = mapnodes[dest];

				graph.newEdge(node_orig, node_dest, {color: '#00A0B0'});
			}

			var tuple_str = tuple_to_string(tpl);

			$('#content').text("Derived tuple " + tuple_str + " at node " + node);
		} else {
			alert('Received message of type ' + typ);
		}

		$('#version').text("Meld Virtual Machine " + msg.version);
	}
}

$(document).ready(function () {

	$('#graph').springy({ graph: graph });
	init_connection();
	disable_controls();

	$('#button-next').click(function () {
			var msg = {msg: 'next'};
			sock.send($.toJSON(msg));
			return false;
	});
});
})());
