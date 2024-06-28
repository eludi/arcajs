app.setBackground(127,127,127);
var c1 = app.createCircleResource(50,[255,0,255], 20, [0,255,0,127]);
var path = app.createPathResource(230,120,"M 10 10 L 210 60 L 10 110 L 60 60 Z", [255,255,85],10,[255,85,85]);
var logo = app.createSVGResource('<svg width="64" height="64"><path d="M 32,4 C 16.535557,4 4,16.535558 4,32 4,47.464447 16.53556,60 32,60 46.447962,60 58.339971,49.040462 59.840909,34.990909 58.163998,43.568548 50.589917,50.072727 41.481818,50.072727 31.14367,50.072725 22.740909,41.717468 22.740909,31.427273 27.159837,37.291842 34.116939,40.75 41.481818,40.75 48.743306,40.749998 55.573858,37.397054 60,31.681818 59.914601,24.373955 56.964824,17.383004 51.790909,12.209091 46.539733,6.9579124 39.426292,4 32,4 z M 32.159091,19.018182 C 35.691774,19.018182 38.554545,21.880954 38.554545,25.413636 38.554545,28.946319 35.691774,31.809091 32.159091,31.809091 28.626408,31.809091 25.763636,28.946319 25.763636,25.413636 25.763636,21.880954 28.626408,19.018182 32.159091,19.018182 z" style="fill:#ffffff;fill-opacity:1;fill-rule:nonzero;stroke:none" /></svg>');
var lines = app.createSVGResource('<svg width="64" height="64"><line x1="0" y1="5.5" x2="30" y2="5.5" stroke="white" stroke-width="3" /><line x1="0" y1="15.5" x2="30" y2="15.5" stroke="white" stroke-dasharray="8 4" stroke-width="5"/><line x1="5" y1="25" x2="25" y2="25" stroke="white" stroke-width="10" stroke-linecap="round" /></svg>');
var solid = app.createImageResource(1,1, new Uint8Array([0xff, 0xff, 0xff, 0xff]));

var canvas = app.createImageResource(64,64, function(gfx) {
	gfx.color(0xFFffFFff).lineWidth(5);
	gfx.drawLine(0,0,64,64);
	gfx.drawLine(0,64,64,0);
});

app.on('draw', function(gfx) {
	gfx.color(255,255,255).drawImage(c1,60,60);
	gfx.drawImage(path,0,120);
	gfx.drawImage(logo,120,0);
	gfx.drawImage(lines,240,120);
	gfx.drawImage(solid,360,40,40,40);
	gfx.color(0xFFaa55ff).drawImage(canvas,480,0)
});
