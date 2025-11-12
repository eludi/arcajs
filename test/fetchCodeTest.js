var codeName = 'fib';
var codeUrl = "https://eludi.net/labs/" + codeName + ".js";

// first check if code is locally cached:
const cachedRemoteSources = localStorage.getItem('remoteSources') || [];
for(var i=cachedRemoteSources.length; i-->0; ) {
	const codeName = cachedRemoteSources[i];
	const codeCached = localStorage.getItem(codeName);
	if(codeCached) {
		console.log('evaluating cached remote source', codeName, '...');
		(new Function(codeCached))();
	}
	else {
		console.warn('cached remote source', codeName, 'not found');
		cachedRemoteSources.splice(i, 1);
		localStorage.setItem('remoteSources', cachedRemoteSources);
	}
}
if(cachedRemoteSources.indexOf(codeName) >= 0) {
	console.log('READY.');
	app.close(0);
}
else { // fetch remote code:
	app.httpGet(codeUrl, function(js, statusCode) {
		console.log("fetch("+codeUrl+')', statusCode, '->', typeof js, js.length);
		if(statusCode !== 200)
			return app.close(statusCode);
		cachedRemoteSources.push(codeName);
		localStorage.setItem('remoteSources', cachedRemoteSources);
		localStorage.setItem(codeName, js);
		(new Function(js))();
		console.log('READY.');
		app.close(0);
	});
}

app.on('update', function(deltaT) {
	console.log('.');
});

// test if code could be alternatively loaded from resources: Yes
console.log('---\n' + app.getResource('fetchCodeTest.js'));
