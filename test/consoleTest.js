app.setBackground(170,170,170);
console.visible(true);

var lineBuf = '', now=0;
var info = "Enjoy a meticulously handcrafted 12x16 built-in font!";

app.on('load', function() {
	console.log('Hello, arcjs console.');
	console.log('Umlauts are finally supported: ÄäÖöÜöß');
});

app.on('update', function(deltaT, tNow) {
	now = tNow;
});

app.on('draw', function(gfx) {
	gfx.color(0,0,0,127).fillRect(0,app.height-20, app.width,20);
	gfx.color(255,255,85).fillText(0,app.height-40,'>'+lineBuf+(now%1>0.5?'':'_'));
	gfx.color(0xffFFffFF).fillText(0,app.height-18, info);
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
	case 'Escape': {
		var input = app.prompt('Please enter your message:')
		if (input) {
			console.log(input);
			app.message(['You have entered a message:', input, 'Thank you!']);
		}
		break;
	}
	case 'Backspace':
		if(lineBuf.length)
			lineBuf = lineBuf.substr(0, lineBuf.length-1);
		break;
	}
});
