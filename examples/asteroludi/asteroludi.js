const isx = app.require('intersects');

function createPolygon(r, nSegments) {
	var arr = new Float32Array(nSegments*2);
	for(var i=0; i<nSegments; ++i) {
		const angle = Math.PI*2 * i/nSegments;
		arr[i*2] = r*Math.cos(angle);
		arr[i*2+1] = r*Math.sin(angle);
	}
	return arr;
}

function randf(v1, v2) {
	if(v2===undefined) { v2=v1; v1=0; }
	return v1+(Math.random()*(v2-v1));
}
function randi(v1, v2) {
	return Math.floor(randf(v1,v2));
}

function dist(x1, y1, x2, y2) {
	return Math.sqrt(Math.pow(x2-x1, 2) + Math.pow(y2-y1, 2));
}

function lpad(value, numDigits, char) {
	if(char===undefined)
		char = '0';
	var s = ''+value;
	if(value>=0)
		while(s.length<numDigits)
			s = char+s;
	return s;
}

function Info() {
	this.set = function(msg, duration) {
		this.text = msg;
		this.offsetX = msg.length*-6;
		this.duration  = duration ? duration : Number.MAX_VALUE;
	}
	this.text = '';
	this.offsetX = 0;
	this.offsetY = -8;
	this.duration = Number.MAX_VALUE;

	this.update = function(deltaT) {
		this.duration -= deltaT;
		if(this.duration<=0)
			this.text = '';
	}
	this.draw = function(gfx) {
		if(!this.text.length)
			return;
		gfx.color(0xffffffff);
		gfx.fillText(app.width/2+this.offsetX, app.height/2+this.offsetY, this.text);
	}
}

function wrap(obj, w, h) {
	var isWrapped = false;
	if(obj.x<0.0) {
		obj.x += w;
		isWrapped = true;
	}
	else if(obj.x>w) {
		obj.x -= w;
		isWrapped = true;
	}
	if(obj.y<0.0) {
		obj.y += h;
		isWrapped = true;
	}
	else if(obj.y>h) {
		obj.y -= h;
		isWrapped = true;
	}
	return isWrapped;
}

//--- constants ----------------------------------------------------
const ARENA_H = 60, ARENA_W=Math.max(ARENA_H, Math.floor(ARENA_H*app.width/app.height));
const COMET_VEL_MIN = 12, COMET_VEL_MAX = 16;

const images = app.getResource('graphics.svg', {scale:2, centerX:0.5, centerY:0.5});
const digit0 = app.createTileResource(images, 0,0.75, 1/16,0.25, {centerX:0, centerY:0});
for(var i=1; i<10; ++i)
	app.createTileResource(images, i/16,0.75, 1/16,0.25, {centerX:0, centerY:0});
var digitDims;

//--- effects ------------------------------------------------------
function FXExplo1(x,y, nParticles, particleSz) {
	const stride = 9; //x,y,scale, r,g,b,a, vx,vy
	const data = new Float32Array(nParticles*stride);
	var lifetime = 1;

	for(var i=0; i<nParticles; ++i) {
		const angle = Math.random()*2*Math.PI, dist = randi(10,20);
		data.set([0,0, particleSz, 255,255,255,255, Math.cos(angle)*dist, Math.sin(angle)*dist], i*stride);
	}

	this.draw = function(gfx) {
		gfx.save().transform(x,y);
		gfx.drawImages(FXExplo1.img, stride, gfx.COMP_SCALE|gfx.COMP_COLOR_RGBA, data);
		gfx.restore();
	}
	this.update = function(deltaT) {
		lifetime -= deltaT;
		app.transformArray(data, stride, deltaT, function(input, output, deltaT) {
			output[0] += input[7] * deltaT;
			output[1] += input[8] * deltaT;
			output[4] -= 255*deltaT;
			output[5] -= 512*deltaT;
			output[6] -= 255*deltaT;
			output[7] *= 1-2*deltaT;
			output[8] *= 1-2*deltaT;
		});
		return lifetime>=0;
	}
}
FXExplo1.img = app.createImageResource(2,2, new Uint32Array([0xFFffFFaa, 0xFFffFFaa, 0xFFffFFaa, 0xFFffFFaa]));


