var audio = app.require('audio');

var img = app.getResource('hello_arcajs.svg');
var sprites = app.createSpriteSet(img), hello;
//var sample = app.getResource('bellchord_mono.mp3');

app.on('load', function() {
    hello = sprites.createSprite();
    hello.setPos(app.width/2, app.height/2);
    hello.setVelRot(Math.PI/2);
});

app.on('resize', function(winSzX, winSzY) {
    if(hello)
        hello.setPos(winSzX/2, winSzY/2);
});

app.on('update', function(deltaT, now) {
    sprites.update(deltaT);
    if(hello)
        hello.setScale(Math.sin(now*3));
});

app.on('draw', function(gfx) {
    gfx.drawSprites(sprites);
});

app.on('pointer', function(evt) {
    if(evt.type==='start')
        audio.sound('square', 50+Math.random()*1000, 0.5, 0.5);
});

app.on('keyboard', function(evt) {
    if(evt.type=='keydown' && evt.key==' ') {
        //audio.replay(sample);
        audio.melody("{w:tri a:.025 d:.025 s:.25 r:.05 b:120} A3/12 C#4/12 E4/12 {s:.5 r:.45 g:1.5} A4/4", 0.7, 0.0);
    }
});
