var audio = app.require('audio');
console.visible(true);

var patt1_1 = "{w:saw a:.025 d:.025 s:.25 r:.05 b:65}"
	+ " E2/8 B2/8 B2/8 E2/8 B2/8 B2/8 E2/8 B2/8"
	+ " C2/8 G2/8 G2/8 C2/8 G2/8 G2/8 C2/8 G2/8"
	+ " D2/8 A2/8 A2/8 D2/8 A2/8 A2/8 G2/8 F#2/8";
var track1 = -1;
var samplePool = [];

var cowbell = audio.createSoundBuffer(
	'sin', 'G#5',0,0,0.125, 'G#5',0.8,0.003,0.125, 'G#5',0.2,0.02,0.25, 'G#5',0.05,0.1,0.5, 'G#5',0,0.15,1);
for(var i=0, end=cowbell.length; i<end; ++i)
	cowbell[i] *= 0.85+Math.random()*0.3;
cowbell = audio.uploadPCM(cowbell);
console.log('cowbell', cowbell);

function audioUpdate(now) {
	if(!audio.playing(track1)) {
		track1 = audio.melody(patt1_1, 0.25, 0.0);
	}
}

function currentTime() {
	return (new Date()).toISOString().replace(/[A-Z]/g, ' ');
}

function createSinSample(freq, duration) {
	const sampleRate = audio.sampleRate;
	const numSamples = Math.floor(sampleRate * duration);
	var sample = new Float32Array(numSamples);
	for(var i=0; i<numSamples; ++i)
		sample[i]=Math.sin(freq*i*2*Math.PI/sampleRate);
	return audio.uploadPCM(sample);
}

app.on('load', function() {
	const sampleRate = audio.sampleRate;
	console.log('load', currentTime(), 'sampleRate', sampleRate);
	var id = createSinSample(440, 1);
	for(var i=1; i<5; ++i)
		samplePool.push(createSinSample(i*200, 0.5));

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

app.on('keyboard', function(evt) {
	if(evt.type!=='keydown' || evt.repeat)
		return;
	switch(evt.key) {
	case 'r':
		return audio.replay(samplePool,0.25);
	case 'c':
		return audio.replay(cowbell);
	}
});
