app.setBackground(48,48,48);
const circle = app.createCircleResource(64);
var blendMode = 1;

const beamRed = app.createImageResource(64,64, 0xff, function(gfx) {
	gfx.color(0xFF5555ff).lineWidth(5).drawLine(0,0,64,64);
});
const beamCyan = app.createImageResource(64,64, 0xff, function(gfx) {
	gfx.color(0x00aaAAff).lineWidth(5).drawLine(0,64,64,0);
});


function blendModeName(mode) {
    switch(mode) {
        case 0: return "NONE";
        case 1: return "ALPHA";
        case 2: return "ADD";
        case 4: return "MOD";
        case 8: return "MUL";
        default: return "UNKNOWN";
    }
}

function nextBlendMode() {
    if(!blendMode)
        blendMode = 1;
    else
        blendMode *= 2;
    if(blendMode>8)
        blendMode = 0;
}

app.on('draw', function(gfx) {
    gfx.blend(blendMode);
    gfx.drawImage(beamRed, 0, 0);
    gfx.drawImage(beamCyan, 16, 16);

    var ox = app.width*0.5-64, oy=app.height*0.5-200;
    gfx.color(255,0,0,127).drawImage(circle,ox-50,oy+25);
    gfx.color(0,255,0,127).drawImage(circle,ox,oy-50);
    gfx.color(0,0,255,127).drawImage(circle,ox+50,oy+25);

    oy+=210;
    gfx.color(255,0,0,127).fillRect(ox-50,oy+25,128,128);
    gfx.color(0,255,0,127).fillRect(ox,oy-50,128,128);
    gfx.color(0,0,255,127).fillRect(ox+50,oy+25,128,128);

	gfx.blend(1).color(255,255,255);
	gfx.fillText(0,app.height-20, 'blend mode ' + blendModeName(blendMode));
});

app.on('gamepad', function(evt) {
    if(evt.type==='axis' && evt.value==1.0 && evt.axis in {1:true,3:true})
        nextBlendMode();
});

app.on('keyboard', function(evt) {
    if(evt.type==='keydown' && evt.key===' ')
        nextBlendMode();
});
