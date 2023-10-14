arcajs.intersects = {
	transform: function(arr, x,y,rot=0) {
		var cos = Math.cos(rot), sin = Math.sin(rot);
		for(var i=0, end=arr.length; i<end; i+=2) {
			var ax=arr[i], ay=arr[i+1];
			arr[i] = ax*cos - ay*sin + x, arr[i+1] = ax*sin + ay*cos + y;
		}
	},
	transformInv: function(arr, x,y,rot=0) {
		var cos = Math.cos(-rot), sin = Math.sin(-rot);
		for(var i=0, end=arr.length; i<end; i+=2) {
			var ax=arr[i]-x, ay=arr[i+1]-y;
			arr[i] = ax*cos - ay*sin, arr[i+1] = ax*sin + ay*cos;
		}
	},

	pointCircle: function(x, y, cx, cy, r) {
		var dx = cx-x, dy = cy-y;
		return dx*dx + dy*dy < r*r;
	},

	pointAlignedRect: function(x, y, x1, y1, x2, y2) {
		return (x > x2 || x1 > x || y > y2 || y1 > y) ? 0 : 1;
	},
	
	pointPolygon: function(x, y, polygon) {
		if(polygon.length < 6)
			return false; // check if a triangle or higher n-gon

		// keep track of cross product sign changes
		var pos = 0, neg = 0;

		for (var i = 0; i < polygon.length; i+=2) {
			var x1 = polygon[i];
			var y1 = polygon[i+1];

			if (x==x1 && y==y1)
				return true;

			var x2 = polygon[(i+2)%polygon.length];
			var y2 = polygon[(i+3)%polygon.length];

			// compute the cross product
			var d = (x - x1)*(y2 - y1) - (y - y1)*(x2 - x1);

			if (d > 0)
				pos++;
			else if (d < 0)
				neg++;

			// if the sign changes, then point is outside
			if (pos > 0 && neg > 0)
				return false;
		}
		// if on the same side of all segments then inside
		return true;
	},
	circleCircle: function(x1, y1, r1, x2, y2, r2) {
		var dx = x2-x1, dy = y2-y1, sumR = r1 + r2;
		return dx*dx + dy*dy < sumR*sumR;
	},
	circleAlignedRect: function(cx,cy,r, x0,y0,x1,y1) {
		if(cx<x0-r || cx>=x1+r || cy<y0-r || cy>=y1+r)
			return false;
		if(cx<x0) {
			if(cy<y0) {
				var dx=x0-cx, dy=y0-cy;
				return dx*dx + dy*dy < r*r;
			}
			if(cy>y1) {
				var dx=x0-cx, dy=cy-y1;
				return dx*dx + dy*dy < r*r;
			}
		}
		else if(cx>x1) {
			if(cy<y0) {
				var dx=cx-x1, dy=y0-cy;
				return dx*dx + dy*dy < r*r;
			}
			if(cy>y1) {
				var dx=cx-x1, dy=cy-y1;
				return dx*dx + dy*dy < r*r;
			}
		}
		return true;
	},

	circleTriangle: function(cx, cy, radius, x1,y1, x2,y2, x3,y3) {
		// based on http://www.phatcode.net/articles.php?id=459
		// TEST 1: Vertex within circle
		var radiusSqr = radius*radius;

		var c1x = cx - x1, c1y = cy - y1;
		var c1sqr = c1x*c1x + c1y*c1y - radiusSqr;
		if(c1sqr <= 0)
			return true;

		var c2x = cx - x2, c2y = cy - y2;
		var c2sqr = c2x*c2x + c2y*c2y - radiusSqr;
		if(c2sqr <= 0)
			return true;

		var c3x = cx - x3, c3y = cy - y3;
		var c3sqr = c3x*c3x + c3y*c3y - radiusSqr
		if(c3sqr <= 0)
			return true;

		// TEST 2: Circle center within triangle
		// Calculate edges
		var e1x = x2 - x1, e1y = y2 - y1;
		var e2x = x3 - x2, e2y = y3 - y2;
		var e3x = x1 - x3, e3y = y1 - y3;
		if (e1y*c1x >= e1x*c1y) {
			if(e2y*c2x >= e2x*c2y && e3y*c3x >= e3x*c3y)
				return true;
		}
		else if(e2y*c2x < e2x*c2y && e3y*c3x < e3x*c3y)
			return true;

		// TEST 3: Circle intersects edge
		var k = c1x*e1x + c1y*e1y;
		if(k > 0) {
			var lenSqr = e1x*e1x + e1y*e1y;
			if(k < lenSqr) {
				if(c1sqr * lenSqr <= k*k)
					return true;
			}
		}
		// Second edge
		k = c2x*e2x + c2y*e2y;
		if(k > 0) {
			var lenSqr = e2x*e2x + e2y*e2y;
			if(k < lenSqr) {
				if(c2sqr * lenSqr <= k*k)
					return true;
			}
		}
		// Third edge
		k = c3x*e3x + c3y*e3y;
		if(k > 0) {
			var lenSqr = e3x*e3x + e3y*e3y;
			if(k < lenSqr) {
				if(c3sqr * lenSqr <= k*k)
					return true;
			}
		}
		return false;
	},
	circlePolygon: function(x, y, r, polygon) {
		if(polygon.length < 6)
			return false; // check if a triangle or higher n-gon
		// TEST 1: Vertex within circle:
		var rSqr = r*r;
		for(var i=0, end=polygon.length; i<end; i+=2) {
			var dx = polygon[i]-x, dy=polygon[i+1]-y;
			if(dx*dx + dy*dy < rSqr)
				return true;
		}
		// TEST 2: Circle center within polygon:
		if(this.pointPolygon(x,y, polygon))
			return true;
		// TEST 3: Circle intersects edge:
		for(var i=0, end=polygon.length; i<end; i+=2) {
			var x1 = polygon[i];
			var y1 = polygon[i+1];
			var x2 = polygon[(i+2)%polygon.length];
			var y2 = polygon[(i+3)%polygon.length];

			var v1x = x2-x1, v1y = y2-y1; // already calculated as dx before
			var v2x = x-x1, v2y = y-y1;

			var sc = ( v1x*v2x + v1y*v2y ) / ( v1x*v1x + v1y*v1y );
			if(sc<0 || sc>1.0)
				continue;
			// project circle center onto edge:
			var v3x = v1x*sc, v3y = v1y*sc;
			var v4x = v2x-v3x, v4y = v2y-v3y;
			if(v4x*v4x + v4y*v4y < rSqr)
				return true;
		}
		return false;
	},
	circleTriangles: function(cx, cy, radius, arr, tx=0,ty=0,trot=0) {
		var c = [cx,cy];
		this.transformInv(c, tx, ty, trot);
		for(var i=0, numTriangles=Math.floor(arr.length/6); i<numTriangles; ++i)
			if(this.circleTriangle(c[0], c[1], radius, arr[6*i], arr[6*i+1], arr[6*i+2], arr[6*i+3], arr[6*i+4], arr[6*i+5]))
				return true;
		return false;
	},

	alignedRectAlignedRect: function(x1min, y1min, x1max, y1max, x2min, y2min, x2max, y2max) {
		if(x2min > x1max || x1min > x2max)
			return false;
		if(y2min > y1max || y1min > y2max)
			return false;
		return true;
	},
	triangleTriangle: function(x11,y11, x12,y12, x13,y13, x21,y21, x22,y22, x23,y23) {
		var isOutside = function(x11,y11, x12,y12, x21,y21, x22,y22, x23,y23) {
			var ex = x12 - x11, ey = y12 - y11;
			var x = x21 - x11, y = y21 - y11;
			if(ey*x <= ex*y)
				return false;
			x = x22 - x11, y = y22 - y11;
			if(ey*x <= ex*y)
				return false;
			x = x23 - x11, y = y23 - y11;
			return ey*x > ex*y;
		}
		if(isOutside(x11,y11, x12,y12, x21,y21, x22,y22, x23,y23))
			return false;
		if(isOutside(x12,y12, x13,y13, x21,y21, x22,y22, x23,y23))
			return false;
		if(isOutside(x13,y13, x11,y11, x21,y21, x22,y22, x23,y23))
			return false;
		if(isOutside(x21,y21, x22,y22, x11,y11, x12,y12, x13,y13))
			return false;
		if(isOutside(x22,y22, x23,y23, x11,y11, x12,y12, x13,y13))
			return false;
		if(isOutside(x23,y23, x21,y21, x11,y11, x12,y12, x13,y13))
			return false;
		return true;
	},
	/** tests for intersection of two convex polygons using the Separating Axis Theorem
	*
	* @param a an array of connected points [x0, y0, x1, y1,...] that form a closed polygon
	* @param b an array of connected points [x0, y0, x1, y1,...] that form a closed polygon
	* @return true if there is any intersection between the 2 polygons, false otherwise
	*/
	polygonPolygon: function(a, b) {
		var polygons = [a, b];
		var minA, maxA, projected, i, i1, j, minB, maxB;

		for (i = 0; i < polygons.length; i++) {

			// for each polygon, look at each edge of the polygon, and determine
			// if it separates the two shapes
			var polygon = polygons[i];
			for (i1 = 0; i1 < polygon.length; i1+=2) {

				// grab 2 vertices to create an edge
				var i2 = (i1 + 2) % polygon.length;
				var p1x = polygon[i1], p1y = polygon[i1+1];
				var p2x = polygon[i2], p2y = polygon[i2+1];
				// find the line perpendicular to this edge
				var normalX = p2y - p1y, normalY = p1x - p2x;

				minA = maxA = undefined;
				// for each vertex of shape 1, project it onto the line perpendicular
				// to the edge and keep track of the min and max of these values
				for (j = 0; j < a.length; j+=2) {
					projected = normalX * a[j] + normalY * a[j+1];
					if ((minA===undefined) || projected < minA)
						minA = projected;
					if ((maxA===undefined) || projected > maxA)
						maxA = projected;
				}

				// for each vertex of shape 2, project it onto the line perpendicular
				// to the edge and keep track of the min and max of these values
				minB = maxB = undefined;
				for (j = 0; j < b.length; j+=2) {
					projected = normalX * b[j] + normalY * b[j+1];
					if ((minB===undefined) || projected < minB)
						minB = projected;
					if ((maxB===undefined) || projected > maxB)
						maxB = projected;
				}

				// if there is no overlap between the projects, this edge separates
				// the two polygons, and we know there is no overlap
				if (maxA < minB || maxB < minA)
					return false;
			}
		}
		return true;
	},
	/** @function intersects.polygonTriangles
	 * Test if a convex polygon and a triangle list intersect, both optionally transformed
	 * @param {array|ArrayBuffer} polygon - polygon ordinates
	 * @param {number} x1 - polygon x translation
	 * @param {number} y1 - polygon y translation
	 * @param {number} rot1 - polygon rotation
	 * @param {array|ArrayBuffer} triangles - triangle ordinates
	 * @param {number} [x2=0] - triangles x translation
	 * @param {number} [y2=0] - triangles y translation
	 * @param {number} [rot2=0] - triangles rotation
	 * @returns {boolean} true if the two objects intersect
	 */
	polygonTriangles: function(poly, x1,y1,rot1, triangles,x2=0,y2=0,rot2=0) {
		let p1 = poly;
		if(x1 || y1 || rot1) {
			p1 = poly.slice(0); // copy
			this.transform(p1, x1,y1,rot1);
		}
		for(var i=0, numTriangles=Math.floor(triangles.length/6); i<numTriangles; ++i) {
			let tr = triangles.slice(6*i,6*i+6);
			if(x2 || y2 || rot2)
				this.transform(tr, x2,y2,rot2);
			if(this.polygonPolygon(p1, tr))
				return true;
		}
		return false;
	},

	/** @function intersects.trianglesTriangles
	 * Test if two triangle lists intersect, both optionally transformed
	 * @param {array|ArrayBuffer} tr1 - first triangle ordinates
	 * @param {number} x1 - first triangles x translation
	 * @param {number} y1 - first triangles y translation
	 * @param {number} rot1 - first triangles rotation
	 * @param {array|ArrayBuffer} second tr2 - second triangle ordinates
	 * @param {number} [x2=0] - second triangles x translation
	 * @param {number} [y2=0] - second triangles y translation
	 * @param {number} [rot2=0] - triangles rotation
	 * @returns {boolean} true if the two objects intersect
	 */
	trianglesTriangles: function(tr1,x1,y1,rot1, tr2,x2=0,y2=0,rot2=0) {
		for(var j=0, numTriangles1=Math.floor(tr1.length/6); j<numTriangles1; ++j) {
			let t1 = tr1.slice(6*j,6*j+6);
			if(x1 || y1 || rot1)
				this.transform(t1, x1,y1,rot1);

			for(var i=0, numTriangles2=Math.floor(tr2.length/6); i<numTriangles2; ++i) {
				let t2 = tr2.slice(6*i,6*i+6);
				if(x2 || y2 || rot2)
					this.transform(t2, x2,y2,rot2);
				if(this.triangleTriangle(...t1, ...t2))
					return true;
			}
		}
		return false;
	},

	/** @function intersects.spritesCoarse
	 * Test if two sprite objects intersect based on their position (x,y,rot) and bounding radius or rectangle (w,h)
	 * @param {object} s1 - first sprite
	 * @param {object} s2 - second sprite
	 * @returns {boolean} true if the two objects intersect
	 */
	spritesCoarse: function(s1,s2) {
		const isx = this;
		if(s1.radius>=0.0 && s2.radius>=0.0)
			return isx.circleCircle(s1.x, s1.y, s1.radius, s2.x, s2.y, s2.radius);
		if(s1.radius>=0.0) {
			if(!s2.rot) {
				const x1 = s2.x - s2.cx*s2.w || 0, y1=s2.y - s2.cy*s2.h || 0;
				return isx.circleAlignedRect(s1.x, s1.y, s1.radius, x1,y1, x1+s2.w,y1+s2.h);
			}
			let c = [ s1.x, s1.y ];
			const ox = -s2.cx * s2.w || 0, oy = -s2.cy * s2.h || 0;
			const arr = [
				ox, oy,
				ox, oy + s2.h,
				ox + s2.w, oy + s2.h,
				ox + s2.w, oy
			];
			isx.transformInv(c, s2.x, s2.y, s2.rot);
			return isx.circlePolygon(c[0], c[1], s1.radius, arr);
		}
		if(s2.radius>=0.0) {
			if(!s1.rot) {
				const x1 = s1.x - s1.cx * s1.w || 0, y1=s1.y - s1.cy * s1.h || 0;
				return isx.circleAlignedRect(s2.x, s2.y, s2.radius, x1,y1, x1+s1.w,y1+s1.h);
			}
			let c = [ s2.x, s2.y ];
			const ox = -s1.cx * s1.w || 0, oy = -s1.cy * s1.h || 0;
			const arr = [
				ox, oy,
				ox, oy + s1.h,
				ox + s1.w, oy + s1.h,
				ox + s1.w, oy
			];
			isx.transformInv(c, s1.x, s1.y, s1.rot);
			return isx.circlePolygon(c[0], c[1], s2.radius, arr);
		}
		if(!s1.rot && !s2.rot) {
			const x1min = s1.x - s1.cx * s1.w || 0, y1min = s1.y - s1.cy * s1.h || 0;
			const x1max = x1min + s1.w, y1max = y1min + s1.h;
			const x2min = s2.x - s2.cx * s2.w || 0, y2min = s2.y - s2.cy * s2.h || 0;
			const x2max = x2min + s2.w, y2max = y2min + s2.h;
			return isx.alignedRectAlignedRect(
				x1min,y1min, x1max,y1max, x2min,y2min, x2max,y2max);
		}

		let ox = -s1.cx * s1.w || 0, oy = -s1.cy * s1.h || 0;
		let arr1 = [
			ox, oy,
			ox, oy + s1.h,
			ox + s1.w, oy + s1.h,
			ox + s1.w, oy
		];
		isx.transform(arr1, s1.x, s1.y, s1.rot);
		ox = -s2.cx * s2.w || 0;
		oy = -s2.cy * s2.h || 0;
		let arr2 = [
			ox, oy,
			ox, oy + s2.h,
			ox + s2.w, oy + s2.h,
			ox + s2.w, oy
		];
		isx.transform(arr2, s2.x, s2.y, s2.rot);
		return isx.polygonPolygon(arr1, arr2);
	},
	/** @function intersects.sprites
	 * Test if two sprite objects intersect
	 * 
	 * The test is based on the following object attributes:
	 * - position (x,y,rot) currently NO scale
	 * - bounding radius (radius) or rectangle (w,h) with optional center (cx,cy)
	 * - optionally convex hull (shape) or triangle list (triangles)
	 * 
	 * @param {object} s1 - first sprite
	 * @param {object} s2 - second sprite
	 * @returns {boolean} true if the two objects intersect
	 */
	sprites: function(s1,s2) {
		const isx = this;
		const intersectsCoarse = isx.spritesCoarse(s1,s2);
		if(!intersectsCoarse || (!s1.shape && !s2.shape && !s1.triangles && !s2.triangles))
			return intersectsCoarse;

		if(s1.shape && s2.shape) {
			var shape1 = s1.shape, shape2 = s2.shape;
			if(s1.x || s1.y || s1.rot) {
				shape1 = new Float32Array(shape1);
				isx.transform(shape1, s1.x, s1.y, s1.rot);
			}
			if(s2.x || s2.y || s2.rot) {
				shape2 = new Float32Array(shape2);
				isx.transform(shape2, s2.x, s2.y, s2.rot);
			}
			return isx.polygonPolygon(shape1, shape2);
		}
	
		if(s1.triangles && s2.triangles)
			return isx.trianglesTriangles(s1.triangles, s1.x,s1.y,s1.rot, s2.triangles,s2.x,s2.y,s2.rot);
	
		var sShape, sNoShape, shape;
		if(s1.shape) {
			shape = s1.shape;
			sShape = s1;
			sNoShape = s2;
		}
		else {
			shape = s2.shape;
			sShape = s2;
			sNoShape = s1;
		}

		function rect2poly(rect) {
			const ox = -rect.cx * rect.w || 0, oy = -rect.cy * rect.h || 0;
			const poly = [ ox, oy, ox, oy + rect.h, ox + rect.w, oy + rect.h, ox + rect.w, oy ];
			if(rect.x || rect.y || rect.rot)
				isx.transform(poly, rect.x, rect.y, rect.rot);
			return poly;
		}

		if(shape) { // bounding shape against triangles/circle/rect:
			if(sNoShape.triangles)
				return isx.polygonTriangles(sShape.shape, sShape.x, sShape.y, sShape.rot,
					sNoShape.triangles, sNoShape.x, sNoShape.y, sNoShape.rot);
		
			if(sNoShape.radius>=0.0) {
				let c = [sNoShape.x, sNoShape.y];
				isx.transformInv(c, sShape.x, sShape.y, sShape.rot);
				return isx.circlePolygon(c[0], c[1], sNoShape.radius, shape);
			}

			const poly = rect2poly(sNoShape);
			if(sShape.x || sShape.y || sShape.rot) {
				shape = new Float32Array(shape);
				isx.transform(shape, sShape.x, sShape.y, sShape.rot);
			}
			return isx.polygonPolygon(poly, shape);
		}
		
		// triangle list against circle/rect:
		var sTr, sNoTr;
		if(s1.triangles) {
			sTr = s1;
			sNoTr = s2;
		}
		else {
			sTr = s2;
			sNoTr = s1;
		}
		if(sNoTr.radius>=0.0)
			return isx.circleTriangles(sNoTr.x, sNoTr.y, sNoTr.radius,
				sTr.triangles, sTr.x, sTr.y, sTr.rot);
		const poly = rect2poly(sNoTr);
		return isx.polygonTriangles(poly, 0,0,0, sTr.triangles,sTr.x,sTr.y,sTr.rot);
	}
};
