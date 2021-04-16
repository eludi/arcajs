app.setBackground(0x22,0x44,0x66);
var numObj = 200;
var objCounts = [ 100, 200, 500, 1000, 2000, 4000, 6000 ];

function randi(lo, hi) {
	if(hi===undefined)
		return Math.floor(Math.random()*lo);
	return lo + Math.floor(Math.random()*(hi-lo));
}

var counter = 0, frames=0;
var fps = '';
var pointer = { buttons:0, x:0, y:0 };
var sprites = app.createSpriteSet(app.getResource('flags.png'), 6,5,2);

const LINE_SIZE = 32;
var font = app.getResource('Orbitron-Regular.ttf', {size:LINE_SIZE});

function GfxObj(seed, winSzX, winSzY, szMin, szMax) {
	this.update = function(deltaT) {
		var x = this.sprite.getX(), y=this.sprite.getY(), r=this.radius;

		if(x>winSzX+r)
			x -= winSzX+2*r;
		else if(x<-r)
			x += winSzX+2*r;
		if(y>winSzY+r)
			y -= winSzY+2*r;
		else if(y<-r)
			y += winSzY+2*r;
	
		this.sprite.setPos(x, y);
	}

	this.radius = randi(szMin, szMax);
	this.type = seed%3;
	this.sprite = null;
	var velRot=0;
	if(this.type==0) { // circle
		this.sprite = sprites.createSprite(29)
			.setColor(randi(256), randi(256), randi(256),randi(63,256))
			.setDim(2*this.radius, 2*this.radius);
	}
	else if(this.type==1) { // rect
		this.sprite = sprites.createSprite(28)
			.setColor(randi(256), randi(256), randi(256),randi(63,256))
			.setDim(2*this.radius, 2*this.radius);
	}
	else if(this.type==2) { // img
		this.sprite = sprites.createSprite(Math.floor(seed/3)%28);
		this.radius = 50;
		velRot = Math.random()*Math.PI - Math.PI*0.5;
	}
	this.sprite.setPos(randi(winSzX), randi(winSzY))
		.setVel(randi(-2*szMin, 2*szMin), randi(-2*szMin, 2*szMin), velRot);
}
var objs = [];

function adjustNumObj(count) {
	if(objs.length>count) {
		for(var i=objs.length; i-->count; )
			sprites.removeSprite(objs[i].sprite);
		objs.length = count;
	}
	else for(var i=objs.length; i<count; ++i)
		objs.push(new GfxObj(i, app.width, app.height, 16, 64));
	numObj = count;
}

app.on('load', function(){
	adjustNumObj(numObj);
});

app.on('pointer', function(evt) {
	pointer.x = evt.x;
	pointer.y = evt.y;
	if(evt.type=='start') {
		pointer.buttons |= (1<<evt.id);
		if(evt.x<115 && evt.y>=100 && evt.y<100+LINE_SIZE*objCounts.length) {
			var selected = Math.floor((evt.y-100)/LINE_SIZE);
			adjustNumObj(objCounts[selected]);
		}
	}
	else if(evt.type=='end')
		pointer.buttons &= ~(1<<evt.id);
});

app.on('keyboard', function(evt) {
	if(evt.type!='keydown')
		return;
	if(evt.key=='ArrowUp') {
		if(numObj>objCounts[0]) {
			for(var i=1;i<objCounts.length; ++i)
				if(objCounts[i]==numObj) {
					adjustNumObj(objCounts[i-1]);
					break;
				}
		}
	}
	else if(evt.key=='ArrowDown') {
		if(numObj<objCounts[objCounts.length-1]) {
			for(var i=0;i<objCounts.length-1; ++i)
				if(objCounts[i]==numObj) {
					adjustNumObj(objCounts[i+1]);
					break;
				}
		}
	}
});

app.on('update', function(deltaT, now) {
	sprites.update(deltaT);
	var isOdd = counter%4;
	objs.forEach(function(obj, index) { if(index%4==isOdd) obj.update(deltaT); });
	++counter, ++frames;
	if(Math.floor(now)!=Math.floor(now-deltaT)) {
		fps = frames+'fps';
		frames = 0;
	}
});

app.on('draw', function(gfx) {
	// scene:
	gfx.drawSprites(sprites);

	// overlay:
	gfx.color(0,0,0,127).fillRect(0, 97, 115, objCounts.length*LINE_SIZE);
	for(var i=0; i<objCounts.length; ++i) {
		var opacity = (objCounts[i]==numObj) ? 255 : 127;
		gfx.color(255,255,255,opacity).fillText(font,0,100+i*LINE_SIZE, objCounts[i]);
	}

	gfx.color(0,0,0,127).fillRect(0, app.height-LINE_SIZE-2, app.width, LINE_SIZE+2);
	gfx.color(255,255,255).fillText(font, 0, app.height, "arcaJS sprite performance test", gfx.ALIGN_BOTTOM);
	gfx.color(255,85,85).fillText(font, app.width, app.height, fps, gfx.ALIGN_RIGHT_BOTTOM);
});
