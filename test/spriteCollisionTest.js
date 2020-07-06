console.log("sprite collision tests");
app.setBackground(63,63,63);
const circleRadius=32;
var sprites = app.createSpriteSet(app.createCircleResource(circleRadius));
var shapes = [];

const baseColor = [255,255,85];
const selColor = [255,85,85];
const iscColor = [85,255,255];
var pointer = { prevX:0, prevY:0};

for(var i=0; i<10; ++i) {
	var rect = sprites.createSprite(circleRadius,circleRadius,1,1); // take only 1 solid pixel as source
	rect.setPos(app.width*Math.random(), app.height*Math.random());
	if(i%2 == 1)
		rect.setRot(Math.random()*2*Math.PI);
	rect.setDim(32+96*Math.random(),32+96*Math.random());
	rect.setColor(baseColor[0], baseColor[1], baseColor[2], 127);
	rect.selected = false;
	shapes.push(rect);
}
for(var i=0; i<5; ++i) {
	var circle = sprites.createSprite();
	circle.setPos(app.width*Math.random(), app.height*Math.random());
	circle.setColor(baseColor[0], baseColor[1], baseColor[2], 127);
	circle.selected = false;
	circle.setRadius(circleRadius);
	shapes.push(circle);
}

function testSpriteIntersections() {
	for(var i=0; i<shapes.length; ++i) {
		var s=shapes[i];
		s.selected = false;
		s.setColor(baseColor[0], baseColor[1], baseColor[2], 127);
	}
	for(var i=0; i<shapes.length; ++i) {
		var s1=shapes[i];
		for(var j=i+1; j<shapes.length; ++j) {
			var s2=shapes[j];
			if(s1.intersects(s2)) {
				s1.setColor(iscColor[0],iscColor[1],iscColor[2], 127);
				s2.setColor(iscColor[0],iscColor[1],iscColor[2], 127);
			}
		}
	}
}
testSpriteIntersections();

app.on('pointer', function(evt) {
	if(evt.type=='start') {
		if(evt.id!==0)
			return;
		pointer.prevX=evt.x;
		pointer.prevY=evt.y;
		for(var i=0; i<shapes.length; ++i) {
			var s=shapes[i];
			s.setColor(baseColor[0],baseColor[1],baseColor[2], 127);
			if(s.intersects(evt.x,evt.y)) {
				console.log("shape "+i+" selected");
				s.selected = true;
				s.setColor(selColor[0],selColor[1],selColor[2], 127);
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
			s.setPos(s.getX()+dx, s.getY()+dy);
		}
		pointer.prevX=evt.x;
		pointer.prevY=evt.y;
	}
});

app.on('draw', function(gfx) {
	gfx.drawSprites(sprites);
});
