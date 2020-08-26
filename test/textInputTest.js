app.setBackground(31,0,47);

var style = {
    font: 0,
    bg: [0,0,0,0],
    fg: [255,255,255,170],
    bgFocus: [0,0,0,0],
    fgFocus: [255,255,255,255],
    hLine: 20
};

var textInput, textArea;

const UI = {
    TextArea: function(style,x,y,w,h) {
        var lines = [], currLine=0;
        for(var ly=y; ly+style.hLine<=h; ly+=style.hLine)
            lines.push('');

        this.draw = function(gfx) {
            var offset = currLine<lines.length ? 0 : currLine%lines.length;
            gfx.color(style.fg);
            for(var ly=y, index=0; index<lines.length; ly+=style.hLine, ++index)
                gfx.fillText(style.font,x,ly,lines[(index+offset)%lines.length]);
        }

        this.push = function(text) {
            lines[currLine++%lines.length] = text;
        }
    },

    TextInput: function(style, x,y,w,h, callback) {
        this.value = '';
        this.maxlength = Math.pow(2,31);
        var originX=0;
        var cursorVisible = true, hasFocus=false;
        var fontDim = app.queryFont(style.font,'_');
        var cursorX=0, cursorW = fontDim.width, cursorH = fontDim.height;
        var self = this;

        function updateCursor() {
            var dim = app.queryFont(style.font,self.value);
            for(originX=0; dim.width+cursorW>w; ++originX)
                dim = app.queryFont(style.font, self.value.substr(originX+1));
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
            if(style.bg[3])
                gfx.color(style.bg).fillRect(x,y,w,h);
            gfx.color(hasFocus ? style.fgFocus : style.fg);
            gfx.fillText(style.font, x,y, this.value.substr(originX));
            if(cursorVisible && hasFocus)
                gfx.fillRect(x+cursorX,y+1,cursorW,cursorH);
        }
    }
};

app.on('load', function(evt) {
    textArea = new UI.TextArea(style,0,0,app.width, app.height - 2*style.hLine);
    textInput = new UI.TextInput(style,0,app.height-style.hLine,app.width,style.hLine, function(value, input) {
        textArea.push(value);
        input.reset();
    });
    textInput.focus(true);
});

app.on('update', function(deltaT, now) {
    textInput.update(deltaT, now);
});

app.on('draw', function(gfx) {
    gfx.color(170,170,170).fillText(style.font,0,app.height-2*style.hLine,'Please enter your name:');
    textArea.draw(gfx);
    gfx.drawLine(0,app.height-2*style.hLine-2,app.width,app.height-2*style.hLine-2);
    textInput.draw(gfx);
});
