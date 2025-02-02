
"use strict";

arcajs.Graphics = (function() {
	const defaultFontStr = `<svg xmlns="http://www.w3.org/2000/svg"
	width='192'
	height='256'
	version='1.1'>
   <g style='fill:none;stroke:#ffffff;stroke-width:2;stroke-linecap:butt;stroke-linejoin:miter' >
	 <circle style='fill:#ffffff' cx='176' cy='16' r='15'/>
	 <path d='m 74,80 v 9 l 4,4 4,-4 v -9' />
	 <path d='m 62,80 v 11 l 2,2 h 4 l 2,-2 V 80' />
	 <path style='stroke-linejoin:round' d='m 86,80 v 13 l 4,-4 4,4 V 80' />
	 <path d='m 46,84 v -1 l -2,-2 h -4 l -2,2 v 2 l 2,2 h 4 l 2,2 v 2 l -2,2 h -4 l -2,-2 v -1' />
	 <path d='m 98,80 v 2 l 4,4 v 2 l 4,4 v 2 m -8,0 v -2 l 4,-4 v -2 l 4,-4 v -2' />
	 <path d='m 114,89 v 5 m -4,-14 v 5 l 4,4 4,-4 v -5' />
	 <path d='m 162,217 v 5 m -4,-13 v 4 l 4,4 4,-4 v -4' />
	 <path d='m 121,81 h 9 v 2 l -8,8 v 2 h 9' />
	 <path d='m 141,81 h -5 v 12 h 5' />
	 <path style='stroke-linecap:round' d='m 146,83 9,9' />
	 <path d='m 26,96 v 13 h 6 l 2,-2 v -4 l -2,-2 h -6' />
	 <path d='m 46,103 v -1 l -1,-1 h -5 l -2,2 v 4 l 2,2 h 5 l 1,-1 v -1' />
	 <path d='m 94,231 v -1 l -1,-1 h -5 l -2,2 v 3 l 2,2 h 5 l 1,-1 v -1' />
	 <path d='m 58,101 h -6 l -2,2 v 4 l 2,2 h 6 V 96' />
	 <path d='m 62,105 h 8 v -2 l -2,-2 h -4 l -2,2 v 4 l 2,2 h 6' />
	 <path d='m 98,233 h 8 v -2 l -2,-2 h -4 l -2,2 v 4 l 2,2 h 6' />
	 <path d='m 110,233 h 8 v -2 l -2,-2 h -4 l -2,2 v 4 l 2,2 h 6'  />
	 <path d='m 80,102 h -7 m 9,-5 h -4 l -2,2 v 11' />
	 <path d='m 86,111 h 6 l 2,-2 v -8 h -6 l -2,2 v 2 l 2,2 h 6' />
	 <path d='m 98,101 h 5 l 2,2 v 7 M 98,96 v 14' />
	 <path d='m 114,97 v 2 m 0,1 v 10' />
	 <path d='m 128,97 v 2 m -5,10 v 1 l 1,1 h 3 l 1,-1 v -10' />
	 <path d='m 135,96 v 14' />
	 <path style='stroke-linecap:round' d='m 141,100 -5,5' />
	 <path style='stroke-linecap:round' d='m 141,109 -4,-4' />
	 <path d='m 149,96 v 12 l 1,1 h 3' />
	 <path d='m 162,101 v 9 m -4,0 v -9 h 7 l 1,1 v 8' />
	 <path d='M 2,94 V 81 h 6 l 2,2 v 3 L 8,88 H 2' />
	 <path style='stroke-linecap:round' d='m 19,90 3,3 m -6,-12 -2,2 v 8 l 2,2 h 3 l 3,-3 v -7 l -2,-2 z' />
	 <path d='M 26,94 V 81 h 6 l 2,2 v 3 l -2,2 h -6' />
	 <path style='stroke-linecap:round' d='M 34,93 29,88' />
	 <path d='m 2,128 v -11 h 6 l 2,2 v 3 l -2,2 H 2' />
	 <path d='m 22,128 v -11 h -6 l -2,2 v 3 l 2,2 h 6' />
	 <path d='m 27,126 v -9 h 5 l 1,1 v 1' />
	 <path d='m 37,125 h 6 l 1,-1 v -2 l -1,-1 h -4 l -1,-1 v -2 l 1,-1 h 6' />
	 <path d='m 49,117 h 8 m -5,-4 v 11 l 1,1 h 4' />
	 <path d='m 62,116 v 7 l 2,2 h 4 l 2,-2 v -7' />
	 <path d='m 111,244 v 7 l 2,2 h 4 l 2,-2 v -7' />
	 <path d='m 123,244 v 7 l 2,2 h 4 l 2,-2 v -7'  />
	 <path d='m 134,244 v 7 l 2,2 h 4 l 2,-2 v -7'  />
	 <path d='m 146,244 v 7 l 2,2 h 4 l 2,-2 v -7' />
	 <path d='m 146,211 v 8 l 2,2 h 4 l 2,-2 v -8' />
	 <path d='m 74,116 v 5 l 4,4 4,-4 v -5' />
	 <path d='m 90,123 v -5 m -4,-2 v 7 l 2,2 2,-2 2,2 2,-2 v -7' />
	 <path style='stroke-linecap:round' d='m 98,125 7,-8 m -7,0 7,8' />
	 <path style='stroke-linecap:round' d='m 88,219 4,-4 m -4,0 4,4' />
	 <path d='m 118,116 v 9 l -2,2 h -6 m 0,-11 v 5 l 2,2 h 6' />
	 <path d='m 166,244 v 9 l -2,2 h -6 m 0,-11 v 5 l 2,2 h 6' />
	 <path style='stroke-linejoin:round' d='m 121,117 h 8 v 1 l -7,7 h 8' />
	 <path d='m 150,112 v 15' />
	 <path style='stroke-linecap:round' d='m 170,117 2,-2 h 1 l 2,2 h 1 l 2,-2' />
	 <path d='m 171,110 v -9 h 5 l 2,2 v 7' />
	 <path d='m 182,103 v 4 l 2,2 h 4 l 2,-2 v -4 l -2,-2 h -4 z' />
	 <path d='M 9,133 H 1 m 8,4 H 1 m 9,-8 H 6 l -3,3 v 6 l 3,3 h 4' />
	 <path style='stroke-linecap:round' d='m 82,45 -5,-5 v -1 l -1,-1 v -3 l 1,-1 h 2 l 1,1 v 2 l -3,3 h -1 l -1,1 v 3 l 1,1 h 2 l 4,-4' />
	 <path d='m 104,33 h -1 l -2,2 v 8 l 2,2 h 1' />
	 <path d='M 90,38 V 33 H 88' />
	 <path style='stroke-linecap:round' d='m 123,44 3,-3 3,3' />
	 <path d='M 131,40 H 121' />
	 <path d='M 126,46 V 34' />
	 <path style='stroke-linecap:round' d='m 123,36 3,3 3,-3' />
	 <path style='stroke-linecap:round' d='M 44,44 46,34' />
	 <path style='stroke-linecap:round' d='M 39,44 41,34' />
	 <path d='M 38,36 H 48' />
	 <path d='M 37,42 H 47' />
	 <path d='m 54,32 v 14 m 4,-11 h -6 l -1,1 v 2 l 1,1 h 4 l 1,1 v 2 l -1,1 h -6' />
	 <path d='m 112,33 h 1 l 2,2 v 8 l -2,2 h -1' />
	 <path d='m 134,40 h 8' />
	 <path d='m 138,36 v 8' />
	 <path d='m 150,48 v -5 h -2' />
	 <path d='m 158,40 h 8' />
	 <path style='stroke-linecap:round' d='m 182,44 9,-9' />
	 <path style='stroke-linecap:round' d='m 153,49 -6,6 6,6' />
	 <path style='stroke-linecap:round' d='m 171,49 6,6 -6,6' />
	 <path d='m 167,54 h -9' />
	 <path d='m 167,58 h -9' />
	 <path d='m 186,62 v -2 m -4,-8 v -1 l 2,-2 h 4 l 2,2 v 1 l -4,4 v 3' />
	 <path d='m 122,74 v 1 l 2,2 h 4 l 2,-2 V 64' />
	 <path d='M 134,64 V 78' />
	 <path style='stroke-linecap:round' d='m 142,65 -6,6 6,6' />
	 <path d='m 146,64 v 13 h 9' />
	 <path style='stroke-linejoin:round' d='M 158,78 V 65 l 4,4 4,-4 v 13' />
	 <path style='stroke-linejoin:round' d='m 178,64 v 14 m -8,0 V 65 l 8,8' />
	 <path d='m 182,67 v 8 l 2,2 h 4 l 2,-2 v -8 l -2,-2 h -4 z' />
	 <path style='stroke-linecap:round' d='m 170,86 4,-4 h 1 l 4,4' />
	 <path d='m 181,95 h 11' />
	 <path d='m 159,93 h 5 V 81 h -5' />
	 <path style='stroke-linejoin:round' d='m 26,177 h 3 l 1,1 -1,1 h -1 l -1,1 v 1 h 4' />
	 <path style='stroke-linejoin:round' d='m 38,177 h 3 l 1,1 -1,1 1,1 -1,1 h -3' />
	 <path style='stroke-linejoin:round' d='m 163,185 h 3 l 1,1 -1,1 h -1 l -1,1 v 1 h 4' />
	 <path style='stroke-linejoin:round' d='m 169,178 h 3 l 1,1 -1,1 1,1 -1,1 h -3' />
	 <path style='stroke-linecap:round' d='m 53,179 2,-2' />
	 <g>
	   <path style='stroke-linecap:round' d='m 113,178 1,-1' />
	   <path d='m 114,182 v -5' />
	 </g>
	 <g transform='translate(33,1)'>
	   <path style='stroke-linecap:round' d='m 113,178 1,-1' />
	   <path d='m 114,182 v -5' />
	 </g>
	 <path d='m 125,177 h 2 l 1,1 v 2 l -1,1 h -2 l -1,-1 v -2 z' />
	 <path d='m 152,184 v 3 h 3' />
	 <path d='m 155,184 v 6' />
	 <path style='stroke-linecap:round' d='m 146,187 8,-8' />
	 <g transform='translate(45,1)'>
	   <path   style='stroke-linecap:round' d='m 113,178 1,-1' />
	   <path     d='m 114,182 v -5' />
	 </g>
	 <path style='stroke-linecap:round' d='m 158,187 8,-8' />
	 <path style='stroke-linecap:round' d='m 170,187 8,-8' />
	 <path d='m 176,184 v 3 h 3' />
	 <path d='m 179,184 v 6' />
	 <path d='m 5,177 h 2 l 1,1 v 2 l -1,1 H 5 l -1,-1 v -2 z' />
	 <path d='m 18,160 v 2' />
	 <path d='m 15,171 h 3' />
	 <path d='m 18,164 v 10' />
	 <path d='m 33,166 v -1 l -1,-1 h -4 l -1,1 v 5 l 1,1 h 4 l 1,-1 v -1' />
	 <path d='m 30,161 v 13' />
	 <path d='m 45,164 v -1 l -1,-1 h -2 l -1,1 v 8 l -1,1 h -1' />
	 <path d='m 47,172 h -5 l -1,-1' />
	 <path d='m 39,167 h 5' />
	 <path style='stroke-linecap:round' d='m 52,162 1,2' />
	 <path style='stroke-linecap:round' d='m 53,164 h 3 l 2,2 v 2 l -2,2 h -3 l -2,-2 v -2 z' />
	 <path style='stroke-linecap:round' d='m 52,172 1,-2' />
	 <path style='stroke-linecap:round' d='m 57,172 -1,-2' />
	 <path style='stroke-linecap:round' d='m 56,164 1,-2' />
	 <path d='m 18,180 v 6' />
	 <path d='m 15,183 h 6' />
	 <path d='m 15,188 h 6' />
	 <path d='m 62,160 v 4 l 4,4 4,-4 v -4' />
	 <path d='m 66,168 v 6' />
	 <path d='m 62,172 h 8' />
	 <path d='m 62,169 h 8' />
	 <path d='m 78,160 v 7' />
	 <path d='m 78,168 v 7' />
	 <path d='m 93,162 h -5 l -1,1 v 5 l 1,1 h 5' />
	 <path d='m 87,165 h 5 l 1,1 v 5 l -1,1 h -5' />
	 <path d='m 100,161 v 2' />
	 <path d='m 104,161 v 2' />
	 <path d='m 83,177 h -8 l -1,1 v 3 l 1,1 c 0,0 3,0 3,0' />
	 <path d='m 78,177 v 12' />
	 <path d='m 81,177 v 12' />
	 <path d='M 62,191 V 180' />
	 <path d='m 62,184 3,3 h 1 l 3,-3' />
	 <path d='m 69,180 v 8' />
	 <path d='m 99,190 h 2 l 1,-1 v -1' />
	 <path d='m 87,239 h 2 l 1,-1 v -1' />
	 <path d='m 144,164 h 10 v 6' />
	 <path style='stroke-linecap:round;stroke-linejoin:round' d='m 134,186 2,-2 -2,-2' />
	 <path style='stroke-linecap:round;stroke-linejoin:round' d='m 139,186 2,-2 -2,-2' />
	 <path style='stroke-linecap:round;stroke-linejoin:round' d='m 141,170 -2,-2 2,-2' />
	 <path style='stroke-linecap:round;stroke-linejoin:round' d='m 136,170 -2,-2 2,-2' />
	 <path d='m 186,179 v 3 l -4,4 v 1 l 2,2 h 4 l 2,-2 v -1' />
	 <path d='m 186,178 v -1' />
	 <path d='m 182,147 v 2 l 4,4 4,-4 v -2' />
	 <path d='m 186,158 v -5' />
	 <path d='m 182,146 v -2' />
	 <path d='m 190,146 v -2' />
	 <path style='stroke-linecap:round;stroke-linejoin:round' d='m 89,184 1,1 1,-1 -1,-1 z' />
	 <path d='M 10,77 H 4 L 2,75 v -8 l 2,-2 h 5 l 2,2 v 6 l -1,1 H 6 L 5,73 v -4 l 1,-1 h 2 v 6' />
	 <path d='M 6,103 V 98 H 8' />
	 <path style='stroke-linecap:round' d='m 16,194 1,-1' />
	 <g>
	   <path style='stroke-linejoin:round' d='m 14,206 v -6 l 4,-4 4,4 v 6' />
	   <path d='m 14,203 h 8' />
	 </g>
	 <path style='stroke-linecap:round' d='M 8,194 7,193' />
	 <g transform='translate(12)'>
	   <path style='stroke-linejoin:round' d='m 14,206 v -6 l 4,-4 4,4 v 6' />
	   <path d='m 14,203 h 8' />
	 </g>
	 <path style='stroke-linecap:round' d='m 28,194 1,-1 h 2 l 1,1' />
	 <path style='stroke-linecap:round' d='m 40,194 1,-1 h 2 l 1,1 h 1 l 1,-1' />
	 <g transform='translate(24)'>
	   <path style='stroke-linejoin:round' d='m 14,206 v -6 l 4,-4 4,4 v 6' />
	   <path d='m 14,203 h 8' />
	 </g>
	 <path style='stroke-linecap:round;stroke-linejoin:round' d='m 65,194 1,1 1,-1 -1,-1 z' />
	 <path d='m 52,192 v 2' />
	 <path d='m 56,192 v 2' />
	 <g transform='translate(36)'>
	   <path style='stroke-linejoin:round' d='m 14,206 v -6 l 4,-4 4,4 v 6' />
	   <path d='m 14,203 h 8' />
	 </g>
	 <g transform='translate(48)'>
	   <path style='stroke-linejoin:round' d='m 14,206 v -6 l 4,-4 4,4 v 6' />
	   <path d='m 14,203 h 8' />
	 </g>
	 <path d='m 74,206 v -9 l 4,-4 h 1' />
	 <path d='m 83,193 h -4 v 12 h 4' />
	 <path d='M 82,200 H 74' />
	 <path d='m 94,196 v -1 l -2,-2 h -4 l -2,2 v 7 l 2,2 h 4 l 2,-2 v -1' />
	 <path d='m 46,68 v -1 l -2,-2 h -4 l -2,2 v 8 l 2,2 h 4 l 2,-2 v -1' />
	 <path d='m 87,207 h 2 l 1,-1 v -2' />
	 <path d='m 107,195 h -9 v 10 h 9' />
	 <path d='M 105,200 H 98' />
	 <path d='m 117,200 h -7 m 9,-5 h -9 v 10 h 9' />
	 <path style='stroke-linecap:round' d='m 102,193 2,2' />
	 <path style='stroke-linecap:round' d='m 113,195 2,-2' />
	 <path style='stroke-linecap:round' d='m 125,194 1,-1 h 1 l 1,1' />
	 <path d='m 129,200 h -7 m 9,-5 h -9 v 10 h 9' />
	 <path d='m 141,200 h -7 m 9,-5 h -9 v 10 h 9' />
	 <path style='stroke-linecap:round' d='m 162,194 1,-1' />
	 <path d='m 162,196 v 10' />
	 <path d='m 150,196 v 10' />
	 <path style='stroke-linecap:round' d='m 150,194 -1,-1' />
	 <path d='m 136,192 v 1' />
	 <path d='m 140,192 v 1' />
	 <path style='stroke-linecap:round' d='m 173,194 1,-1 h 1 l 1,1' />
	 <path d='m 174,196 v 10' />
	 <path d='m 186,196 v 10' />
	 <path d='m 162,228 v 10' />
	 <path d='m 150,228 v 10' />
	 <path d='m 174,228 v 10' />
	 <path d='m 186,228 v 10' />
	 <path d='m 184,193 v 2' />
	 <path d='m 188,193 v 2' />
	 <path d='m 2,221 h 6 l 2,-2 v -8 L 8,209 H 2 Z' />
	 <path d='M 0,214 H 5' />
	 <path style='stroke-linejoin:round' d='m 22,211 v 11 m -8,0 v -10 l 8,8'/>
	 <path style='stroke-linecap:round' d='m 16,210 1,-1 h 1 l 1,1 h 1 l 1,-1' />
	 <path style='stroke-linecap:round' d='m 30,210 -1,-1' />
	 <path style='stroke-linecap:round' d='m 41,210 1,-1' />
	 <path style='stroke-linecap:round' d='m 52,210 1,-1 h 1 l 1,1' />
	 <path style='stroke-linecap:round' d='m 64,210 1,-1 h 1 l 1,1 h 1 l 1,-1' />
	 <path d='m 76,208 v 2' />
	 <path d='m 80,208 v 2' />
	 <path d='m 26,214 v 5 l 2,2 h 4 l 2,-2 v -5 l -2,-2 h -4 z' />
	 <path d='m 38,214 v 5 l 2,2 h 4 l 2,-2 v -5 l -2,-2 h -4 z' />
	 <path d='m 50,214 v 5 l 2,2 h 4 l 2,-2 v -5 l -2,-2 h -4 z' />
	 <path d='m 62,214 v 5 l 2,2 h 4 l 2,-2 v -5 l -2,-2 h -4 z' />
	 <path d='m 74,214 v 5 l 2,2 h 4 l 2,-2 v -5 l -2,-2 h -4 z' />
	 <path style='stroke-linecap:round' d='m 30,242 -1,-1' />
	 <path style='stroke-linecap:round' d='m 41,242 1,-1' />
	 <path style='stroke-linecap:round' d='m 52,242 1,-1 h 1 l 1,1' />
	 <path style='stroke-linecap:round' d='m 64,242 1,-1 h 1 l 1,1 h 1 l 1,-1' />
	 <path d='m 76,241 v 2' />
	 <path d='m 80,241 v 2' />
	 <path d='m 26,247 v 4 l 2,2 h 4 l 2,-2 v -4 l -2,-2 h -4 z' />
	 <path d='m 38,247 v 4 l 2,2 h 4 l 2,-2 v -4 l -2,-2 h -4 z' />
	 <path d='m 50,247 v 4 l 2,2 h 4 l 2,-2 v -4 l -2,-2 h -4 z' />
	 <path d='m 62,247 v 4 l 2,2 h 4 l 2,-2 v -4 l -2,-2 h -4 z' />
	 <path d='m 74,247 v 4 l 2,2 h 4 l 2,-2 v -4 l -2,-2 h -4 z' />
	 <path d='m 62,229 h 6 l 2,2 v 6 h -7 l -1,-1 v -2 l 1,-1 h 7' />
	 <path d='m 78,233 h 4 v -1 -2 l -1,-1 h -2 l -1,1 v 5 l 2,2 h 2 m -8,-8 h 2 l 2,2 v 6 h -3 l -1,-1 v -2 l 1,-1 h 3' />
	 <path style='stroke-linecap:round;stroke-linejoin:round' d='m 65,226 1,1 1,-1 -1,-1 z' />
	 <path style='stroke-linecap:round' d='M 6,226 5,225' />
	 <path style='stroke-linecap:round' d='m 17,226 1,-1' />
	 <path style='stroke-linecap:round' d='m 28,226 1,-1 h 1 l 1,1' />
	 <path style='stroke-linecap:round' d='m 40,226 1,-1 h 1 l 1,1 h 1 l 1,-1' />
	 <path d='m 52,225 v 2' />
	 <path d='m 56,225 v 2' />
	 <path d='m 50,229 h 6 l 2,2 v 6 h -7 l -1,-1 v -2 l 1,-1 h 7' />
	 <path d='m 38,229 h 6 l 2,2 v 6 h -7 l -1,-1 v -2 l 1,-1 h 7' />
	 <path d='m 26,229 h 6 l 2,2 v 6 h -7 l -1,-1 v -2 l 1,-1 h 7' />
	 <path d='m 14,229 h 6 l 2,2 v 6 h -7 l -1,-1 v -2 l 1,-1 h 7' />
	 <path d='m 2,229 h 6 l 2,2 v 6 H 3 l -1,-1 v -2 l 1,-1 h 7' />
	 <path d='m 14,101 h 6 l 2,2 v 6 h -7 l -1,-1 v -2 l 1,-1 h 7' />
	 <path style='stroke-linecap:round' d='m 16,242 1,-1 h 1 l 1,1 h 1 l 1,-1' />
	 <path d='m 15,254 v -9 h 5 l 2,2 v 7' />
	 <path d='M 8,243 H 5 l -1,1 v 2 l 4,4 v 2 l -1,1 H 5 l -1,-1 v -2 l 1,-1' />
	 <path d='m 122,233 h 8 v -2 l -2,-2 h -4 l -2,2 v 4 l 2,2 h 6' />
	 <path d='m 134,233 h 8 v -2 l -2,-2 h -4 l -2,2 v 4 l 2,2 h 6' />
	 <path style='stroke-linecap:round' d='m 103,226 -1,-1' />
	 <path style='stroke-linecap:round' d='m 114,226 1,-1' />
	 <path style='stroke-linecap:round' d='m 125,226 1,-1 h 1 l 1,1' />
	 <path d='m 136,225 v 2' />
	 <path d='m 140,225 v 2' />
	 <path style='stroke-linecap:round' d='m 151,226 -1,-1' />
	 <path style='stroke-linecap:round' d='m 162,226 1,-1' />
	 <path style='stroke-linecap:round' d='m 162,210 1,-1' />
	 <path style='stroke-linecap:round' d='m 173,226 1,-1 h 1 l 1,1' />
	 <path d='m 184,225 v 2' />
	 <path d='m 188,225 v 2' />
	 <path style='stroke-linecap:round' d='m 115,242 -1,-1' />
	 <path style='stroke-linecap:round' d='m 126,242 1,-1' />
	 <path style='stroke-linecap:round' d='m 137,242 1,-1 h 1 l 1,1' />
	 <path d='m 148,241 v 2' />
	 <path d='m 152,241 v 2' />
	 <path style='stroke-linecap:round' d='m 115,210 -1,-1' />
	 <path style='stroke-linecap:round' d='m 126,210 1,-1' />
	 <path style='stroke-linecap:round' d='m 137,210 1,-1 h 1 l 1,1' />
	 <path d='m 148,208 v 2' />
	 <path d='m 152,208 v 2' />
	 <path d='m 134,211 v 8 l 2,2 h 4 l 2,-2 v -8' />
	 <path d='m 122,211 v 8 l 2,2 h 4 l 2,-2 v -8' />
	 <path d='m 110,211 v 8 l 2,2 h 4 l 2,-2 v -8' />
	 <path d='m 190,244 v 9 l -2,2 h -6 m 0,-11 v 5 l 2,2 h 6' />
	 <path d='m 173,251 h 3 l 2,-2 v -2 l -2,-2 h -3 m 0,10 v -14' />
	 <path d='m 184,243 v -2' />
	 <path d='m 188,243 v -2' />
	 <path d='m 173,219 h 3 l 2,-2 v -2 l -2,-2 h -3 m 0,9 v -12 m -3,0 h 6 m -6,12 h 6' />
	 <path style='stroke-linecap:round' d='m 162,243 1,-1' />
	 <path d='m 183,223 v -11 l 2,-2 h 2 l 2,2 v 1 l -2,2 2,2 v 2 l -1,1 h -3' />
	 <path style='stroke-linecap:round' d='m 105,244 -5,10 m 4,-8 2,2 v 2 l -2,2 h -3 l -2,-2 v -2 l 2,-2 z' />
	 <path d='m 86,248 h 8 m -4,2 v 2 m 0,-8 v 2' />
	 <path style='stroke-linecap:round' d='m 104,211 2,2 v 6 l -2,2 h -4 l -2,-2 v -6 l 2,-2 z m -7,10 10,-10' />
	 <path d='M 54,81 V 94 M 49,81 h 10' />
	 <path d='m 50,77 h 6 l 2,-2 v -8 l -2,-2 h -6 z' />
	 <path d='M 114,78 V 64' />
	 <path d='M 19,62 V 48' />
	 <path d='m 106,64 v 14 m -8,-7 h 8 m -8,7 V 64' />
	 <path d='m 94,68 v -1 l -2,-2 h -4 l -2,2 v 8 l 2,2 h 6 v -6' />
	 <path d='m 81,71 h -7 m 9,-6 h -9 v 13' />
	 <path d='m 71,65 h -9 v 12 h 9 m -2,-6 h -7' />
	 <path d='m 14,73 h 8 m -8,5 c 0,-3 0,-9 0,-9 l 4,-4 4,4 v 9' />
	 <path d='m 26,71 h 6 l 2,-2 v -2 l -2,-2 h -6 v 12 h 6 l 2,-2 v -2 l -2,-2' />
	 <path d='m 18,44 v 2 m 0,-14 v 10' />
	 <path d='m 32,34 v 4 m -4,-4 v 4' />
	 <path style='stroke-linecap:round;stroke-linejoin:round' d='m 69,43 h 1 v 1 h -1 z m -6,-8 h 1 v 1 h -1 z m -1,9 9,-9 M 125,59 h 1 v 1 h -1 z m 0,-6 h 1 v 1 h -1 z M 137,53 h 1 v 1 h -1 z m 1,10 v -4 h -1 M 173,44 h 1 v 1 h -1 z' />
	 <path style='stroke-linecap:round' d='m 15,52 3,-3 h 1' />
	 <path style='stroke-linejoin:round' d='m 26,51 v -1 l 1,-1 h 6 l 2,2 v 1 l -9,9 h 10' />
	 <path d='m 45,55 2,2 v 2 l -2,2 h -6 l -1,-1 v -1 m 0,-8 v -1 l 1,-1 h 6 l 2,2 v 2 l -2,2 h -5' />
	 <path style='stroke-linecap:round' d='m 56,49 -6,6 v 2' />
	 <path d='m 57,54 v 8' />
	 <path d='M 72,49 H 62 v 5 h 7 l 2,2 v 3 l -2,2 h -5 l -2,-2 v -1' />
	 <path d='m 83,49 h -6 l -3,3 v 7 l 2,2 h 5 l 2,-2 v -2 l -2,-2 h -7' />
	 <path style='stroke-linecap:round' d='m 95,49 v 4 l -8,8' />
	 <path style='stroke-linecap:round;stroke-linejoin:round' d='m 6,54 h 1 v 2 H 6 Z m 3,-5 2,2 v 8 L 9,61 H 4 L 2,59 v -8 l 2,-2 z' />
	 <path d='m 105,55 2,2 v 2 l -2,2 h -5 l -2,-2 v -2 l 2,-2 h 5 l 2,-2 v -2 l -2,-2 h -5 l -2,2 v 2 l 2,2' />
	 <path d='m 110,61 h 6 l 3,-3 v -7 l -2,-2 h -5 l -2,2 v 2 l 2,2 h 7' />
	 <path d='M 85,49 H 96' />
	 <path d='M 60,57 H 49' />
	 <g transform='translate(-12)'>
	   <path style='stroke-linejoin:round' d='m 14,206 v -6 l 4,-4 4,4 v 6' />
	   <path d='m 14,203 h 8' />
	 </g>
	 <path d='m 160,126 h 1 l 2,-2 v -3 l 1.5,-1.5 -1.5,-1.5 v -3 l -2,-2 h -1' />
	 <path d='m 140,126 h -1 l -2,-2 v -3 l -1.5,-1.5 1.5,-1.5 v -3 l 2,-2 h 1' />
   </g>
 </svg>`;
const vertexShader = `
// input from vertex buffer:
attribute vec2 a_position;
attribute vec4 a_color;
attribute vec2 a_texCoord;
// output to fragment shader:
varying vec4 vtxColor;
varying vec2 texCoord;
// context shared across all vertices in this batch:
uniform vec2 u_resolution;
uniform mat3 u_matrix;

void main() {
	vec2 position = (u_matrix * vec3(a_position, 1)).xy;
	// convert the rectangle from pixels to 0.0 to 1.0
	vec2 zeroToOne = position / u_resolution;

	// convert from 0->1 to 0->2
	vec2 zeroToTwo = zeroToOne * 2.0;

	// convert from 0->2 to -1->+1 (clipspace)
	vec2 clipSpace = zeroToTwo - 1.0;

	gl_Position = vec4(clipSpace * vec2(1, -1), 0, 1);
	vtxColor = a_color;
	texCoord = a_texCoord / vec2(16383.0,16383.0);
}`;

const fragmentShader = `
precision mediump float;

uniform sampler2D u_texUnit0;

varying vec4 vtxColor;
varying vec2 texCoord;

void main() {
	vec4 texColor = texture2D(u_texUnit0, texCoord);
	gl_FragColor = vtxColor * texColor;
}`;

function renderFontTexture(font) {
	let canvas = document.createElement('canvas');
	let texW = canvas.width = 1024;
	let texH = canvas.height = 1024;
	let ctx = canvas.getContext('2d');

	ctx.font = font;
	ctx.textBaseline = 'top';
	let dims = [];
	let x=0,y=0, currLineH = 0;

	for(let i=32; i<256; ++i) {
		const ch = String.fromCharCode(i);
		const dim = ctx.measureText(ch);
		let metrics = {
			w:Math.ceil(dim.width),
			h:Math.ceil(dim.actualBoundingBoxAscent
				+ dim.actualBoundingBoxDescent)+4,
			xoff: 0,
			yoff: Math.floor(-dim.actualBoundingBoxAscent)-1
		}
		//console.log(i, ch, metrics, dim);
		if(x+metrics.w>texW) {
			y += currLineH+2;
			currLineH = x = 0;
		}
		const currH = metrics.h + metrics.yoff;
		if(currH > currLineH)
			currLineH = currH;

		metrics.x = x;
		metrics.y = y + metrics.yoff;
		dims.push(metrics);
		x += metrics.w+1;
	}

	canvas.height = y+currLineH;
	ctx.fillStyle = 'white';
	ctx.font = font;
	ctx.textBaseline = 'top';

	x=0,y=0, currLineH = 0;
	for(let i=32; i<256; ++i) {
		const ch = String.fromCharCode(i);
		const metrics = dims[i-32];
		if(x+metrics.w>texW) {
			y += currLineH+2;
			currLineH = x = 0;
		}
		const currH = metrics.h + metrics.yoff;
		if(currH > currLineH)
			currLineH = currH;

		ctx.fillText(ch,x+0.5,y+0.5);
		x += metrics.w+1;
	}

	return { data:ctx.getImageData(0,0, canvas.width, canvas.height).data,
		width:canvas.width, height:canvas.height, glyphs:dims };
}

/// utility class for packing 2 short/4 byte precision values into single floats
function FloatPacker() {
	let int8 = new Int8Array(4);
	let uint32 = new Uint32Array(int8.buffer, 0, 1);
	let float32 = new Float32Array(int8.buffer, 0, 1);
	let int16 = new Int16Array(int8.buffer, 0, 2);

	/// @function rgba() - packs an rgb(a) color (components range [0..255]) into one float
	this.rgba = function(r,g,b,a=255) {
		uint32[0] = (a << 24 | b << 16 | g << 8 | r) & 0xfeffffff;
		return float32[0];
	}

	this.uint32 = function(uint) {
		uint32[0] = uint & 0xfeffffff;
		return float32[0];
	}

	/// @function int14() - packs two values within range [-16384..16383] into one float
	this.int14 = function(v1, v2=0) {
		int16[0] = v1;
		int16[1] = v2;
		return float32[0];
	}
}
let fpack = new FloatPacker();

function defineConst(obj, key, value) {
	Object.defineProperty( obj, key, {
		value: value, writable: false, enumerable: true, configurable: true });
}

//------------------------------------------------------------------
function createShader(gl, type, source) {
	let shader = gl.createShader(type);
	gl.shaderSource(shader, source);
	gl.compileShader(shader);
	var success = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
	if (success)
		return shader;
	console.error(gl.getShaderInfoLog(shader));
	gl.deleteShader(shader);
}

function createProgram(gl, vertexShader, fragmentShader) {
	const vs = createShader(gl, gl.VERTEX_SHADER, vertexShader);
	const fs = createShader(gl, gl.FRAGMENT_SHADER, fragmentShader);
	let program = gl.createProgram();
	gl.attachShader(program, vs);
	gl.attachShader(program, fs);
	gl.linkProgram(program);
	var success = gl.getProgramParameter(program, gl.LINK_STATUS);
	if (success)
		return program;
	console.error(gl.getProgramInfoLog(program));
	gl.deleteProgram(program);
}

//------------------------------------------------------------------
return function (canvas, capacity=500) {
	const floatSz = 4;
	const vtxSz = 4;
	const elemSz = 6*vtxSz;
	let buf = new Float32Array(capacity * elemSz);
	let sz = 0, idx=0;

	let mat = new Float32Array([
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 1.0 ]);
	let gs = [ new Float32Array([/* transf*/0.0, 0.0, 0.0, 1.0, /*color*/fpack.rgba(255,255,255), /*lineWidth*/1.0]) ]; // TODO add blendMode
	const transfMax = 8;
	function setMat(transf, mat) {
		mat[0] = Math.cos(transf[2])*transf[3]; mat[3] = -Math.sin(transf[2])*transf[3]; mat[6] = transf[0];
		mat[1] = -mat[3]; mat[4] = mat[0]; mat[7] = transf[1];
	}

	let blendFunc = 1;
	const texCoordMax = 16383;
	let fonts = [], textures=[], tex=null;

	// setup GLSL program
	const gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
	let program = createProgram(gl, vertexShader, fragmentShader);
	const a_position = gl.getAttribLocation(program, "a_position");
	gl.enableVertexAttribArray(a_position);
	const a_color = gl.getAttribLocation(program, "a_color");
	gl.enableVertexAttribArray(a_color);
	const a_texCoord = gl.getAttribLocation(program, "a_texCoord");
	gl.enableVertexAttribArray(a_texCoord);
	let u_resolution = gl.getUniformLocation(program, "u_resolution");
	let u_matrix = gl.getUniformLocation(program, "u_matrix");
	let u_texUnit0 = gl.getUniformLocation(program, "u_texUnit0");
	gl.useProgram(program);

	function normalizeGlyphs(font) {
		const scX = texCoordMax/font.width, scY=texCoordMax/font.height;
		let out = [];
		for(let i=0; i<font.glyphs.length; ++i) {
			let glyph = font.glyphs[i];
			const x1 = glyph.x*scX, y1=glyph.y*scY;
			out.push({
				xoff:glyph.xoff, yoff:glyph.yoff, w:glyph.w, h:glyph.h,
				tx1:x1, ty1:y1, tx2:x1+glyph.w*scX, ty2:y1+glyph.h*scY
			});
		}
		return out;
	}

	this.loadFont = function(font, params={}, callback) {
		const suffix = font.substr(font.lastIndexOf('.')+1).toLowerCase();
		if(suffix=='ttf') {
			let fontName = font.substr(0, font.indexOf('.'));
			fontName = fontName.toLowerCase().split('-')
				.map((s) => s.charAt(0).toUpperCase() + s.substring(1)).join(' ');
			let fontId = fonts.length;
			let obj = new FontFace(fontName, 'url('+font+')');
			obj.load().then((loadedFace)=>{
				document.fonts.add(loadedFace);
				this.loadFont(params.size+'px '+fontName, { fontId:fontId }, callback);
			}).catch(err=>{
				console.error("loading font resource", font, "failed:", err);
			});
			fonts.push({ texture:null, glyphs:null, size:params.size, width:0, height:0, ready:false });
			return fontId;
		}
		let fontData = renderFontTexture(font);
		const tex = gl.createTexture();
		gl.bindTexture(gl.TEXTURE_2D, tex);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, fontData.width, fontData.height, 0,
			gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array(fontData.data.buffer));
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
		const fontId = ('fontId' in params) ? params.fontId : fonts.length;
		fonts[fontId] = { texture:tex, glyphs:normalizeGlyphs(fontData),
			width:fontData.width, height:fontData.height, ready:true };
		if(callback)
			callback();
		return fontId;
	}

	function setTexParams(params) {
		const filtering = ('filtering' in params) ? params.filtering : 1;
		const repeat = ('repeat' in params) ? params.repeat : false;
		const filter = filtering>1 ? gl.LINEAR_MIPMAP_LINEAR : filtering==1 ? gl.LINEAR : gl.NEAREST;
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, filter);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, filter);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, repeat ? gl.REPEAT : gl.CLAMP_TO_EDGE);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, repeat ? gl.REPEAT : gl.CLAMP_TO_EDGE);
		if(filtering>1)
			gl.generateMipmap(gl.TEXTURE_2D);
	}

	function setBlendFunc() {
		switch(blendFunc) {
		case 0: gl.blendFunc(gl.ONE, gl.ZERO); break;
		case 1: 
			gl.blendFuncSeparate(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA, gl.ONE, gl.ONE_MINUS_SRC_ALPHA); break;
		case 2:
			gl.blendFuncSeparate(gl.SRC_ALPHA, gl.ONE, gl.ZERO, gl.ONE); break;
		case 4:
			gl.blendFuncSeparate(gl.DST_COLOR, gl.ZERO, gl.ZERO, gl.ONE); break;
		case 8:
			gl.blendFuncSeparate(gl.DST_COLOR, gl.ONE_MINUS_SRC_ALPHA, gl.DST_COLOR, gl.ONE_MINUS_SRC_ALPHA); break;
		}
	}

	/// loads a texture from a URL
	this.loadTexture = function(url, params={}, callback) {
		let texInfo = { texture:gl.createTexture(), ready:false, x:0, y:0, width:1, height:1, cx:0, cy:0, sc:1 };
		gl.bindTexture(gl.TEXTURE_2D, texInfo.texture);
		// fill texture with a placeholder
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 1, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE,
			new Uint8Array([0, 0, 255, 255]));

		const onLoad = ()=>{ // copy the loaded image to the texture:
			gl.bindTexture(gl.TEXTURE_2D, texInfo.texture);
			if('scale' in params) {
				let canvas = document.createElement('canvas');
				canvas.width = Math.ceil(image.width*params.scale);
				canvas.height = Math.ceil(image.height*params.scale);
				let ctx = canvas.getContext('2d');
				ctx.drawImage(image, 0,0, canvas.width, canvas.height);
				
				const imgData = new Uint8Array(ctx.getImageData(0,0, canvas.width, canvas.height).data.buffer);
				gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, canvas.width, canvas.height, 0, gl.RGBA, gl.UNSIGNED_BYTE, imgData);
				texInfo.width = canvas.width;
				texInfo.height = canvas.height;
			}
			else {
				gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image);
				texInfo.width = image.width;
				texInfo.height = image.height;
			}
			if('centerX' in params || 'centerY' in params) {
				texInfo.cx = params.centerX || 0;
				texInfo.cy = params.centerY || 0;
			}
			if(texInfo.cx)
				texInfo.cx *= texInfo.width;
			if(texInfo.cy)
				texInfo.cy *= texInfo.height;
			setTexParams(params);
			texInfo.ready = true;
			delete texInfo.img;
			if(callback)
				callback(texInfo);
		}
		// Asynchronously load an image
		let image = texInfo.img = new Image();
		image.src = url;

		if(image.complete)
			setTimeout(onLoad, 0);
		else
			image.addEventListener('load', onLoad);

		textures.push(texInfo);
		return textures.length-1;
	}

	/// creates a new texture from an RGBA uint8 array
	this.createTexture = function(width, height, data, params={}) {
		if(typeof params === 'function')
			throw "app.createImageResource with callback drawing function not supported by browser runtime";
		if(typeof(width) === 'object') {
			const obj = width;
			width = obj.width;
			height = obj.height;
			data = obj.data;
		}
		let texInfo = { texture:gl.createTexture(), x:0, y:0, width:width, height:height, cx:0, cy:0, sc:1.0, ready:true };
		gl.bindTexture(gl.TEXTURE_2D, texInfo.texture);
		if(Array.isArray(data))
			data = new Uint8Array(data);
		else
			data = new Uint8Array(data.buffer);
	
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, data);
		if('centerX' in params || 'centerY' in params) {
			texInfo.cx = params.centerX * width;
			texInfo.cy = params.centerY * height;
		}
		setTexParams(params);
		textures.push(texInfo);
		return textures.length-1;
	}
	this.createCircleTexture = function(r, fill=[255,255,255,255], lineW=0, stroke=[0,0,0,0]) {
		let canvas = document.createElement('canvas');
		canvas.width = canvas.height = Math.ceil(2*r+lineW);

		let ctx = canvas.getContext('2d');
		ctx.lineWidth = lineW;
		ctx.fillStyle = fill ?
			'rgba('+fill[0]+','+fill[1]+','+fill[2]+','
			+((typeof fill[3]==='number') ? fill[3]/255 : 1)+')' : 'transparent';
		ctx.strokeStyle = lineW ?
			'rgba('+stroke[0]+','+stroke[1]+','+stroke[2]+','
			+((typeof stroke[3]==='number') ? stroke[3]/255 : 1)+')' : 'transparent';

		ctx.beginPath();
		ctx.arc(r+lineW/2,r+lineW/2, r, 0, 2*Math.PI, true);
		ctx.closePath();
		if(fill)
			ctx.fill();
		if(lineW)
			ctx.stroke();
		
		const imgData = new Uint8Array(ctx.getImageData(0,0, canvas.width, canvas.height).data.buffer);
		const texId = this.createTexture(canvas.width, canvas.height, imgData);
		this.setTextureCenter(texId, 0.5, 0.5)
		return texId;
	}
	this.createTileTexture = function(parent, x,y,w,h, params) {
		const par = textures[parent];
		if(!par)
			return console.error('createTileTexture() invalid parent id', parent);
		const texId = textures.length;
		textures.push({});

		const createTileInfo = ()=>{
			w *= par.width;
			h *= par.height;
			const cx = (params && ('centerX' in params)) ? params.centerX : par.cx/par.width;
			const cy = (params && ('centerY' in params)) ? params.centerY : par.cy/par.height;
			textures[texId] = { texture:par.texture, x:par.width*x, y:par.height*y, width:w, height:h,
				cx:w*cx, cy:h*cy, sc:par.sc, ready:par.ready, parent:par.parent ? par.parent : parent };
		}
		if(par.ready)
			createTileInfo();
		else if(par.img)
			par.img.addEventListener('load', createTileInfo);
		return texId;
	}
	this.createTileTextures = function(parent,tilesX, tilesY, border, params) {
		if(typeof parent === 'string')
			parent = this.loadTexture(parent, params);
		const par = textures[parent];
		if(!par)
			return console.error('createTileTextures() invalid parent id', parent);

		const baseTexId = textures.length;
		for(let i=0, end=tilesX*tilesY; i<end; ++i)
			textures.push({ parent:par.parent ? par.parent : parent, ready:par.ready });

		const createTileInfos = ()=>{
			const gridW = par.width/tilesX, tileW = gridW - 2*border;
			const gridH = par.height/tilesY, tileH = gridH - 2*border;
			let texId = baseTexId;
			for(let y=0; y<tilesY; ++y) for(let x=0; x<tilesX; ++x) {
				textures[texId++] = { texture:par.texture, x:gridW*x+border, y:gridH*y+border, width:tileW, height:tileH,
					cx:par.cx/tilesX, cy:par.cy/tilesY, sc:par.sc, ready:par.ready, parent:par.parent ? par.parent : parent };
			}
		}

		if(par.ready)
			createTileInfos();
		else if(par.img)
			par.img.addEventListener('load', createTileInfos);

		return baseTexId;
	}
	this.releaseTexture = function(texId) {
		const tex = textures[texId];
		if(!tex)
			return;
		if(tex.texture) {
			gl.deleteTexture(tex.texture);
			tex.texture = 0;
		}
		tex.w = tex.h = 0;
		tex.ready = false;
	}
	this.createImageFontResource = function(img, params) {
		if(typeof img === 'string')
			img = this.loadTexture(img, params);
		else if(img===0)
			img = this.loadTexture(URL.createObjectURL(new Blob([defaultFontStr], {type: 'image/svg+xml'})), params);
		let texSz = this.queryTexture(img);
		if(!texSz)
			return console.error('createImageFontResource() invalid parent image id', img);

		const border = ((typeof params==='object') && ('border' in params)) ? params.border : 0;
		const fontId = ((typeof params==='object') && ('fontId' in params)) ? params.fontId : fonts.length;
		const par = textures[img];
		const font = fonts[fontId] = { texture:null, glyphs:[], width:0, height:0, ready:false };

		const createGlyphInfos = (isAsync)=>{
			if(isAsync)
				texSz = this.queryTexture(img);
			font.texture = par.texture;
			font.width = texSz.width;
			font.height = texSz.height;
			const glyphW = Math.floor(texSz.width/16);
			const glyphH = Math.floor(texSz.height/16);

			for(let i=32; i<256; ++i)
				font.glyphs.push({
					x: glyphW*(i%16)+border,
					y: glyphH*Math.floor(i/16)+border,
					w:glyphW-2*border,
					h:glyphH-2*border,
					xoff: 0,
					yoff: 0,
				});
			font.glyphs = normalizeGlyphs(font);
			font.ready = true;
		}

		if(par.ready)
			createGlyphInfos(false);
		else if(par.img)
			par.img.addEventListener('load', ()=>{createGlyphInfos(true)});
		return fontId;
	}
	this.queryTexture = function(texId) {
		if(!texId || texId<1 || texId>=textures.length)
			return null;
		const texInfo = textures[texId];
		return { width:texInfo.width, height:texInfo.height, sc:texInfo.sc,
			cx:texInfo.cx/texInfo.width, cy:texInfo.cy/texInfo.height, ready:texInfo.ready };
	}
	this.setTextureCenter = function(texId, cx, cy) {
		if(texId>0 && texId<textures.length) {
			const tex = textures[texId];
			const setTextureCenter = ()=>{
				const w = tex.width ? tex.width : 1, h = tex.height ? tex.height : 1;
				tex.cx = cx * w;
				tex.cy = cy * h;	
			}
			if(tex.ready)
				setTextureCenter();
			else {
				const img = tex.img ? tex.img : tex.parent ? textures[tex.parent].img : null;
				if(img)
					img.addEventListener('load', setTextureCenter);
			}
		}
	}

	const setTexture = (texInfo)=>{
		if(texInfo.texture != tex) {
			this.flush();
			tex = texInfo.texture;
		}
		return texInfo;
	}

	function vertex(x, y, u=1, v=1) {
		buf[idx++] = x; buf[idx++] = y; buf[idx++] = gs[gs.length-1][4]; buf[idx++] = fpack.int14(u,v);
	}
	function coloredVertex(x, y, color) {
		buf[idx++] = x; buf[idx++] = y; buf[idx++] = fpack.uint32(color); buf[idx++] = fpack.int14(1,1);
	}

	this.color = function(r,g,b,a=255) {
		if(Array.isArray(r))
			gs[gs.length-1][4] = fpack.rgba(r[0],r[1], r[2], r[3]);
		else if(g===undefined) {
			const v = (typeof r === 'string') ? app.cssColor(r) : r;
			r = (v & 0xff000000) >>> 24;
			g = (v & 0x00ff0000) >>> 16;
			b = (v & 0x0000ff00) >>> 8;
			a = (v & 0x000000ff);
			gs[gs.length-1][4] = fpack.rgba(r,g,b,a);
		}
		else if(b===undefined) {
			const v = r;
			a = g;
			r = (v & 0xff000000) >>> 24;
			g = (v & 0x00ff0000) >>> 16;
			b = (v & 0x0000ff00) >>> 8;
			gs[gs.length-1][4] = fpack.rgba(r,g,b,a);
		}
		else
			gs[gs.length-1][4] = fpack.rgba(r,g,b,a);
		return this;
	}
	this.colorf = function(r,g,b,a=1.0) {
		gs[gs.length-1][4] = fpack.rgba(r*255,g*255,b*255,a*255);
		return this;
	}
	this.lineWidth = function(w) {
		if(w===undefined)
			return gs[gs.length-1][5];
		if(w==gs[gs.length-1][5])
			return this;
		this.flush();
		gs[gs.length-1][5] = Number(w);
		return this;
	}
	this.blend = function(mode) {
		if(mode===undefined)
			return blendFunc;
		if(mode===blendFunc)
			return this;
		this.flush();
		blendFunc = mode;
		setBlendFunc();
		return this;
	}

	this.save = function() {
		if(gs.length < transfMax)
			gs.push(gs[gs.length-1].slice());
		return this;
	}
	this.transform = function(x,y,rot=0,sc=1.0) {
		this.flush();
		const tr = gs[gs.length-1];
		if(typeof x === 'object') {
			tr[0] += mat[0]*x.x + mat[3]*x.y;
			tr[1] += mat[1]*x.x + mat[4]*x.y;
			if('rot' in x)
				tr[2] += x.rot;
			if('sc' in x)
				tr[3] *= x.sc;
		}
		else {
			tr[0] += mat[0]*x + mat[3]*y;
			tr[1] += mat[1]*x + mat[4]*y;
			tr[2] += rot;
			tr[3] *= sc;
		}
		setMat(tr, mat);
		return this;
	}
	this.setTransform = function(x,y,rot=0,sc=1.0) {
		this.flush();
		const tr = gs[gs.length-1];
		tr[0] = x;
		tr[1] = y;
		tr[2] = rot;
		tr[3] = sc;
		setMat(tr, mat);
		return this;
	}
	this.restore = function() {
		const stacksz = gs.length;
		if(stacksz<2)
			return this;
		this.flush();
		gs.pop();
		// todo restore blendFunc
		setMat(gs[stacksz-2], mat);
		return this;
	}
	this.reset = function() {
		this.flush();
		gs.length = 1;
		gs[0][0] = gs[0][1] = gs[0][2] = 0.0;
		gs[0][3] = gs[0][5] = 1.0;
		gs[0][4] = fpack.rgba(255,255,255);
		setMat(gs[0], mat);
		blendFunc = 1;
		setBlendFunc();
		return this;
	}

	this.fillText = function(x,y, text, fontId=0, align=0) {
		const font = fonts[fontId];
		if(font===undefined || font.texture===null)
			return console.error('font not ready', font);
		setTexture(font);

		if(align!==0) {
			let metrics = this.measureText(fontId, text);
			if(align & this.ALIGN_RIGHT)
				x-=metrics.width;
			else if(align & this.ALIGN_CENTER)
				x-=metrics.width/2;
			if(align & this.ALIGN_BOTTOM)
				y-=metrics.height;
			else if(align & this.ALIGN_MIDDLE)
				y-=metrics.height/2;
		}

		const s = String(text);
		for(let i=0; i<s.length; ++i) {
			const ch = s.charCodeAt(i);
			if(ch<32 || ch>256)
				continue;
			const glyph = font.glyphs[ch-32];
			const x1 = x+glyph.xoff, y1=y+glyph.yoff;
			const x2 = x1 + glyph.w, y2 = y1 + glyph.h;
			const tx1 = glyph.tx1, ty1=glyph.ty1, tx2=glyph.tx2, ty2=glyph.ty2;

			vertex(x1,y1, tx1,ty1);
			vertex(x2,y1, tx2,ty1);
			vertex(x1,y2, tx1,ty2);
			vertex(x1,y2, tx1,ty2);
			vertex(x2,y1, tx2,ty1);
			vertex(x2,y2, tx2,ty2);
			if(++sz==capacity)
				this.flush();
			x += glyph.w;
		}
	}
	this.measureText = function(fontId, text) {
		const font = fonts[fontId];
		if(font===undefined || font.glyphs===null || !font.ready)
			return null;
		const vbar = font.glyphs[124-32], M=font.glyphs[77-32];
		let metrics = { height:vbar.yoff+vbar.h, fontBoundingBoxAscent:M.h,
			fontBoundingBoxDescent:vbar.yoff+vbar.h - M.yoff-M.h };
		if(text!==undefined) {
			if(typeof text !=='string')
				text = ''+text;
			metrics.width = 0;
			for(let i=0; i<text.length; ++i) {
				let ch = text.charCodeAt(i);
				if(ch<32 || ch>256)
					ch=32;
				const glyph = font.glyphs[ch-32];
				metrics.width += glyph.w;
			}
		}
		return metrics;
	}
	this.clipRect = function(x, y, w, h) {
		this.flush();
		if(typeof x === 'number') {
			gl.enable(gl.SCISSOR_TEST);
			gl.scissor(x, canvas.height-y-h, w, h);
		}
		else {
			gl.scissor(0, 0, canvas.width, canvas.height);
			gl.disable(gl.SCISSOR_TEST);
		}
	}
	this.fillRect = function(x1, y1, w, h) {
		setTexture(texWhite);
		const x2 = x1 + w, y2 = y1 + h;
		vertex(x1,y1); vertex(x2,y1); vertex(x1,y2);
		vertex(x1,y2); vertex(x2,y1); vertex(x2,y2);
		if(++sz==capacity)
			this.flush();
	}
	this.drawRect = function(x1, y1, w, h) {
		const lw = gs[gs.length-1][5], lw2 = lw/2;
		y1 += lw2;
		const x2 = x1 + w, y2 = y1 + h - lw;
		this.drawLine(x1,y1,x2,y1);
		this.drawLine(x2-lw2,y1+lw2,x2-lw2,y2-lw2);
		this.drawLine(x2,y2,x1,y2);
		this.drawLine(x1+lw2,y2-lw2,x1+lw2,y1+lw2);
	}
	this.drawLine = function(x1,y1, x2,y2) {
		setTexture(texWhite);
		const dx = x2-x1, dy=y2-y1, d=Math.sqrt(dx*dx+dy*dy);
		const lw2 = gs[gs.length-1][5]/2, nx = lw2*-dy/d, ny=lw2*dx/d;
		const x3=x1+nx, y3=y1+ny, x4=x2+nx, y4=y2+ny;
		x1-=nx; y1-=ny; x2-=nx; y2-=ny;
		vertex(x1,y1); vertex(x2,y2); vertex(x3,y3);
		vertex(x3,y3); vertex(x2,y2); vertex(x4,y4);
		if(++sz==capacity)
			this.flush();
	}

	this.drawPoints = function(arr) {
		const lw = gs[gs.length-1][5], lw2 = lw/2;
		if(lw<=2)
			for(let i=0, end=arr.length-1; i<end; i+=2)
				this.fillRect(arr[i]-lw2,arr[i+1]-lw2, lw,lw);
		else {
			setTexture(textures[texPointId]);
			const tx1 = 0, ty1=0, tx2=texCoordMax, ty2=texCoordMax;
			for(let i=0, end=arr.length-1; i<end; i+=2) {
				const x1 = arr[i]-lw2, y1= arr[i+1]-lw2;
				const x2 = x1 + lw, y2 = y1 + lw;
				vertex(x1,y1, tx1,ty1);
				vertex(x2,y1, tx2,ty1);
				vertex(x1,y2, tx1,ty2);
				vertex(x1,y2, tx1,ty2);
				vertex(x2,y1, tx2,ty1);
				vertex(x2,y2, tx2,ty2);
				if(++sz==capacity)
					this.flush();
			}
		}
	}

	this.drawLineStrip = function(arr) {
		for(let i=2, end=arr.length-1; i<end; i+=2)
			this.drawLine(arr[i-2], arr[i-1], arr[i], arr[i+1]);
		this.drawPoints(arr.slice(2,-2));
	}

	this.drawLineLoop = function(arr) {
		var prevX = arr[arr.length-2], prevY = arr[arr.length-1];
		for(let i=0, end=arr.length-1; i<end; i+=2) {
			var x = arr[i], y = arr[i+1];
			this.drawLine(prevX, prevY, x, y);
			prevX = x; prevY = y;
		}
		this.drawPoints(arr);
	}

	this._drawImage = function(texId, x1, y1, w1, h1, x2, y2, w2, h2, cx=0,cy=0,rot=0, flip=0) {
		//console.log(texId, x1, y1, w1, h1, x2, y2, w2, h2, cx,cy,rot, flip)
		const texInfo = setTexture(textures[texId]);

		const cos = Math.cos(rot), sin=Math.sin(rot);
		//const m = [ cos, -sin, x2, sin, cos, y2];

		const xmin = -cx, ymin = -cy;
		const xmax = xmin+w2, ymax = ymin+h2;

		const v1x = cos*xmin - sin*ymin + x2;
		const v1y = sin*xmin + cos*ymin + y2;
		const v2x = cos*xmax - sin*ymin + x2;
		const v2y = sin*xmax + cos*ymin + y2;
		const v3x = cos*xmin - sin*ymax + x2;
		const v3y = sin*xmin + cos*ymax + y2;
		const v4x = cos*xmax - sin*ymax + x2;
		const v4y = sin*xmax + cos*ymax + y2;

		let tx1 = texCoordMax * x1 / texInfo.width;
		let ty1 = texCoordMax * y1 / texInfo.height;
		let tx2 = tx1 + texCoordMax * w1 / texInfo.width;
		let ty2 = ty1 + texCoordMax * h1 / texInfo.height;
		if(flip & 0x01) {
			let tmp = tx1;
			tx1 = tx2;
			tx2 = tmp;
		}
		if(flip & 0x02) {
			let tmp = ty1;
			ty1 = ty2;
			ty2 = tmp;
		}

		vertex(v1x,v1y, tx1,ty1);
		vertex(v2x,v2y, tx2,ty1);
		vertex(v3x,v3y, tx1,ty2);
		vertex(v3x,v3y, tx1,ty2);
		vertex(v2x,v2y, tx2,ty1);
		vertex(v4x,v4y, tx2,ty2);
		if(++sz==capacity)
			this.flush();
	}

	this.drawImage = function(texId, dstX=0, dstY=0, angle=0, scale=1, flip=this.FLIP_NONE) {
		const tex = setTexture(textures[texId]);
		scale *= tex.sc;
		this._drawImage(tex.parent ? tex.parent : texId, tex.x, tex.y, tex.width, tex.height,
			dstX, dstY, tex.width*scale, tex.height*scale, tex.cx*scale, tex.cy*scale, angle, flip);
	}

	this.stretchImage = function(texId, x, y, w, h) {
		const tex = setTexture(textures[texId]);
		this._drawImage(tex.parent ? tex.parent : texId, tex.x, tex.y, tex.width, tex.height, x, y, w, h);
	}

	this.drawSprite = function(s) {
		if('color' in s)
			this.color(s.color);
		const x = s.x || 0, y = s.y || 0, rot = s.rot || 0, sc = s.sc || 1;
		this.drawImage(s.image, x,y,rot,sc,s.flip);
	}

	this.fillTriangle = function(x0,y0, x1,y1, x2,y2) {
		setTexture(texWhite);
		vertex(x0,y0); vertex(x1,y1); vertex(x2,y2);
		vertex(x0,y0); vertex(x0,y0); vertex(x0,y0); // fake triangle FIXME?
		if(++sz==capacity)
			this.flush();
	}
	this.fillTriangles = function(arr, colors) {
		if(colors) {
			const numQuads=Math.floor(arr.length/12), numVertices=6*numQuads;
			for(var i=0; i<numVertices; ++i) {
				coloredVertex(arr[i*2],arr[i*2+1], colors[i]);
				if(i%6===0 && ++sz==capacity)
					this.flush();
			}
			if(2*numVertices < arr.length) {
				for(var i=numVertices; i<colors.length; ++i)
					coloredVertex(arr[i*2],arr[i*2+1], colors[i]);
				coloredVertex(0,0,0);
				coloredVertex(0,0,0);
				coloredVertex(0,0,0);
				if(++sz==capacity)
					this.flush();
			}
		}
		else for(var i=0, numTriangles=Math.floor(arr.length/6); i<numTriangles; ++i)
			this.fillTriangle(arr[i],arr[i+1], arr[i+2],arr[i+3], arr[i+4],arr[i+5]);
		
	}
	this.drawTiles = function(imgBase, tilesX, tilesY, imgOffsets=undefined, colors=undefined, stride=tilesX) {
		const texInfo = setTexture(textures[imgBase]);
		const w=texInfo.width, h = texInfo.height;
		for(var j=0; j<tilesY; ++j) for(var i=0; i<tilesX; ++i) {
			const index = j*stride+i;
			if(colors)
				this.color(colors[index]);
			const img = imgOffsets ? imgBase+imgOffsets[index] : imgBase;
			this.drawImage(img, i*w, j*h);
		}
	}
	this.drawImages = function(imgBase, stride, comps, arr) {
		const numInstances = Math.floor(arr.length/stride);
		const hasColors = (comps&this.COMP_COLOR_RGBA) != 0;
		var r=255, g=255, b=255, a=255; // TODO set to current color
		for(var i=0; i<numInstances; ++i) {
			const data = arr.subarray(i*stride, i*stride+stride);
			var img = imgBase, j=0;
			if(comps & this.COMP_IMG_OFFSET)
				img += data[j++];
			if(!img || img >= textures.length)
				continue;
	
			const x = data[j++], y = data[j++];
			const rot = (comps & this.COMP_ROT) ? data[j++] : 0.0;
			const sc = (comps & this.COMP_SCALE) ? data[j++] : 1.0;
			if(hasColors) {
				r = (comps & this.COMP_COLOR_R) ? data[j++] : r;
				g = (comps & this.COMP_COLOR_G) ? data[j++] : g;
				b = (comps & this.COMP_COLOR_B) ? data[j++] : b;
				a = (comps & this.COMP_COLOR_A) ? data[j++] : a;
				if(a<=0)
					continue;
				this.color(r, g, b, a);
			}
			this.drawImage(img, x,y,rot,sc);
		}
	}

	function initBuf() {
		// Create a buffer for vertex attribute data:
		let glBuf = gl.createBuffer();
		// Bind it to ARRAY_BUFFER (think of it as ARRAY_BUFFER = glBuf)
		gl.bindBuffer(gl.ARRAY_BUFFER, glBuf);
		// tell vertex attributes how to get values out of glBuf:
		const stride = floatSz*vtxSz;
		gl.vertexAttribPointer(
			a_position, 2, gl.FLOAT, false, stride, floatSz*0);
		gl.vertexAttribPointer(
			a_color, 4, gl.UNSIGNED_BYTE, true, stride, floatSz*2);
		gl.vertexAttribPointer(
			a_texCoord, 2, gl.SHORT, false, stride, floatSz*3);
	}

	this.flush = function() {
		gl.uniform2f(u_resolution, canvas.width, canvas.height);
		gl.uniformMatrix3fv(u_matrix, false, mat);
		gl.uniform1i(u_texUnit0, 0);

		gl.activeTexture(gl.TEXTURE0);
		gl.bindTexture(gl.TEXTURE_2D, tex);

		gl.bufferData(gl.ARRAY_BUFFER, buf, gl.STREAM_DRAW);
		gl.drawArrays(gl.TRIANGLES, 0, 6*sz);
		gl.flush();

		sz = 0;
		idx = 0;
	}

	this._frameBegin = function(r,g,b) {
		gl.viewport(0, 0, canvas.width, canvas.height);
		gl.clearColor(r, g, b, 1);
		gl.clear(gl.COLOR_BUFFER_BIT);
		gl.enable(gl.BLEND);
		setBlendFunc();
	}

	this._frameEnd = this.reset;

	initBuf();
	const texWhite = textures[this.createTexture(2,2,new Uint8Array([
		255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255]))];
	const texPointId = this.createCircleTexture(32);
	this.setTextureCenter(texPointId,0.5,0.5);
	textures[texPointId].sc = 1/64;
	this.createImageFontResource(0);
	tex = texWhite.texture;

	defineConst(this, "ALIGN_LEFT", 0);
	defineConst(this, "ALIGN_CENTER", 1);
	defineConst(this, "ALIGN_RIGHT", 2);
	defineConst(this, "ALIGN_TOP", 0);
	defineConst(this, "ALIGN_MIDDLE", 4);
	defineConst(this, "ALIGN_BOTTOM", 8);
	defineConst(this, "FLIP_NONE", 0.0);
	defineConst(this, "ALIGN_LEFT_TOP", 0);
	defineConst(this, "ALIGN_CENTER_TOP", 1);
	defineConst(this, "ALIGN_RIGHT_TOP", 2);
	defineConst(this, "ALIGN_LEFT_MIDDLE", 4);
	defineConst(this, "ALIGN_CENTER_MIDDLE", 5);
	defineConst(this, "ALIGN_RIGHT_MIDDLE", 6);
	defineConst(this, "ALIGN_LEFT_BOTTOM", 8);
	defineConst(this, "ALIGN_CENTER_BOTTOM", 9);
	defineConst(this, "ALIGN_RIGHT_BOTTOM", 10);
	defineConst(this, "FLIP_X", 1.0);
	defineConst(this, "FLIP_Y", 2.0);
	defineConst(this, "FLIP_XY", 3.0);
	defineConst(this, "IMG_CIRCLE", texPointId);
	defineConst(this, "IMG_SQUARE", texWhite);
	defineConst(this, "BLEND_NONE", 0);
	defineConst(this, "BLEND_ALPHA", 1);
	defineConst(this, "BLEND_ADD", 2);
	defineConst(this, "BLEND_MOD", 4);
	defineConst(this, "BLEND_MUL", 8);
	defineConst(this, "COMP_IMG_OFFSET", 1<<0);
	defineConst(this, "COMP_ROT", 1<<3);
	defineConst(this, "COMP_SCALE", 1<<4);
	defineConst(this, "COMP_COLOR_R", 1<<5);
	defineConst(this, "COMP_COLOR_G", 1<<6);
	defineConst(this, "COMP_COLOR_B", 1<<7);
	defineConst(this, "COMP_COLOR_A", 1<<8);
	defineConst(this, "COMP_COLOR_RGB", 7<<5);
	defineConst(this, "COMP_COLOR_RGBA", 15<<5);
}
})();