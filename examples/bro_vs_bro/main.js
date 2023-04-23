// BOEHM VS BOEHM REMAKE 2021-04-05 - original C64 source code (~1988) lost
// proposed extension (non-original): meals/cherries splash up for extra score, mushrooms for increased girth, 2 make you blow up

const isx = app.require('intersects'), audio = app.require('audio');
const sprites = app.createTileResources('boehms.png',2,4,0, {filtering:0, centerX:0.5, centerY:0.5});
const powerups = app.createTileResources('powerups.png',4,1,0, {filtering:0, centerX:0.5, centerY:0.5});
const attackInterval = 1.0, attackHop = 20;

var spriteJ, spriteT, powerup, now=0, numKnockOuts, state='init';

const sfx = {
	bling: audio.createSound('tri', 880,.8,0,0.5, 880,.1,.07,0.5, 1760,.7,0,0.5, 1760,.1,.07,0.5, 3520,.6,0,0.5, 3520,0.1,.05,0.5, 3520,0.0,.1,0.5),
	hit: audio.createSound('bin', 0,1,0.1,0.005, 0,0.3,0.1,0.005, 0,0,0.3,0.005), // explo
	moop: audio.createSound('squ', 40,0.5,0.33,0.5),
	burst: audio.createSound('noi', 1,0.3,0,0.4, 1,0.1,0.1,0.2, 1,0,0.3,0.1), // snarelo
	bwb: audio.createSound('squ', 110,0.8,0.01,0.5, 55,0,0.01,0.5),
	grow: audio.createSound('saw',
		'E5',0,0,1, 'E5',0.6,0.005,1, 'E5',0.1,0.05,1, 'F5',0,0,1, 'F5',0.5,0.005,1, 'F5',0.1,0.05,1,
		'F#5',0,0,1, 'F#5',0.4,0.005,1, 'F#5',0.1,0.05,1, 'G5',0,0,1, 'G5',0.3,0.005,1, 'G5',0.1,0.05,1 ),
};

function lpad(value, numDigits, char) {
	if(char===undefined)
		char = '0';
	var s = ''+value;
	if(value>=0)
		while(s.length<numDigits)
			s = char+s;
	return s;
}

function randi(v1, v2) {
	if(v2===undefined) { v2=v1; v1=0; }
	return Math.floor(v1+Math.random()*(v2-v1));
}

function Petscii(charset, sz) {
	this.write = function(gfx, x,y,str) {
		for(var i=0; i<str.length; ++i, x+=sz) {
			const ch = str.charCodeAt(i);
			gfx.stretchImage(charset+ch, x,y,sz,sz);
		}
	}
}

function Player(id) {
	this.id = id;
	this.cx = this.cy = 0.5;
	this.color = 0xffFFaaFF;
	this.radius = 8;
	this.score = 0;

	this.reset = function() {
		this.x = Math.floor(256*(id+1)/3);
		this.y = 100;
		this.rot = 0;
		this.sc = 1.0;// + this.id;
		this.velx = this.vely = 0;
		this.tAttack = 0;
		this.status = 'ok';
		this.image = sprites+id;
	}

	this.knockOut = function() {
		if(this.status!=='ok')
			return;
		this.status = 'ko';
		this.tAttack = 2;
		if(this.y>23.5*8)
			this.y=23.5*8;
		this.image = sprites+id+2;
		++numKnockOuts;
	}
	this.burst = function() {
		if(this.status!=='ok')
			return;
		this.status = 'burst';
		this.tAttack = 0.5;
		audio.replay(sfx.burst, 0.7, 2*this.x/256 - 1);
	}

	this.attack = function() {
		if(this.tAttack>0 || this.status!=='ok')
			return;
		this.x += attackHop*(id===0 ? 0.5:-0.5);
		audio.replay(sfx.bwb, 0.25, 2*this.x/256 - 1, randi(0,7));
		this.tAttack = attackInterval;
	}
	this.over = function() {
		if(this.status === 'ko')
			return;
		this.status = 'celebrating';
		this.image = sprites+id+4;
		this.tAttack = 1;
	}

	this.update = function(deltaT) {
		if(this.status==='ok') {
			this.x += this.velx * deltaT;
			this.y += this.vely * deltaT;
			this.x = Math.round(this.x);
			this.y = Math.round(this.y);
		}
		else if(this.status==='burst') {
			this.sc = 1/this.tAttack; 
		}
		else if(this.status==='celebrating') {
			if(this.tAttack<=0) {
				this.tAttack = 1;
				this.image = (this.image === sprites+id+4) ? sprites+id+6 : sprites+id+4;
			}
		}
		this.tAttack -= deltaT;
		if((this.status==='ko' || this.status==='burst') && this.tAttack<=0)
			gameReset();
	}
	this.reset();
}

function Powerup() {
	const POWERUP_CHERRY = 0, POWERUP_MEAL = 1, POWERUP_MUSH = 2, POWERUP_HEART = 3;

	this.cx = this.cy = 0.5;
	this.color = 0xffFFaaFF;
	this.radius = 4;

	this.type = randi(4);
	this.score = 10+10*this.type;

	this.x = 127;
	this.y = randi(2) ? 20 : 180;
	this.sc = 0.5;
	this.image = powerups+this.type;

	this.affect = function(player) {
		if(this.type === POWERUP_MUSH) {
			if(player.sc > 1.0)
				return player.burst();
			else {
				player.sc = 2;
				audio.replay(sfx.grow, 0.25, 2*this.x/256 - 1);
			}
		}
		else audio.replay(sfx.bling, 0.25, 2*this.x/256 - 1);
		player.score += this.score;
	}
	this.update = function(deltaT) {
	}
}

