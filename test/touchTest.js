var touches = {};

app.on('pointer', function(evt) {
	console.log(evt);
	if(evt.device!='touch')
		return;
	if(evt.type=='start' || evt.type=='move') 
		touches[evt.id] = { x:evt.x, y:evt.y };
	else if(evt.type=='end')
		delete touches[evt.id];
});

app.on('draw', function(gfx) {
	for(var key in touches) {
		var touch = touches[key];
		gfx.color(85,255,85).drawLine(0,touch.y,app.width,touch.y);
		gfx.drawLine(touch.x,0,touch.x,app.height);
	}
});