function initStarfield(arr, w,h,sc, isColored) {
	const stride = arr.stride = isColored ? 6 : 4;
	for(var i=0, end = arr.length/stride; i<end; ++i) {
		arr[i*stride]	= Math.random() * w;
		arr[i*stride+1] = Math.random() * h;
		const luminance =  Math.random();
		arr[i*stride+2] = luminance<0.5 ? sc : sc + sc*luminance/2;
		if(!isColored)
			arr[i*stride+3] = Math.floor(Math.min(32+192*luminance,128));
		else {
			arr[i*stride+3] = randi(128,256);
			arr[i*stride+4] = randi(128,256);
			arr[i*stride+5] = randi(128,256);
		}
	}
}

function drawFancyNumber(gfx, x,y, value, padding) {
	const str = lpad(value, padding), codeZero = '0'.charCodeAt(0);
	gfx.save().transform(x,y,0,0.5);
	for(var i=0; i<str.length; ++i)
		gfx.drawImage(digit0 + str.charCodeAt(i) - codeZero, digitDims.width*i,0);
	gfx.restore();
}

//--- game objects -------------------------------------------------
function Ship(id) {
	this.id = id;
	this.x = this.y = 0;
	this.rot = Math.random() * Math.PI*2;
	this.radius = 2;
	this.tNextShot = this.tCollision = 0.0;
	this.xv = this.yv = 0;
	this.shield = this.shoot = false;
	this.left = this.right = this.thrust = false;
	this.score = 0;
	this.health = 1;
	this.energy = 1;
	this.color = this.id<2 ? 0xffff00ff : this.id===2 ? 0x55ff55ff : this.id===3 ? 0xffaa55ff : 0xaaaaffff;
	this.triangles = Ship.TRIANGLES;

	this.update = function(deltaT, events) {
		if(this.health<=0)
			return false;
		if (this.left)
			this.rot -= deltaT * Math.PI*2 * this.left;
		if (this.right)
			this.rot += deltaT * Math.PI*2 * this.right;

		const shieldCost = 0.4, breakFactor = 4;
		if(this.shield) {
			this.energy -= deltaT * shieldCost;
			if(this.energy<0) {
				this.energy = 0;
				this.shield = false;
			}
			else if(breakFactor) {
				var v = Math.sqrt(this.xv*this.xv + this.yv*this.yv);
				if(v<breakFactor/4) {
					this.xv = this.yv = 0;
				}
				else {
					this.xv -= this.xv * deltaT * breakFactor;
					this.yv -= this.yv * deltaT * breakFactor;
				}
			}
		}
		else if (this.thrust) {
			this.energy -= deltaT*0.15;
			if(this.energy<0)
				this.energy = 0;
			else if(Math.abs(this.xv)<ARENA_H && Math.abs(this.yv)<ARENA_H) {
				const v = deltaT * 12;
				this.xv += Math.cos(this.rot) * v;
				this.yv += Math.sin(this.rot) * v;
			}
		}
		else if(this.energy < 0.5)
			this.energy += 0.05*deltaT;

		this.x += this.xv * deltaT;
		this.y += this.yv * deltaT;
		wrap(this, ARENA_W, ARENA_H);

		this.tNextShot -= deltaT;
		if(this.shoot && !this.shield && this.tCollision<=0 && this.tNextShot<=0 && this.energy>=0.1) {
			const bullet = new Bullet(this);
			bullets.push(bullet);
			this.energy-=0.1;
			this.tNextShot = 0.15;
			events.push({evt:'bullet', x:bullet.x, y:bullet.y, rot:bullet.rot, parent:this.id});
		}

		this.tCollision -= deltaT;
		return this.health>0;
	};

	this.draw = function(gfx) {
		if(this.health<=0)
			return;
		gfx.save().transform(this.x, this.y, this.rot);
		if (this.shield)
			gfx.color(0x5555ffff).drawLineLoop(Ship.SHIELD);
		else if (this.thrust)
			gfx.color(0xff0000ff).drawLineStrip([-0.75, -1.0, -1.75, 0, -0.75, 1.0]);
		if((this.tCollision>0) && ((this.tCollision%0.2)<0.1))
			gfx.color(0xff5555ff);
		else
			gfx.color(this.color);
		gfx.drawLineLoop(Ship.CONTOUR);
		gfx.restore();
	};

	this.collide = function() {
		if(this.tCollision>0 || this.health<=0)
			return;
		this.tCollision = 0.7;
		this.health -= 0.25;
		if(this.health<=0) {
			for(var i=0, end=randi(Math.min(Math.floor(this.score/200), 50)); i<end; ++i)
				particles.push(new Particle(this.x, this.y, this.radius));
		}
	}
	this.bounce = function(other, bounceBoth) {
		const d = dist(other.x, other.y, this.x, this.y);
		const p = other.radius + this.radius - d, pRel = p/d/(bounceBoth ? 2:1);
		var dx = this.x-other.x, dy = this.y-other.y;
		this.x += dx*pRel;
		this.y += dy*pRel;
		this.xv = dx;
		this.yv = dy;
		if(bounceBoth) {
			other.x -= dx*pRel;
			other.y -= dy*pRel;
			other.xv = -dx;
			other.yv = -dy;
		}
	}

	this.reset = function() {
		switch(this.id) {
		case 1:
			this.x=ARENA_W*0.5; this.y=ARENA_H*0.33; break;
		case 2:
			this.x = ARENA_W*0.5; this.y = ARENA_H*0.67; break;
		case 3:
			this.x = ARENA_W*0.33; this.y = ARENA_H*0.5; break;
		case 4:
			this.x = ARENA_W*0.67; this.y = ARENA_H*0.5; break;
		default:
			this.x=ARENA_W*0.5; this.y=ARENA_H*0.5; break;
		}

		this.xv = this.yv = 0;
		this.energy = 1.0;
		this.shield = this.shoot = this.left = this.right = this.thrust = false;
		this.health = Math.min(this.health+0.25, 1);
	}
	this.intersects = function(other) {
		if(this.health<=0 || !isx.circleCircle(this.x, this.y, this.radius, other.x, other.y, other.radius))
			return false;
		if(this.shield)
			return true;
		return isx.sprites(this, other);
	}
};
Ship.TRIANGLES = new Float32Array([2.0,0.0, -1.0,1.5, -0.5, 0.0,   2.0,0.0, -0.5, 0.0, -1.0,-1.5]);
Ship.CONTOUR = new Float32Array([2.0, 0.0, -1.0, -1.5, -0.5, 0.0, -1.0, 1.5]);
Ship.SHIELD = createPolygon(2.5, 24);

