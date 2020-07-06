app.setBackground(255,255,255);
var radius = 64;
var circle = app.createCircleResource(radius, [255,255,255]);
var arrow = app.createPathResource(128,128, 'M 63 0 L 0 127 L 63 95 L 127 127 z')

var sprites = app.createSpriteSet(circle);
var s1 = sprites.createSprite();
s1.setColor(85,255,85);
s1.setPos(radius, app.height-radius);

var s2 = sprites.createSprite();
s2.setColor(255,255,85);
s2.setPos(app.width-radius, radius);


app.on('draw', function(gfx) {
    gfx.color(255,85,85).drawImage(circle, 0,0, 2*radius,2*radius, 0,0,2*radius,2*radius, radius,radius);
    gfx.color(85,85,255).drawImage(circle, app.width-2*radius,app.height-2*radius);
    gfx.color(0,0,0).drawImage(arrow, 0,0,2*radius,2*radius, app.width/2-radius,app.height/2-radius,2*radius,2*radius, radius,radius, Math.PI);
    gfx.drawSprites(sprites);
});
