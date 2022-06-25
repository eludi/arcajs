app.setBackground(0,0,0);

function randi(lo, hi) {
	if(hi===undefined)
		return Math.floor(Math.random()*lo);
	return lo + Math.floor(Math.random()*(hi-lo));
}

function Sprite(x,y) {
	this.x = x;
	this.y = y;
	this.rot = 0;
	this.radius = 32;
	this.color = 0xFFffFFff;
	this.image = Sprite.IMG;

	this.draw = function(gfx) {
		gfx.drawSprite(this);
	}
	this.update = function(deltaT) {
		if(this.vrot)
			this.rot += this.vrot * deltaT;
	}
}
Sprite.IMG = app.getResource('eludi.icon.svg', {centerX:0.5, centerY:0.5});

var sprites = [];
{
	var sprite = new Sprite(randi(32,app.width-64), randi(32,app.height-64));
	sprite.color = (randi(256) << 24) + (randi(256) << 16) + (randi(256) << 8) + 0xFF;
	sprite.vrot = Math.random()*Math.PI - Math.PI*0.5;
	sprites.push(sprite);

	sprite =  new Sprite(64,32);
	sprite.sc= 2;
	sprites.push(sprite);

	sprite = new Sprite(32,96);
	sprite.flip = 1;
	sprites.push(sprite);

	sprite = new Sprite(96,96)
	sprite.flip = 2;
	sprites.push(sprite);
}

//------------------------------------------------------------------
var counter = 0, frames=0;
var fps = '';

app.on('update', function(deltaT, now) {
	sprites.forEach(function(s) { s.update(deltaT); });

	++counter, ++frames;
	if(Math.floor(now)!=Math.floor(now-deltaT)) {
		fps = frames+'fps';
		frames = 0;
	}
});

app.on('draw', function(gfx) {
	sprites.forEach(function(s) { s.draw(gfx); });
	gfx.color(0xffFFffFF).drawImage(Sprite.IMG, app.width-64,0);

	gfx.color(255,255,255,127).fillRect(0,app.height-20, app.width,20);
	gfx.color(0,0,0).fillText(0,app.height-18, "arcajs sprites test");
	gfx.color(255,255,85).fillText(app.width, app.height, fps, 0, gfx.ALIGN_RIGHT_BOTTOM);
});
