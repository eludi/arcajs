var tileSet = app.getResource('basic_shapes.svg');
var icons = app.getResource('icons.svg');
var font = app.getResource('BebasNeue-Regular.ttf', {size:56});
var circle = app.createCircleResource(128);
const scorePerClick = [100,25,10,5,4,3,2,1];

app.setBackground([0x33, 0x33, 0x33]);
var buttons = [];

var numShapes = 12;
var tilesX=5, tilesY=4;

function randi(v1, v2) {
    if(v2===undefined) { v2=v1; v1=0; }
    return v1+Math.floor(Math.random()*(v2-v1));
}

function padZeros(value, numDigits) {
    var s = ''+value;
    if(value>=0)
        while(s.length<numDigits)
            s = '0'+s;
    return s;
}

const UI = {
    Button: function(image, x,y,w,h, callback) {
        this.color = [255,255,255,170];
        this.isSelected = false;
        var imgDims = app.queryImage(image);
        var srcX = 0, srcY = 0, srcW=imgDims.width, srcH=imgDims.height;

        this.setSource = function(x,y,w,h) {
            srcX=x;
            srcY=y;
            if(w!==undefined) {
                srcW=w;
                srcH=h;
            }
        }

        this.handlePointer = function(evt) {
            if(evt.x<x || evt.y<y || evt.x >= x+w || evt.y>=y+h)
                this.isSelected = false;
            else
                this.isSelected = true;
            if(this.isSelected && evt.type=='start') {
                callback();
                return true;
            }
            return false;
        }
        this.update = function(deltaT, tNow) { }
        this.draw = function(gfx) {
            gfx.color(this.color[0],this.color[1],this.color[2],
                this.color[3]*(this.isSelected ? 1 : 0.5))
                .drawImage(image,srcX, srcY,srcW,srcH, x,y,w,h);
        }
    },

    Label: function(font, x,y, text, align) {
        this.color = [255,255,255,85];
        this.selColor = [255,255,255,255];
        this.text = text;
        var hilighted = false, tHilighted=0;

        this.setValue = function(text, hilight) {
            this.text = text;
            tHilighted = hilight/3;
        }
        this.handlePointer = function(evt) { }
        this.update = function(deltaT, tNow) {
            if(tHilighted<=0)
                return;
            tHilighted = Math.max(tHilighted-deltaT, 0);
            hilighted = tHilighted*3-Math.floor(tHilighted*3)>0.5;
        }
        this.draw = function(gfx) {
            gfx.color(hilighted ? this.selColor : this.color).fillText(font,x,y, this.text, align);
        }
    }
}

var layout = { ox:0, oy:0, gridSz:100, tileSz:90, borderSz:5 };

function initLayout(layout) {
    const winSzX = app.width, winSzY = app.height;
    if(winSzY>winSzX) {
        var tmp = tilesX;
        tilesX = tilesY;
        tilesY = tmp;
    }
    const maxTileW=winSzX/tilesX, maxTileH = winSzY/tilesY;
    layout.gridSz = Math.floor(Math.min(maxTileW, maxTileH));
    layout.tileSz = Math.floor(layout.gridSz*0.9);
    layout.borderSz = Math.floor(layout.gridSz/20);
    layout.ox = Math.floor(0.5*(winSzX-tilesX*(layout.tileSz+2*layout.borderSz)));
    layout.oy = Math.floor(0.5*(winSzY-tilesY*(layout.tileSz+2*layout.borderSz)));
}
initLayout(layout);

