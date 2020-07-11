arcajs.audio = (function() {

let audioCtx = window.AudioContext ? new AudioContext() : new webkitAudioContext();
const numTracksMax = 16;
let samples = [], tracks=[];
for(let i=0; i<numTracksMax; ++i)
	tracks.push(null);
let noiseBuffer = null;

let masterVolume = audioCtx.createGain();
masterVolume.gain.value = 1.0;
masterVolume.connect(audioCtx.destination);

/// loads sample asynchronously
function loadSample(url, id, callback) {
	let request = new XMLHttpRequest();
	request.open("GET", url, true);
	request.responseType = "arraybuffer";

	request.onload = ()=>{
		// asynchronously decode the audio file data in request.response
		audioCtx.decodeAudioData(
			request.response,
			(buffer)=>{
				if(!buffer)
					return console.error('error decoding file data:', id);
				let sample = samples[id-1];
				sample.buffer = buffer;
				sample.ready = true;
				if(callback)
					callback(sample);
			},
			(error)=>{
				console.error('decodeAudioData error', id, error);
			}
		);
	}
	request.onerror = ()=>{
		console.error('loadSample: XHR error', url);
	}
	request.send();
}

function createNoiseSource() {
	if(noiseBuffer===null) {
		const bufferSize = 2 * audioCtx.sampleRate;
		noiseBuffer = audioCtx.createBuffer(1, bufferSize, audioCtx.sampleRate);
		output = noiseBuffer.getChannelData(0);
		for (let i = 0; i < bufferSize; i++)
			output[i] = Math.random() * 2 - 1;
	}

	let whiteNoise = audioCtx.createBufferSource();
	whiteNoise.buffer = noiseBuffer;
	whiteNoise.loop = true;
	return whiteNoise;
}

function findAvailableTrack() {
	let trackId=0;
	for( ; trackId<numTracksMax; ++trackId)
		if(tracks[trackId]===null)
			return trackId;
	return numTracksMax;
 }

 function createSource(wave, freq) {
	 if(wave==='noi')
	 	return createNoiseSource();
	
	let source = audioCtx.createOscillator();
	if (!source.start)
		source.start = source.noteOn;

	switch(wave) {
	case 'saw':
		source.type = 'sawtooth'; break;
	case 'sin':
		source.type = 'sine'; break;
	case 'squ':
	case 'sqr':
		source.type = 'square'; break;
	case 'tri':
		source.type = 'triangle'; break;
	}
	source.frequency.value = freq;
	return source;
}

function setEnvelope(sourceNode, gainNode, params, duration, when=0) {
	const volume = params.gain;
	duration *= params.beatLen;
	sourceNode.start(when);

	if(params.attack>0) {
		gainNode.gain.setValueAtTime(0, when);
		gainNode.gain.linearRampToValueAtTime(volume, when + params.attack);
	}
	else
		gainNode.gain.setValueAtTime(volume, when);
	if(params.sustain<1) {
		gainNode.gain.setValueAtTime(volume, when + params.attack + params.hold);
		gainNode.gain.linearRampToValueAtTime(params.sustain*volume, when + params.attack + params.hold + params.decay);
	}
	if(params.release>0) {
		gainNode.gain.linearRampToValueAtTime(0, when + duration + params.release);
		if('frequency' in sourceNode)
			sourceNode.frequency.setValueAtTime( 0, when + duration + params.release );
	}
	else if('frequency' in sourceNode)
		sourceNode.frequency.setValueAtTime( 0, when + duration );
	return duration;
}

function connectSource(source, gain, pan) {
	if (!source.start)
		source.start = source.noteOn;

	let pred = source;

	if(gain!=1.0) {
		let gainNode = audioCtx.createGain();
		gainNode.gain.value = gain;
		pred.connect(gainNode);
		pred = gainNode;
	}
	if(pan) {
		let panNode = audioCtx.createStereoPanner();
		panNode.pan.value = pan;
		pred.connect(panNode);
		pred = panNode;
	}
	pred.connect(masterVolume);
}

function Melody(melody) {

	function initParams() {
		return {
			waveForm:'sqr',
			attack: 0.0,
			hold: 0.0,
			decay: 0.0,
			sustain: 1.0,
			release: 0.0,
			gain: 1.0,
			beatLen: 4.0*60.0/72.0 // 72 bpm
		};
	}

	function readSubsequentFloat(melody, pos, p, key) {
		++pos;
		if(melody.charAt(pos)===':')
			++pos;
		while(/\s/.test(melody.charAt(pos)))
			++pos;

		let sign = 1;
		if(melody.charAt(pos) in {'-':true, '+':true}) {
			if(melody.charAt(pos)==='-')
				sign = -1;
			++pos;
		}
		let f=NaN, sawSign = false, dec=0.1;
		while(pos<melody.length) {
			const ch = melody.charAt(pos);
			if(ch==='.') {
				if(sawSign)
					break;
				sawSign = true;
				++pos;
				continue;
			}
			if(!/\d/.test(ch))
				break;
			if(Number.isNaN(f))
				f = 0;
			if(!sawSign)
				f=f*10+Number(ch);
			else {
				f+=dec*Number(ch);
				dec/=10;
			}
			++pos;
		}
		f *= sign;
		if(Number.isNaN(f))
			++pos;
		else {
			p[key]=f;
			--pos;
		}
		return pos;
	}

	function readParams(melody, pos, p) {
		for(let end=melody.length; pos<end; ++pos) {
			const ch = melody.charAt(pos);
			if(/\s/.test(ch))
				continue;
			if(ch==='}')
				break;
			switch(ch) {
			case 'a':
				pos = readSubsequentFloat(melody, pos, p, 'attack');
				break;
			case 'b': {
				pos = readSubsequentFloat(melody, pos, p, 'beatLen');
				if('beatLen' in p)
					p.beatLen = 4.0*60.0/p.beatLen;
				break;
			}
			case 'd':
				pos = readSubsequentFloat(melody, pos, p, 'decay');
				break;
			case 'g':
				pos = readSubsequentFloat(melody, pos, p, 'gain');
				break;
			case 'h':
				pos = readSubsequentFloat(melody, pos, p, 'hold');
				break;
			case 'r':
				pos = readSubsequentFloat(melody, pos, p, 'release');
				break;
			case 's':
				pos = readSubsequentFloat(melody, pos, p, 'sustain');
				break;
			case 'w': // waveForm
				++pos;
				if(melody.charAt(pos) === ':')
					++pos;
				if(melody.substr(pos, 3) in {"sin":true, "tri":true, "sqr": true, "saw":true, "noi":true})
					p.waveForm = melody.substr(pos, 3);
				pos += 3;
				break;
			}
		}
		return pos;
	}

	function transposeFreq(base, n) {
		const step = 1.0594630943592952646; // pow(2.0, 1.0/12.0);
		return base * Math.pow(step, n);
	}

	function note2freq(note, accidental, octave) {
		let steps;
		switch(note) {
		case 'B':
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

	function readNote(melody, pos, note) {
		for(let end=melody.length; pos<end; ++pos) {
			let ch = melody.charAt(pos);
			if(/\s/.test(ch))
				continue;

			if(!(ch in {'-':true, 'A':true, 'B':true, 'C':true, 'D':true, 'E':true, 'F':true, 'G':true}))
				return console.error("invalid note", ch, "at pos", pos);
			const noteCh = ch;
			if(++pos>=melody.length)
				return;
			ch = melody.charAt(pos);
			const accidental = (ch == '#' || ch=='b') ? ch : ' ';
			if(accidental!=' ') {
				if(++pos>=melody.length)
					return;
				ch = melody.charAt(pos);
			}
			if(noteCh!='-' && !/\d/.test(ch))
				return console.error("invalid octave", ch, "at pos", pos);
			const octave = ch.charCodeAt(0) - '0'.charCodeAt(0);
			if(noteCh!='-') {
				if(++pos>=melody.length)
					return;
				ch = melody.charAt(pos);
			}
			if(ch!='/' && ch!='*')
				return console.error("invalid duration signifier at pos", pos);
			const isDivision = ch=='/';

			let obj={};
			pos = readSubsequentFloat(melody, pos, obj, 'v');
			if(('v' in obj) && obj.v>0.0) {
				note.push(note2freq(noteCh, accidental, octave));
				note.push(isDivision ? 1.0/obj.v : obj.v);
			}
			break;
		}
		return pos;
	}

	function read(melody) {
		let notes = [];
		for(let pos=0, end=melody.length; pos<end; ++pos) {
			const ch = melody.charAt(pos);
			if(/\s/.test(ch))
				continue;
			if(ch=='{') {
				let p={};
				pos = readParams(melody, pos+1, p);
				if(Object.keys(p).length !== 0)
					notes.push(p);
				continue;
			}
			let note = [];
			pos = readNote(melody, pos, note);
			if(note.length===2)
				notes.push(note);
			else break;
		}
		return notes;
	}

	function scheduleNote(output, freq, duration, params, deltaT=0, balance=0.0) {
		let source = createSource(params.waveForm, freq);
		let gainNode = audioCtx.createGain();
		source.connect(gainNode);
		deltaT += setEnvelope(source, gainNode, params, duration, audioCtx.currentTime + deltaT);
		if(!balance)
			gainNode.connect(output);
		else {
			let panNode = audioCtx.createStereoPanner();
			panNode.pan.value = balance;
			gainNode.connect(panNode);
			panNode.connect(output);
		}
		return deltaT;
	}

	let notes = read(melody);
	let melodyVolume = audioCtx.createGain();

	this.replay = function(vol=1.0, balance=0.0) {
		let params = initParams();
		let t=0;
		melodyVolume.gain.value = vol;
		melodyVolume.connect(masterVolume);
		for(let i=0; i<notes.length; ++i) {
			let note = notes[i];
			if(Array.isArray(note))
				t = scheduleNote(melodyVolume, note[0], note[1], params, t, balance);
			else if(typeof note === 'object')
				for(let key in note)
					params[key] = note[key];
		}
		return t;
	}
	this.stop = function() {
		melodyVolume.disconnect();
	}
}


return {
	load: function(url, params, callback) {
		let sample = { id:samples.length+1, ready:false, url:url, buffer:null };
		samples.push(sample);
		loadSample(url, sample.id, callback);
		return sample.id;
	},
	replay: function(id, gain=1.0, pan=0, deltaT=0) {
		if(id===0 || id>samples.length)
			return;
		const sample = samples[id-1];
		if(!sample.ready || !sample.buffer)
			return;
		let trackId = findAvailableTrack();
		if(trackId === numTracksMax)
			return 0xffffffff;
	
		let source = tracks[trackId] = audioCtx.createBufferSource();
		source.buffer = sample.buffer;
		connectSource(source, gain, pan);
		source.start(audioCtx.currentTime + deltaT);
		source.addEventListener('ended', ()=>{ tracks[trackId]=null; })
		return trackId;
	},
	stop: function(track) {
		if(track===undefined) for(let i=0; i<numTracksMax; ++i)
			if(tracks[i]!==null) {
				let source = tracks[i];
				source.stop();
				source = null;
			}

		let source = tracks[track];
		if(!source)
			return;
		source.stop();
		source = null;
	},
	volume: function(v) {
		if(v===undefined)
			return masterVolume.gain.value;
		masterVolume.gain.value = v;
	},
	playing: function(track) {
		if(track===undefined) {
			for(let i=0; i<numTracksMax; ++i)
				if(tracks[i]!==null)
					return true;
			return false;
		}
		let source = tracks[track];
		return source ? true : false;
	},
	sound: function(wave, freq, duration, vol=1.0, balance=0.0) {
		let source = createSource(wave.substr(0,3).toLowerCase(), freq);

		let trackId = findAvailableTrack();
		if(trackId === numTracksMax)
			return 0xffffffff;
		tracks[trackId] = source;

		connectSource(source, vol, balance);
		source.addEventListener('ended', ()=>{ tracks[trackId]=null; })
		source.start();
		source.stop(audioCtx.currentTime + duration);
	},
	melody: function(melody, vol=1.0, balance=0.0) {
		let trackId = findAvailableTrack();
		if(trackId === numTracksMax)
			return 0xffffffff;

		let m = tracks[trackId] = new Melody(melody);
		let duration = m.replay(vol, balance);
		setTimeout(()=>{ tracks[trackId].stop(); tracks[trackId]=null; }, duration*1000);
	},
	sample: function(data) {
		let arr = Array.isArray(data) ? new Float32Array(data) : data;
		samples.push({ id:samples.length+1, ready:true, url:'', buffer:arr.buffer });
		return samples.length;
	},
	sampleRate: audioCtx.sampleRate
}

})();
