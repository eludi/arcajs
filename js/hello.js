var audio = app.require('audio');

var img = app.getResource('hello_arcajs.svg');
var sprites = app.createSpriteSet(img), hello;

app.on('ready', function() {
    hello = sprites.createSprite();
    hello.setPos(app.width/2, app.height/2);
});

app.on('update', function(deltaT, now) {
    hello.setRot(now*Math.PI*0.5);
    hello.setScale(Math.sin(now*3));
});

app.on('draw', function(gfx) {
    gfx.drawSprites(sprites);
});

app.on('pointer', function(evt) {
    if(evt.type==='start')
        audio.sound('square', 50+Math.random()*1000, 0.5, 0.5);
});
