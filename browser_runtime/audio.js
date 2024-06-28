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
				const arr = buffer.getChannelData(0);
				for(let i=0; i<arr.length; ++i)
					if(arr[i]>0.002 || arr[i]<-0.002) {
						sample.offset = i/buffer.sampleRate;
						//console.log(sample.url, 'offset:', sample.offset);
						break;
					}
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

function createNoiseSource(freq=2093) {
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
	if(freq>0)
		whiteNoise.playbackRate.value = freq/2093;
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
		return createNoiseSource(freq);
	
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

	let gainNode = audioCtx.createGain();
	gainNode.gain.value = gain;
	source.connect(gainNode);
	let pred = gainNode;

	if(pan) {
		let panNode;
		if (audioCtx.createStereoPanner) {
			panNode = audioCtx.createStereoPanner();
			panNode.pan.value = pan;
		}
		else {
			panNode = audioCtx.createPanner();
			panNode.panningModel = 'equalpower';
			panNode.setPosition(pan, 0, 1 - Math.abs(pan));
		}
		pred.connect(panNode);
		pred = panNode;
	}
	pred.connect(masterVolume);
	return gainNode;
}

function note2freq(note, accidental, octave) {
	if(typeof note === 'number')
		return note;
	if(note[0]>= '0' && note[0] <= '9')
		return parseFloat(note);

	let steps;
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
	const base = 27.5 * Math.pow(2.0, octave), step = Math.pow(2.0, 1.0/12.0);
	return base * Math.pow(step, steps);
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

	function readFloatAt(melody, pos, p, key) {
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

	function readSubsequentFloat(melody, pos, p, key) {
		++pos;
		if(melody.charAt(pos)===':')
			++pos;
		while(/\s/.test(melody.charAt(pos)))
			++pos;
		return readFloatAt(melody, pos, p, key);
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

	function readNote(melody, pos, note) {
		for(let end=melody.length; pos<end; ++pos) {
			let ch = melody.charAt(pos);
			if(/\s/.test(ch))
				continue;

			let freq = -1;
			if((ch >= '0' && ch <= '9')||ch==='.') {
				let obj = {};
				pos = readFloatAt(melody, pos, obj, 'v');
				freq = obj.v;
				if(++pos>=melody.length)
					return;
				ch = melody.charAt(pos);
			}
			else {
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
				freq = note2freq(noteCh, accidental, octave);
			}

			if(ch!='/' && ch!='*')
				return console.error("invalid duration signifier at pos", pos);
			const isDivision = ch=='/';

			let obj = {};
			pos = readSubsequentFloat(melody, pos, obj, 'v');
			if(('v' in obj) && obj.v>0.0) {
				note.push(freq);
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
	this.volume = function(vol) {
		melodyVolume.gain.value = vol;
	}
}


return {
	resume: function() {
		if(audioCtx.state !== 'running')
			audioCtx.resume();
	},
	suspend: function() {
		audioCtx.suspend();
	},
	load: function(url, params, callback) {
		if(Array.isArray(url)) {
			let samples = [];
			for(let i=0; i<url.length; ++i)
				samples.push(this.load(url[i], params, callback));
			return samples; 
		}
		let sample = { id:samples.length+1, ready:false, url:url, buffer:null, offset:0 };
		samples.push(sample);
		loadSample(url, sample.id, callback);
		return sample.id;
	},
	replay: function(id, gain=1.0, pan=0, detune=0, deltaT=0, loop=false) {
		if(Array.isArray(id))
			id = id[Math.floor(Math.random()*id.length)];
		if(id===0 || id>samples.length)
			return;
		const sample = samples[id-1];
		if(!sample.ready || !sample.buffer)
			return;
		let trackId = findAvailableTrack();
		if(trackId === numTracksMax)
			return 0xffffffff;
	
		let source = audioCtx.createBufferSource();
		if(detune!==0)
			source.playbackRate.value = Math.pow(2, detune/12);
		source.buffer = sample.buffer;
		source.loop = loop;
		tracks[trackId] = { src:source, gain:connectSource(source, gain, pan) };
		source.start(audioCtx.currentTime + deltaT, sample.offset || 0);
		source.addEventListener('ended', ()=>{ tracks[trackId]=null; })
		return trackId;
	},
	loop: function(id, gain=1.0, pan=0, detune=0, deltaT=0) {
		return this.replay(id, gain, pan, detune, deltaT, true);
	},
	stop: function(track) {
		if(track===undefined) for(let i=0; i<numTracksMax; ++i)
			if(tracks[i]!==null) {
				let source = tracks[i].src;
				source.stop();
				tracks[i] = null;
			}

		let tr = tracks[track];
		if(!tr)
			return;
		if('src' in tr)
			tr.src.stop();
		else
			tr.stop();
		tracks[track] = null;
	},
	volume: function(arg0, arg1) {
		if(arg0===undefined)
			return masterVolume.gain.value;
		if(arg1===undefined) {
			masterVolume.gain.value = arg0;
			return;
		}
		let tr = tracks[arg0];
		if(!tr)
			return;
		if('gain' in tr)
			tr.gain.gain.value = arg1;
		else
			tr.volume(arg1);
	},
	fadeOut: function(track, duration) {
		let tr = tracks[track];
		if(!tr)
			return;
		if('gain' in tr) {
			tr.gain.gain.linearRampToValueAtTime(0, audioCtx.currentTime + duration);
			tr.src.stop(audioCtx.currentTime + duration);
		}
	},
	playing: function(track) {
		if(audioCtx.state !== 'running')
			return false;
		if(track===undefined) {
			for(let i=0; i<numTracksMax; ++i)
				if(tracks[i]!==null)
					return true;
			return false;
		}
		let tr = tracks[track];
		return tr ? true : false;
	},
	sound: function(wave, freq, duration, vol=1.0, balance=0.0) {
		let source = createSource(wave.substr(0,3).toLowerCase(), this.note2freq(freq));

		let trackId = findAvailableTrack();
		if(trackId === numTracksMax)
			return 0xffffffff;
		tracks[trackId] = { src:source, gain:connectSource(source, vol, balance) };
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
	uploadPCM: function(data, numChannels=1, offset = 0) {
		if(typeof data == 'object' && data.samples) {
			numChannels = data.channels || 1;
			offset = data.offset || 0;
			data = data.samples;
		}
		var buffer = audioCtx.createBuffer(numChannels, data.length/numChannels, audioCtx.sampleRate);
		for(let j=0; j<numChannels; ++j) {
			var channel = buffer.getChannelData(j);
			for (let i = 0, end=data.length/numChannels; i < end; ++i)
				channel[i] = data[i*numChannels+j];
		}
		samples.push({ id:samples.length+1, ready:true, url:'', buffer:buffer, offset:offset });
		return samples.length;
	},
	release: function(id) {
		if(id===0 || id>samples.length)
			return;
		const sample = samples[id-1];
		sample.ready = false;
		sample.buffer = null;
	},
	sampleRate: audioCtx.sampleRate,
	tracks: numTracksMax,

	createSoundBuffer: function(/*arguments*/) {
		const sampleRate = audio.sampleRate;
		const PI2 = 2.0 * Math.PI;

		function oscSin(phase, timbre) {
			if(timbre===undefined || timbre>=1)
				return Math.sin(PI2 * phase);
			const s = Math.sin(PI2* phase);
			return Math.sign(s) * Math.abs(s)**timbre;
		}
		function oscSqu(phase, timbre) { return (phase%1 < timbre) ? 1.0 : -1.0; }
		function oscSaw(phase, timbre) {
			const timbre2 = timbre/2, x = phase%1, x2=1-timbre2;
			return (x<timbre2) ? x/timbre2 :
				(x<x2) ? 1-2*(x-timbre2)/(1-timbre) :
				-1+(x-x2)/timbre2;
		}
		function oscNoi(phase, timbre) {
			const waveLen = Math.ceil(sampleRate/20000/timbre);
			if(--oscNoi.counter<0) {
				oscNoi.counter = waveLen;
				oscNoi.amplPrev = oscNoi.amplitude;
				oscNoi.amplitude = Math.random()*2 - 1.0;
			}
			return oscNoi.amplitude * (1-oscNoi.counter/waveLen) + oscNoi.amplPrev * (oscNoi.counter/waveLen);
		}
		oscNoi.counter = -1;
		oscNoi.amplitude = oscNoi.amplPrev = 0;

		function oscBin(phase, timbre) { // binary noise
			const freq = timbre*20000;
			if(--oscBin.counter < 0) {
				oscBin.counter = Math.ceil(Math.random()*sampleRate/freq);
				oscBin.amplitude = -oscBin.amplitude;
			}
			return oscBin.amplitude;
		}
		oscBin.counter = -1;
		oscBin.amplitude = 1;

		function defaultTimbre(osc) {
			switch(osc) {
			case oscSqu: return 0.5;
			case oscBin: return 0.5;
			default: return 1.0;
			}
		}

		function chirp(osc, buffer, pos, phase, freq1, freq2, vol1, vol2, timbre1, timbre2, duration) {
			const dvol = vol2-vol1, dfreq = freq2-freq1, dtimbre = timbre2-timbre1;
			const len =sampleRate*duration, dt=1/sampleRate;
			for(var i=0, t=0, dest=Math.floor(pos*sampleRate); i<len; ++i, ++dest, t+=dt) {
				const rel = i/len;
				const vol = vol1 + dvol*rel, freq=freq1 + dfreq*rel, timbre=timbre1 + dtimbre*rel;
				buffer[dest] = Math.max(-1, Math.min(1, osc(phase, timbre)*vol));
				phase += freq*dt;
			}
			return phase;
		}

		if(arguments.length===1 && Array.isArray(arguments[0]))
			arguments = arguments[0];
		var osc;
		switch(arguments[0].slice(0,2)) {
			case 'si': osc = oscSin; break;
			case 'sa': osc = oscSaw; break;
			case 'sq': osc = oscSqu; break;
			case 'bi': osc = oscBin; break;
			case 'no': osc = oscNoi; break;
			case 'tr': osc = oscSaw; break;
			default: throw 'invalid wave form name '+JSON.stringify(arguments[0]);
		}

		var totalDuration = 0;
		for(var i=3; i<arguments.length; i+=4)
			totalDuration += arguments[i];
	
		var buffer = new Float32Array(Math.ceil(totalDuration*sampleRate)), pos=0, phase=0;
		var freq1, vol1, timbre1;
		for(var i=1; i<arguments.length; i+=4) {
			const freq2 = note2freq(arguments[i]);
			const vol2 = arguments[i+1] || 0, duration = arguments[i+2];
			const timbre2 = (typeof arguments[i+3]==='number') ? arguments[i+3] : defaultTimbre(osc);
			if(duration) {
				if(freq1===undefined)
					freq1 = freq2;
				if(vol1===undefined)
					vol1 = vol2;
				if(timbre1===undefined)
					timbre1 = timbre2;
				phase = chirp(osc, buffer, pos, phase, freq1, freq2, vol1, vol2, timbre1, timbre2, duration);
				pos += duration;
			}
			freq1 = freq2;
			vol1 = vol2;
			timbre1 = timbre2;
		}
		return buffer;
	},
	mixToBuffer: function(stereoBuffer, sample, startTime, volume, balance) {
		if(typeof sample === 'number')
			sample = this.sampleBuffer(sample);
		const stereoBufLen = Math.floor(stereoBuffer.length/2), sampleLen = sample.length;

		const volL = volume*(-0.4*balance+0.6), volR = volume*(+0.4*balance+0.6);
		for(var frame = Math.round(startTime*audio.sampleRate), pos = 0; frame<stereoBufLen && pos<sampleLen; ++frame, ++pos) {
			stereoBuffer[frame*2] += sample[pos] * volL;
			stereoBuffer[frame*2+1] += sample[pos] * volR;
		}
	},
	clampBuffer: function(buffer, minValue, maxValue) {
		for(var i=0, end=buffer.length; i<end; ++i)
			buffer[i] = Math.min(Math.max(buffer[i], minValue), maxValue);
	},
	sampleBuffer: function(id) {
		if(id>0 && id<=samples.length)
			return samples[id-1].buffer;
	},
	note2freq: function(note) {
		if(typeof note === 'number')
			return note;

		const noteCh = note.charAt(0);
		if(noteCh >= '0' && noteCh <= '9')
			return parseFloat(note);
		if(!(noteCh in {'-':true, 'A':true, 'B':true, 'C':true, 'D':true, 'E':true, 'F':true, 'G':true, 'H':true}))
			return console.error("invalid note", noteCh);

		const ch = note.charAt(1);
		const accidental = (ch == '#' || ch=='b') ? ch : ' ';

		const octaveCh = note.charAt((accidental!=' ') ? 2 : 1);
		if(noteCh!='-' && !/\d/.test(octaveCh))
			return console.error("invalid octave", octaveCh);
		const octave = octaveCh.charCodeAt(0) - '0'.charCodeAt(0);

		return note2freq(noteCh, accidental, octave);
	},
	createSound: function(...args) {
		return this.uploadPCM(this.createSoundBuffer(args));
	}
}

})();
