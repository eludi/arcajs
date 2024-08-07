#pragma once
static const char* emitAsGamepadEvent_js =
"function(evt, index, axes, buttons) {\n"
"	if(evt.repeat)\n"
"		return;\n"
"\n"
"	const self = arguments.callee;\n"
"	if(!('state' in self))\n"
"		self.state = {};\n"
"	if(!(index in self.state)) {\n"
"		const state = self.state[index] = { axes:[], buttons:[] };\n"
"		if(axes)\n"
"			for(var i=0; i<axes.length; i+=2)\n"
"				state.axes.push(0);\n"
"		if(buttons)\n"
"			for(var i=0; i<buttons.length; ++i)\n"
"				state.buttons.push(false);\n"
"		this.emit('gamepad', {index:index, name:'keyboard', type:'connect', axes:state.axes.length, buttons:state.buttons.length});\n"
"	}\n"
"\n"
"	const state = self.state[index];\n"
"	const keydown = evt.type==='keydown';\n"
"	if(!keydown && evt.type!=='keyup')\n"
"		return;\n"
"\n"
"	if(axes) for(var i=0; i<axes.length; ++i) {\n"
"		if(evt.key !== axes[i])\n"
"			continue;\n"
"		const axis = Math.floor(i/2), value = (i%2)?1:-1;\n"
"		if(keydown) {\n"
"			if(state.axes[axis]===value)\n"
"				return;\n"
"		}\n"
"		else if(state.axes[axis]===0 || state.axes[axis]!==value)\n"
"			return;\n"
"		state.axes[axis] = keydown ? value : 0;\n"
"		return this.emit('gamepad', {index:index, name:'keyboard', type:'axis', axis:axis, value:state.axes[axis]});\n"
"	}\n"
"\n"
"	if(buttons) for(var i=0; i<buttons.length; ++i) {\n"
"		const key = (typeof buttons[i] === 'object') ? buttons[i].key : buttons[i];\n"
"		const location = (typeof buttons[i] === 'object') ? buttons[i].location : 0;\n"
"		if(evt.key === key && state.buttons[i]!=keydown && (location===0 || evt.location===location)) {\n"
"			state.buttons[i]=keydown;\n"
"			return this.emit('gamepad', {index:index, name:'keyboard', type:'button', button:i, value:keydown?1:0});\n"
"		}\n"
"	}\n"
"}\n";
