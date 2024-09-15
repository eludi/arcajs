const audio = app.require('audio');

function timeoutCb(arg) {
	console.log("hello "+arg+" at "+(new Date()));
}

var img = 0, svg = 0, sample = 0;
function httpGetResource() {
	const imgurl = "https://eludi.net/caramboli/resource/icon.128.png";
	const svgurl = "https://eludi.net/droned/eludi.link.svg";
	const audiourl = "https://eludi.net/louloudia/resource/flute.mp3";
	const jsurl = "https://eludi.net/labs/fib.js";
	app.httpGet(imgurl, function(obj, statusCode) {
		console.log("\tGET Image", statusCode, '->', obj.width+'x'+obj.height+'x'+obj.depth, '=', obj.data.length);
		img = app.createImageResource(obj);
	});
	app.httpGet(svgurl, function(obj, statusCode) {
		console.log("\tGET SVG", statusCode, '->', typeof obj, obj.length);
		svg = app.createSVGResource(obj);
	});
	app.httpGet(audiourl, function(obj, statusCode) {
		console.log("\tGET MP3", statusCode, '->', obj.samples, obj.channels);
		sample = audio.uploadPCM(obj);
		audio.replay(sample);
	});
	app.httpGet(jsurl, function(js, statusCode) {
		console.log("\tGET JS", statusCode, '->', typeof js, js.length);
		const fib = new Function(js);
		const ret = fib();
		console.log('return value:', ret);
	});
}

setTimeout(timeoutCb, 3000, 3000);
setTimeout(function() { app.close(); }, 8000);
setTimeout(timeoutCb, 5000, 5000);
setTimeout(timeoutCb, 500, 500);

console.visible(true);
console.log("arcajs", app.version);

console.log("GET...");
app.httpGet("https://eludi.net/stats", function(data, statusCode) {
	console.log("\tGET", statusCode, '->', data);
});
console.log("after GET");


console.log("POST...");
app.httpPost("https://eludi.net/echo", { hello:"world", "wer war's?":"Die kleine Maus!" }, function(data, statusCode) {
	console.log("\tPOST", statusCode, '->', data);
	httpGetResource();
});

app.on('draw', function(gfx) {
	if(img)
		gfx.drawImage(img, 100,200);
	if(svg)
		gfx.drawImage(svg, 400,200);
});

app.on('close', function() {
	console.log('test manual cleanup');
	app.releaseResource(img, 'image');
	app.releaseResource(svg, 'image');
	app.releaseResource(sample, 'audio');
});
