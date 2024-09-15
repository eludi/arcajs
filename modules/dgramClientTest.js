var socket = null;
const port = 4567;

app.on('load', function() {
    console.visible(true);
    socket = app.require("dgram").createSocket(port, "localhost");
    console.log('connecting to localhost:'+ port+'...');
    socket.write({msg:"hello", id:Math.floor(Math.random()*100)});

    const s = socket.read(1/60, 'string');
    console.log(s ? s : 'no reply');
    setTimeout(function() { app.close(); }, 16);
});
