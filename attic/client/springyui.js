/**
Copyright (c) 2010 Dennis Hotson
				  2012 Flavio Cruz

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
*/

(function() {

var NODE_FONT = "25px Verdana, sans-serif";

jQuery.fn.springyUpdate = function () {
	var canvas = this[0];
	canvas.renderer.start();
};

// animates a message 'content' from node 'from' to 'to'
jQuery.fn.message = function (from, to, content) {
	var canvas = this[0];
	var layout = canvas.layout;
	var graph = canvas.graph;
	var ctx = canvas.getContext("2d");
	var step = 20;
	var fulldistance = 100;
	var stepdivide = fulldistance - step;
	var speed = 0.75;
	
	canvas.renderer.addAnimation(
		function () {
			return step == stepdivide;
		},
		function () {
			var p1 = toScreen(layout.point(from).p);
			var p2 = toScreen(layout.point(to).p);
			var x1 = p1.x;
			var y1 = p1.y;
			var x2 = p2.x;
			var y2 = p2.y;
			
			var direction = new Vector(x2-x1, y2-y1);
			var angle = direction.angle();
			var diffx = x2 - x1;
			var diffy = y2 - y1;
			
			ctx.save();
			
			if(angle <= (Math.PI / 2.0) && angle >= -(Math.PI / 2.0))
				angle = -angle;
			else
				angle = -angle + Math.PI;
			
			ctx.translate(x1 + (diffx * step/fulldistance), y1 + (diffy * step / fulldistance));
			ctx.rotate(angle);
			
			// f(y) = 10 - 1/90 * y^2 
			var ytranslate = 10 - 1/90.0 * Math.pow(step - fulldistance/2.0, 2);
			
			ctx.translate(0, 2.5 * ytranslate);
			ctx.textAlign = "center";
			ctx.textBaseline = "top";
			ctx.font = "12px Helvetica, sans-serif";
			ctx.fillStyle = "black";
			ctx.fillText(content, 0, 0);
			
			ctx.restore();
			
			step = step + speed;
		}
	);
		
	return true;
};

jQuery.fn.springy = function(params, node_selected, node_dblclicked) {

	var stiffness = params.stiffness || 400.0;
	var repulsion = params.repulsion || 400.0;
	var damping = params.damping || 0.5;

	var canvas = this[0];
	var ctx = canvas.getContext("2d");

	canvas.renderer = null;
	canvas.graph = this.graph = params.graph;

	var layout = this.layout = new Layout.ForceDirected(canvas.graph, stiffness, repulsion, damping);
	
	canvas.layout = layout;

	// calculate bounding box of graph layout.. with ease-in
	var currentBB = layout.getBoundingBox();
	var targetBB = {bottomleft: new Vector(-2, -2), topright: new Vector(2, 2)};

	// auto adjusting bounding box
	Layout.requestAnimationFrame(function adjust() {
		targetBB = layout.getBoundingBox();
		// current gets 20% closer to target every iteration
		currentBB = {
			bottomleft: currentBB.bottomleft.add( targetBB.bottomleft.subtract(currentBB.bottomleft)
				.divide(10)),
			topright: currentBB.topright.add( targetBB.topright.subtract(currentBB.topright)
				.divide(10))
		};

		Layout.requestAnimationFrame(adjust);
	});

	// convert to/from screen coordinates
	toScreen = function(p) {
		var size = currentBB.topright.subtract(currentBB.bottomleft);
		var sx = p.subtract(currentBB.bottomleft).divide(size.x).x * canvas.width;
		var sy = p.subtract(currentBB.bottomleft).divide(size.y).y * canvas.height;
		return new Vector(sx, sy);
	};

	fromScreen = function(s) {
		var size = currentBB.topright.subtract(currentBB.bottomleft);
		var px = (s.x / canvas.width) * size.x + currentBB.bottomleft.x;
		var py = (s.y / canvas.height) * size.y + currentBB.bottomleft.y;
		return new Vector(px, py);
	};

	// half-assed drag and drop
	var selected = null;
	var nearest = null;
	var dragged = null;
	var moved = false;
	
	jQuery(canvas).click(function (e) {
		// click is called after mousemove if at all
		if(moved == true) {
			moved = false;
			return true;
		}
		
		var pos = jQuery(this).offset();
		var p = fromScreen({x: e.pageX - pos.left, y: e.pageY - pos.top});
		var sel = layout.nearest(p);

		if (sel.node !== null) {
			if(typeof(node_selected !== 'undefined'))
				node_selected(sel.node);
		}
		
		return true;
	});
	
	jQuery(canvas).dblclick(function (e) {
		if(moved == true) {
			moved = false;
			return true;
		}
		
		var pos = jQuery(this).offset();
		var p = fromScreen({x: e.pageX - pos.left, y: e.pageY - pos.top});
		var sel = layout.nearest(p);

		if (sel.node !== null) {
			if(typeof(node_dblclicked) !== 'undefined')
				node_dblclicked(sel.node);
		}
		
		return true;
	});

	jQuery(canvas).mousedown(function(e) {
		jQuery('.actions').hide();

		var pos = jQuery(this).offset();
		var p = fromScreen({x: e.pageX - pos.left, y: e.pageY - pos.top});
		selected = nearest = dragged = layout.nearest(p);

		if (selected.node !== null) {
			// Part of the same bug mentioned later. Store the previous mass
			// before upscaling it for dragging.
			dragged.point.m = 10000.0;
		}

		canvas.renderer.start();
	});

	jQuery(canvas).mousemove(function(e) {
		var pos = jQuery(this).offset();
		var p = fromScreen({x: e.pageX - pos.left, y: e.pageY - pos.top});
		nearest = layout.nearest(p);

		if (dragged !== null && dragged.node !== null) {
			dragged.point.p.x = p.x;
			dragged.point.p.y = p.y;
			moved = true;
		}

		canvas.renderer.start();
	});

	jQuery(window).bind('mouseup',function(e) {
		// Bug! Node's mass isn't reset on mouseup. Nodes which have been
		// dragged don't repulse very well. Store the initial mass in mousedown
		// and then restore it here.
		dragged = null;
	});

	Node.prototype.getWidth = function() {
		var text = typeof(this.data.label) !== 'undefined' ? this.data.label : this.id;
		if (this._width && this._width[text])
			return this._width[text];

		ctx.save();
		ctx.font = NODE_FONT;
		var width = ctx.measureText(text).width + 10;
		ctx.restore();

		this._width || (this._width = {});
		this._width[text] = width;

		return width;
	};

	Node.prototype.getHeight = function() {
		// Magic number with no explanation.
		return 35;
	};

	canvas.renderer = new Renderer(1, layout,
		function clear() {
			ctx.clearRect(0,0,canvas.width,canvas.height);
		},
		function drawEdge(edge, p1, p2) {
			var x1 = toScreen(p1).x;
			var y1 = toScreen(p1).y;
			var x2 = toScreen(p2).x;
			var y2 = toScreen(p2).y;

			var direction = new Vector(x2-x1, y2-y1);
			var normal = direction.normal().normalise();

			var from = canvas.graph.getEdges(edge.source, edge.target);
			var to = canvas.graph.getEdges(edge.target, edge.source);

			var total = from.length + to.length;

			// Figure out edge's position in relation to other edges between the same nodes
			var n = 0;
			for (var i=0; i<from.length; i++) {
				if (from[i].id === edge.id) {
					n = i;
				}
			}

			var spacing = 6.0;

			// Figure out how far off center the line should be drawn
			var offset = normal.multiply(-((total - 1) * spacing)/2.0 + (n * spacing));

			var s1 = toScreen(p1).add(offset);
			var s2 = toScreen(p2).add(offset);

			var boxWidth = edge.target.getWidth() + 8.0;
			var boxHeight = edge.target.getHeight() + 8.0;

			var intersection = intersect_line_box(s1, s2, {x: x2-boxWidth/2.0, y: y2-boxHeight/2.0}, boxWidth, boxHeight);

			if (!intersection)
				intersection = s2;

			var stroke = typeof(edge.data.color) !== 'undefined' ? edge.data.color : '#000000';

			var arrowWidth;
			var arrowLength;

			var weight = typeof(edge.data.weight) !== 'undefined' ? edge.data.weight : 1.0;

			ctx.lineWidth = Math.max(weight *  2, 0.1);
			arrowWidth = 5 + ctx.lineWidth;
			arrowLength = 20;

			var directional = typeof(edge.data.directional) !== 'undefined' ? edge.data.directional : true;

			// line
			var lineEnd;
			if (directional) {
				lineEnd = intersection.subtract(direction.normalise().multiply(arrowLength * 0.5));
			} else {
				lineEnd = s2;
			}

			ctx.strokeStyle = stroke;
			ctx.beginPath();
			ctx.moveTo(s1.x, s1.y);
			ctx.lineTo(lineEnd.x, lineEnd.y);
			ctx.stroke();

			// arrow

			if (directional) {
				ctx.save();
				ctx.fillStyle = stroke;
				ctx.translate(intersection.x, intersection.y);
				ctx.rotate(Math.atan2(y2 - y1, x2 - x1));
				ctx.beginPath();
				ctx.moveTo(-arrowLength, arrowWidth);
				ctx.lineTo(0, 0);
				ctx.lineTo(-arrowLength, -arrowWidth);
				ctx.lineTo(-arrowLength * 0.8, -0);
				ctx.closePath();
				ctx.fill();
				ctx.restore();
			}

			// label

			if (typeof(edge.data.label) !== 'undefined' && edge.data.label !== '') {
				text = edge.data.label;
				ctx.save();
				ctx.textAlign = "center";
				ctx.textBaseline = "top";
				ctx.font = "20px Helvetica, sans-serif";
				ctx.fillStyle = "black";
				ctx.fillText(text, (x1+x2)/2, (y1+y2)/2);
				ctx.restore();
			}

		},
		function drawNode(node, p) {
			var s = toScreen(p);

			ctx.save();

			var boxWidth = node.getWidth();
			var boxHeight = node.getHeight();
			var background = node.data.color;
			
			var inner_x = s.x - boxWidth / 2;
			var inner_y = s.y - 12;
			var inner_width = boxWidth;
			var inner_height = boxHeight;
			
			var extra_outer = 6;
			var outer_x = inner_x - extra_outer;
			var outer_y = inner_y - extra_outer;
			var outer_width = inner_width + 2 * extra_outer;
			var outer_height = inner_height + 2 * extra_outer;

			// fill background
			ctx.clearRect(outer_x, outer_y, outer_width, outer_height);
			
			var is_selected = selected !== null && nearest.node !== null && selected.node !== null && selected.node.id === node.id;
			var is_close = nearest !== null && nearest.node !== null && nearest.node.id === node.id;
			
			ctx.save();
			
			if(is_selected) {
				ctx.lineWidth = 1.8;
				ctx.strokeStyle = "#000000";
				ctx.strokeRect(outer_x, outer_y, outer_width, outer_height);
			} else if(is_close) {
				ctx.strokeStyle = "#DAA520";
				ctx.strokeRect(outer_x, outer_y, outer_width, outer_height);
			}
			ctx.restore();
			
			ctx.save();
			
			ctx.globalAlpha = 0.7;
			
			// fill background
			if(background !== null)
				ctx.fillStyle = background;
			else
				ctx.fillStyle = "#FFFFFF";

			ctx.beginPath();
			ctx.arc(inner_x + inner_width/2, inner_y + inner_height/2, inner_height/2.0, 0, Math.PI * 2, false);
			ctx.closePath();
			ctx.fill();
			ctx.restore();
			
			//ctx.fillRect(inner_x, inner_y, inner_width, inner_height);

			ctx.textAlign = "left";
			ctx.textBaseline = "top";
			ctx.font = NODE_FONT;
			ctx.fillStyle = "#FFFFFF";
			ctx.font = NODE_FONT;
			var text = typeof(node.data.label) !== 'undefined' ? node.data.label : node.id;
			ctx.fillText(text, s.x - boxWidth/2 + 5, s.y - 8);

			ctx.restore();
		}
	);

	canvas.renderer.start();

	// helpers for figuring out where to draw arrows
	function intersect_line_line(p1, p2, p3, p4) {
		var denom = ((p4.y - p3.y)*(p2.x - p1.x) - (p4.x - p3.x)*(p2.y - p1.y));

		// lines are parallel
		if (denom === 0) {
			return false;
		}

		var ua = ((p4.x - p3.x)*(p1.y - p3.y) - (p4.y - p3.y)*(p1.x - p3.x)) / denom;
		var ub = ((p2.x - p1.x)*(p1.y - p3.y) - (p2.y - p1.y)*(p1.x - p3.x)) / denom;

		if (ua < 0 || ua > 1 || ub < 0 || ub > 1) {
			return false;
		}

		return new Vector(p1.x + ua * (p2.x - p1.x), p1.y + ua * (p2.y - p1.y));
	}

	function intersect_line_box(p1, p2, p3, w, h) {
		var tl = {x: p3.x, y: p3.y};
		var tr = {x: p3.x + w, y: p3.y};
		var bl = {x: p3.x, y: p3.y + h};
		var br = {x: p3.x + w, y: p3.y + h};

		var result;
		if (result = intersect_line_line(p1, p2, tl, tr)) { return result; } // top
		if (result = intersect_line_line(p1, p2, tr, br)) { return result; } // right
		if (result = intersect_line_line(p1, p2, br, bl)) { return result; } // bottom
		if (result = intersect_line_line(p1, p2, bl, tl)) { return result; } // left

		return false;
	}

	return this;
}

})();
