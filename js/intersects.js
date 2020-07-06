arcajs.intersects = {
	transform: function(arr, x,y,rot) {
		var cos = Math.cos(rot), sin = Math.sin(rot);
		for(var i=0, end=arr.length; i<end; i+=2) {
			var ax=arr[i], ay=arr[i+1];
			arr[i] = ax*cos - ay*sin + x, arr[i+1] = ax*sin + ay*cos + y;
		}
	},
	transformInv: function(arr, x,y,rot) {
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
	}
};
