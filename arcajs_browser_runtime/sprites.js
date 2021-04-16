arcajs.createSpriteSet = function(texture, tilesX=1, tilesY=1, border=0) {
	const Sprite_prototype = {
		setPos: function(x,y,rot) {
			this.x = x;
			this.y = y;
			if(rot!==undefined)
				this.rot = rot;
			return this;
		},
		setX: function(x) { this.x = x; return this; },
		setY: function(y) { this.y = y; return this; },
		getX: function() { return this.x; },
		getY: function() { return this.y; },
		getRot: function() { return this.rot; },
		setRot: function(rot) { this.rot = rot; return this; },
		setSource: function(srcX,srcY,srcW,srcH) {
			this.srcX = srcX; this.srcY = srcY;
			this.srcW = srcW; this.srcH = srcH;
			return this;
		},
		setTile: function(tile) {
			const parent = this.parent;
			if(parent===null)
				return;
			if(tile>=parent.tilesX*parent.tilesY)
				return;
			const srcW = parent.texWidth/parent.tilesX;
			const srcH = parent.texHeight/parent.tilesY;
			const srcX = (tile%parent.tilesX)*srcW;
			const srcY = Math.floor(tile/parent.tilesX)*srcH;
			this.setSource(srcX+parent.border,srcY+parent.border,srcW-2*parent.border,srcH-2*parent.border);
			return this;
		},
		setScale: function(scX,scY) {
			this.w = scX * this.srcW;
			this.h = (scY===undefined?scX:scY) * this.srcH;
			return this;
		},
		getScaleX: function() { return this.w / this.srcW; },
		getScaleY: function() { return this.h / this.srcH; },
		setDim: function(w,h) { this.w=w; this.h=h; return this; },
		getDimX: function() { return this.w; },
		getDimY: function() { return this.h; },
	
		setVel: function(x,y,rot) {
			this.velX = x;
			this.velY = y;
			if(rot!==undefined)
				this.velRot = rot;
			return this;
		},
		setVelX: function(x) { this.velX = x; return this; },
		setVelY: function(y) { this.velY = y; return this; },
		getVelX: function() { return this.velX; },
		getVelY: function() { return this.velY; },
		getVelRot: function() { return this.velRot; },
		setVelRot: function(rot) { this.velRot = rot; return this; },
		setCenter: function(x,y) { this.cx = x; this.cy = y; return this; },
		getCenterX: function() { return this.cx; },
		getCenterY: function() { return this.cy; },
	
		setColor: function(r,g,b,a=255) {
			if(g===undefined) {
				this.r = r[0];
				this.g = r[1];
				this.b = r[2];
				this.a = (r[3] !== undefined) ? r[3] : 255;
			}
			else {
				this.r = r;
				this.g = g;
				this.b = b;
				this.a = a;
			}
			return this;
		},
		getColor: function() { return [this.r, this.g, this.b, this.a]; },
		getAlpha: function() { return this.a; },
		setAlpha: function(a) { this.a = Math.floor(a*255); return this; },
		getRadius: function() { return this.radius; },
		setRadius: function(r) { this.radius = r; return this; },
		set: function(properties) {
			for(let key in properties)
				this[key] = properties[key];
			return this;
		},
		intersects: function(x, y) {
			const isec = arcajs.intersects;
			if(typeof y==='number') {
				if(this.radius>=0)
					return isec.pointCircle(x,y, this.x, this.y, this.radius);
				if(this.rot==0) {
					const x1 = this.x - this.cx*this.w, y1=this.y - this.cy*this.h;
					return isec.pointAlignedRect(x,y, x1,y1, x1+this.w, y1+this.h);
				}
				let c = [x,y];
				const ox = -this.cx * this.w, oy = -this.cy * this.h;
				const poly = [
					ox, oy,
					ox, oy + this.h,
					ox + this.w, oy + this.h,
					ox + this.w, oy
				];
 				isec.transformInv(c, this.x, this.y, this.rot);
				return isec.pointPolygon(c[0], c[1], poly);
			}

			const s2=x;
			if(this.radius>=0.0 && s2.radius>=0.0)
				return isec.circleCircle(this.x, this.y, this.radius, s2.x, s2.y, s2.radius);
			if(this.radius>=0.0) {
				if(!s2.rot) {
					const x1 = s2.x - s2.cx*s2.w, y1=s2.y - s2.cy*s2.h;
					return isec.circleAlignedRect(this.x, this.y, this.radius, x1,y1, x1+s2.w,y1+s2.h);
				}
				let c = [ this.x, this.y ];
				const ox = -s2.cx * s2.w, oy = -s2.cy * s2.h;
				const arr = [
					ox, oy,
					ox, oy + s2.h,
					ox + s2.w, oy + s2.h,
					ox + s2.w, oy
				];
				isec.transformInv(c, s2.x, s2.y, s2.rot);
				return isec.circlePolygon(c[0], c[1], this.radius, arr);
			}
			if(s2.radius>=0.0) {
				if(!this.rot) {
					const x1 = this.x - this.cx * this.w, y1=this.y - this.cy * this.h;
					return isec.circleAlignedRect(s2.x, s2.y, s2.radius, x1,y1, x1+this.w,y1+this.h);
				}
				let c = [ s2.x, s2.y ];
				const ox = -this.cx * this.w, oy = -this.cy * this.h;
				const arr = [
					ox, oy,
					ox, oy + this.h,
					ox + this.w, oy + this.h,
					ox + this.w, oy
				];
				isec.transformInv(c, this.x, this.y, this.rot);
				return isec.circlePolygon(c[0], c[1], s2.radius, arr);
			}
			if(!this.rot && !s2.rot) {
				const x1min = this.x - this.cx * this.w, y1min = this.y - this.cy * this.h;
				const x1max = x1min + this.w, y1max = y1min + this.h;
				const x2min = s2.x - s2.cx * s2.w, y2min = s2.y - s2.cy * s2.h;
				const x2max = x2min + s2.w, y2max = y2min + s2.h;
				return isec.alignedRectAlignedRect(
					x1min,y1min, x1max,y1max, x2min,y2min, x2max,y2max);
			}
		
			let ox = -this.cx * this.w, oy = -this.cy * this.h;
			let arr1 = [
				ox, oy,
				ox, oy + this.h,
				ox + this.w, oy + this.h,
				ox + this.w, oy
			];
			isec.transform(arr1, this.x, this.y, this.rot);
			ox = -s2.cx * s2.w;
			oy = -s2.cy * s2.h;
			let arr2 = [
				ox, oy,
				ox, oy + s2.h,
				ox + s2.w, oy + s2.h,
				ox + s2.w, oy
			];
			isec.transform(arr2, s2.x, s2.y, s2.rot);
			return isec.polygonPolygon(arr1, arr2);
		}
	};

	const Sprite = function(sps, srcX,srcY,srcW,srcH) {
		this.x = this.y = this.rot = 0.0;
		this.w = srcW;
		this.h = srcH;
		this.cx = 0.5;
		this.cy = 0.5;
		this.r = 255;
		this.g = 255;
		this.b = 255;
		this.a = 255;
		this.srcX = srcX;
		this.srcY = srcY;
		this.srcW = srcW;
		this.srcH = srcH;
		this.index = sps.sprites.length;
		this.parent = sps;
		this.velX = this.velY = this.velRot = 0.0;
		this.radius = -1;
		sps.sprites.push(this);
	}

	const SpriteSet_prototype = {
		createSprite: function(srcX,srcY,srcW,srcH) {
			if(!this.texWidth) {
				let img = app.queryImage(this.texture);
				if(img.ready) {
					this.texWidth = img.width;
					this.texHeight = img.height;
				}
			}
			let sprite;
			if(srcX===undefined)
				sprite = new Sprite(this, 0,0, this.texWidth, this.texHeight);
			else if(srcY!==undefined)
				sprite = new Sprite(this, srcX,srcY,srcW,srcH);
			else {
				const tile=srcX;
				if(tile>=this.tilesX*this.tilesY)
					throw "tile index out of range";
				srcW = this.texWidth/this.tilesX;
				srcH = this.texHeight/this.tilesY;
				srcX = (tile%this.tilesX)*srcW;
				srcY = Math.floor(tile/this.tilesX)*srcH;
				sprite = new Sprite(this, srcX+this.border,srcY+this.border,srcW-2*this.border,srcH-2*this.border);
			}
			Object.setPrototypeOf(sprite, Sprite_prototype);
			return sprite;
		},
		removeSprite: function(sprite) {
			if(this.sprites[sprite.index]===sprite) {
				this.sprites[sprite.index]=null;
				sprite.parent = null;
				sprite.index = -1;
			}
			while(this.sprites.length && this.sprites[this.sprites.length-1]===null)
				this.sprites.pop();
		},
		update: function(deltaT) {
			this.sprites.forEach((sprite)=>{
				if(sprite===null)
					return;
				sprite.x += sprite.velX * deltaT;
				sprite.y += sprite.velY * deltaT;
				sprite.rot += sprite.velRot * deltaT;
			});
		}
	}

	let img = app.queryImage(texture);
	let sps = {
		texture: texture,
		tilesX: tilesX,
		tilesY: tilesY,
		texWidth: img.ready ? img.width : 0,
		texHeight: img.ready ? img.height : 0,
		border: border,
		sprites: []
	};
	Object.setPrototypeOf(sps, SpriteSet_prototype);
	return sps;
}
