var socket = null;
const port = 4567;

app.on('load', function() {
    console.visible(true);
    socket = app.require("dgram").createSocket(port);
    console.log('listening on port '+ port+'...');
});

app.on('update', function() {
    var msg = socket.read(0.02, 'json');
    if(msg) {
        console.log(msg);
        socket.write('server received: '+JSON.stringify(msg));
    }
});

app.on('close', function() {
    console.log('closing port '+ port+'...');
    socket.close();
});
