var audio = app.require('audio');
audio.sound('squ', 440, 0.25, 0.2);
setTimeout(function(){ app.close()}, 500)
