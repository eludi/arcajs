const audio = app.require('audio');

plot = [];

(function(audio) {
	function transposeFreq(base, n) {
		const step = 1.0594630943592952646; // pow(2.0, 1.0/12.0);
		return base * Math.pow(step, n);
	}

	function note2freq(note) {
		if(typeof note === 'number')
			return note;
		if(note[0]>= '0' && note[0] <= '9')
			return parseFloat(note);

		var octave = parseInt(note[note.length-1]);
		const accidental = note.length>2 ? note[1] : '';
		note = note[0].toUpperCase()

		var steps;
		switch(note) {
		case 'B':
		case 'H':
			steps = 2; break;
		case 'C':
			steps = 3; break;
		case 'D':
			steps = 5; break;
		case 'E':
			steps = 7; break;
		case 'F':
			steps = 8; break;
		case 'G':
			steps = 10; break;
		case '-':
			return 0.0;
		default:
			steps = 0;
		}

		if(accidental == 'b')
			--steps;
		else if(accidental == '#')
			++steps;

		if(steps>2)
			--octave;
		const base = 27.5 * Math.pow(2.0, octave);
		return transposeFreq(base, steps);
	}

	function SoundGenerator(sounds) {
		var cache = {};
		this.getSample = function(instrument, note, duration) {
			const key = instrument+'/'+note;
			if(key in cache)
				return cache[key];
			const sound = sounds[instrument];
			if(Array.isArray(sound)) {
				var args = sound.slice(0); // shallow copy
				const baseFreq = note2freq(note);
				for(var i=1; i<args.length; i+=4)
					args[i] *= baseFreq;
				const sample = audio.createSoundBuffer.apply(audio, args);
				cache[key] = sample;
				return sample;
			}
			else if(typeof sound === 'object' && note in sound) {
				const buffer = audio.createSoundBuffer.apply(audio, sound[note]);
				cache[key] = buffer;
				return buffer;
			}
			throw "invalid instrument "+instrument+" or note "+note;
		}
	}

	/// a simple yet blazingly fast music generator
	audio.createMusicResource = function(song, soundGenerator) {
		const
			beatLen = 60/song[0],
			sounds = song[1],
			phrases = song[2],
			patterns = song[3],
			sequence = song[4];
		if(soundGenerator === undefined)
			soundGenerator = new SoundGenerator(sounds);

		function Track(sound, phrase, volume, balance, repetitions) {
			if(typeof phrase === 'number')
				phrase = phrases[phrase];
			if(typeof phrase === 'string')
				phrase = phrase.trim().split(/[\s,]+/);

			this.mixToBuffer = function(stereoBuffer, startTime) {
				var deltaT = 0;
				for(var j=0; j<repetitions; ++j) {
					for(var i=0; i<phrase.length; ++i) {
						const note = phrase[i].split('/');
						const duration = beatLen * 4 / parseFloat(note[1]);
						if(note[0]!='-') {
							const sample = soundGenerator.getSample(sound, note[0], duration);
							const volSc = (note[2]!==undefined) ? parseFloat(note[2]) : 1;
							audio.mixToBuffer(stereoBuffer,sample, startTime+deltaT, volume*volSc, balance)
						}
						deltaT += duration;
					}
				}
			}
			this.calculateDuration = function() {
				var duration = 0;
				for(var i=0; i<phrase.length; ++i) {
					const note = phrase[i].split('/');
					duration += beatLen * 4 / parseFloat(note[1]);
				}
				return repetitions * duration;
			}
		}

		function calculateDuration() {
			var songDuration = 0;
			for(var j=0; j<sequence.length; ++j) {
				const pattern = patterns[sequence[j]];
				var patternDuration = 0;
				for(var i=0; i<pattern.length; ++i) {
					const tr = pattern[i];
					const repetitions = tr.length>4 ? tr[4] : 1;
					var track = new Track(tr[0], tr[1], 0, 0, repetitions);
					patternDuration = Math.max(patternDuration, track.calculateDuration());
				}
				songDuration += patternDuration;
			}
			return songDuration;
		}

		var buffer = new Float32Array(2 * Math.ceil(audio.sampleRate * calculateDuration()));

		for(var j=0, startTime=0; j<sequence.length; ++j) {
			const pattern = patterns[sequence[j]];
			var patternDuration = 0;
			for(var i=0; i<pattern.length; ++i) {
				const tr = pattern[i];
				const volume = tr.length>2 ? tr[2] : 1.0;
				const balance = tr.length>3 ? tr[3] : 0.0;
				const repetitions = tr.length>4 ? tr[4] : 1;
				var track = new Track(tr[0], tr[1], volume, balance, repetitions);
				track.mixToBuffer(buffer, startTime);
				patternDuration = Math.max(patternDuration, track.calculateDuration());
			}
			startTime += patternDuration;
		}
		audio.clampBuffer(buffer, -1, 1);
		return audio.uploadPCM(buffer, 2);
	}
})(app.require('audio'));

