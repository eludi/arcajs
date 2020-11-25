var points = new Float32Array(2*(1+Math.floor(app.width/50))*(1+Math.floor(app.height/50)));
app.on('load', function() {
    app.setBackground(0,0,0);
    for(var y=0, i=0; y<app.height; y+=50)
        for(var x=0; x<app.width; x+=50) {
            points[i++]=x;
            points[i++]=y;
        }
});

app.on('draw', function(gfx) {
    gfx.color(255,255,85,85).lineWidth(10).drawPoints(points);
    gfx.color(255,255,255,127).lineWidth(1);
    gfx.drawRect(50,50,100,100);
	gfx.color(255,255,255);
	for(var lw = 1; lw<10; ++lw) {
        gfx.lineWidth(lw);
        gfx.drawLine(50+10*lw, 50, 50+10*lw, 150);
    }
    gfx.lineWidth(1);
    gfx.color(255,85,85);
	for(var lw = 1; lw<10; ++lw) {
        gfx.drawLine(50+10*lw, 50, 50+10*lw, 150);
    }

    gfx.lineWidth(1);
    gfx.drawRect(150,50,100,100);
	gfx.color(255,255,255);
	for(var lw = 1; lw<10; ++lw) {
        gfx.lineWidth(lw);
        gfx.drawLine(150, 50+10*lw, 250, 50+10*lw);
    }
    gfx.lineWidth(1).color(255,85,85);
	for(var lw = 1; lw<10; ++lw) {
        gfx.drawLine(150, 50+10*lw, 250, 50+10*lw);
    }

    gfx.lineWidth(1);
    gfx.drawRect(300,50,100,100);
	gfx.lineWidth(11).color(255,255,255,85);
    gfx.drawLine(300, 50, 400, 150);
    gfx.drawLine(300, 150, 400, 50);
    gfx.lineWidth(1).color(255,85,85);
    gfx.drawLine(300, 50, 400, 150);
    gfx.drawLine(300, 150, 400, 50);

    gfx.color(255,255,255,127).lineWidth(10).drawRect(50,200,100,100);
    gfx.color(255,85,85).lineWidth(1).drawRect(50,200,100,100);
    gfx.color(255,255,255,127).lineWidth(20).drawRect(150,200,100,100);
    gfx.color(255,85,85).lineWidth(1).drawRect(150,200,100,100);
    gfx.color(255,255,255,127).lineWidth(30).drawRect(250,200,100,100);
    gfx.color(255,85,85).lineWidth(1).drawRect(250,200,100,100);
});
