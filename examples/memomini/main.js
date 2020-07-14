var tileSet = app.getResource('basic_shapes.svg');
var icons = app.getResource('icons.svg');
var circle = app.createCircleResource(128);

app.setBackground(0x33, 0x33, 0x33);
var buttons = [];

var numShapes = 12;
var tilesX=5, tilesY=4;

function randi(v1, v2) {
    if(v2===undefined) { v2=v1; v1=0; }
    return v1+Math.floor(Math.random()*(v2-v1));
}

function Button(image, x,y,w,h, callback) {
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
    this.draw = function(gfx) {
        gfx.color(this.color[0],this.color[1],this.color[2],
            this.color[3]*(this.isSelected ? 1 : 0.5))
            .drawImage(image,srcX, srcY,srcW,srcH, x,y,w,h);
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
    var numPairsRemaining = numPairs;
    var state = 'input';
    var tiles=[], tilesSelected=[];
    var cursorX = 0, cursorY = 0, cursorVisible = true;
    this.clicks = 0;

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
                fg:[randi(2,4)*85,randi(2,4)*t85,randi(2,4)*85] };

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
        ++this.clicks;
        if(tilesSelected[0].shape === tilesSelected[1].shape) {
            setTimeout(function() {
                tiles[tilesSelected[0].index] = tiles[tilesSelected[1].index] = null;
                if(--numPairsRemaining==0) {
                    state = 'over';
                    setTimeout(start, 4000)
                }
                else
                    state = 'input';
                tilesSelected.length = 0;
            }, 1200);
        }
        else {
            setTimeout(function() {
                tilesSelected[0].selected = tilesSelected[1].selected = false;
                tilesSelected.length = 0;
                state = 'input';
            }, 1200);
        }
    }

    function handleClick(x,y) {
        if(state!='input')
            return;
        var tileId = y*tilesX+x, tile=tiles[tileId];
        if(!tile || tilesSelected.length>1 || tile.selected)
            return;
        tile.selected = !tile.selected;
        tilesSelected.push(tile);
        if(tilesSelected.length>1)
            evaluateSelection();
    };

    this.handlePointer = function(evt) {
        cursorVisible = false;
        app.setPointer(true);
        if(evt.type!=='start')
            return;
        var x = Math.floor((evt.x-layout.ox) / layout.gridSz);
        var y = Math.floor((evt.y-layout.oy) / layout.gridSz);
        if(x>=0 && y>=0 && x<tilesX && y<tilesY)
            handleClick(x, y);
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
                return handleClick(cursorX, cursorY);
        }
    }

    this.draw = function(gfx) {
        const l=layout;
        if(state==='over') {
            gfx.color(255,255,255);
            for(var i=0, end=Math.min(++numCirclesToDraw,circles.length) ; i<end; i+=2)
                gfx.drawImage(circle, circles[i], circles[i+1], cd, cd);
            return;
        }

        for(var j=0, y=l.oy+l.borderSz, tileId=0; j<tilesY; ++j, y+=l.gridSz)
            for(var i=0, x=l.ox+l.borderSz; i<tilesX; ++i, x+=l.gridSz, ++tileId) {
                const tile = tiles[tileId]
                if(!tile)
                    continue;
                if(!tile.selected) {
                    gfx.color(170,0,85).fillRect(x,y, l.tileSz, l.tileSz);
                    continue;
                }
                const shape = tile.shape;
                const srcX = shape % tileSet.tilesX, srcY = Math.floor(shape/tileSet.tilesX), srcSz = tileSet.sz;
                gfx.color(tile.bg[0],tile.bg[1],tile.bg[2]).fillRect(x,y, l.tileSz, l.tileSz);
                gfx.color(tile.fg[0],tile.fg[1],tile.fg[2]).drawImage(
                    tileSet.img,
                    srcX*srcSz,srcY*srcSz,tileSet.sz,tileSet.sz,
                    x,y, l.tileSz,l.tileSz);
            }

        if(cursorVisible) {
            gfx.color(255,255,255);
            var x = l.ox+l.gridSz*cursorX, y=l.oy+l.gridSz*cursorY;
            gfx.drawRect(x+1,y+1, l.gridSz-2, l.gridSz-2);
        }
    }

    randomizeTiles(tileSet.tilesX*tileSet.tilesY, numPairs).forEach(function(tile, index) {
        tiles.push({shape:tile.shape, index:index, selected:false, bg:tile.bg, fg:tile.fg });
    });
}
var game = null;

function start() {
    var tileSetDims = app.queryImage(tileSet);
    game = new Game(tilesX, tilesY,
        { img:tileSet, tilesX:4, tilesY:3, sz:tileSetDims.width/4 });
}

app.on('load', function() {
    var btnClose = new Button(icons,app.width-48,8,40,40, function() {
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
