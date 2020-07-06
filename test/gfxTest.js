app.setBackground(0,0,0);

var img = app.getResource('eludi.icon.svg');
var circleFilled = app.createCircleResource(50);
var circleOutline = app.createCircleResource(52, [0,0,0,0], 2);

var sineWave = [];
for(var i=0; i<1250; i+=10) {
    sineWave.push(10+i/2);
    sineWave.push(420+Math.sin(i*Math.PI/180.0)*30);
}

app.on('draw', function(gfx) {
    gfx.colorf(1.0,1.0,1.0);
    for(var i=1; i<12; i+=2)
        gfx.drawLine(i, i*10,20, i*10, 120);
    gfx.drawPoints(sineWave);

    //var coords = [ 125,30, 175,120, 205,10 ];
    //gfx.color(85,255,85,63).fillPolygon.apply(gfx, coords);
    //gfx.color(85,255,85).drawPolygon.apply(gfx, coords);

    gfx.color(255,85,85).fillRect(220,10,50,50);
    gfx.drawRect(340,70,50,50);
    gfx.color(255,255,85).fillRect(280,10,50,50);
    gfx.drawRect(400,70,50,50);
    gfx.color(85,85,255).fillRect(220,70,50,50);
    gfx.drawRect(340,10,50,50);
    gfx.color(85,170,85).fillRect(280,70,50,50);
    gfx.drawRect(400,10,50,50);

    //for(var i=1; i<16; ++i)
    //    gfx.color(i*16, i*16, i*16).drawArc(460,130,i*4, -Math.PI/2, Math.PI);
    gfx.color(255,170,85).drawImage(circleFilled, 10,130);
    gfx.drawImage(circleOutline, 7,127);

    gfx.color(85,85,255);
    for(var i=1; i<12; ++i)
        gfx.drawPoints([120+24*i,146, 120+24*i,168, 120+24*i,190, 120+24*i,212]);

    gfx.drawImage(img, 10,240);
    gfx.drawImage(img, 100,240, 160,60);
    gfx.drawImage(img, 0,0, 50,50, 340,240,100,100, 0,0,Math.PI/4);

    for(var i=0; i<360; i+=5) {
        var angle = 2*Math.PI*i/360;
        gfx.colorHSL(i,0.5,0.5).drawLine(
            550,100, 550 + 90*Math.cos(angle), 100 - 90*Math.sin(angle));
    }

    for(var i=0; i<5; ++i)
        gfx.colorHSL(50,1.0,0.5-0.1*i).fillText(gfx.defaultFont, 440,240+20*i, "hello, world.");

    gfx.color(255,255,255,127).fillRect(0,app.height-20, app.width,20);
    gfx.color(0,0,0).fillText(0, 0,app.height-18, "arcajs graphics test");
});
