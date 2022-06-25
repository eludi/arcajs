console.log("sprite collision tests");
const isx = app.require('intersects');

app.setBackground(63,63,63);
const circleRadius=32, circleImg = app.createCircleResource(circleRadius);
var shapes = [];

const baseColor = [255,255,85,127];
const selColor = [255,85,85,127];
const iscColor = [85,255,255,127];
var pointer = { prevX:0, prevY:0};


function Rect(x,y,w,h,rot) {
	this.x = x;
	this.y = y;
	this.rot = rot===undefined ? 0 : rot;
	this.w = w;
	this.h = h;
	this.cx = 0.5;
	this.cy = 0.5;

	this.color = baseColor;
	this.selected = false;

	this.draw = function(gfx) {
		gfx.save().color(this.color).transform(this);
		gfx.fillRect(-this.w*this.cx,-this.h*this.cx,this.w,this.h);
		gfx.restore();
	}
}
function Circle(x,y,radius) {
	this.x = x;
	this.y = y;
	this.radius = radius;

	this.color = baseColor;
	this.selected = false;

	this.draw = function(gfx) {
		gfx.color(this.color).drawImage(circleImg, this.x, this.y, 0, this.radius/circleRadius);
	}
}
function Poly(x,y,coords) {
	this.x = x;
	this.y = y;
	this.radius = 0;
	this.triangles = Array.isArray(coords) ? new Float32Array(coords) : coords;

	for(var i=0; i+1<coords.length; i+=2)
		this.radius = Math.max(this.radius, coords[i]*coords[i] + coords[i+1]*coords[i+1]);
	this.radius = Math.sqrt(this.radius);

	this.color = baseColor;
	this.selected = false;

	this.draw = function(gfx) {
		gfx.save().transform(this).color(this.color).fillTriangles(this.triangles);
		gfx.restore();
	}
}

function testSpriteIntersections() {
	shapes.forEach(function(s) {
		s.selected = false;
		s.color = baseColor;
	});
	for(var i=0; i<shapes.length; ++i) {
		var s1=shapes[i];
		for(var j=i+1; j<shapes.length; ++j) {
			var s2=shapes[j];
			if(isx.sprites(s1, s2))
				s1.color = s2.color = iscColor;
		}
	}
}

app.on('load', function() {
	for(var i=0; i<10; ++i)
		shapes.push(new Rect(
			app.width*Math.random(),app.height*Math.random(),
			32+96*Math.random(), 32+96*Math.random(),
			(i%2 == 1) ? Math.random()*2*Math.PI : 0
		));
	for(var i=0; i<5; ++i)
		shapes.push(new Circle(app.width*Math.random(), app.height*Math.random(), circleRadius));
	shapes.push(new Poly(100,100,[0,-32, 32,32, -32,0]), new Poly(200,100,[0,-40, 20,40, -40,0]));
	testSpriteIntersections();
});

app.on('pointer', function(evt) {
	if(evt.type=='start') {
		if(evt.id!==0)
			return;
		pointer.prevX=evt.x;
		pointer.prevY=evt.y;
		for(var i=0; i<shapes.length; ++i) {
			var s=shapes[i];
			s.color = baseColor;
			if(isx.sprites(s,{x:evt.x,y:evt.y, radius:0})) {
				console.log("shape "+i+" selected");
				s.selected = true;
				s.color = selColor;
			}
		}
	}
	else if(evt.type=='end') {
		if(evt.id!==0)
			return;
		testSpriteIntersections();
	}
	else if(evt.type=='move') {
		var dx = evt.x-pointer.prevX, dy = evt.y-pointer.prevY;
		for(var i=0; i<shapes.length; ++i) {
			var s=shapes[i];
			if(!s.selected)
				continue;
			s.x+=dx;
			s.y+=dy;
		}
		pointer.prevX=evt.x;
		pointer.prevY=evt.y;
	}
});

app.on('draw', function(gfx) {
	shapes.forEach(function(s) { s.draw(gfx); });
});
