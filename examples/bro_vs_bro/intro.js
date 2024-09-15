app.setBackground(0,0,0);
app.setPointer(false);

const titleTheme = app.getResource('Cunning plan.mp3');
const tilesz = 8*Math.floor(Math.min(Math.floor(app.width/40), Math.floor(app.height/25))/8);
const originX = 0.5*(app.width-tilesz*40), originY = 0.5*(app.height-tilesz*25);
const charset = app.createTileResources('petscii.png', 16,16,0, {filtering:0});
var petscii;

var screenIntro = (function() {
	function swap(arr, i,j) {
		const tmp = arr[i];
		arr[i] = arr[j]
		arr[j] = tmp;	
	}
	function randomSequence(n) {
		var seq = new Uint32Array(n);
		for(var i=0; i<n; ++i) seq[i] = i;
		for(var i=n-1; i>0; --i) swap(seq, i, Math.floor(Math.random()*(i+1)));
		return seq;
	};

	const audio = app.require('audio');
	var frame = 0, titleTrack = -1;
	const tileSet = app.createTileResources('bro_vs_bro64.png',40,25,0, {filtering:0});
	const tiles = randomSequence(1000);
	const pressedButtons = {};

	return {
		enter: function() {
			frame = 0;
		},
		update: function(deltaT) {
			++frame;
			if(!audio.playing(titleTrack))
				titleTrack = audio.replay(titleTheme);
		},

		draw: function(gfx) {
			gfx.transform(originX, originY, 0, tilesz).color(0xFFffFFff);
			const sz = 1/8;
			for(var i=0, end = Math.min(1000, frame*4); i<end; ++i) {
				const tile = tiles[i], x = tile%40, y = Math.floor(tile/40);
				gfx.drawImage(tileSet+tile, x,y,0,sz);
			}

			gfx.reset().transform(originX, originY, 0, tilesz/8);
			gfx.color(0,0,0,85);
			for(var y=-0.125; y<200; ++y) // scanlines
				gfx.fillRect(0,y,320,0.25);
		},

		keyboard: function(evt) {
			if(evt.type=='keydown') {
				if(evt.key == 'Escape')
					return app.close();
				app.on(screenGame);
			}
		},
		gamepad: function(evt) {
			if(evt.type==='button') {
				if(evt.button===0 && evt.value>0)
					return app.on(screenGame);
				pressedButtons[evt.button] = evt.value>0;
				if((pressedButtons[6] && pressedButtons[7])
					|| (pressedButtons[8] && pressedButtons[9]))
					return app.close();
			}
		},
		leave: function() {
			audio.fadeOut(titleTrack, 1.5);
		}
	};
}());