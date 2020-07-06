app.setBackground(0,0,0);

function randi(lo, hi) {
	if(hi===undefined)
		return Math.floor(Math.random()*lo);
	return lo + Math.floor(Math.random()*(hi-lo));
}

var sprites = app.createSpriteSet(app.getResource('eludi.icon.svg'));
var sprite = sprites.createSprite();
sprite.setPos(randi(app.width), randi(app.height));
sprite.setColor(randi(256), randi(256), randi(256));
sprite.setVelRot(Math.random()*Math.PI - Math.PI*0.5);

//------------------------------------------------------------------
var counter = 0, frames=0;
var fps = '';

app.on('update', function(deltaT, now) {
	sprites.update(deltaT);

	++counter, ++frames;
	if(Math.floor(now)!=Math.floor(now-deltaT)) {
		fps = frames+'fps';
		frames = 0;
	}
	if(counter==200)
		sprite = null;
	if(counter>=275)
		app.close();
});

app.on('draw', function(gfx) {
	gfx.drawSprites(sprites);

	gfx.color(255,255,255,127).fillRect(0,app.height-20, app.width,20);
	gfx.color(0,0,0).fillText(0, 0,app.height-18, "arcajs graphics experiments test");
	gfx.color(255,0,0).fillText(0, app.width, app.height, fps, gfx.ALIGN_RIGHT | gfx.ALIGN_BOTTOM);
});