function Bullet(ship) {
	const cos = Math.cos(ship.rot);
	const sin = Math.sin(ship.rot);

	this.parent = ship.id;
	this.x = ship.x + ship.radius * cos;
	this.y = ship.y + ship.radius * sin;
	const v = 30;
	this.xv = ship.xv + cos * v;
	this.yv = ship.yv + sin * v;
	this.lifetime = 0.5;
	this.radius = 0.1;

	this.draw = function(gfx) {
		if (!this.lifetime<0)
			return;
		gfx.save().transform(this.x, this.y,0,0.5);
		gfx.color(0xff00ffff);
		gfx.drawImage(gfx.IMG_CIRCLE);
		gfx.restore();
	};
	this.update = function(deltaT, events) {
		if (this.lifetime<0)
			return false;
		this.lifetime -= deltaT;

		this.x += this.xv * deltaT;
		this.y += this.yv * deltaT;
		return true;
	};
};

function Asteroid(radius) {
	while(true) {
		this.x = randi(ARENA_W);
		this.y = randi(ARENA_H);
		var minDist = ARENA_H;
		for(var i=0; i<ships.length && minDist>15; ++i)
			minDist = Math.min(minDist, dist(this.x, this.y, ships[i].x, ships[i].y));
		if(minDist>15)
			break;
	}
	const v = randi(1,4);
	this.rot = Math.random() * Math.PI*2;
	this.xv = Math.cos(this.rot) * v;
	this.yv = Math.sin(this.rot) * v;
	this.rotv = randf(0.1,0.2) * (randi(2) ? Math.PI*2 : -Math.PI*2);
	this.radius = radius;
	this.segments = randi(3,10);
	this.shape = createPolygon(this.radius, this.segments);

	this.burst = function() {
		for(var i=0, end=randi(4,9); i<end; ++i)
			particles.push(new Particle(this.x, this.y, this.radius));
		this.radius-=1;
		if(this.radius<=0)
			return;

		this.segments = randi(3,10);
		this.shape = createPolygon(this.radius, this.segments);
		var other = new Asteroid(this.radius);
		other.x = this.x;
		other.y = this.y;
		asteroids.push(other);
	}

	this.draw = function(gfx) {
		gfx.color(0x55aaaaff);
		gfx.save().transform(this.x, this.y, this.rot);
		gfx.drawLineLoop(this.shape);
		gfx.restore();
	};
	this.update = function(deltaT, events) {
		this.x += this.xv * deltaT;
		this.y += this.yv * deltaT;
		this.rot += this.rotv * deltaT;
		wrap(this, ARENA_W, ARENA_H);
		return this.radius>0;
	};
};

