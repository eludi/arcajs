arcajs = {};
arcajs.infra = {
	addPointerEventListener: function(target, callback) {
		let cb = (e)=>{
			for(let i=0, events = this.normalizeEvents(e); i<events.length; ++i)
				callback(events[i]);
			return true;
		}

		if(typeof target.style.touchAction != 'undefined')
			target.style.touchAction = 'none';
		target.oncontextmenu = (e)=>{ return false; }

		this.normalizeEvents.pointerDown=[];
		if(window.PointerEvent)
			target.onpointerdown = target.onpointermove = target.onpointerup = target.onpointerout
				= target.onpointerenter = target.onpointerleave = cb;
		else
			target.ontouchstart = target.ontouchend = target.ontouchmove = cb;
	},
	normalizeEvents: function(e) {
		let pointersDown = this.normalizeEvents.pointerDown;
		let readPointerId = function(pointerId) {
			for(let i=0, end=pointersDown.length; i!=end; ++i)
				if(pointersDown[i]==pointerId)
					return i;
		}
		let writePointerId = function(pointerId) {
			for(let i=0; ; ++i)
				if(pointersDown[i] === undefined)
					return i;
		}

		let events = [];
		if(window.PointerEvent) { // pointer events
			let id, type = null;
			if(e.type in {'pointerdown':true, 'pointerenter':true}) {
				if(e.pointerType!=='mouse')
					id = writePointerId(e.pointerId);
				else switch(e.button) {
					case 1: id = 2; break;
					case 2: id = 1; break;
					default: id = e.button;
				}
				if(id>=0) {
					type = 'start';
					pointersDown[id] = e.pointerId;
				}
			}
			else if(e.type in {'pointerup':true, 'pointerout':true, 'pointerleave':true}) {
				id = readPointerId(e.pointerId);
				if(id===undefined)
					return false;
				type = 'end';
				delete pointersDown[id];
			}
			else if(e.type=='pointermove') {
				id = readPointerId(e.pointerId);
				if(id!==undefined)
					type = 'move';
				else if(e.pointerType=='mouse')
					type = 'hover';
			}
			if(type) events.push({ type:type, target:e.target, id:id || 0, pointerType:e.pointerType,
				pageX:e.pageX, pageY:e.pageY,
				clientX:e.clientX,clientY:e.clientY,
				x:e.offsetX,y:e.offsetY,
				timeStamp:Math.round(e.timeStamp) });
		}
		else if(e.type in { 'touchstart':true, 'touchmove':true, 'touchend':true, 'touchcancel':true, 'touchleave':true }) {
			e.preventDefault();

			let node = e.target;
			let offsetX = 0, offsetY=0;
			while(node && (typeof node.offsetLeft != 'undefined')) {
				offsetX += node.offsetLeft;
				offsetY += node.offsetTop;
				node = node.offsetParent;
			}
			for(let i=0; i<e.changedTouches.length; ++i) {
				let touch = e.changedTouches[i];
				let id, type = e.type.substr(5);
				if(type=='start') {
					id = writePointerId(touch.identifier);
					pointersDown[id] = touch.identifier;
				}
				else {
					id = readPointerId(touch.identifier);
					if(id===undefined)
						continue;
					if(type=='cancel' || type=='leave')
						type = 'end';
					if(type=='end')
						delete pointersDown[id];
				}
				events.push({ type:type, id:id, pointerType:'touch',
					pageX:touch.pageX, pageY:touch.pageY,
					clientX:touch.clientX,clientY:touch.clientY,
					x: touch.clientX - offsetX, y: touch.clientY - offsetY,
					timeStamp:Math.round(e.timeStamp) });
			}
		}
		return events.length ? events : false;
	}
};

arcajs.http = {
	encodeURI: function(obj) {
		var s = '';
		for(var key in obj) {
			if(s.length) s += '&';
			var value = obj[key];
			if(typeof value == 'object')
				value = JSON.stringify(value);
			s += key+'='+encodeURIComponent(value);
		}
		return s;
	},

	request: function(method, url, params, callback) {
		const binaryType = ((url)=>{
			url = url.toLowerCase();
			if(url.endsWith('.png') || url.endsWith('.jpg'))
				return 'image';
			if(url.endsWith('.wav') || url.endsWith('.mp3'))
				return 'audio';
			return url.endsWith('.ttf') ? 'font' : '';
		})(url);
		function getContentType(xhr) {
			const mimeStr = xhr.contentType ? xhr.contentType :
				('getResponseHeader' in xhr) ? xhr.getResponseHeader('Content-Type') : '';
			return mimeStr || '';
		}
		function readBinaryData(response, binaryType, status, callback) {
			if(binaryType === 'image') {
				const img = document.createElement("img");
				img.src = URL.createObjectURL(response);
				img.onload = () => {
					URL.revokeObjectURL(img.src);
	
					let canvas = document.createElement('canvas');
					canvas.width = img.width;
					canvas.height = img.height;
					let ctx = canvas.getContext('2d');
					ctx.drawImage(img, 0,0, canvas.width, canvas.height);
					
					const data = new Uint8Array(ctx.getImageData(0,0, canvas.width, canvas.height).data.buffer);
					callback({width:img.width, height:img.height, depth:4, data:data, mediaType:binaryType}, status);
				};
			}
			else if(binaryType === 'audio') {
				var reader = new FileReader();
				reader.onload = function() {
					const audioContext = new AudioContext();
					audioContext.decodeAudioData(reader.result, (decoded)=>{
						const channels = decoded.numberOfChannels, channel0 = decoded.getChannelData(0), nSamples = channel0.length;
						const samples = channels==1 ?  channel0 : new Float32Array(channels*nSamples);

						if(channels>1) {
							const ch = [ channel0 ];
							for(var i=1; i<channels; ++i)
								ch.push(decoded.getChannelData(i));
							for(var i=0; i<nSamples; ++i)
								for(var j=0; j<channels; ++j)
									samples[i*channels+j] = ch[j][i];
						}
						
						callback({samples:samples, channels:channels, offset:0, mediaType:binaryType}, status);
					});
				};
				reader.readAsArrayBuffer(response);
			}
			else console.error('readBinaryData', binaryType, 'not yet implemented');
		}
	
		const xhr = new XMLHttpRequest();
		try {
			xhr.open( method, url, true );
			if(this.timeout)
				xhr.timeout = this.timeout;
			if(params && method=='POST')
				xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
			else if(method=='GET' && binaryType!=='')
				xhr.responseType = 'blob';
			if(callback) xhr.onload = xhr.onerror = xhr.ontimeout = function(event) {
				var status = (xhr.status===undefined) ? -1 : (xhr.status==1223) ? 204 : xhr.status;
				const mime = getContentType(xhr);
				var response = (xhr.status==-1 || xhr.status==204) ? null
					: mime == 'application/json' ? JSON.parse(xhr.responseText)
					: binaryType!=='' ? xhr.response : xhr.responseText;
				if(event.type == 'error')
					console.error(url, event);
				else if(event.type == 'timeout')
					status = 408;
				if(response === xhr.response)
					return readBinaryData(response, binaryType, status, callback);
				callback(response, status);
			}
			xhr.send( (params && method=='POST') ? this.encodeURI(params) : null );
		} catch(error) {
			console && console.error(error);
			if(callback)
				callback(error, -2);
			return false;
		}
		return xhr;
	},

	get: function(url, params, callback) {
		if(params)
			url+='?'+this.encodeURI(params);
		return this.request('GET', url, null, callback);
	},
	post: function(url, params, callback) {
		return this.request('POST', url, params, callback);
	},
	timeout: 32000
};
