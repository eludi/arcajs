app.on('draw', function(gfx) {
	for(var i=0; i<360; i+=10) {
		gfx.colorHSL(i,1.0,0.5).fillRect(1.5*i,0,15,45);
		gfx.colorHSL(i,1.0,0.5,0.5).fillRect(1.5*i,45,15,45);
		gfx.colorHSL(i,0.5,0.5).fillRect(1.5*i,90,15,45);
		gfx.colorHSL(i,0,0.5).fillRect(1.5*i,135,15,45);
		gfx.colorHSL(i,1.0,0.75).fillRect(1.5*i,180,15,45);
		gfx.colorHSL(i,1.0,0.85).fillRect(1.5*i,225,15,45);
		gfx.colorHSL(i,1.0,1.0).fillRect(1.5*i,270,15,45);
	}
});
