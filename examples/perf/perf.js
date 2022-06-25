app.setBackground(0x22,0x44,0x66);
var numObj = 200;
const objCounts = [ 100, 200, 400, 500, 1000, 2000, 4000, 8000, 16000 ];
const numComponents = 12;

function randi(lo, hi) {
	if(hi===undefined)
		return Math.floor(Math.random()*lo);
	return lo + Math.floor(Math.random()*(hi-lo));
}

var counter = 0, frames=0;
var fps = '';
var pointer = { buttons:0, x:0, y:0 };
var sprites = app.createTileResources('flags.png', 6,5,2, {centerX:0.5, centerY:0.5});

const LINE_SIZE = 24;

function createSprite(arr, seed, winSzX, winSzY, szMin, szMax) {
	var mem = arr.subarray(seed*numComponents, seed*numComponents+numComponents);
	const type = seed%3;
	if(type==0) { // circle
		mem[0]=29;
		mem[4] = randi(256);
		mem[5] = randi(256);
		mem[6] = randi(256);
		mem[7] = randi(63,256);
	}
	else if(type==1) { // rect
		mem[0]=28;
		mem[4] = randi(256);
		mem[5] = randi(256);
		mem[6] = randi(256);
		mem[7] = randi(63,256);
	}
	else if(type==2) { // img
		mem[0] = Math.floor(seed/3)%28;
		mem[3] = Math.random()*Math.PI*2; // rot
		mem[10] = Math.random()*Math.PI - Math.PI*0.5; // velRot
		mem[4] = mem[5] = mem[6] = mem[7] = 255; // color
	}
	mem[1] = randi(winSzX); // x
	mem[2] = randi(winSzY); // y
	mem[8] = randi(-2*szMin, 2*szMin); // velX
	mem[9] = randi(-2*szMin, 2*szMin); // velY
	mem[11] = seed;
}
var objs = new Float32Array(numComponents*objCounts[objCounts.length-1]);

function adjustNumObj(count) {
	numObj = count;
}

app.on('load', function() {
	for(var i=0, end=objCounts[objCounts.length-1]; i<end; ++i)
		createSprite(objs, i, app.width, app.height, 16, 64);
	adjustNumObj(numObj);
});

app.on('pointer', function(evt) {
	pointer.x = evt.x;
	pointer.y = evt.y;
	if(evt.type=='start') {
		pointer.buttons |= (1<<evt.id);
		if(evt.x<115 && evt.y>=100 && evt.y<100+LINE_SIZE*objCounts.length) {
			var selected = Math.floor((evt.y-100)/LINE_SIZE);
			adjustNumObj(objCounts[selected]);
		}
	}
	else if(evt.type=='end')
		pointer.buttons &= ~(1<<evt.id);
});

app.on('keyboard', function(evt) {
	if(evt.type!='keydown')
		return;
	if(evt.key=='ArrowUp') {
		if(numObj>objCounts[0]) {
			for(var i=1;i<objCounts.length; ++i)
				if(objCounts[i]==numObj) {
					adjustNumObj(objCounts[i-1]);
					break;
				}
		}
	}
	else if(evt.key=='ArrowDown') {
		if(numObj<objCounts[objCounts.length-1]) {
			for(var i=0;i<objCounts.length-1; ++i)
				if(objCounts[i]==numObj) {
					adjustNumObj(objCounts[i+1]);
					break;
				}
		}
	}
	else if(evt.key=='F11')
		app.fullscreen(!app.fullscreen());
});

app.on('update', function(deltaT, now) {
    app.transformArray(objs.subarray(0, numObj*numComponents), numComponents, deltaT, frames%15,
		function(input, output, deltaT, frame) {
			output[1] += input[8] * deltaT;
			output[2] += input[9] * deltaT;
			output[3] += input[10] * deltaT;

			if((input[11]+frame)%15 == 0) {
				const r = 48*1.41;
				if(output[1]>app.width+r)
					output[1] = -r;
				else if(output[1]<-r)
					output[1] = app.width+r;
				if(output[2]>app.height+r)
					output[2] = -r;
				else if(output[2]<-r)
					output[2] = app.height+r;
			}
		});

	++counter, ++frames;
	if(Math.floor(now)!=Math.floor(now-deltaT)) {
		fps = frames+'fps';
		frames = 0;
	}
});

app.on('draw', function(gfx) {
	// scene:
	gfx.drawImages(sprites, numComponents, gfx.COMP_IMG_OFFSET|gfx.COMP_ROT|gfx.COMP_COLOR_RGBA, objs.subarray(0, numObj*numComponents));

	// overlay:
	gfx.color(0,0,0,127).fillRect(0, 97, 115, objCounts.length*LINE_SIZE);
	for(var i=0; i<objCounts.length; ++i) {
		var opacity = (objCounts[i]==numObj) ? 255 : 127;
		gfx.color(255,255,255,opacity).fillText(0,100+i*LINE_SIZE, objCounts[i]);
	}

	gfx.color(0,0,0,127).fillRect(0, app.height-LINE_SIZE-2, app.width, LINE_SIZE+2);
	gfx.color(255,255,255).fillText(0, app.height-20, "arcaJS graphics2 performance test");
	gfx.color(255,85,85).fillText(app.width-60, app.height-20, fps);
});