function gameReset() {
	if(state === 'over')
		return;
	spriteJ.reset();
	spriteT.reset();
	powerup = null;
	now = 0;
}

var screenGame = {
	enter: function() {
		spriteJ = new Player(0);
		spriteT = new Player(1);
		state = 'running';
		numKnockOuts = 0;
	},
	update: function(deltaT) {
		if(state=='over') {
			spriteJ.update(deltaT);
			spriteT.update(deltaT);
			return;
		}

		const tBefore = now;
		now += deltaT;

		if(tBefore%8 > now%8) {
			if(Math.floor(now/8)%2)
				powerup = new Powerup();
			else if(powerup)
				powerup = null;
		}

		if(isx.sprites(spriteJ, spriteT)) {
			if(spriteJ.tAttack == attackInterval && spriteT.tAttack != attackInterval && spriteT.status==='ok') {
				spriteT.x += attackHop * spriteJ.sc / spriteT.sc;
			}
			else if(spriteJ.tAttack != attackInterval && spriteT.tAttack == attackInterval && spriteJ.status==='ok') {
				spriteJ.x -= attackHop * spriteT.sc / spriteJ.sc;
			}
			// else block movement?
		}
		spriteJ.update(deltaT);
		spriteT.update(deltaT);
		if(powerup)
			powerup.update(deltaT);

		[spriteJ, spriteT].forEach(function(s) {
			if(s.status !== 'ok')
				return;
			const verticalOff = s.y<s.radius || s.y>26*8-2*s.radius;
			if(s.x<9*8-s.radius || s.x > 23*8 + s.radius || verticalOff) {
				s.knockOut();
				audio.replay(verticalOff ? sfx.moop : sfx.hit, 0.25, 2*s.x/256 - 1);
				if(s === spriteJ)
					spriteT.score += 10;
				else
					spriteJ.score += 10;
			}
			if(powerup && isx.sprites(s, powerup)) {
				powerup.affect(s);
				powerup = null;
			}
		});
		if(numKnockOuts>4 && state==='running') {
			state = 'over';
			powerup = null;
			[spriteJ, spriteT].forEach(function(s) { s.over(); });
			// todo: play happy tune
		}
	},

	draw: function(gfx) {
		// playground:
		gfx.transform(originX, originY, 0, tilesz);
		gfx.color(0x25, 0x00, 0x20).fillRect(0,0, 40,25)
		gfx.color(0x77,0x55,0x00).fillRect(32,0, 1,25);
		for(var y=1; y<25; y+=2) {
			gfx.fillRect(9, y, 1,1);
			gfx.fillRect(22, y, 1,1);
		}

		// stats:
		gfx.color(0xff, 0xd8, 0x66);
		petscii.write(gfx, 34, 2, 'J.BRO');
		petscii.write(gfx, 35, 5, 'VS.');
		petscii.write(gfx ,34, 7, 'T.BRO');
		const progress = '\u00D1'.repeat(numKnockOuts)+'\u00D7'.repeat(5-numKnockOuts);
		petscii.write(gfx ,34, 23, progress);

		gfx.color(0xffffaaff);
		petscii.write(gfx,34,3, lpad(spriteJ.score, 5));
		petscii.write(gfx,34,8, lpad(spriteT.score, 5));

		if(state=='over') {
			const y = (spriteJ.y<80 || spriteJ.y>120) ? 12 : 18;
			petscii.write(gfx ,7,y, 'G A M E   O V E R .');
		}

		// sprites:
		gfx.reset().transform(originX, originY, 0, tilesz/8);
		gfx.color(0xffd866ff);
		if(powerup)
			gfx.drawSprite(powerup);
		gfx.drawSprite(spriteJ);
		gfx.drawSprite(spriteT);

		gfx.color(0,0,0,85);
		for(var y=-0.125; y<200; ++y) // scanlines
			gfx.fillRect(0,y,320,0.25);

	},

	keyboard: function(evt) {
		if(evt.type=='keydown' && evt.key=='Escape')
			return app.on(screenIntro);
		if(evt.type=='keydown' && evt.key=='1')
			audio.replay(sfx.bwb, 0.25, 0);
		if(evt.type=='keydown' && evt.key=='2')
			audio.sound('squ', 'F#2', 0.125, 0.1);
		if(evt.type=='keydown' && evt.key=='3')
			audio.melody('{w:squ a:.01 d:0 s:1 r:.01 b:170} F#2/8 -/8 F#2/8 -/8 F#2/8 -/8 F#2/8 -/8', 0.1);

		app.emitAsGamepadEvent(evt, 0, ['a','d', 'w','s'], ['Tab']);
		app.emitAsGamepadEvent(evt, 1, ['ArrowLeft','ArrowRight', 'ArrowUp','ArrowDown'], [' ']);
	},

	gamepad: function(evt) {
		var player = evt.index===0 ? spriteJ : evt.index===1 ? spriteT : null;
		if(!player)
			return;
 		if(evt.type==='axis') {
			if(evt.axis===0)
				player.velx = evt.value*tilesz*2;
			else if(evt.axis===1)
				player.vely = evt.value*tilesz*2;
		}
		else if(evt.type==='button' && evt.button===0 && evt.value>0)
			player.attack();
	}
};

app.on('load', function() {
	petscii = new Petscii(charset, 1);
	app.on(screenIntro);
});
