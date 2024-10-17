var worker = null, now=0, info = 'hello, arcajs workers!';
app.on('load', function() {
	worker = new Worker('fibWorker.js');
	worker.onmessage(function(msg) {
		console.log('message by worker:', msg);
		info = JSON.stringify(msg);
        if(msg.result)
            setTimeout(function() { app.close() }, 4000);
	})
	worker.postMessage({call:'fib', n:35});
})

app.on('update', function(deltaT) {
	now += deltaT;
})

app.on('draw', function(gfx) {
	gfx.transform(app.width/2,app.height/2,now*Math.PI/2)
		.fillText(0,0, info,0, gfx.ALIGN_CENTER_MIDDLE)
})