//------------------------------------------------------------------
const song = [
	100, // bpm
	{ // instruments
		drums: {
			bd: ['sin', 220,1.20,0,1, 5,0,0.2,1], // base drum
			tm: ['squ', 200,0,0,0.5, 200,0.8,0.005,0.5, 80,0.6,0.08,0.5, 2,0,0.115,0.5], // tom
			sh: ['noi', 1,0.3,0,0.8, 1,0.1,0.1,0.2, 1,0,0.3,0.1], // snare high
			sl: ['noi', 1,0.3,0,0.4, 1,0.1,0.1,0.2, 1,0,0.3,0.1], // snare low
			cb: ['sin', 'G#5',0,0,0.125, 'G#5',0.8,0.003,0.125, 'G#5',0.2,0.02,0.25, 'G#5',0.05,0.1,0.5, 'G#5',0,0.15,1], // cowbell
		},
		bass: ['saw', 1,0,0,1, 1,0.8,0.01,1, 1,0.8,0.02,1, 1,0.0,0.3,0.85],
		bassL: ['saw', 1,0,0,1, 1,0.8,0.01,1, 1,0.8,0.03,1, 1,0.0,1.5,0.75],
		lead: ['squ', 1,0.8,0.02,0.25, 1,0,0.48,0.75],
	},
	[ // phrases
		'A4/8 A3/8 E4/8 A3/8 D4/8 A3/8 E4/8 A3/8 ' + 'G4/8 A3/8 E4/8 A3/8 B4/8 A3/8 C5/8 A3/8 ' +
			'A4/8 A3/8 E4/8 A3/8 D4/8 A3/8 E4/8 A3/8 ' + 'C4/8 A3/8 D4/8 A3/8 B4/8 A3/8 G4/8 A3/8',
		"A1/8 ".repeat(8),
		"G1/8 ".repeat(8),
		"F1/8 ".repeat(8),
		"F1/8 ".repeat(16) + "G1/8 ".repeat(16),
	],
	[ // patterns
		[['drums', "bd/4 bd/4 bd/4 bd/4", 1.0, 0, 2], ['bass', 1, 0.3, -0.5, 2]],
		[['drums', "bd/4 bd/4 bd/4 bd/4", 1.0, 0, 8], ['bass', 1, 0.3, -0.5, 8], ['lead', 0, 0.2, 0.5, 2],
			['drums', "-/1 -/1 -/1 -/1 -/1 -/1 -/1 -/2 -/4 -/8 sh/16 sh/16", 1.0, 0.5]],
		[['drums', "bd/4 sh/4 bd/4 sh/4", 1.0, 0, 4], ['bass', 1, 0.3, -0.5, 4], ['lead', 0, 0.2, 0.5]],
		[['drums', "bd/4 sh/4 bd/4 sh/4", 1.0, 0, 4], ['bass', 3, 0.3, -0.5, 4], ['lead', 0, 0.2, 0.5]],
		[['drums', "bd/4 sh/4 bd/4 sh/4", 1.0, 0, 4], ['bass', 2, 0.3, -0.5, 4], ['lead', 0, 0.2, 0.5]],
		[['bass', 1, 0.3, -0.5, 4], ['lead', 0, 0.2, 0.5]],
		[['bass', 4, 0.3, -0.5], ['lead', 0, 0.2, 0.5]],
		[['bass', 1, 0.3, -0.5, 2], ['lead', 0, 0.2, 0.5]],
		[['drums', "bd/1", 1.0], ['bassL', "A1/1", 0.3, -0.5]],
	],
	[0, 1, 2, 3, 2, 3, 4, 5, 6, 7, 8], // sequence
	{ // metadata
		title: 'Elsewehere',
		author: 'Jim Hall',
		license: 'CC BY 4.0'
	}
];

