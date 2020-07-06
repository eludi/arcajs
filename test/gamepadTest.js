
var connected = [];
var states = [];

app.on('gamepad', function(evt) {
    console.log(JSON.stringify(evt));
    if(evt.type=='connected')
        connected[evt.index]=true;
    else if(evt.type=='disconnected')
        connected[evt.index]=false;
});

app.on('update', function(deltaT,now) {
    if(Math.floor(now-deltaT)!=Math.floor(now)) {
        for(var i=0; i<connected.length; ++i) {
            if(!connected[i]) {
                states[i]='';
                continue;
            }
            var gamepadState = app.getGamepad(i);
            delete gamepadState.connected;
            states[i] = JSON.stringify(gamepadState);
            console.log(states[i]);
        }
    }
});

app.on('draw', function(gfx) {
    if(connected.length)
        gfx.color(85,255,85);
    else
        gfx.color(255,85,85);
    gfx.fillRect(app.width-32,app.height-32,32,32);
    gfx.color(0,0,0);
    for(var i=0; i<states.length; ++i)
        gfx.fillText(0,0,24*i, states[i]);
});

