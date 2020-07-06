app.setBackground(255,255,255);
var radius = 50;

function getCircleResource(radius, filled) {
    var circles = filled ? getCircleResource.fills : getCircleResource.outlines;
    if(radius in circles)
        return circles[radius];
    var res = filled ? app.createCircleResource(this.radius)
        : app.createCircleResource(this.radius,'transparent',1);
    circles[radius] = res;
    return res;
}
getCircleResource.fills = {}
getCircleResource.outlines = {}

app.on('draw', function(gfx) {
    gfx.color(0xcc,0xcc,0xcc).drawImage(getCircleResource(radius, true), app.width/2-radius*1.5,app.height/2-radius);
    gfx.color(0,0,0).drawImage(getCircleResource(radius, false), app.width/2-radius/2,app.height/2-radius);
    gfx.color(0,0,0).fillText(0,0, app.height-16, 'radius:'+radius);
});

app.on('keyboard', function(evt) {
    if(evt.type=='keydown') {
        if(evt.key=='ArrowUp')
            ++radius;
        else if(evt.key=='ArrowDown' && radius>1)
            --radius;
    }
});
