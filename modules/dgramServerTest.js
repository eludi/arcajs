var socket = null;
const port = 4567;

app.on('load', function() {
    console.visible(true);
    socket = app.require("dgram").createSocket(port);
    console.log('listening on port '+ port+'...');
});

app.on('update', function() {
    var arrBuf = socket.read(2);
    if(arrBuf) {
        var arr = new Uint8Array(arrBuf), s= '';
        for(var i=0; i<arr.length; ++i)
            if(arr[i]<128)
                s+=String.fromCharCode(arr[i]);
        console.log(s);
    }
});

app.on('close', function() {
    console.log('closing port '+ port+'...');
    socket.close();
});
