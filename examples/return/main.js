var sc = Math.floor(app.width/80);
const circle = app.createCircleResource(sc/2,[255,255,255]);
const triangle = app.createPathResource(8*sc,8*sc, ['M', 0, 0, 'L', 8*sc, 4*sc, 'L', 0, 8*sc, 'Z'], [255,255,255]);

const audio = app.require('audio');
const sndReturn = app.getResource("sd0000.wav");
const sndReflect = app.getResource("sd7500.wav");
const sndScore = app.getResource("808-cowbell.mp3");
const sndMissed = app.getResource("bass01.mp3");

const arenaW=80.0, arenaH=Math.floor(arenaW*app.height/app.width);
app.setBackground(0x00,0xaa,0xaa);
app.setPointer(0);

function Ball() {
	const speedFactor = 20;
	this.x =  this.y = this.velX = this.velY = this.numReturns = 0;

	this.init = function(velX) {
		this.x = arenaW/2;
		this.y = arenaH/2;
		this.velX = velX ? velX : (Math.random()>0.5 ? 1 : -1);
		this.velY = -1 + 2*Math.random();
		this.numReturns = 0;
	}

	this.update = function(deltaT) {
		this.x += this.velX*deltaT*speedFactor;
		this.y += this.velY*deltaT*speedFactor;
	}
	this.collision = function(player, prevX) {
		const sgn = (player.x>arenaW/2) ? -1.0 : 1.0;
		var playerX = player.x + sgn * 0.75; // 0.75 incorrect but looks better
		var penetrationX = this.x-playerX;
		function sign(x) { return (x>0) ? 1 : (x<0) ? -1 : 0; }
		if ((sign(penetrationX) != sign(prevX-playerX))
			&& (this.y>=(player.y-player.width/2))
			&& (this.y<=(player.y+player.width/2)))
		{
			this.velY += ((this.y-player.y)/(player.width/2));
			this.velX *= -player.impulse();
			this.x = playerX-penetrationX;
			++this.numReturns;
			return true;
		}
		return false;
	}

	this.draw = function(gfx) {
		gfx.color(255,255,255).drawImage(circle,this.x*sc-sc/2,this.y*sc-sc/2);
	}
	this.init();
}

function Racket(id) {
	const widthDefault = (arenaH/10+1);
	const speedFactor = 27;

	this.x = (id===0) ? 5 : arenaW-5;
	this.y = arenaH/2;
	this.velX =  this.velY = 0;
	this.width = widthDefault;
	this.score = 0;

	this.update = function(deltaT) {
		this.y += this.velY*deltaT*speedFactor;
		if(this.y<this.width/2) {
			this.y = this.width/2;
			this.velY = 0;
		}
		else if(this.y>arenaH-this.width/2) {
			this.y = arenaH-this.width/2;
			this.velY = 0;
		}
	}
	this.draw = function(gfx) {
		gfx.color(255,255,255).fillRect(this.x*sc-sc/2, (this.y-this.width/2)*sc, sc, this.width*sc);
	}
	this.stretch = function(stretch) {
		if(stretch>1)
			stretch = 1;
		else if(stretch<-1)
			stretch = -1;
		this.width = stretch>0 ? (1+stretch)*widthDefault : (1+stretch/2)*widthDefault;
	}
	this.impulse = function() { return widthDefault/this.width; }
}

const screenGame = {
	enter: function() {
		this.ball = new Ball();
		this.player1 = new Racket(0);
		this.player2 = new Racket(1);
	},
	update: function(deltaT) {
		this.player1.update(deltaT);
		this.player2.update(deltaT);
		const prevX = this.ball.x;
		this.ball.update(deltaT);

		if (this.ball.velX<0) {
			if(this.ball.collision(this.player1, prevX))
				audio.replay(sndReturn, 0.5, -0.9);
		}
		else if(this.ball.collision(this.player2, prevX))
			audio.replay(sndReturn, 0.5, 0.9);

		if (this.ball.y<0.5) {
			this.ball.y=0.5;
			this.ball.velY*=-1;
			audio.replay(sndReflect, 0.5, this.ball.y*2/arenaW-1);
		}
		else if (this.ball.y>arenaH-0.5) {
			this.ball.y=arenaH-0.5;
			this.ball.velY*=-1;
			audio.replay(sndReflect, 0.5, this.ball.y*2/arenaW-1);
		}

		if (this.ball.x<0) {
			if(!this.ball.numReturns) {
				audio.replay(sndMissed, 0.5, -0.9);
				this.ball.init(1);
			}
			else {
				++this.player2.score;
				audio.replay(sndScore, 0.5, 0.5);
				this.ball.init();
			}
		}
		else if (this.ball.x>arenaW) {
			if(!this.ball.numReturns) {
				audio.replay(sndMissed, 0.5, 0.9);
				this.ball.init(-1);
			}
			else {
				++this.player1.score;
				audio.replay(sndScore, 0.5, -0.5);
				this.ball.init();
			}
		}
	},
	draw: function(gfx) {
		this.player1.draw(gfx);
		this.player2.draw(gfx);
		this.ball.draw(gfx);
		gfx.color(255,255,255).fillText(0, app.width/2, sc,
			this.player1.score + ' : ' + this.player2.score, gfx.ALIGN_CENTER);
	},
	keyboard: function(evt) {
		if(evt.type==='keydown') switch(evt.key) {
		case 'ArrowUp':
			this.player2.velY = -1; break;
		case 'ArrowDown':
			this.player2.velY = 1; break;
		case 'ArrowRight':
			this.player2.stretch(+1); break;
		case 'ArrowLeft':
			this.player2.stretch(-1); break;
		case 'w':
			this.player1.velY = -1; break;
		case 's':
			this.player1.velY = 1; break;
		case 'a':
			this.player1.stretch(+1); break;
		case 'd':
			this.player1.stretch(-1); break;
		}
		else if(evt.type==='keyup') switch(evt.key) {
		case 'ArrowUp':
			if(this.player2.velY<0) this.player2.velY = 0; break;
		case 'ArrowDown':
			if(this.player2.velY>0) this.player2.velY = 0; break;
		case 'ArrowRight':
			this.player2.stretch(0); break;
		case 'ArrowLeft':
			this.player2.stretch(0); break;
		case 'w':
			if(this.player1.velY<0) this.player1.velY = 0; break;
		case 's':
			if(this.player1.velY>0) this.player1.velY = 0; break;
		case 'a':
			this.player1.stretch(0); break;
		case 'd':
			this.player1.stretch(0); break;
		}
	},
};

app.on({ // launch screen
	hue:0,
	update: function(deltaT) {
		this.hue += deltaT*60.0;
		if(this.hue>=360.0)
			this.hue -= 360.0;
	},
	draw: function(gfx) {
		gfx.colorHSL(this.hue,0.7,0.6).fillRect(0,0, app.width, app.height);
		gfx.color(255,255,255);
		gfx.drawImage(triangle,app.width/2-4*sc, app.height/2-4*sc);
		gfx.fillText(0, app.width/2, app.height*0.66, 'R E T U R N', gfx.ALIGN_CENTER);
	},
	keyboard: function() { app.on(screenGame); },
	pointer: function(evt) { if(evt.type==='start') app.on(screenGame); },
});