function Particle(x,y, spread, lifetime) {
	this.x = x + (2*spread*Math.random()-spread);
	this.y = y + (2*spread*Math.random()-spread);
	this.xv = 1.5*Math.random()-0.75;
	this.yv = 1.5*Math.random()-0.75;
	this.lifetime = lifetime ? lifetime : Math.random()*12;

	this.draw = function(gfx) {
		gfx.color(this.lifetime<0.1 ? 0xffffaaff : 0xaa0000ff);
		gfx.drawRect(this.x,this.y,0.05,0.05);
	};
	this.update = function(deltaT, events) {
		this.lifetime -= deltaT;
		this.x += this.xv * deltaT;
		this.y += this.yv * deltaT;
		return this.lifetime>0;
	};
}

function Comet(spawnDelay, vel, dir) {
	this.spawnDelay = spawnDelay;
	if(dir === undefined)
		dir = this.rot = Math.random()*2*Math.PI;

	this.xv = Math.cos(dir)*vel;
	this.yv = Math.sin(dir)*vel;
	this.rotv = randf(0.3, 0.4) * (randi(2) ? Math.PI*2 : -Math.PI*2);
	this.radius = 0.75;
	this.shape = createPolygon(this.radius, 3);
	this.particleDelay = randf(0.05);

	if(Math.abs(this.xv)>Math.abs(this.yv)) {
		this.y = randf(ARENA_H*0.5);
		if(this.yv<0)
			this.y += ARENA_H*0.5;
		if(this.xv>0)
			this.x = -this.radius;
		else
			this.x = ARENA_W+this.radius;
	}
	else {
		this.x = randf(ARENA_W*0.5);
		if(this.xv<0)
			this.x += ARENA_W*0.5;
		if(this.yv>0)
			this.y = -this.radius;
		else
			this.y = ARENA_H+this.radius;
	}

	this.burst = function() {
		for(var i=0, end=randi(4,9); i<end; ++i)
			particles.push(new Particle(this.x, this.y, this.radius));
		this.radius-=1;
	}
	this.draw = function(gfx) {
		if(this.spawnDelay>0)
			return;
		gfx.save().transform(this.x, this.y, this.rot);
		gfx.color(0xff7f7fff).drawLineLoop(this.shape);
		gfx.restore();
	}
	this.update = function(deltaT, events) {
		this.spawnDelay -= deltaT;
		if(this.spawnDelay>0)
			return true;
		if(this.spawnDelay + deltaT > 0)
			events.push({evt:'comet', x:this.x, y:this.y, rot:this.rot, xv:this.xv, yv:this.yv});
		if(this.radius<=0)
			return false;
		this.particleDelay -= deltaT;

		this.x += this.xv * deltaT;
		this.y += this.yv * deltaT;
		this.rot += this.rotv * deltaT;

		if(this.particleDelay<=0) {
			particles.push(new Particle(this.x, this.y, this.radius, 1.5));
			this.particleDelay = randf(0.05);
		}
	
		if(this.xv>0) {
			if(this.x>ARENA_W+this.radius)
				return false;
		}
		else if(this.x<-this.radius)
			return false;
		if(this.yv>0) {
			if(this.y>ARENA_H+this.radius)
				return false;
		}
		else if(this.y<-this.radius)
			return false;
		return true;
	}
}

//--- Game ---------------------------------------------------------
var ships = [], bullets = [], asteroids = [], particles = [], comets=[], fx = [];
var highscore = localStorage.getItem("asteroludi_highscore") || 0;

const sfx = {
	laser: audio.createSound('squ', 1000,0.8,0,0.1, 200,0.4,0.275,0.1), // tju
	hit: audio.createSound('bin', 0,1,0.1,0.005, 0,0.3,0.1,0.005, 0,0,0.3,0.005), // explo
	kaboom: audio.createSound('noi', 1,1,0.5,0.04, 1,0.1,.5,0.04, 1,0.0,1,0.03),
	moop: audio.createSound('squ', 40,0.5,0.33,0.5),
	blb: audio.createSound('sin', 600,0,0,1, 600,0.8,0.001,1, 600,0.8,0.008,1, 600,0,0.011,1),
	comet: audio.createSound('bin', 12000,0,0,0.6, 8000,0.8,0.2,0.4, 400,0,1,0.02), // sonic
};

