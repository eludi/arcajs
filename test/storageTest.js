var timeStamp = 0;

if(!localStorage.getItem("hello"))
	localStorage.setItem("hello", "localStorage");

app.on('pointer', function(evt) {
	if(evt.type!='start')
		return; 

	if(evt.id==1)
		localStorage.removeItem("hello");
	else if(evt.id==2)
		localStorage.setItem("hello", timeStamp);
	console.log(localStorage.getItem("hello"));
});

app.on('update', function(deltaT, now) {
	timeStamp = now;
});

app.on('draw', function(gfx) {
	gfx.colorf(1,1,1).fillText(0,0,0, timeStamp);
});
