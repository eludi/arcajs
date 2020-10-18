
"use strict";

arcajs.Graphics = (function() {

function renderFontTexture(font) {
	let canvas = document.createElement('canvas');
	let texW = canvas.width = 512;
	let texH = canvas.height = 512;
	let ctx = canvas.getContext('2d');

	ctx.font = font;
	ctx.textBaseline = 'top';
	let dims = [];
	let x=0,y=0, currLineH = 0;

	for(let i=32; i<127; ++i) {
		const ch = String.fromCharCode(i);
		const dim = ctx.measureText(ch);
		let metrics = {
			w:Math.ceil(dim.width),
			h:Math.ceil(dim.actualBoundingBoxAscent
				+ dim.actualBoundingBoxDescent)+4,
			xoff: 0,
			yoff: Math.floor(-dim.actualBoundingBoxAscent)-1
		}
		//console.log(i, ch, metrics, dim);
		if(x+metrics.w>texW) {
			y += currLineH+1;
			currLineH = x = 0;
		}
		const currH = metrics.h + metrics.yoff;
		if(currH > currLineH)
			currLineH = currH;

		metrics.x = x;
		metrics.y = y + metrics.yoff;
		dims.push(metrics);
		x += metrics.w+1;
	}

	canvas.height = y+currLineH;
	ctx.fillStyle = 'white';
	ctx.font = font;
	ctx.textBaseline = 'top';

	x=0,y=0, currLineH = 0;
	for(let i=32; i<127; ++i) {
		const ch = String.fromCharCode(i);
		const metrics = dims[i-32];
		if(x+metrics.w>texW) {
			y += currLineH+1;
			currLineH = x = 0;
		}
		const currH = metrics.h + metrics.yoff;
		if(currH > currLineH)
			currLineH = currH;

		ctx.fillText(ch,x+0.5,y+0.5);
		x += metrics.w+1;
	}

	return { data:ctx.getImageData(0,0, canvas.width, canvas.height).data,
		width:canvas.width, height:canvas.height, glyphs:dims };
}

/// utility class for packing 2 short/4 byte precision values into single floats
function FloatPacker() {
	let int8 = new Int8Array(4);
	let uint32 = new Uint32Array(int8.buffer, 0, 1);
	let float32 = new Float32Array(int8.buffer, 0, 1);
	let int16 = new Int16Array(int8.buffer, 0, 2);

	/// @function rgba() - packs an rgb(a) color (components range [0..255]) into one float
	this.rgba = function(r,g,b,a=255) {
		uint32[0] = (a << 24 | b << 16 | g << 8 | r) & 0xfeffffff;
		 return float32[0];
	}

	/// @function int14() - packs two values within range [-16384..16383] into one float
	this.int14 = function(v1, v2=0) {
		int16[0] = v1;
		int16[1] = v2;
		 return float32[0];
	}
}
let fpack = new FloatPacker();

function defineConst(obj, key, value) {
	Object.defineProperty( obj, key, {
		value: value, writable: false, enumerable: true, configurable: true });
}

let colorConv = {
	r:0,
	g:0,
	b:0,
	//H 0..360, S&L 0.0..1.0
	hsl2rgb: function(h,s,l) {
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

		this.r = Math.floor(hue2rgb( v1, v2, h/360 + ( 1 / 3 ) )*255);
		this.g = Math.floor(hue2rgb( v1, v2, h/360 )*255);
		this.b = Math.floor(hue2rgb( v1, v2, h/360 - ( 1 / 3 ) )*255);
	}
}

//------------------------------------------------------------------
function createShader(gl, type, source) {
	let shader = gl.createShader(type);
	gl.shaderSource(shader, source);
	gl.compileShader(shader);
	var success = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
	if (success)
		return shader;
	console.error(gl.getShaderInfoLog(shader));
	gl.deleteShader(shader);
}

function createProgram(gl, vertexShader, fragmentShader) {
	let program = gl.createProgram();
	gl.attachShader(program, vertexShader);
	gl.attachShader(program, fragmentShader);
	gl.linkProgram(program);
	var success = gl.getProgramParameter(program, gl.LINK_STATUS);
	if (success)
		return program;
	console.error(gl.getProgramInfoLog(program));
	gl.deleteProgram(program);
}

function createProgramFromScripts(gl, vs, fs) {
	let vertexShaderSource = document.getElementById(vs).text;
	let fragmentShaderSource = document.getElementById(fs).text;
	
	let vertexShader = createShader(gl, gl.VERTEX_SHADER, vertexShaderSource);
	let fragmentShader = createShader(gl, gl.FRAGMENT_SHADER, fragmentShaderSource);
	return createProgram(gl, vertexShader, fragmentShader);
}

//------------------------------------------------------------------
return function (canvas, capacity=500) {
	const floatSz = 4;
	const vtxSz = 4;
	const elemSz = 6*vtxSz;
	let buf = new Float32Array(capacity * elemSz);
	let sz = 0, idx=0;
	let color_f = fpack.rgba(255,255,255);
	let lineWidth = 1.0;
	const texCoordMax = 16383;
	let fonts = [], textures=[], tex=null;

	// setup GLSL program
	const gl = canvas.getContext("webgl");
	let program = createProgramFromScripts(gl, "vs", "fs");
	const a_position = gl.getAttribLocation(program, "a_position");
	gl.enableVertexAttribArray(a_position);
	const a_color = gl.getAttribLocation(program, "a_color");
	gl.enableVertexAttribArray(a_color);
	const a_texCoord = gl.getAttribLocation(program, "a_texCoord");
	gl.enableVertexAttribArray(a_texCoord);
	let u_resolution = gl.getUniformLocation(program, "u_resolution");
	let u_texUnit0 = gl.getUniformLocation(program, "u_texUnit0");
	gl.useProgram(program);

	function normalizeGlyphs(font) {
		const scX = texCoordMax/font.width, scY=texCoordMax/font.height;
		let out = [];
		for(let i=0; i<font.glyphs.length; ++i) {
			let glyph = font.glyphs[i];
			const x1 = glyph.x*scX, y1=glyph.y*scY;
			out.push({
				xoff:glyph.xoff, yoff:glyph.yoff, w:glyph.w, h:glyph.h,
				tx1:x1, ty1:y1, tx2:x1+glyph.w*scX, ty2:y1+glyph.h*scY
			});
		}
		return out;
	}

	this.loadFont = function(font, params={}, callback) {
		const suffix = font.substr(font.lastIndexOf('.')+1).toLowerCase();
		if(suffix=='ttf') {
			let fontName = font.substr(0, font.indexOf('.'));
			fontName = fontName.toLowerCase().split('-')
				.map((s) => s.charAt(0).toUpperCase() + s.substring(1)).join(' ');
			let fontId = fonts.length;
			let obj = new FontFace(fontName, 'url('+font+')');
			obj.load().then((loadedFace)=>{
				document.fonts.add(loadedFace);
				this.loadFont(params.size+'px '+fontName, { fontId:fontId }, callback);
			}).catch(err=>{
				console.error("loading font resource", font, "failed:", err);
			});
			fonts.push({ texture:null, glyphs:null, size:params.size, width:0, height:0, ready:false });
			return fontId;
		}
		let fontData = renderFontTexture(font);
		const tex = gl.createTexture();
		gl.bindTexture(gl.TEXTURE_2D, tex);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, fontData.width, fontData.height, 0,
			gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array(fontData.data.buffer));
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
		const fontId = ('fontId' in params) ? params.fontId : fonts.length;
		fonts[fontId] = { texture:tex, glyphs:normalizeGlyphs(fontData),
			width:fontData.width, height:fontData.height, ready:true };
		if(callback)
			callback();
		return fontId;
	}

	function setTexParams(params) {
		const filtering = ('filtering' in params) ? params.filtering : 1;
		const repeat = ('repeat' in params) ? params.repeat : false;
		const filter = filtering>1 ? gl.LINEAR_MIPMAP_LINEAR : filtering==1 ? gl.LINEAR : gl.NEAREST;
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, filter);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, filter);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, repeat ? gl.REPEAT : gl.CLAMP_TO_EDGE);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, repeat ? gl.REPEAT : gl.CLAMP_TO_EDGE);
		if(filtering>1)
			gl.generateMipmap(gl.TEXTURE_2D);
	}

	/// loads a texture from a URL
	this.loadTexture = function(url, params={}, callback) {
		let texInfo = { texture:gl.createTexture(), ready:false, width:1, height:1 };
		gl.bindTexture(gl.TEXTURE_2D, texInfo.texture);
		// fill texture with a placeholder
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 1, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE,
			new Uint8Array([0, 0, 255, 255]));

		const onLoad = ()=>{
			// Now that the image has loaded copy it to the texture.
			gl.bindTexture(gl.TEXTURE_2D, texInfo.texture);
			gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image);
			setTexParams(params);
			texInfo.ready = true;
			texInfo.width = image.width;
			texInfo.height = image.height;
			if(callback)
				callback(texInfo);
		}
		// Asynchronously load an image
		let image = new Image();
		image.src = url;

		if(image.complete)
			setTimeout(onLoad, 0);
		else
			image.addEventListener('load', onLoad);

		textures.push(texInfo);
		return textures.length-1;
	}

	/// creates a new texture from an RGBA uint8 array
	this.createTexture = function(width, height, data, params={}) {
		let texInfo = { texture:gl.createTexture(), width:width, height:height, ready:true };
		gl.bindTexture(gl.TEXTURE_2D, texInfo.texture);
		if(Array.isArray(data))
			data = new Uint8Array(data);
	
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, data);
		setTexParams(params);
		textures.push(texInfo);
		return textures.length-1;
	}
	this.queryTexture = function(texId) {
		if(texId<1 || texId>=textures.length)
			return null;
		const texInfo = textures[texId];
		return { width:texInfo.width, height:texInfo.height, ready:texInfo.ready };
	}
	const setTexture = (texture)=>{
		if(texture.texture != tex) {
			this.flush();
			tex = texture.texture;
		}
		return texture;
	}

	function vertex(x, y, u=1, v=1) {
		buf[idx++] = x; buf[idx++] = y; buf[idx++] = color_f; buf[idx++] = fpack.int14(u,v);
	}

	this.color = function(r,g,b,a=255) {
		if(g===undefined)
			color_f = fpack.rgba(r[0],r[1], r[2], r[3]);
		else
			color_f = fpack.rgba(r,g,b,a);
		return this;
	}
	this.colorf = function(r,g,b,a=1.0) {
		color_f = fpack.rgba(r*255,g*255,b*255,a*255);
		return this;
	}
	this.colorHSL = function(h,s,l,a=1.0) {
		colorConv.hsl2rgb(h,s,l);
		color_f = fpack.rgba(colorConv.r, colorConv.g, colorConv.b, a*255);
		return this;
	}
	this.fillText = function(fontId, x,y, text, align=0) {
		const font = fonts[fontId];
		if(font===undefined || font.texture===null)
			return;
		setTexture(font);

		if(align!==0) {
			let metrics = this.measureText(fontId, text);
			if(align & this.ALIGN_RIGHT)
				x-=metrics.width;
			else if(align & this.ALIGN_CENTER)
				x-=metrics.width/2;
			if(align & this.ALIGN_BOTTOM)
				y-=metrics.height;
			else if(align & this.ALIGN_MIDDLE)
				y-=metrics.height/2;
		}

		const s = String(text);
		for(let i=0; i<s.length; ++i) {
			const ch = s.charCodeAt(i);
			if(ch<32 || ch>126)
				continue;
			const glyph = font.glyphs[ch-32];
			const x1 = x+glyph.xoff, y1=y+glyph.yoff;
			const x2 = x1 + glyph.w, y2 = y1 + glyph.h;
			const tx1 = glyph.tx1, ty1=glyph.ty1, tx2=glyph.tx2, ty2=glyph.ty2;

			vertex(x1,y1, tx1,ty1);
			vertex(x2,y1, tx2,ty1);
			vertex(x1,y2, tx1,ty2);
			vertex(x1,y2, tx1,ty2);
			vertex(x2,y1, tx2,ty1);
			vertex(x2,y2, tx2,ty2);
			if(++sz==capacity)
				this.flush();
			x += glyph.w;
		}
	}
	this.measureText = function(fontId, text) {
		const font = fonts[fontId];
		if(font===undefined || font.glyphs===null)
			return null;
		const vbar = font.glyphs[124-32], M=font.glyphs[77-32];
		let metrics = { height:vbar.yoff+vbar.h, fontBoundingBoxAscent:M.h,
			fontBoundingBoxDescent:vbar.yoff+vbar.h - M.yoff-M.h };
		if(text!==undefined) {
			metrics.width = 0;
			for(let i=0; i<text.length; ++i) {
				let ch = text.charCodeAt(i);
				if(ch<32 || ch>126)
					ch=32;
				const glyph = font.glyphs[ch-32];
				metrics.width += glyph.w;
			}
		}
		return metrics;
	}
	this.fillRect = function(x1, y1, w, h) {
		setTexture(texWhite);
		const x2 = x1 + w, y2 = y1 + h;
		vertex(x1,y1); vertex(x2,y1); vertex(x1,y2);
		vertex(x1,y2); vertex(x2,y1); vertex(x2,y2);
		if(++sz==capacity)
			this.flush();
	}
	this.drawRect = function(x1, y1, w, h) {
		const x2 = x1 + w, y2 = y1 + h;
		this.drawLine(x1,y1,x2,y1);
		this.drawLine(x2,y1,x2,y2);
		this.drawLine(x2,y2,x1,y2);
		this.drawLine(x1,y2,x1,y1);
	}
	this.drawLine = function(x1,y1, x2,y2) {
		setTexture(texWhite);
		const dx = x2-x1, dy=y2-y1, d=Math.sqrt(dx*dx+dy*dy);
		const nx = lineWidth*-dy/d, ny=lineWidth*dx/d;
		const x3=x1+nx, y3=y1+ny, x4=x2+nx, y4=y2+ny;
		vertex(x1,y1); vertex(x2,y2); vertex(x3,y3);
		vertex(x3,y3); vertex(x2,y2); vertex(x4,y4);
		if(++sz==capacity)
			this.flush();
	}
	this.drawPoints = function(arr) {
		for(let i=0, end=arr.length-1; i<end; i+=2)
			this.fillRect(arr[i],arr[i+1],lineWidth,lineWidth);
	}

	this.drawImage = function(texId, x1, y1, w1, h1, x2, y2, w2, h2, cx=0,cy=0,rot=0, flip=0) {
		const texInfo = setTexture(textures[texId]);
		if(x2===undefined) {
			if(w1===undefined) {
				w1=texInfo.width;
				h1=texInfo.height;
			}
			x2 = x1 + w1;
			y2 = y1 + h1;
			const tx1 = 0, ty1=0, tx2=texCoordMax, ty2=texCoordMax;
			vertex(x1,y1, tx1,ty1);
			vertex(x2,y1, tx2,ty1);
			vertex(x1,y2, tx1,ty2);
			vertex(x1,y2, tx1,ty2);
			vertex(x2,y1, tx2,ty1);
			vertex(x2,y2, tx2,ty2);
			if(++sz==capacity)
				this.flush();
			return;
		}
		const cos = Math.cos(rot), sin=Math.sin(rot);
		//const m = [ cos, -sin, x2, sin, cos, y2];

		const xmin = -cx, ymin = -cy;
		const xmax = xmin+w2, ymax = ymin+h2;

		x2 += cx;
		y2 += cy;
		const v1x = cos*xmin - sin*ymin + x2;
		const v1y = sin*xmin + cos*ymin + y2;
		const v2x = cos*xmax - sin*ymin + x2;
		const v2y = sin*xmax + cos*ymin + y2;
		const v3x = cos*xmin - sin*ymax + x2;
		const v3y = sin*xmin + cos*ymax + y2;
		const v4x = cos*xmax - sin*ymax + x2;
		const v4y = sin*xmax + cos*ymax + y2;

		let tx1 = texCoordMax * x1 / texInfo.width;
		let ty1 = texCoordMax * y1 / texInfo.height;
		let tx2 = tx1 + texCoordMax * w1 / texInfo.width;
		let ty2 = ty1 + texCoordMax * h1 / texInfo.height;
		if(flip & 0x01) {
			let tmp = tx1;
			tx1 = tx2;
			tx2 = tmp;
		}
		if(flip & 0x02) {
			let tmp = ty1;
			ty1 = ty2;
			ty2 = tmp;
		}

		vertex(v1x,v1y, tx1,ty1);
		vertex(v2x,v2y, tx2,ty1);
		vertex(v3x,v3y, tx1,ty2);
		vertex(v3x,v3y, tx1,ty2);
		vertex(v2x,v2y, tx2,ty1);
		vertex(v4x,v4y, tx2,ty2);
		if(++sz==capacity)
			this.flush();
	}
	this.drawSprites = function(sps) {
		sps.sprites.forEach((s)=>{
			if(s===null)
				return;
			const cx = s.cx*s.w, cy = s.cy*s.h;
			this.color(s.r, s.g, s.b, s.a).drawImage(
				sps.texture, s.srcX, s.srcY, s.srcW, s.srcH,
				s.x-cx, s.y-cy, s.w, s.h, cx, cy, s.rot);
		});
	}
	this.drawTile = function(sps, tile, x, y) {
		let srcW = sps.imgW/sps.tilesX, srcH=sps.imgH/sps.tilesY;
		const srcX = (tile%sps.tilesX)*srcW + sps.border;
		const srcY = (tile/sps.tilesX)*srcH + sps.border;
		srcW -= 2*sps.border;
		srcH -= 2*sps.border;
		this.drawImage( sps.texture, srcX, srcY, srcW, srcH, x, y, srcW, srcH);
	}
	function initBuf() {
		// Create a buffer for vertex attribute data:
		let glBuf = gl.createBuffer();
		// Bind it to ARRAY_BUFFER (think of it as ARRAY_BUFFER = glBuf)
		gl.bindBuffer(gl.ARRAY_BUFFER, glBuf);
		// tell vertex attributes how to get values out of glBuf:
		const stride = floatSz*vtxSz;
		gl.vertexAttribPointer(
			a_position, 2, gl.FLOAT, false, stride, floatSz*0);
		gl.vertexAttribPointer(
			a_color, 4, gl.UNSIGNED_BYTE, true, stride, floatSz*2);
		gl.vertexAttribPointer(
			a_texCoord, 2, gl.SHORT, false, stride, floatSz*3);
	}

	this.flush = function() {
		// set the resolution
		gl.uniform2f(u_resolution, gl.canvas.width, gl.canvas.height);
		gl.uniform1i(u_texUnit0, 0);

		gl.activeTexture(gl.TEXTURE0);
		gl.bindTexture(gl.TEXTURE_2D, tex);

		gl.bufferData(gl.ARRAY_BUFFER, buf, gl.STREAM_DRAW);
		gl.drawArrays(gl.TRIANGLES, 0, 6*sz);
		gl.flush();

		sz = 0;
		idx = 0;
	}

	/// experimental extension
	this.setLineWidth = function(width) {
		if(width==lineWidth)
			return;
		this.flush();
		lineWidth = width;
	}

	this._frameBegin = function(r,g,b) {
		gl.viewport(0, 0, canvas.width, canvas.height);

		gl.clearColor(r, g, b, 1);
		gl.clear(gl.COLOR_BUFFER_BIT);
		gl.enable(gl.BLEND);
		gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
	}

	this._frameEnd = this.flush;


	initBuf();
	this.createTexture(2,2,new Uint8Array([
		255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255]));
	this.loadFont('bold 20px monospace');
	const texWhite = textures[0];
	tex = texWhite.texture;

	defineConst(this, "ALIGN_LEFT", 0);
	defineConst(this, "ALIGN_CENTER", 1);
	defineConst(this, "ALIGN_RIGHT", 2);
	defineConst(this, "ALIGN_TOP", 0);
	defineConst(this, "ALIGN_MIDDLE", 4);
	defineConst(this, "ALIGN_BOTTOM", 8);
	defineConst(this, "FLIP_NONE", 0.0);
	defineConst(this, "FLIP_X", 1.0);
	defineConst(this, "FLIP_Y", 2.0);
	defineConst(this, "FLIP_XY", 3.0);
}
})();