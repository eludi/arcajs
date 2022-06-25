
var gamepads = [];
console.visible(true);

app.on('gamepad', function(evt) {
    delete evt.evt;
    console.log(evt);
    if(evt.type=='connected') {
        gamepads[evt.index]={ axes:new Array(evt.axes), buttons:new Array(evt.buttons), name:evt.name };
        //app.message(JSON.stringify(gamepads[evt.index]));
        for(var i=0; i<evt.axes; ++i)
            gamepads[evt.index].axes[i]=0;
        for(var i=0; i<evt.buttons; ++i)
            gamepads[evt.index].buttons[i]=0;
    }
    else if(evt.type=='disconnected')
        gamepads[evt.index]=null;
    else if(evt.type=='axis' && gamepads[evt.index])
        gamepads[evt.index].axes[evt.axis] = evt.value;
    else if(evt.type=='button' && gamepads[evt.index])
        gamepads[evt.index].buttons[evt.button] = evt.value;
});

app.on('draw', function(gfx) {
    for(var i=0; i<gamepads.length; ++i) {
        const oy=i*80+20;
        if(gamepads[i]) {
            const gp = gamepads[i];
            var ox = app.width - (Math.ceil(gp.buttons.length/2)*30+80+(gp.axes.length-2)*30);
            gfx.color(255,255,255);
            gfx.fillText(ox,oy-18, gp.name);
            gfx.drawRect(ox,oy,50,50);
            gfx.fillRect(ox+20+25*gp.axes[0], oy+20+25*gp.axes[1], 10,10);
            ox+=60;

            for(var j=2; j<gp.axes.length; ++j, ox+=30) {
                gfx.drawRect(ox,oy,20,50);
                gfx.fillRect(ox, oy, 20, 25*(gp.axes[j]+1));
            }

            for(var j=0; j<gp.buttons.length; ++j)
                if(gp.buttons[j])
                    gfx.fillRect(ox + Math.floor(j/2)*30, oy+(j%2)*30, 20, 20);
                else
                    gfx.drawRect(ox + Math.floor(j/2)*30, oy+(j%2)*30, 20, 20);
            gfx.color(85,255,85);
        }
        else
            gfx.color(255,85,85);
        gfx.fillText(app.width-20,oy,i);
    }
});
