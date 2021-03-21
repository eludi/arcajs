
var connected = [];
console.visible(true);

app.on('gamepad', function(evt) {
    console.log(evt);
    if(evt.type=='connected')
        connected[evt.index]=true;
    else if(evt.type=='disconnected')
        connected[evt.index]=false;
});

app.on('draw', function(gfx) {
    for(var i=0; i<connected.length; ++i) {
        if(connected[i])
            gfx.color(85,255,85);
        else
            gfx.color(255,85,85);
        gfx.fillRect(app.width-32,i*32,32,32);
    }
});

