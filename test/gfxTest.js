app.setBackground(0,0,0);

var img = app.getResource('eludi.icon.svg');
var circleFilled = app.createCircleResource(50);
var circleOutline = app.createCircleResource(52, [0,0,0,0], 2);

var sineWave = [];
for(var i=0; i<1250; i+=10) {
    sineWave.push(10+i/2);
    sineWave.push(420+Math.sin(i*Math.PI/180.0)*30);
}

console.log(0xFF000000, 0xFF0000, 0xFF00, 0xFF, '->', app.createColorArray(0xFF000000, 0xFF0000, 0xFF00, 0xFF))

app.on('draw', function(gfx) {
    gfx.color(0xffFFffFF);
    for(var i=1; i<12; i+=2)
        gfx.lineWidth(i).drawLine(i, i*10,20, i*10, 120);
    gfx.lineWidth(32);
    gfx.drawPoints([app.width-20,20]);
    gfx.lineWidth(1);
    gfx.drawPoints(sineWave);

    const coords = [ 140,60, 175,120, 205,10 ], coords2 = [ 155,60, 173,91, 195,10 ];
    gfx.color(85,255,85).drawLineStrip(coords);
    gfx.drawLineLoop(coords2);
    gfx.transform(-90,0).lineWidth(8);
    gfx.color(0,127,0).drawLineStrip(coords);
    gfx.fillTriangles(coords2);
    gfx.reset().lineWidth(1);
    
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
    gfx.color(255,170,85).drawImage(circleFilled, 60,180);
    gfx.drawImage(circleOutline, 60,180);

    gfx.color(0x55aaffff);
    for(var i=1; i<12; ++i)
        gfx.lineWidth(i).drawPoints([120+24*i,146, 120+24*i,168, 120+24*i,190, 120+24*i,212]);
    gfx.lineWidth(1);

    gfx.color(85,85,255).drawImage(img, 10,240);

    for(var i=0; i<360; i+=5) {
        var angle = 2*Math.PI*i/360;
        gfx.color(app.hsl(i,0.5,0.5)).drawLine(
            550,100, 550 + 90*Math.cos(angle), 100 - 90*Math.sin(angle));
    }

    for(var i=0; i<5; ++i)
        gfx.color(app.hsl(50,1.0,0.5-0.1*i)).fillText(440,240+20*i, "hello, world.");

    gfx.color(255,255,255,127).fillRect(0,app.height-20, app.width,20);
    gfx.color(0,0,0).fillText(0,app.height-18, "arcajs graphics test. (äöü ÄÖÜ)");
});
