app.setBackground(31,0,47);

var font = 0, lineHeight=20;
var textInput, textArea;

function TextArea(font,x,y,w,h, lineHeight) {
    var lines = [], currLine=0;
    for(var ly=y; ly+lineHeight<=h; ly+=lineHeight)
        lines.push('');

    this.draw = function(gfx) {
        var offset = currLine<lines.length ? 0 : currLine%lines.length;
        for(var ly=y, index=0; index<lines.length; ly+=lineHeight, ++index) {
            gfx.fillText(font,x,ly,lines[(index+offset)%lines.length]);
        }
    }

    this.push = function(text) {
        lines[currLine++%lines.length] = text;
    }
}

function TextInput(font, x,y,w,h, callback) {
    this.bg = [0,0,0,0];
    this.fg = [255,255,255,255];
    this.value = '';
    this.maxlength = Math.pow(2,31);
    var originX=0;
    var cursorVisible = true, hasFocus=false;
    var fontDim = app.queryFont(font,'_');
    var cursorX=0, cursorW = fontDim.width, cursorH = fontDim.height;
    var self = this;

    function updateCursor() {
        var dim = app.queryFont(font,self.value);
        for(originX=0; dim.width+cursorW>w; ++originX)
            dim = app.queryFont(font, self.value.substr(originX+1));
        cursorX = dim.width+1;
    }

    function handleInput(evt) {
        if(('char' in evt) && (self.value.length<self.maxlength)) {
            self.value += evt.char;
            updateCursor();
        }
        else if(evt.key == 'Backspace') {
            if(self.value.length) {
                self.value = self.value.substr(0,self.value.length-1);
                updateCursor();
            }
        }
        else if(evt.key == 'Enter' && callback)
            callback(self.value, self);
    }

    this.update = function(deltaT, now) {
        cursorVisible = (now%1.0 < 0.75);
    }

    this.focus = function(yesno) {
        if(yesno!==undefined) {
            hasFocus = yesno ? true : false;
            app.on('textinput', hasFocus ? handleInput : null);
        }
        return hasFocus;
    }

    this.reset = function(value) {
        this.value = (value===undefined) ? '' : String(value);
        updateCursor();
    }

    this.draw = function(gfx) {
        if(this.bg[3])
            gfx.color(this.bg).fillRect(x,y,w,h);
        gfx.color(this.fg[0], this.fg[1], this.fg[2], this.fg[3]*(hasFocus?1:0.66));
        gfx.fillText(font, x,y, this.value.substr(originX));
        if(cursorVisible && hasFocus)
            gfx.fillRect(x+cursorX,y+1,cursorW,cursorH);
    }
}

app.on('load', function(evt) {
    textArea = new TextArea(font,0,0,app.width, app.height - 2*lineHeight, lineHeight);
    textInput = new TextInput(font,0,app.height-lineHeight,app.width,lineHeight, function(value, input) {
        textArea.push(value);
        input.reset();
    });
    textInput.focus(true);
});

app.on('update', function(deltaT, now) {
    textInput.update(deltaT, now);
});

app.on('draw', function(gfx) {
    gfx.color(170,170,170).fillText(font,0,app.height-2*lineHeight,'Please enter your name:');
    textArea.draw(gfx);
    gfx.drawLine(0,app.height-2*lineHeight-2,app.width,app.height-2*lineHeight-2);
    textInput.draw(gfx);
});
