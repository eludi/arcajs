const defaultFontBig = app.createImageFontResource(0, {scale:2});

app.on('draw', function(gfx) {
    gfx.fillText(app.width/2,app.height/2, 'Hello, built-in font.', 0, gfx.ALIGN_CENTER_BOTTOM);
    gfx.fillText(app.width/2,app.height/2, 'Hello, built-in font.', defaultFontBig, gfx.ALIGN_CENTER_TOP);
});
