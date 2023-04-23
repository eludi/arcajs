const version = '001';
const cacheWhitelist = ['static_'+version];

this.addEventListener('install', (event)=>{
	console.log('cache version '+version+' installing...');
	event.waitUntil(caches.open('static_'+version).then((cache)=>{
		return cache.addAll([
			//'index.html',
			//'./', // alias for index.html
			//'arcajs.min.js',
			//...
		]);
	}));
});

this.addEventListener('activate', (event)=>{
	event.waitUntil(
		this.clients.claim().then(()=>{
		caches.keys().then((cacheNames)=>{
			return Promise.all(cacheNames.map((name)=>{
				if (cacheWhitelist.indexOf(name) === -1)
					return caches.delete(name);
			}));
		}).then(()=>{
			console.log('cache version '+version+' ready to handle fetches.');
		})
	}));
});

this.addEventListener('fetch', (event)=>{
	if (event.request.headers.get('range')) {
		var pos = Number(/^bytes\=(\d+)\-$/g.exec(event.request.headers.get('range'))[1]);
		console.log('Range request for', event.request.url, ', starting position:', pos);
		event.respondWith(caches.open(CURRENT_CACHES.prefetch)
			.then(function(cache) { return cache.match(event.request.url); })
			.then(function(res) {
				if (!res) {
					return fetch(event.request).then(res => {
						return res.arrayBuffer();
					});
				}
				return res.arrayBuffer();
			})
		  	.then(function(ab) {
				return new Response(ab.slice(pos), {
					status: 206,
					statusText: 'Partial Content',
					headers: [
						['Content-Range', 'bytes ' + pos + '-' + (ab.byteLength - 1) + '/' + ab.byteLength]
					]
				});
			})
		);
	}
	else event.respondWith(
		caches.match(event.request).then((response)=>{ // cache falling back to network
			if (event.request.cache === 'only-if-cached' && event.request.mode !== 'same-origin')
				return;
			return response || fetch(event.request);
		})
	);
});
