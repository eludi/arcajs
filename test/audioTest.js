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

function currentTime() {
	return (new Date()).toISOString().replace(/[A-Z]/g, ' ');
}


app.on('load', function() {
	const sampleRate = audio.sampleRate;
	console.log('load', currentTime(), 'sampleRate', sampleRate);
	var sample = new Float32Array(sampleRate);
	for(var i=0; i<sample.length; ++i)
		sample[i]=Math.sin(440*i*2*Math.PI/sampleRate);
	var id = audio.sample(sample);

	console.log('sample 440', id, currentTime());
	audio.replay(id);
	setTimeout(function() {
		console.log('sound 440', currentTime()); audio.sound('sin', 440, 1);
	}, 1100);
	setTimeout(function() {
		console.log('sample 440 >> 12', currentTime()); audio.replay(id,1.0,0.0,12);
	}, 2200);
	setTimeout(function() {
		console.log('sound 880', currentTime()); audio.sound('sin', 880, 0.5);
	}, 3300);
});

app.on('update', function(deltaT, now) {
	if(now>4.4)
		audioUpdate(now);
});
