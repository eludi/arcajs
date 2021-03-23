const sc = app.width/80;
const circle = app.createCircleResource(sc/2,[255,255,255]);
const title = app.createSpriteSet(app.getResource('return.svg'),1,1);
const icons = app.createSpriteSet(app.getResource('icons.svg'), 4,1);

const audio = app.require('audio');
const sndReturn = app.getResource("sd0000.wav");
const sndReflect = app.getResource("sd7500.wav");
const sndScore = app.getResource("808-cowbell.mp3");
const sndMissed = app.getResource("bass01.mp3");
const sndApplause = app.getResource("applause.mp3");

const arenaW=80.0, arenaH=Math.floor(arenaW*app.height/app.width);
const scoreWin = 7, deltaWin=2;

function Ball() {
	const speedFactor = 20;
	this.x = this.y = this.velX = this.velY = this.numReturns = 0;

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
		var playerX = player.x + sgn * 0.75;
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

function Racket(id, autopilot) {
	const widthDefault = (arenaH/10+1);
	const speedFactor = 27;
	const updateInterval = 0.1;

	this.x = (id===0) ? 5 : arenaW-5;
	this.y = arenaH/2;
	this.velX =  this.velY = 0;
	this.width = widthDefault;
	this.score = 0;
	this.tUpdate = updateInterval;

	this.update = function(deltaT, ball) {
		this.tUpdate -= deltaT;
		if(autopilot && this.tUpdate<0) {
			this.tUpdate = updateInterval;
			this.velY=0.0;
			if(((id===0)&&ball.velX<0.0) || ((id===1)&&ball.velX>0.0)) {
				if(ball.y<this.y-this.width*0.25)
					this.velY=-1.0;
				else if(ball.y>this.y+this.width*0.25)
					this.velY=1.0;
			}
		}

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
	this.input = function(axis, value) {
		if(autopilot)
			return;
		if(value>1)
			value = 1;
		else if(value<-1)
			value = -1;
		if(axis===0) {
			if(id===0)
				value *=-1;
			this.width = value>0 ? (1+value)*widthDefault : (1+value/2)*widthDefault;
		}
		else if(axis===1)
			this.velY = value;
	}
	this.draw = function(gfx) {
		gfx.color(255,255,255).fillRect(this.x*sc-sc/2, (this.y-this.width/2)*sc, sc, this.width*sc);
	}
	this.impulse = function() { return widthDefault/this.width; }
}

function Game(players) {
	app.setPointer(0);
	var ball = new Ball();
	var player1 = new Racket(0, players<2);
	var player2 = new Racket(1, players<1);
	var running = true;
	var info = '';

	function gameOver(winner) {
		audio.replay(sndApplause, 0.7);
		info = winner+'   P L A Y E R   W I N S !';
		running = false;
		setTimeout(function() { app.on(new GameMenu()); }, 4000);
	}

	this.update = function(deltaT) {
		player1.update(deltaT, ball);
		player2.update(deltaT, ball);
		const prevX = ball.x;
		ball.update(deltaT);

		if (ball.velX<0) {
			if(ball.collision(player1, prevX))
				audio.replay(sndReturn, 0.5, -0.9);
		}
		else if(ball.collision(player2, prevX))
			audio.replay(sndReturn, 0.5, 0.9);

		if (ball.y<0.5) {
			ball.y=0.5;
			ball.velY*=-1;
			audio.replay(sndReflect, 0.5, ball.y*2/arenaW-1);
		}
		else if (ball.y>arenaH-0.5) {
			ball.y=arenaH-0.5;
			ball.velY*=-1;
			audio.replay(sndReflect, 0.5, ball.y*2/arenaW-1);
		}

		if (running && ball.x<0) {
			if(!ball.numReturns) {
				audio.replay(sndMissed, 0.5, -0.9);
				ball.init(1);
			}
			else {
				audio.replay(sndScore, 0.5, 0.5);
				if(++player2.score < scoreWin || player2.score < player1.score+deltaWin)
					ball.init();
				else
					gameOver('R I G H T');
			}
		}
		else if (running && ball.x>arenaW) {
			if(!ball.numReturns) {
				audio.replay(sndMissed, 0.5, 0.9);
				ball.init(-1);
			}
			else {
				audio.replay(sndScore, 0.5, -0.5);
				if(++player1.score < scoreWin || player1.score < player2.score+deltaWin)
					ball.init();
				else
					gameOver('L E F T');
			}
		}
	}
	this.draw = function(gfx) {
		player1.draw(gfx);
		player2.draw(gfx);
		ball.draw(gfx);
		gfx.color(255,255,255).fillText(0, app.width/2, sc,
			player1.score + ' : ' + player2.score, gfx.ALIGN_CENTER);
		gfx.fillText(0, app.width/2, app.height/2, info, gfx.ALIGN_CENTER_MIDDLE);
		gfx.color(0,0,0).fillRect(0,arenaH*sc, app.width,app.height-arenaH*sc+1);
	},
	this.keyboard = function(evt) {
		if(evt.type==='keydown') switch(evt.key) {
			case 'ArrowUp':
				player2.input(1, -1); break;
			case 'ArrowDown':
				player2.input(1, 1); break;
			case 'ArrowRight':
				player2.input(0, +1); break;
			case 'ArrowLeft':
				player2.input(0, -1); break;
			case 'w':
				player1.input(1, -1); break;
			case 's':
				player1.input(1, 1); break;
			case 'a':
				player1.input(0, -1); break;
			case 'd':
				player1.input(0, 1); break;
			case ' ':
			case 'Enter':
				if(players) break; // otherwise fall-through
			case 'Escape':
				app.on(new GameMenu()); break;
		}
		else if(evt.type==='keyup') switch(evt.key) {
			case 'ArrowUp':
				if(player2.velY<0) player2.input(1, 0); break;
			case 'ArrowDown':
				if(player2.velY>0) player2.input(1, 0); break;
			case 'ArrowRight':
			case 'ArrowLeft':
				player2.input(0, 0); break;
			case 'w':
				if(player1.velY<0) player1.input(1, 0); break;
			case 's':
				if(player1.velY>0) player1.input(1, 0); break;
			case 'a':
			case 'd':
				player1.input(0, 0); break;
		}
	}
	this.pointer = function(evt) {
		if(!players && evt.type==='start')
			return app.on(new GameMenu());
		if(evt.pointerType!=='touch')
			return;
		const x=evt.x/sc, y=evt.y/sc;
		if(x<6) {
			if(evt.type==='end' || Math.floor(y)==Math.floor(player1.y))
				player1.input(1, 0);
			else
				player1.input(1, (y-player1.y < 0) ? -1 : +1);
		}
		else if(x>=arenaW-6) {
			if(evt.type==='end' || Math.floor(y)==Math.floor(player2.y))
				player2.input(1, 0);
			else
				player2.input(1, (y-player2.y < 0) ? -1 : +1);
		}
	}
	this.gamepad = function(evt) {
		if(evt.type==='axis') {
			if(evt.index===0)
				player2.input(evt.axis, evt.value);
			else if(evt.index===1)
				player1.input(evt.axis, evt.value);
		}
	}
}

function GameMenu() {
	const marquee =" +++ Welcome to RETURN!, a little showcase app for the arcajs multimedia framework. +++ https://github.com/eludi/arcajs"
		+ " +++ This pong-like virtual tennis game can be controlled via touch, gamepad, or keyboard."
		+ " +++ left player: W,A,S,D; right player: cursor keys"
		+ " +++ To win a match, score at least "+scoreWin+" times and "+deltaWin+" times more than your opponent. +++ Enjoy! +++";
	const marqueeLen = marquee.length, marqueeWidth = marqueeLen*12;
	var marqueeX = app.width, hue=0;
	app.setPointer(1);

	this.update = function(deltaT) {
		hue += deltaT*60.0;
		if(hue>=360.0)
			hue -= 360.0;
		app.setBackground(app.hsl(hue,0.7,0.6));

		marqueeX -= deltaT*90;
		if(marqueeX<-marqueeWidth) {
			app.setBackground(app.hsl(Math.random()*360,0.7,0.6));
			app.on(new Game(0));
		}
	}
	this.draw = function(gfx) {
		gfx.color(255,255,255);
		gfx.drawTile(title,0, app.width*0.5, app.height*0.4, gfx.ALIGN_CENTER_BOTTOM);
		gfx.drawTile(icons, 1, app.width*0.3, app.height*0.6, gfx.ALIGN_CENTER_MIDDLE);
		gfx.fillText(0, app.width*0.3, app.height*0.6+100, 'SINGLE PLAYER',  gfx.ALIGN_CENTER_MIDDLE);
		gfx.drawTile(icons, 2, app.width*0.7, app.height*0.6,  gfx.ALIGN_CENTER_MIDDLE);
		gfx.fillText(0, app.width*0.7, app.height*0.6+100, 'TWO PLAYERS',  gfx.ALIGN_CENTER_MIDDLE);
		gfx.clipRect(20, app.height-30, app.width-40,20);
		gfx.fillText(0, marqueeX, app.height-30, marquee);
		gfx.clipRect();
		gfx.color(255,255,255, 85).drawTile(icons, 0, app.width, 0, gfx.ALIGN_RIGHT);
	}
	this.keyboard = function(evt) {
		switch(evt.key) {
		case '1': return app.on(new Game(1));
		case '2': return app.on(new Game(2));
		case 'Escape': if(evt.type==='keydown' && !evt.repeat)
			return app.close();
		}
	}
	this.pointer = function(evt) {
		if(evt.type==='start') {
			if(evt.x>=app.width-40 && evt.y<40)
				return app.close();
			app.on(new Game(evt.x<app.width/2 ? 1 : 2));
		}
	}
	this.gamepad = function(evt) {
		if(evt.type==='axis' && evt.axis===0)
			app.on(new Game(evt.value<0 ? 1 : 2));
	}
}

app.on(new GameMenu());