function Game(nPlayers) {
	const STATE_RUNNING=0, STATE_PAUSE=1, STATE_TRANSITION=2, STATE_OVER=3;
	var events = [];

	function newLevel() {
		ships.forEach(function(s) { s.reset(); });
		++level;
		for(var i=0; i<7+level*2; ++i)
			asteroids.push(new Asteroid(randi(1,4+level)));
		bullets.splice(0);
		particles.splice(0);
		comets.splice(0);
		fx.splice(0);
		initStarfield(starfield, ARENA_W, ARENA_H, 0.1);
		for(var i=0; i<level-1; ++i)
			comets.push(new Comet(randi(1,10), randi(COMET_VEL_MIN, COMET_VEL_MAX)));
		info.set('LEVEL '+level, 2);
		state = STATE_RUNNING;
	}

	this.keyboard = function(evt) {
		if(state===STATE_OVER)
			return;
		if(state===STATE_RUNNING || state===STATE_TRANSITION) {
			app.emitAsGamepadEvent(evt, 0,
				['ArrowLeft','ArrowRight', 'ArrowUp','ArrowDown'], [{key:'Control',location:2}]);
			if(ships.length>1)
				app.emitAsGamepadEvent(evt, 1, ['a','d', 'w','s'], ['Tab']);
		}

		if(evt.type==='keydown') switch(evt.key) {
		case 'p' :
			if(state===STATE_RUNNING) {
				state = STATE_PAUSE;
				info.set('PAUSE');
			}
			else if(state===STATE_PAUSE) {
				state = STATE_RUNNING;
				info.set('');
			}
			break;
		}
	}

	this.gamepad = function(evt) {
		if(evt.index>=ships.length)
			return;
		var s = ships[evt.index];
		if(evt.type==='axis') {
			if(evt.axis===0 || evt.axis===6) {
				s.right = s.left = 0;
				if(evt.value>0.1)
					s.right = evt.value;
				else if(evt.value<-0.1)
					s.left = -evt.value;
			}
			else if(evt.axis===1) {
				s.thrust = s.shield = false;
				if(evt.value>0.1)
					s.shield = true;
				else if(evt.value<-0.1)
					s.thrust = true;
			}
			else if(evt.axis===7) {
				s.thrust = s.shield = false;
				if(evt.value<-0.1)
					s.shield = true;
				else if(evt.value>0.1)
					s.thrust = true;
			}
		}
		else if(evt.type==='button' && evt.button===0)
			s.shoot = evt.value>0 ? 1 : 0;
	}

	this.update = function(deltaT, now) {
		info.update(deltaT);

		if(state===STATE_RUNNING || state===STATE_OVER || state===STATE_TRANSITION) {
			events.splice(0);
			for(var i=particles.length; i-->0; ) {
				var p = particles[i];
				if(state===STATE_RUNNING || state===STATE_TRANSITION) ships.forEach(function(s) {
					if(isx.pointCircle(p.x, p.y, s.x, s.y, s.radius)) {
						const bonus = s.energy>=1;
						if(bonus)
							s.score += 20*level; // bonus
						else {
							s.energy = Math.min(s.energy + 0.1, 1);
							s.score += 5*level;
						}
						p.lifetime = 0;
						events.push({evt:'particleCollected', x:p.x, y:p.y, ship:s.id, bonus:bonus});
					}
				});
				if(!p.update(deltaT, events))
					particles.splice(i, 1);
			}
			for(var i=asteroids.length; i-->0; ) {
				var a = asteroids[i];
				if(!a.update(deltaT, events))
					asteroids.splice(i, 1);
			}
			for(var i=comets.length; i-->0; ) {
				var c = comets[i];
				if(!c.update(deltaT, events))
					comets.splice(i, 1, new Comet(randi(1,10), randi(COMET_VEL_MIN, COMET_VEL_MAX)));
			}
			for(var i=ships.length; i-->0; ) {
				var s = ships[i];
				if(!s.update(deltaT, events) && state === STATE_RUNNING) {
					events.push({evt:'shipDestroyed', x:s.x, y:s.y, ship:s.id});
					state = STATE_OVER;

					var bestScore = 0, bestShip=0;
					for(var j=ships.length; j-->0; ) if(ships[j].score>bestScore) {
						bestScore = ships[j].score;
						bestShip = ships[j].id;
					}
					var nextScreen = Menu;
					if(bestScore > highscore) {
						events.push({evt:'highscore', score:bestScore, ship:bestShip});
						highscore = bestScore;
						localStorage.setItem("asteroludi_highscore", highscore);
						nextScreen = Highscore;
					}

					info.set('GAME OVER.');
					setTimeout(function() { app.on(new nextScreen(bestShip)); }, 4000);
					continue;
				}
				if(s.tCollision>0)
					continue;
				asteroids.forEach(function(a) {
					if(s.intersects(a)) {
						if(s.shield) 
							s.bounce(a);
						else {
							a.burst();
							s.collide();
							if(s.health>0)
								events.push({evt:'shipCollision', x:(a.x+s.x)/2, y:(a.y+s.y)/2, ship:s.id});
						}
					}
				});
				comets.forEach(function(c) {
					if(s.intersects(c)) {
						c.burst();
						s.collide();
						if(s.health>0)
							events.push({evt:'shipCollision', x:(a.x+s.x)/2, y:(a.y+s.y)/2, ship:s.id});
					}
				});
				for(var j=0; j<i; ++j) {
					var s2 = ships[j];
					if(s.intersects(s2)) {
						if(s.shield || s2.shield)
							s.bounce(s2, true);
						if(!s.shield)
							s.collide();
						if(!s2.shield)
							s2.collide();
						if(s.health>0 && s2.health>0 && !(s.shield&&s2.shield))
							events.push({evt:'shipCollision', x:(s2.x+s.x)/2, y:(s2.y+s.y)/2, ship:s.id, ship2:s2.id});
					}
				}
			}
			for(var i=bullets.length; i-->0; ) {
				var bullet = bullets[i];
				var bulletAlive = bullet.update(deltaT, events);
				for(var j=asteroids.length; bulletAlive && j-->0;) {
					var a = asteroids[j];
					if(isx.circleCircle(bullet.x, bullet.y, bullet.radius, a.x, a.y, a.radius)) {
						events.push({evt:'hit', x:bullet.x, y:bullet.y, target:'asteroid'});
						a.burst();
						bulletAlive = false;
					}
				}
				for(var j=comets.length; bulletAlive && j-->0;) {
					var c = comets[j];
					if(isx.circleCircle(bullet.x, bullet.y, bullet.radius, c.x, c.y, c.radius)) {
						events.push({evt:'hit', x:bullet.x, y:bullet.y, target:'comet'});
						c.burst();
						bulletAlive = false;
					}
				}
				for(var j=ships.length; bulletAlive && j-->0;) {
					var s = ships[j];
					if(bullet.parent === s.id)
						continue;
					if(isx.circleCircle(bullet.x, bullet.y, bullet.radius, s.x, s.y, s.radius)) {
						if(s.shield)
							events.push({evt:'bulletRepelled', x:bullet.x, y:bullet.y, ship:s.id});
						else {
							events.push({evt:'hit', x:bullet.x, y:bullet.y, target:'ship'});
							s.collide();
						}
						bulletAlive = false;
					}
				}
				if(!bulletAlive)
					bullets.splice(i, 1);
			}

			for(var i=events.length; i-->0; ) {
				const evt = events[i];
				switch(evt.evt) {
				case 'bullet':
					audio.replay(sfx.laser, 0.25, 2*evt.x/ARENA_W - 1, randi(-6,7));
					break;
				case 'hit':
					audio.replay(sfx.hit, 0.25, 2*evt.x/ARENA_W - 1, randi(-6,7));
					break;
				case 'shipCollision':
					audio.replay(sfx.moop, 0.25, 2*evt.x/ARENA_W - 1);
					break;
				case 'shipDestroyed':
					fx.push(new FXExplo1(evt.x, evt.y, 40, 0.3));
					audio.replay(sfx.kaboom, 0.7, 2*evt.x/ARENA_W - 1, randi(-6,7));
					break;
				case 'particleCollected':
					audio.replay(sfx.blb, 0.15, 2*evt.x/ARENA_W - 1, randi(0,7));
					break;
				case 'comet':
					audio.replay(sfx.comet, 0.15, 2*evt.x/ARENA_W - 1, randi(-6,3));
					break;
				}
			}

			for(var i=fx.length; i-->0; ) {
				var f = fx[i];
				if(!f.update(deltaT))
					fx.splice(i, 1);
			}

			if(state===STATE_RUNNING && !asteroids.length) {
				state = STATE_TRANSITION;
				setTimeout(newLevel, 2000);
			}
		}
	}

	this.draw = function(gfx) {
		// scene:
		const vpSc = app.height/ARENA_H, vpWidth = vpSc*ARENA_W;
		const ox = (app.width-vpWidth)*0.5, oy = 0;

		gfx.clipRect(ox,oy, vpWidth, app.height);
		gfx.save().transform(ox, oy, 0, vpSc);
		gfx.color(0xffffffff).drawImages(gfx.IMG_CIRCLE, 4, gfx.COMP_SCALE|gfx.COMP_COLOR_A, starfield);

		gfx.lineWidth(0.15);
		ships.forEach(function(elem) { elem.draw(gfx); });
		asteroids.forEach(function(elem) { elem.draw(gfx); });
		comets.forEach(function(elem) { elem.draw(gfx); });
		particles.forEach(function(elem) { elem.draw(gfx); });
		bullets.forEach(function(elem) { elem.draw(gfx); });
		gfx.color(0xffffffff);
		fx.forEach(function(elem) { elem.draw(gfx); });

		// overlay:
		gfx.restore().lineWidth(3);
		info.draw(gfx);
		ships.forEach(function(s, i) {
			const width = 6*digitDims.width/2, height=digitDims.height/2, x = ox+i*width*7/6;
			gfx.color(s.color - 0x7f);
			drawFancyNumber(gfx,x,0, s.score,6);
			gfx.drawLine(x, height+2, x+s.health*width, height+2);
			gfx.color(0xff00007f).drawLine(x, height+8, x+s.energy*width, height+8);
		});
	}

	var state = STATE_RUNNING, level = 0, numPlayers = 1;
	var info = new Info(), starfield = new Float32Array(ARENA_W*ARENA_H/2);

	app.setBackground(0xff);
	app.setPointer(0);

	if(nPlayers)
		numPlayers = nPlayers;
	level = 0;
	switch(numPlayers) {
	case 2:
		ships = [new Ship(1), new Ship(2)];
		break;
	case 3:
		ships = [new Ship(1), new Ship(2), new Ship(3)];
		break;
	case 4:
		ships = [new Ship(1), new Ship(2), new Ship(3), new Ship(4)];
		break;
	default:
		ships = [new Ship(0)];
	}
	asteroids.splice(0);
	newLevel();
}

