"use strict";

let app = arcajs.app = (function(canvas_id='arcajs_canvas') {
	let clearColor = [ 0,0,0 ];
	let eventListeners = {}, nextListeners = null, gamepadListener = null;
	let resources = {};
	let resourcesLoading = 0;
	let modules = { audio:arcajs.audio, intersects:arcajs.intersects };
	let gamepads = [], gamepadResolution = 0.1;
	let tLastFrame=0;
	let loadEmitted = false, startedByUser = false;

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

	function emitAsGamepadEvent(evt, index, axes, buttons) {
		if(evt.repeat)
			return;

		if(!('state' in emitAsGamepadEvent))
			emitAsGamepadEvent.state = {};
		if(!(index in emitAsGamepadEvent.state)) {
			const state = emitAsGamepadEvent.state[index] = { axes:[], buttons:[] };
			if(axes)
				for(var i=0; i<axes.length; i+=2)
					state.axes.push(0);
			if(buttons)
				for(var i=0; i<buttons.length; ++i)
					state.buttons.push(false);
			app.emit('gamepad', {index:index, type:'connect', axes:state.axes.length, buttons:state.buttons.length});
		}

		const state = emitAsGamepadEvent.state[index];
		const keydown = evt.type==='keydown';
		if(!keydown && evt.type!=='keyup')
			return;

		if(axes) for(var i=0; i<axes.length; ++i) {
			if(evt.key !== axes[i])
				continue;
			const axis = Math.floor(i/2), value = (i%2)?1:-1;
			if(keydown) {
				if(state.axes[axis]===value)
					return;
			}
			else if(state.axes[axis]===0 || state.axes[axis]!==value)
				return;
			state.axes[axis] = keydown ? value : 0;
			return app.emit('gamepad', {index:index, type:'axis', axis:axis, value:state.axes[axis]});
		}

		if(buttons) for(var i=0; i<buttons.length; ++i) {
			const btn = buttons[i];
			if((evt.key === btn 
					|| ((typeof btn === 'object') && evt.key === btn.key && evt.location == btn.location))
				&& state.buttons[i]!=keydown)
			{
				state.buttons[i]=keydown;
				return app.emit('gamepad', {index:index, type:'button', button:i, value:keydown?1:0});
			}
		}
	}

	const app = {
		version: 'v0.20230422a',
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
				clearColor[0] = ((r & 0xff000000) >>> 24)/255;
				clearColor[1] = ((r & 0x00ff0000) >>> 16)/255;
				clearColor[2] = ((r & 0x0000ff00) >>> 8)/255;
			}
			else {
				clearColor[0] = r/255;
				clearColor[1] = g/255;
				clearColor[2] = b/255;
			}
		},
		close: function() {
			if(history.length>1)
				history.back();
			else if(this.platform === 'tv')
				window.open("", "_self").close();
			else
				window.close();
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
		emitAsGamepadEvent: emitAsGamepadEvent,
		getResource: function(name, params={}) {
			if(Array.isArray(name)) {
				let ret = [];
				for(let i=0; i<name.length; ++i)
					ret.push(this.getResource(name[i], params));
				return ret;
			}
			const suffix = name.substr(name.lastIndexOf('.')+1).toLowerCase();
			const scale = params.scale || 1.0;
			const key = (suffix==='svg') ? (name+'/'+scale) : name;
			if(key in resources)
				return resources[key];

			const readyCb = ()=>{
				if(resourcesLoading<=0)
					return;
				if(--resourcesLoading===0)
					setTimeout(()=>{ this._run() }, 5);
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
				resources[key] = res;
			}
			return res;
		},
		createImageResource: function(...args) { return gfx.createTexture(...args); },
		createCircleResource: function(r, fill=[255,255,255,255], lineW=0, stroke=[255,255,255,255]) {
			return gfx.createCircleTexture(r, fill, lineW, stroke); },
		createPathResource: function(...args) { return createPathResource(...args); },
		createSVGResource: function(svg, params) { return createSVGResource(svg, params); },
		createTileResources: function(parent, tilesX, tilesY=1, border=0, params) {
			return gfx.createTileTextures(parent, tilesX, tilesY, border, params); },
		createTileResource: function(parent, x,y,w,h, params) { return gfx.createTileTexture(parent,x,y,w,h, params); },
		createImageFontResource: function(img, params) { return gfx.createImageFontResource(img, params); },
		releaseResource: function(handle, mediaType) {
			if(mediaType.startsWith('image'))
				gfx.releaseTexture(handle);
			else if(mediaType.startsWith('audio'))
				audio.release(handle);
		},
		setImageCenter: function(img, cx, cy) { gfx.setTextureCenter(img, cx, cy); },
		queryImage: function(texId) { return gfx.queryTexture(texId); },
		queryFont: function(fontId, text) { return gfx.measureText(fontId, text); },
		setPointer: function(onOrOff) {
			document.getElementById(canvas_id).style.cursor = onOrOff ? '' : 'none'; },
		httpGet: function(url, callback) { arcajs.http.get(url, null, callback) },
		httpPost: function(url, data, callback) { arcajs.http.post(url, data, callback) },
		createSpriteSet: function(...args) { return arcajs.createSpriteSet(...args); },
		include: function(url, callback) { 
			const node = document.createElement('script');
			node.type = "text/javascript";
			node.src = url;
			node.async = false;
			if(callback)
				node.addEventListener("load", callback, false);
			document.querySelector("head").appendChild(node);
		},
		require: function(name) { return modules[name]; },
		exports: function(name, module) { modules[name] = module; },
		resizable: function(isResizable) { /* browser windows always resizable */ },
		fullscreen: function(fullscreen) {
			if(fullscreen) {
				if(document.fullscreenEnabled && !document.fullscreenElement)
					document.documentElement.requestFullscreen();
				else if(document.webkitFullscreenEnabled && !document.webkitFullscreenElement)
					document.documentElement.webkitRequestFullscreen();
			}
			else if(document.fullscreenElement)
				document.exitFullscreen();
			else if(document.webkitFullscreenElement)
				document.webkitExitFullscreen();
		},
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
				l = Math.floor(l*255);
				return [l,l,l, a];		
			}
			if(isNaN(h))
				return [0,0,0,a];
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
		},
		transformArray: function(arr, stride/*[, cbArgs], callback*/) {
			const cbArgs = [], callback = arguments[arguments.length-1];
			for(let i=2, end= arguments.length-1; i<end; ++i)
				cbArgs.push(arguments[i]);
			for(let i=0, end=arr.length; i<end; i+=stride)
				callback(arr.slice(i, i+stride), arr.subarray(i, i+stride), ...cbArgs)
		},
		_run: function(byUser=false) {
			if(loadEmitted)
				return;
			if(byUser)
				startedByUser = true;
			if(startedByUser===true && loadEmitted===false && resourcesLoading===0) {
				app.emit('load');
				loadEmitted = true;
			}
			if(loadEmitted)
				setTimeout(()=>{ requestAnimationFrame(update); }, 0);
		}
	}

	const userAgent = navigator.userAgent;
	if(userAgent.match(/AFT/) || userAgent.match(/smart\-tv/i) || userAgent.match(/smarttv/i)) // Fire OS TV, other Smart TVs
		app.platform = 'tv';

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
		['update', 'draw', 'resize', 'keyboard', 'pointer', 'gamepad', 'enter', 'leave',
			'visibilitychange', 'custom'].forEach(function(evt)
		{
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

	const stopMediaKeyEventPropagation = (app.platform==='tv') ? function(evt) {
		switch(evt.key) {
		case 'MediaRewind':
		case 'MediaFastForward':
		case 'MediaPlayPause':
		case 'ContextMenu':
		case 'GoBack':
			evt.stopImmediatePropagation();
			evt.stopPropagation();
			evt.preventDefault();
		};
	} : function() {};

	document.body.addEventListener("touchmove", (evt)=>{ }, {passive: true});
	window.addEventListener('unload', ()=>{ app.emit('close'); });
	window.addEventListener('keydown', (evt)=>{
		stopMediaKeyEventPropagation(evt);
		if(evt.ctrlKey && evt.key==='v')
			return;
		if(('keyboard' in eventListeners) || ('textinput' in eventListeners)) {
			app.emit('keyboard', evt); emitTextInput(evt); evt.preventDefault();
		}
	});
	window.addEventListener('keyup', (evt)=>{ stopMediaKeyEventPropagation(evt); app.emit('keyboard', evt); });
	document.addEventListener('keypress', (evt)=>{ stopMediaKeyEventPropagation(evt); });
	window.addEventListener("gamepadconnected", (evt)=>{
		gamepads[evt.gamepad.index] = { id:evt.gamepad.id, connected:true, state:null };
		app.emit('gamepad', { index:evt.gamepad.index, name:evt.gamepad.id, type:'connected',
			axes:evt.gamepad.axes.length, buttons:evt.gamepad.buttons.length });
	});
	window.addEventListener("gamepaddisconnected", (evt)=>{
		gamepads[evt.gamepad.index] = { id:evt.gamepad.id, connected:false, state:null };
		app.emit('gamepad', { index:evt.gamepad.index, name:evt.gamepad.id, type:'disconnected' });
	});
	canvas.addEventListener("dragover", (evt)=>{ evt.preventDefault(); });
	canvas.addEventListener("drop", (evt)=>{
		evt.preventDefault();
		if (evt.dataTransfer.items) [...evt.dataTransfer.items].forEach((item) => {
			if (item.kind === 'file') {
				const reader = new FileReader();
				reader.addEventListener("load", () => { app.emit('textinsert', {type:'drop', data: reader.result}); });
				reader.readAsText(item.getAsFile());
			}
			else if(item.kind === 'string') {
				item.getAsString((data)=>{
					if(data)
						app.emit('textinsert', {type:'drop', data: data});
				});
			}
		});
		else if(evt.dataTransfer.files) [...evt.dataTransfer.files].forEach((file) => {
			const reader = new FileReader();
			reader.addEventListener("load", () => { app.emit('textinsert', {type:'drop', data: reader.result}); });
			reader.readAsText(file);
		});
		else {
			const data = evt.dataTransfer.getData("text/plain");
			if(data)
				app.emit('textinsert', {type:'drop', data: data});
		}
	});
	window.addEventListener("paste", (evt)=>{
		evt.preventDefault();
		const text = (evt.clipboardData || window.clipboardData).getData('text');
		if((typeof text === 'string') && text.length)
			app.emit('textinsert', {type:'paste', data:text});
	});
	arcajs.infra.addPointerEventListener(canvas, (evt)=>{ app.emit('pointer', evt); });

	if(!('hidden' in document) && ('webkitHidden' in document))
		document.addEventListener('webkitvisibilitychange', ()=>{
			app.emit('visibilitychange', { visible:!document.webkitHidden }); });
	else document.addEventListener("visibilitychange", ()=>{
		app.emit('visibilitychange', { visible:!document.hidden }); });
	//window.addEventListener("blur", ()=>{ app.emit('visibilitychange', {visible:false}); });
	//window.addEventListener("focus", ()=>{ app.emit('visibilitychange', {visible:true}); });
	document.addEventListener("pause", ()=>{ app.emit('visibilitychange', {visible:false}) }, false);
	document.addEventListener("resume", ()=>{ app.emit('visibilitychange', {visible:true}) }, false);

	setTimeout(()=>{ app.emit('resize', {width:canvas.width, height:canvas.height}); }, 0);
	return app;
})();
