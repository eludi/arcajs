
var fs = app.require('fs');

function docExtract(inFilename) {
	var input = fs.readFileSync(inFilename, 'utf-8');

	var doc = [];
	var longDoc = false;
	input.split(/\r?\n/).forEach(function(line) {
		if(!longDoc && line.indexOf('/**')>=0) {
			line = line.substr(line.indexOf('/**')+3);
			longDoc = true;
		}
		if(longDoc) {
			if(line.indexOf("*/")>=0) {
				longDoc = false;
				line = line.substr(0, line.indexOf("*/"));
			}
			line = line.trim();
			if(line.startsWith('*'))
				line = line.substr(2);
			doc.push(line);
		}
		else if(line.indexOf('///')>=0)
			doc.push(line.substr(line.indexOf('///')+3).trim());
	});
	return doc;
}

function doc2md(doc) {
	function removeTag(line, tag) {
		return line.substr(line.indexOf(tag)+tag.length+1).trim();
	}

	var out = [];
	var mode = '';
	doc.forEach(function(line) {
		if (mode!='' && line.indexOf("@"+mode)<0) {
			mode='';
			out.push('');
		}
	
		if(line.indexOf("@module")>=0) {
			out.push("## module " + removeTag(line, "@module"), '');
		}
		else if(line.indexOf("@function")>=0) {
			out.push("### function " + removeTag(line, "@function"), '');
		}
		else if(line.indexOf("@brief")>=0) {
			out.push(removeTag(line, "@brief"), '');
		}
		else if(line.indexOf("@param")>=0) {
			if(mode!='param') {
				mode='param';
				out.push('', '#### Parameters:', '');
			}
			out.push("- " + removeTag(line, "@param"));
		}
		else if(line.indexOf("@constant")>=0) {
			if(mode!='constant') {
				mode='constant';
				out.push('', '### Constants:', '');
			}
			out.push("- " + removeTag(line, "@constant"));
		}
		else if(line.indexOf("@property")>=0) {
			if(mode!='property') {
				mode='property';
				out.push('', '### Properties:', '');
			}
			out.push("- " + removeTag(line, "@property"));
		}
		else if(line.indexOf("@return")>=0) {
			if(mode!='return') {
				mode='return';
				out.push('', '#### Returns:', '');
			}
			out.push("- " + removeTag(line, "@return"), '');
		}
		else
			out.push(line);
	});

	out.forEach(function(line, index) { out[index] = line.replace(/\|/g, '\\|'); });
	return out;
}


var doc = ['# arcajs API','','- [app](#module-app)','- [audio](#module-audio)',
	'- [console](#module-console)','- [graphics](#module-gfx)',
	'- [intersects](#module-intersects)','- [sprites](#module-sprites)', '']
	.concat(docExtract("../jsBindings.c"))
	.concat(docExtract("../spritesBindings.c")).concat(docExtract("../modules/intersectsBindings.c"));
doc = doc2md(doc);
fs.writeFileSync("API.md", doc.join('\n').replace(/\n(\n)+/g, '\n\n'));
app.close();
