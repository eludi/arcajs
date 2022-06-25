app.setBackground(255,255,255);
app.resizable(true);
var radius = 64;
var circle = app.createCircleResource(radius, [255,255,255]);
var arrow = app.createPathResource(128,128, 'M 63 0 L 0 127 L 63 95 L 127 127 z')

var sprites = [
    {x:radius, y:app.height-radius, image:circle, color:0x55ff55ff},
    {x:app.width-radius, y:radius, image:circle, color:0xFFff55ff},
];

app.on('resize', function(winSzX, winSzY) {
    sprites[0].y = winSzY-radius;
    sprites[1].x = winSzX-radius;
});

app.on('keyboard', function(evt) {
	if(evt.type==='keydown') switch(evt.key) {
	case 'F11':
        app.fullscreen(!app.fullscreen()); break;
    }
});

app.on('draw', function(gfx) {
    gfx.fillTriangles([0,0, app.width,0, 0,app.height, app.width,0, app.width,app.height, 0,app.height],
        [0,0,85,255, 0,0,85,255, 170,85,85,255, 0,0,85,255, 170,85,85,255, 170,85,85,255 ]);
    gfx.color(255,85,85).drawImage(circle, radius, radius);
    gfx.color(85,85,255).drawImage(circle, app.width-radius, app.height-radius);
    gfx.color(0,0,0).drawImage(arrow, 0,0,2*radius,2*radius, app.width/2-radius,app.height/2-radius,2*radius,2*radius, radius,radius, Math.PI);
    sprites.forEach(function(s) { gfx.color(s.color).drawImage(s.image, s.x, s.y); });
    gfx.color(0,0,0).fillText(0,app.height-20, app.width+':'+app.height);
});
