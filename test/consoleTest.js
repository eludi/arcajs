app.setBackground(215,215,215);
console.visible(true);
var lineBuf = '';

var s=[];
for(var y=0; y<16; ++y) {
    s.push(new Uint8Array(16));
    for(var x=0; x<16; ++x)
        s[s.length-1][x] = (y==0&&x==0) ? 32 : y*16+x;
}

app.on('load', function() {
    console.log('Hello, arcjs console');
});

app.on('draw', function(gfx) {
    gfx.color(0,0,0);
    for(var y=0; y<s.length; ++y)
        gfx.fillText(0, app.width-12*16,y*16, s[y]);

    gfx.colorf(0,0,0,0.5).fillRect(0,app.height-20, app.width,20);
    gfx.fillText(0,0,app.height-40,lineBuf);
    gfx.colorf(1.0,1.0,1.0).fillText(0, 0,app.height-18,
        "Enjoy a meticulously handcrafted 12x16 built-in font!");
});

app.on('textinput', function(evt) {
    if('char' in evt)
        lineBuf += evt.char;
	else switch(evt.key) {
	case 'F12':
		return console.visible(!console.visible());
    case 'Enter':
        console.log(lineBuf);
        lineBuf = '';
        break;
    case 'Backspace':
        if(lineBuf.length)
            lineBuf = lineBuf.substr(0, lineBuf.length-1);
        break;
    }
});
