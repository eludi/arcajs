app.setBackground(63,63,63);
const circle = app.createCircleResource(64);
var blendMode = 1;

app.on('draw', function(gfx) {
    gfx.blend(blendMode);
    var ox = app.width*0.5-64, oy=app.height*0.5-200;
    gfx.color(255,0,0,127).drawImage(circle,ox-50,oy+25);
    gfx.color(0,255,0,127).drawImage(circle,ox,oy-50);
    gfx.color(0,0,255,127).drawImage(circle,ox+50,oy+25);

    oy+=210;
    gfx.color(255,0,0,127).fillRect(ox-50,oy+25,128,128);
    gfx.color(0,255,0,127).fillRect(ox,oy-50,128,128);
    gfx.color(0,0,255,127).fillRect(ox+50,oy+25,128,128);
});

app.on('keyboard', function(evt) {
    if(evt.type=='keydown' && evt.key==' ') {
        if(!blendMode)
            blendMode = 1;
        else
            blendMode *= 2;
        if(blendMode>8)
            blendMode = 0;
    }
});