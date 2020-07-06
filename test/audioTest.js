var audio = app.require('audio');

var patt1_1 = "{w:saw a:.025 d:.025 s:.25 r:.05 b:65}"
	+ " E2/8 B2/8 B2/8 E2/8 B2/8 B2/8 E2/8 B2/8"
	+ " C2/8 G2/8 G2/8 C2/8 G2/8 G2/8 C2/8 G2/8"
	+ " D2/8 A2/8 A2/8 D2/8 A2/8 A2/8 G2/8 F#2/8";
var track1 = -1;
function audioUpdate(now) {
	if(!audio.playing(track1)) {
		track1 = audio.melody(patt1_1, 0.25, 0.0);
	}
}


app.on('update', function(deltaT, now) {
	audioUpdate(now);
});
