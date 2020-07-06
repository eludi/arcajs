app.setBackground(215,215,215);

var s=[];
for(var y=0; y<16; ++y) {
    s.push(new Uint8Array(16));
    for(var x=0; x<16; ++x)
        s[s.length-1][x] = (y==0&&x==0) ? 32 : y*16+x;
}

app.on('draw', function(gfx) {
    gfx.color(0,0,0);
    for(var y=0; y<s.length; ++y)
        gfx.fillText(0, 0,y*16, s[y]);

    gfx.colorf(0,0,0,0.5).fillRect(0,app.height-20, app.width,20);
    gfx.colorf(1.0,1.0,1.0).fillText(0, 0,app.height-18, "Enjoy a meticulously handcrafted 12x16 built-in font!");
});
