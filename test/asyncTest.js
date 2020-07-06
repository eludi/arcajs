function timeoutCb(arg) {
	console.log("hello "+arg+" at "+(new Date()));
}

setTimeout(timeoutCb, 3000, 3000);
setTimeout(function() { app.close(); }, 8000);
setTimeout(timeoutCb, 5000, 5000);
setTimeout(timeoutCb, 500, 500);

console.log("arcajs", app.version);

console.log("GET...");
app.httpGet("https://eludi.net/stats", function(data, statusCode) {
	console.log("\tGET", statusCode, '->', data);
});
console.log("after GET");


console.log("POST...");
app.httpPost("https://eludi.net/echo", { hello:"world", "wer war's?":"Die kleine Maus!" }, function(data, statusCode) {
	console.log("\tPOST", statusCode, '->', data);
});
