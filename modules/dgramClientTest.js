var socket = null;
const port = 4567;

app.on('load', function() {
    console.visible(true);
    socket = app.require("dgram").createSocket(port, "localhost");
    console.log('connecting to localhost:'+ port+'...');
    socket.write("hello, "+Math.floor(Math.random()*100));
    app.close();
});
