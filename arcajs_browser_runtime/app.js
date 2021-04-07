"use strict";

let app = arcajs.app = (function(canvas_id='arcajs_canvas') {
	let clearColor = [ 0,0,0 ];
	let eventListeners = {}, nextListeners = null, gamepadListener = null;
	let resources = {};
	let resourcesLoading = 0;
	let modules = { audio:arcajs.audio, intersects:arcajs.intersects };
	let gamepads = [], gamepadResolution = 0.1;
	let tLastFrame=0;

	if(!('visible' in window.console))
		window.console.visible = function() {}; // dummy

	const canvas = document.getElementById(canvas_id);
	let gfx = new arcajs.Graphics(canvas);

	function createPathResource(width, height, path, fill=[255,255,255,255], lineW=0, stroke=[255,255,255,255]) {
		let canvas = document.createElement('canvas');
		canvas.width = width;
		canvas.height = height;
		let ctx = canvas.getContext('2d');

		ctx.lineWidth = lineW;
		ctx.fillStyle = fill ?
			'rgba('+fill[0]+','+fill[1]+','+fill[2]+','
			+((typeof fill[3]==='number') ? fill[3]/255 : 1)+')' : 'transparent';
		ctx.strokeStyle = lineW ?
			'rgba('+stroke[0]+','+stroke[1]+','+stroke[2]+','
			+((typeof stroke[3]==='number') ? stroke[3]/255 : 1)+')' : 'transparent';

		let p = new Path2D(path);
		if(fill)
			ctx.fill(p);
		if(lineW)
			ctx.stroke(p);

		const imgData = new Uint8Array(ctx.getImageData(0,0, width, height).data.buffer);
		return gfx.createTexture(width, height, imgData);
	}

	function createSVGResource(svgStr, params) {
		if(!svgStr.startsWith('<svg xmlns="http://www.w3.org/2000/svg"'))
			svgStr = '<svg xmlns="http://www.w3.org/2000/svg"' + svgStr.substr(4);
		let blob = new Blob([svgStr], {type: 'image/svg+xml'});
		return gfx.loadTexture(URL.createObjectURL(blob), params);
	}

	function getGamepad(index) {
		let gp = gamepads[index];
		if(!gp || !gp.connected)
			return { index:index, connected:false };
		let state = navigator.getGamepads()[index];
		let ret = { index:index, connected:true, buttons:[], axes:[] };
		for(let i=0; i<state.buttons.length; ++i)
			ret.buttons[i] = state.buttons[i].pressed;
		for(let i=0; i<state.axes.length; ++i)
			ret.axes[i] = state.axes[i];
		return ret;
	}

	function pollGamepads() {
		for(let index=0, end=gamepads.length; index<end; ++index) {
			const gp = gamepads[index];
			if(!gp || !gp.connected)
				continue;
			let state = getGamepad(index);
			if(gp.state!==null) {
				for(let i=0; i<state.buttons.length; ++i)
					if(state.buttons[i] != gp.state.buttons[i])
						app.emit('gamepad',
							{ type:'button', index:index, button:i, value:state.buttons[i]?1:0 });
				for(let i=0; i<state.axes.length; ++i)
					if(Math.round(state.axes[i]/gamepadResolution) != Math.round(gp.state.axes[i]/gamepadResolution)) {
						if(state.axes[i]<gamepadResolution*0.5 && state.axes[i]>-gamepadResolution*0.5)
							state.axes[i] = 0.0;
						app.emit('gamepad',
							{ type:'axis', index:index, axis:i, value:state.axes[i] });
					}
			}
			gp.state = state;
		}
	}

	let app = {
		version: 'v0.20210402a',
		platform: 'browser',
		width: window.innerWidth,
		height: window.innerHeight,
		pixelRatio: 1, // todo, consider devicePixelRatio

		setBackground: function(r,g,b) {
			if(Array.isArray(r)) {
				clearColor[0] = r[0]/255;
				clearColor[1] = r[1]/255;
				clearColor[2] = r[2]/255;
			}
			else if(g===undefined) {
				const v = r;
				r = (v & 0xff000000) >>> 24;
				g = (v & 0x00ff0000) >>> 16;
				b = (v & 0x0000ff00) >>> 8;
			}
			else {
				clearColor[0] = r/255;
				clearColor[1] = g/255;
				clearColor[2] = b/255;
			}
		},
		close: function() {
			history.back();
		},
		on: function(event, callback) {
			if(typeof event === 'object') {
				if('load' in event) // usually only one per app, must become effective immediately
					this.on('load', event.load);
				if('close' in event) // usually only one per app, must become effective immediately
					this.on('close', event.close);
				nextListeners = event;
				return;
			}
			if(!callback) {
				delete eventListeners[event];
				if(event==='gamepad')
					gamepadListener = null;
			}
			else {
				eventListeners[event] = callback;
				if(event==='gamepad')
					gamepadListener = callback;
			}
		},
		emit: function(evt, ...args) {
			if(evt in eventListeners)
				eventListeners[evt](...args);
			if('*' in eventListeners)
				eventListeners['*'](evt, ...args);
		},
		getResource: function(name, params={}) {
			if(Array.isArray(name)) {
				let ret = [];
				for(let i=0; i<name.length; ++i)
					ret.push(this.getResource(name[i], params));
				return ret;
			}
			if(name in resources)
				return resources[name];

			const suffix = name.substr(name.lastIndexOf('.')+1).toLowerCase();
			const readyCb = ()=>{
				if(resourcesLoading<=0)
					return;
				if(--resourcesLoading===0)
					setTimeout(()=>{ this.emit('load'); }, 0);
			}
			let res = 0;
			if(params.type=='font' || suffix=='ttf')
				res = gfx.loadFont(name, params, readyCb);
			else if(suffix=='png' || suffix=='jpg' || suffix=='svg')
				res = gfx.loadTexture(name, params, readyCb);
			else if(suffix=='mp3' || suffix=='wav')
				res = arcajs.audio.load(name, params, readyCb);
			if(res) {
				++resourcesLoading;
				resources[name] = res;
			}
			return res;
		},
		createImageResource: function(...args) { return gfx.createTexture(...args); },
		createCircleResource: function(r, fill=[255,255,255,255], lineW=0, stroke=[255,255,255,255]) {
			return gfx.createCircleTexture(r, fill, lineW, stroke); },
		createPathResource: function(...args) { return createPathResource(...args); },
		createSVGResource: function(svg, params) { return createSVGResource(svg, params); },
		queryImage: function(texId) { return gfx.queryTexture(texId); },
		queryFont: function(fontId, text) { return gfx.measureText(fontId, text); },
		setPointer: function(onOrOff) {
			document.getElementById(canvas_id).style.cursor = onOrOff ? '' : 'none'; },
		httpGet: function(url, callback) { arcajs.http.get(url, null, callback) },
		httpPost: function(url, data, callback) { arcajs.http.post(url, data, callback) },
		createSpriteSet: function(...args) { return arcajs.createSpriteSet(...args); },
		require: function(name) { return modules[name]; },
		exports: function(name, module) { modules[name] = module; },
		vibrate: function(duration) {
			if('vibrate' in navigator)
				navigator.vibrate(duration*1000);
			else if('mozVibrate' in navigator)
				navigator.mozVibrate(duration*1000);
		},
		prompt: function(msg, initialValue, options) {
			if(Array.isArray(msg))
				msg = msg.join('\n');
			return window.prompt(msg, initialValue);
		},
		message: function(msg, options) {
			if(Array.isArray(msg))
				msg = msg.join('\n');
			if(options && ('button1' in options))
				return window.confirm(msg);
			return window.alert(msg);
		},
		//H 0..360, S&L 0.0..1.0
		hsl: function(h,s,l,a) {
			const hue2rgb = function( v1, v2, vH ) {
				if ( vH < 0 ) vH += 1;
				else if ( vH > 1 ) vH -= 1;
				if ( ( 6 * vH ) < 1 ) return ( v1 + ( v2 - v1 ) * 6 * vH );
				if ( ( 2 * vH ) < 1 ) return ( v2 );
				if ( ( 3 * vH ) < 2 ) return ( v1 + ( v2 - v1 ) * ( ( 2 / 3 ) - vH ) * 6 );
				return v1;
			}
			
			if (s < 5.0e-6) {
				this.r = this.g = this.b = Math.floor(l*255);
				return;
			}
			if(isNaN(h)) {
				this.r = this.g = this.b = 0;
				return;
			}
			while(h>=360.0)
				h-=360.0;
			while(h<0.0)
				h+=360.0;
			
			let v2 = ( l < 0.5 ) ? l * ( 1 + s ) : ( l + s ) - ( s * l );
			let v1 = 2 * l - v2;

			return[
				Math.floor(hue2rgb( v1, v2, h/360 + ( 1 / 3 ) )*255),
				Math.floor(hue2rgb( v1, v2, h/360 )*255),
				Math.floor(hue2rgb( v1, v2, h/360 - ( 1 / 3 ) )*255),
				a
			];
		}
	}

	function emitTextInput(evt) {
		if(!('textinput'in eventListeners))
			return;
		if(evt.key.length==1) {
			const ch = evt.key.charCodeAt(0);
			if(ch>=32 && ch<256)
				app.emit('textinput', { type:'textinput', char:evt.key });
		}
		else switch(evt.key) {
		case 'Alt':
		case 'AltGraph':
		case 'CapsLock':
		case 'Control':
		case 'Shift':
			return;
		default:
			app.emit('textinput', { type:'textinput', key:evt.key });
		}
	}

	function adjustCanvasSize(canvas, pixelRatio=1) {
		var width  = canvas.clientWidth * pixelRatio;
		var height = canvas.clientHeight * pixelRatio;
		if (canvas.width !== width || canvas.height !== height) {
			canvas.width = width;
			canvas.height = height;
			app.emit('resize', canvas.clientWidth, canvas.clientHeight);
		}
	}

	function updateListeners() {
		app.emit('leave');
		const listener = nextListeners;
		['update', 'draw', 'resize', 'keyboard', 'pointer', 'gamepad', 'enter', 'leave', 'custom'].forEach(function(evt) {
			if(evt in listener)
				app.on(evt, function(...args) { listener[evt](...args); });
			else
				app.on(evt);
		});
		app.emit('enter');
		nextListeners = null;
	}

	const update = (now)=>{
		if(nextListeners)
			updateListeners();
		if(gamepadListener!==null)
			pollGamepads();

		now *= 0.001;
		const deltaT = now-tLastFrame;
		tLastFrame = now;
		app.emit('update', deltaT, now);

		let canvas = document.getElementById(canvas_id);
		adjustCanvasSize(canvas/*, window.devicePixelRatio || 1*/);
		app.width = canvas.width;
		app.height = canvas.height;

		gfx._frameBegin(...clearColor);
		app.emit('draw', gfx);
		gfx._frameEnd();

		requestAnimationFrame(update);
	}
	setTimeout(()=>{
		if(resourcesLoading===0 && Object.keys(resources).length===0)
			app.emit('load');
		requestAnimationFrame(update);
	}, 0);

	document.body.addEventListener("touchmove", (evt)=>{ }, {passive: true});
	window.addEventListener('unload', ()=>{ app.emit('close'); });
	window.addEventListener('keydown', (evt)=>{
		if(('keyboard'in eventListeners) || ('textinput'in eventListeners)) {
			app.emit('keyboard', evt); emitTextInput(evt); evt.preventDefault(); } });
	window.addEventListener('keyup', (evt)=>{ app.emit('keyboard', evt); });
	window.addEventListener("gamepadconnected", (evt)=>{
		gamepads[evt.gamepad.index] = { id:evt.gamepad.id, connected:true, state:null };
		app.emit('gamepad', { index:evt.gamepad.index, name:evt.gamepad.id, type:'connected',
			axes:evt.gamepad.axes.length, buttons:evt.gamepad.buttons.length });
	});
	window.addEventListener("gamepaddisconnected", (evt)=>{
		gamepads[evt.gamepad.index] = { id:evt.gamepad.id, connected:false, state:null };
		app.emit('gamepad', { index:evt.gamepad.index, name:evt.gamepad.id, type:'disconnected' });
	});
	arcajs.infra.addPointerEventListener(canvas, (evt)=>{ app.emit('pointer', evt); });
	setTimeout(()=>{ app.emit('resize', canvas.width, canvas.height); }, 0);
	return app;
})();
