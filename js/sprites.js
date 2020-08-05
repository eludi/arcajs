arcajs.createSpriteSet = function(texture, tilesX=1, tilesY=1, border=0) {
	const Sprite_prototype = {
		setPos: function(x,y,rot) {
			this.x = x;
			this.y = y;
			if(rot!==undefined)
				this.rot = rot;
		},
		getX: function() { return this.x; },
		getY: function() { return this.y; },
		getRot: function() { return this.rot; },
		setRot: function(rot) { this.rot = rot; },
		setFlipX: function(yesno) { if(yesno) this.flip |= 1; else if(this.flip%2) --this.flip; },
		setFlipY: function(yesno) { if(yesno) this.flip |= 2; else if(this.flip>1) this.flip-=2; },
		setSource: function(srcX,srcY,srcW,srcH) {
			this.srcX = srcX;
			this.srcY = srcY;
			this.srcW = srcW;
			this.srcH = srcH;
		},
		setTile: function(tile) {
			if(parent===null)
				return;
			if(tile>=parent.tilesX*parent.tilesY)
				return;
			const srcW = parent.texWidth/parent.tilesX;
			const srcH = parent.texHeight/parent.tilesY;
			const srcX = (tile%parent.tilesX)*srcW;
			const srcY = Math.floor(tile/parent.tilesY)*srcH;
			this.setSource(this, srcX+parent.border,srcY+parent.border,srcW-2*parent.border,srcH-2*parent.border);
		},
		setScale: function(scX,scY) {
			this.w = scX * this.srcW;
			this.h = (scY===undefined?scX:scY) * this.srcH;
		},
		setDim: function(w,h) { this.w=w; this.h=h; },
		getDimX: function() { return this.w; },
		getDimY: function() { return this.h; },
	
		setVel: function(x,y,rot) {
			this.velX = x;
			this.velY = y;
			if(rot!==undefined)
				this.velRot = rot;
		},
		getVelX: function() { return this.velX; },
		getVelY: function() { return this.velY; },
		getVelRot: function() { return this.velRot; },
		setVelRot: function(rot) { this.velRot = rot; },
		setCenter: function(x,y) { this.cx = x; this.cy = y; },
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
		},
		getColor: function() { return [this.r, this.g, this.b, this.a]; },
		getAlpha: function() { return this.a; },
		setAlpha: function(a) { this.alpha = a; },
		getRadius: function() { return this.radius; },
		setRadius: function(r) { this.radius = r; },
		intersects: function(sprite, callback) {
			throw 'sprite.intersects not yet implemented';
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
		this.flip = 0;
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