function Game(tilesX, tilesY, tileSet) {
    const numPairs = Math.floor(tilesX*tilesY/2);
    var numPairsFound = 0, clicks = 0, score=0;
    var state = 'intro';
    var tiles=[], tilesSelected=[];
    var cursorX = 0, cursorY = 0, cursorVisible = true;
    var backCol = [170,0,85];
    var labelScore = new UI.Label(font, 4,0, '0000');
    labelScore.selColor = [220,255,170,255];

    var numCirclesToDraw = 0;
    var circles = [];
    var cd = layout.gridSz/2, cx = app.width/2-cd/2, cy = app.height/2-cd;
    for(var angle=Math.PI*0.85; angle>=Math.PI*0.15; angle-=Math.PI*0.025)
        circles.push(cx+2.5*cd*Math.cos(angle), cy+2.5*cd*Math.sin(angle));
    circles.push(cx-1.5*cd, cy-1.5*cd, cx+1.5*cd, cy-1.5*cd);

    function randomizeTiles(numShapes, numPairs) {
        var shapes=[], bg=[], fg=[];
        for(var i=0; i<numShapes; ++i)
            shapes.push(i);
        while(shapes.length>numPairs)
            shapes.splice(randi(shapes.length), 1);

        for(var i=0; i<shapes.length; ++i)
            shapes[i] = { shape:shapes[i], bg:[randi(2)*85,randi(2)*85,randi(2)*85],
                fg:[randi(2,4)*85,randi(2,4)*85,randi(2,4)*85] };

        shapes = shapes.concat(shapes);
        var tiles = [];
        while(shapes.length)
            tiles.push(shapes.splice(randi(shapes.length), 1)[0]);
        return tiles;
    }

    function evaluateSelection() {
        if(tilesSelected.length!=2)
            return;
        state = 'evaluating';
        var t0 = tiles[tilesSelected[0].index], t1 = tiles[tilesSelected[1].index];
        if(t0.shape === t1.shape) {
            t0.tSelected = t1.tSelected = 0;
            t0.state = t1.state = 'disappear';
            ++numPairsFound;
            const delta = clicks<scorePerClick.length ? scorePerClick[clicks] : 1;
            score += delta;
            labelScore.setValue(padZeros(score,4), delta>50 ? 3 : delta>=10 ? 2 : 1);
            setTimeout(function() {
                tiles[t0.index] = tiles[t1.index] = null;
                if(numPairsFound==numPairs) {
                    state = 'over';
                    setTimeout(start, 3000)
                }
                else
                    state = 'input';
                tilesSelected.length = 0;
                clicks = 0;
            }, 1200);
        }
        else {
            ++clicks;
            setTimeout(function() {
                tilesSelected[0].state = tilesSelected[1].state = 'hidden';
                tilesSelected.length = 0;
                state = 'input';
            }, 1200);
        }
    }

    function handleSelect(x,y) {
        if(state!='input')
            return;
        var tileId = y*tilesX+x, tile=tiles[tileId];
        if(!tile || tilesSelected.length>1 || tile.state!=='hidden')
            return;
        tile.state = 'visible';
        tile.tSelected = 0;
        tilesSelected.push(tile);
        if(tilesSelected.length>1)
            evaluateSelection();
    };

    this.handlePointer = function(evt) {
        if(state==='intro')
            return;
        cursorVisible = false;
        app.setPointer(true);
        if(evt.type!=='start')
            return;
        var x = Math.floor((evt.x-layout.ox) / layout.gridSz);
        var y = Math.floor((evt.y-layout.oy) / layout.gridSz);
        if(x>=0 && y>=0 && x<tilesX && y<tilesY)
            handleSelect(x, y);
    }

    this.handleKeyboard = function(evt) {
        cursorVisible = true;
        app.setPointer(false);
        if(evt.type!='keydown')
            return;
        switch(evt.key) {
            case 'ArrowRight': if(++cursorX>=tilesX) cursorX=0; break;
            case 'ArrowDown': if(++cursorY>=tilesY) cursorY=0; break;
            case 'ArrowLeft': if(--cursorX<0) cursorX+=tilesX; break;
            case 'ArrowUp': if(--cursorY<0) cursorY+=tilesY; break;
            case ' ':
            case 'Enter':
                return handleSelect(cursorX, cursorY);
        }
    }

    function update(deltaT, tNow) {
        if(state==='intro')
            tiles.forEach(function(tile) {
                if(tile.z>1000)
                    tile.z *= 0.95;
            });
        else tiles.forEach(function(tile) {
            if(!tile)
                return;
            if(tile.state==='visible') {
                if(tile.tSelected<0.5)
                    tile.z = 1-0.1*Math.sin(tile.tSelected*2*Math.PI);
                else
                    tile.z = 1;
                tile.tSelected += deltaT;
            }
            else if(tile.state==='disappear') {
                if(tile.tSelected<0.5)
                    tile.z = 1+0.1*Math.sin(tile.tSelected*2*Math.PI);
                else if(tile.z>0) {
                    tile.z -= deltaT*2;
                    if(tile.z>0)
                        tile.opacity -= 255*deltaT*2;
                    else {
                        tile.z = 0;
                        tile.opacity = 0;
                    }
                }
                tile.tSelected += deltaT;
            }
        });
        labelScore.update(deltaT, tNow);
    }

    this.draw = function(gfx) {
        const l=layout;
        if(state==='intro') {
            var cx = app.width/2, cy = app.height/2;
            for(var j=0, y=l.oy+l.borderSz, tileId=0; j<tilesY; ++j, y+=l.gridSz)
                for(var i=0, x=l.ox+l.borderSz; i<tilesX; ++i, x+=l.gridSz, ++tileId) {
                    const tile = tiles[tileId]
                    if(tile.z>50000)
                        continue;
                    var tileX = x-cx, tileY = y-cy;
                    var f = tile.z/1000;
                    gfx.color(170,f>=1 ? 0: 255, 85, f>1?127:255)
                        .fillRect(f*tileX+cx, f*tileY+cy, l.tileSz*f, l.tileSz*f);
                    if(f<1)
                        tile.z = 1000;
                }
            return;
        }
        if(state==='over') {
            gfx.color(220,255,170);
            for(var i=0, end=Math.min(++numCirclesToDraw,circles.length) ; i<end; i+=2)
                gfx.drawImage(circle, circles[i], circles[i+1], cd, cd);
            return;
        }

        for(var j=0, y=l.oy+l.gridSz/2, tileId=0; j<tilesY; ++j, y+=l.gridSz)
            for(var i=0, x=l.ox+l.gridSz/2; i<tilesX; ++i, x+=l.gridSz, ++tileId) {
                const tile = tiles[tileId]
                if(!tile)
                    continue;
                var sz = l.tileSz*tile.z, d=sz/2;
                if(tile.state==='hidden') {
                    gfx.color(backCol[0],backCol[1],backCol[2], tile.opacity);
                    gfx.fillRect(x-d,y-d, sz, sz);
                    continue;
                }
                const shape = tile.shape;
                const srcX = shape % tileSet.tilesX, srcY = Math.floor(shape/tileSet.tilesX), srcSz = tileSet.sz;
                gfx.color(tile.bg[0],tile.bg[1],tile.bg[2], tile.opacity).fillRect(x-d,y-d, sz,sz);
                gfx.color(tile.fg[0],tile.fg[1],tile.fg[2], tile.opacity).drawImage(
                    tileSet.img,
                    srcX*srcSz,srcY*srcSz,tileSet.sz,tileSet.sz, x-d,y-d, sz,sz);
            }

        if(cursorVisible) {
            gfx.color(220,255,170);
            var x = l.ox+l.gridSz*cursorX, y=l.oy+l.gridSz*cursorY;
            gfx.drawRect(x+2,y+2, l.gridSz-4, l.gridSz-4);
        }
        labelScore.draw(gfx);
    }

    randomizeTiles(tileSet.tilesX*tileSet.tilesY, numPairs).forEach(function(tile, index) {
        tiles.push({shape:tile.shape, index:index, state:'hidden', bg:tile.bg, fg:tile.fg,
            opacity:255, z:randi(10000,100000) });
    });

    app.on('update', update);
    app.setPointer(false);
    setTimeout(function() {
        cursorVisible = true; state = 'input'; 
        tiles.forEach(function(tile) { tile.z = 1; });
    }, 2000);
}
var game = null;

function start() {
    var tileSetDims = app.queryImage(tileSet);
    game = new Game(tilesX, tilesY,
        { img:tileSet, tilesX:4, tilesY:3, sz:tileSetDims.width/4 });
}

app.on('load', function() {
    var btnClose = new UI.Button(icons,app.width-48,8,40,40, function() {
        app.close();
    });
    btnClose.setSource(0,128,128,128);
    buttons.push(btnClose);

    start();
});

app.on('pointer', function(evt) {
    for(var i=0, end=buttons.length; i<end; ++i)
        if(buttons[i].handlePointer(evt))
            return;
    if(game)
        game.handlePointer(evt);
});

app.on('keyboard', function(evt) {
    if(game)
        game.handleKeyboard(evt);
});

app.on('draw', function(gfx) {
    if(game)
        game.draw(gfx);
    for(var i=0, end=buttons.length; i<end; ++i)
        buttons[i].draw(gfx);
});
