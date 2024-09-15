var audio = app.require('audio');

var hello = {
    x: app.width/2,
    y: app.height/2,
    rot:0,
    sc:0,
    image: app.getResource('hello_arcajs.svg', {centerX:0.5, centerY:0.5})
};

app.on('resize', function(winSzX, winSzY) {
    hello.x = winSzX/2;
    hello.y = winSzY/2;
});

app.on('update', function(deltaT, now) {
    hello.rot = now*Math.PI/2
    hello.sc = Math.sin(now*3);
});

app.on('draw', function(gfx) {
    gfx.drawSprite(hello);
});

app.on('pointer', function(evt) {
    if(evt.type==='start')
        audio.sound('square', 50+Math.random()*1000, 0.5, 0.5);
});

app.on('keyboard', function(evt) {
    if(evt.type=='keydown' && evt.key==' ') {
        audio.melody("{w:tri a:.025 d:.025 s:.25 r:.05 b:120} A3/12 C#4/12 E4/12 {s:.5 r:.45 g:1.5} A4/4", 0.7, 0.0);
    }
});

app.on('gamepad', function(evt) {
	if(evt.type==='button' && evt.value>0) {
		if(evt.button >= 6)
			return app.close();
        audio.sound('square', 50+Math.random()*1000, 0.5, 0.5);
	}
});

app.on('textinsert', function(evt) {
    console.log(evt);
});
