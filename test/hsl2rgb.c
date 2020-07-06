#include <stdio.h>
#include <stdint.h>

static float hue2rgb(float m1, float m2, float h) {
	if (h * 6.0f < 1.0f)
		return m1 + (m2 - m1) * h * 6.0f;
	if (h * 2.0f < 1.0f)
		return m2;
	if (h * 3.0f < 2.0f)
		return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
	return m1;
}

static float clamp(float f, float min, float max) {
	return f<min ? min : f>max ? max : f;
}

/// convert from hsla to rgba
/**  the function expects values between 0.0..1.0, hue between 0.0..360.0 */
static uint32_t hsla2rgba(float h, float s, float l, float a) {
	uint32_t r,g,b;

	if (s < 5.0e-6)
		r = g = b = l*255;
	else {
		h /= 360.0f;
		while(h < 0.0f)
			h += 1.0f;
		while (h > 1.0f)
			h -= 1.0f;

		float m2 = l <= 0.5f ? l * (s + 1.0f) : l + s - l * s;
		float m1 = l * 2.0f - m2;

		r = clamp(hue2rgb(m1, m2, h + 1.0f / 3.0f), 0.0f,1.0f) * 255.0f;
		g = clamp(hue2rgb(m1, m2, h), 0.0f,1.0f) * 255.0f;
		b = clamp(hue2rgb(m1, m2, h - 1.0f / 3.0f), 0.0f,1.0f) * 255.0f;
		printf("%i %i %i\n", r,g,b);
	}
	return (r << 24) + (g << 16) + (b << 8) + (uint32_t)(a*255.0f);
}

int main(int argc, char** argv) {
	printf("%#010x\n", hsla2rgba(0.0f, 1.0f, 0.5f, 1.0f));
	return 0;
}