//--- Menu ---------------------------------------------------------
const title = app.createTileResource(images, 0,0, 416/512, 64/256);
const iconClose = app.createTileResource(images, (512-64)/512,0, 64/512, 64/256);
const iconPlayers1 = app.createTileResource(images, 0,0.25, 0.25,0.5);
const iconPlayers2 = app.createTileResource(images, 0.25,0.25, 0.25,0.5);
const titleTheme = audio.createMusicResource(song);

function Menu() {
	const marquee =" +++ Welcome to ASTEROLUDI, a little showcase app for the arcajs multimedia framework +++ https://github.com/eludi/arcajs"
		+ " +++ This asteroids-like space arcade shooter can be controlled via gamepads or a keyboard"
		+ " +++ keys left player: W,A,S,D + Tab; right player: cursor keys + right Control"
		+ " +++ Evade and destroy the asteroids floating around in space. Harvest the emitted particles for energy fueling your ship"
		+ " +++ Title theme based on 'Elsewhere' by Jim Hall +++ Enjoy! +++";
	const marqueeLen = marquee.length, marqueeWidth = marqueeLen*12;
	var marqueeX = app.width+150, starfield = new Float32Array(4*Math.floor(app.width*app.height/2048));
	var highscoreColor = 0;

	initStarfield(starfield, app.width, app.height, app.height/400, false);
	app.setPointer(1);
	var music = audio.replay(titleTheme);
	var iconCloseHover = false;

	this.update = function(deltaT, now) {
		for(var i=0; i<1; ++i) {
			const idx = randi(starfield.length/starfield.stride)*starfield.stride;
			starfield[idx  ] = randi(app.width);
			starfield[idx+1] = randi(app.height);
		}

		marqueeX -= deltaT*90;
		if(marqueeX<-marqueeWidth)
			app.on(new Game(0));
		highscoreColor += Math.PI*deltaT/0.6;
	}
	this.draw = function(gfx) {
		gfx.color(255,255,255).drawImages(gfx.IMG_CIRCLE, starfield.stride, gfx.COMP_SCALE|gfx.COMP_COLOR_A, starfield);
		if(highscore>0)
			drawFancyNumber(gfx.color(255, 255*Math.abs(Math.sin(highscoreColor)), 85), app.width/2-1.5*digitDims.width,app.height*0.45, highscore, 6);
		gfx.color(255,255,255,iconCloseHover ? 255 : 127).stretchImage(iconClose, app.width-40,8,32,32);
		gfx.color(255,255,255).drawImage(title, app.width/2, app.height/3);
		gfx.drawImage(iconPlayers1, app.width*0.3, app.height*0.6,0,0.5);
		gfx.drawImage(iconPlayers2, app.width*0.7, app.height*0.6,0,0.5);
		gfx.fillText(app.width*0.3-12*6.5, app.height*0.6+64, 'SINGLE PLAYER');
		gfx.fillText(app.width*0.7-12*5.5, app.height*0.6+64, 'TWO PLAYERS');
		gfx.clipRect(20, app.height-30, app.width-40,20);
		gfx.fillText(marqueeX, app.height-30, marquee);
		gfx.clipRect(false);
	}
	this.keyboard = function(evt) {
		switch(evt.key) {
		case '1':
		case '2':
		case '3':
		case '4':
			return app.on(new Game(parseInt(evt.key)));
		case 'GoBack':
		case 'Escape':
			if(evt.type==='keydown' && !evt.repeat)
				return app.close();
		}
	}
	this.pointer = function(evt) {
		iconCloseHover = evt.x>=app.width-40 && evt.y<40;
		if(evt.type==='start') {
			if(evt.x>=app.width-40 && evt.y<40)
				return app.close();
			app.on(new Game(evt.x<app.width/2 ? 1 : 2));
		}
	}
	this.gamepad = function(evt) {
		if(evt.type==='axis' && (evt.axis===0 || evt.axis===6) && Math.abs(evt.value)>0.1)
			app.on(new Game(evt.value<0 ? 1 : 2));
	}
	this.leave = function() {
		audio.fadeOut(music, 1.5);
	}
}

