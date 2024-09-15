
var gamepads=[], log=[], pointers={};
const vpSc = Math.max(1, Math.round(app.pixelRatio));

function logout(msg) {
    log.push((typeof msg === 'object') ? JSON.stringify(msg) : msg);
    if(log.length>200)
        log.shift();
}

app.on('pointer', function(evt) {
	if(evt.type === 'start' || evt.type === 'move') 
		pointers[evt.id] = { x:evt.x, y:evt.y };
	else if(evt.type === 'end')
		delete pointers[evt.id];
    if(evt.type in {'start':true, 'end':true }) {
        delete evt.evt;
        logout(evt);
    }
});

app.on('keyboard', function(evt) {
    if(evt.repeat)
        return;
    delete evt.evt;
    delete evt.repeat;
    logout(evt);
});

app.on('wheel', function(evt) { logout(evt); });

app.on('gamepad', function(evt) {
    if(evt.type=='connected') {
        gamepads[evt.index]={ axes:new Array(evt.axes), buttons:new Array(evt.buttons), name:evt.name };
        for(var i=0; i<evt.axes; ++i)
            gamepads[evt.index].axes[i]=0;
        for(var i=0; i<evt.buttons; ++i)
            gamepads[evt.index].buttons[i]=0;
    }
    else if(evt.type=='disconnected')
        gamepads[evt.index]=null;
    else if(evt.type=='axis' && gamepads[evt.index])
        gamepads[evt.index].axes[evt.axis] = evt.value;
    else if(evt.type=='button' && gamepads[evt.index]) {
        gamepads[evt.index].buttons[evt.button] = evt.value;
		if(gamepads[evt.index].buttons[6] && gamepads[evt.index].buttons[7]) // select and start
			app.close();
	}
    delete evt.evt;
    evt.idx = evt.index;
    delete evt.index;
    logout(evt);
});

app.on('draw', function(gfx) {
    const vpSzX = Math.floor(app.width/vpSc), vpSzY = Math.floor(app.height/vpSc);
    gfx.transform(0,0,0,vpSc);
    gfx.color(85,85,85);
    gfx.drawLine(0,0, vpSzX, vpSzY);
    gfx.drawLine(vpSzX, 0,0, vpSzY);

    gfx.color(85,85,255);
    for(var i=0, nLines = Math.floor(vpSzY/20)-1, j= Math.max(0, log.length-nLines); i<nLines && j<log.length; ++i, ++j)
        gfx.fillText(0,i*20, log[j]);

    for(var i=0; i<gamepads.length; ++i) {
        const oy=i*80+20;
        if(gamepads[i]) {
            const gp = gamepads[i];
            var ox = vpSzX - (Math.ceil(gp.buttons.length/2)*30+80+(gp.axes.length-2)*30);
            gfx.color(0,0,0,170).fillRect(ox,oy-20,vpSzX-ox,80);
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
        gfx.fillText(vpSzX-20,oy,i);
    }

    for(var id in pointers) {
        const evt = pointers[id];
        gfx.color(0xFFff557F).drawImage(gfx.IMG_CIRCLE, evt.x/vpSc, evt.y/vpSc, 0, 32/vpSc);
        gfx.color(0x00FF).fillText(evt.x/vpSc, evt.y/vpSc, id, 0, gfx.ALIGN_CENTER_MIDDLE);
    }

    gfx.color(0xFFff55FF).fillText(0,vpSzY, "window w:" + app.width + " h:" + app.height + " sc:" + app.pixelRatio.toFixed(3)
        + "   vp w:"+vpSzX + " h:" + vpSzY + " sc:" + vpSc, 0, gfx.ALIGN_LEFT_BOTTOM);
});