//------------------------------------------------------------------
function Highscore(id) {
	var starfield = new Float32Array(6*Math.floor(app.width*app.height/2048));
	var highscoreColor = 0;
	initStarfield(starfield, app.width, app.height, app.height/400, true);
	var ship = new Ship(id);
	ship.x = ship.y = 0;
	ship.right = 0.5;
	const vpSc = app.height/ARENA_H;
	const congrats = 'C O N G R A T U L A T I O N S', newHigh = 'You have set a new highscore!'
	audio.melody("{w:saw a:.025 d:.025 s:.25 r:.05 b:120} A3/12 C#4/12 E4/12 A4/8 E4/12 {s:.5 r:.45 g:1.5} A4/4", 0.5, 0.0);
	audio.melody("{w:saw a:.025 d:.025 s:.25 r:.05 b:120} A2/4 C#3/8 E3/12 A2/4", 0.5, 0.0);

	this.update = function(deltaT, now) {
		for(var i=0; i<20; ++i) {
			const idx = randi(starfield.length/starfield.stride)*starfield.stride;
			starfield[idx  ] = randi(app.width);
			starfield[idx+1] = randi(app.height);
		}
		ship.update(deltaT);
		highscoreColor += Math.PI*deltaT/0.6;
	}
	this.draw = function(gfx) {
		gfx.color(255,255,255).drawImages(gfx.IMG_CIRCLE, starfield.stride, gfx.COMP_SCALE|gfx.COMP_COLOR_RGB, starfield);
		gfx.fillText(app.width*0.5-congrats.length*6, app.height*0.35, congrats);
		drawFancyNumber(gfx.color(255, 255*Math.abs(Math.sin(highscoreColor)), 85), app.width/2-1.5*digitDims.width,app.height*0.45, highscore, 6);
		gfx.save().transform(app.width*0.5, app.height*0.575, 0, vpSc).lineWidth(0.15);
		ship.draw(gfx);
		gfx.restore();
		gfx.color(0xFFffFFff).fillText(app.width*0.5-congrats.length*6, app.height*0.35, congrats);
		gfx.fillText(app.width*0.5-newHigh.length*6, app.height*0.7, newHigh);
	}
	this.keyboard = function(evt) {
		switch(evt.key) {
		case ' ':
		case 'GoBack':
		case 'Escape':
		case 'Enter':
		case 'Control':
		case 'Tab':
			app.on(new Menu());
		}
	}
	this.pointer = function(evt) {
		if(evt.type==='start')
			app.on(new Menu());
	}
	this.gamepad = function(evt) {
		if(evt.type==='button')
			app.on(new Menu());
	}
}

app.on('load', function(){
	digitDims = app.queryImage(digit0);
});

app.on(new Menu());
