#version 150

uniform mat4 modelViewProjectionMatrix;
in vec4 position;
in vec4 instanceColor;
uniform samplerBuffer texDist; // using usamplerBuffer?
uniform samplerBuffer texAmp;
out vec4 color;
uniform float size;

const mediump float pi=3.14159265358979323846264;
const mediump float twopi=6.28318530717958;


float vec4ToInt(vec4 val) {
	int r = int((val.x * 255.0));
	int g = int((val.y * 255.0));
	int b = int((val.z * 255.0));
	int a = int((val.w * 255.0));

	return float((a<<24) + (b<<16) + (g<<8) + r);
}


void main(){
    int id = gl_InstanceID;
	
	float t = twopi * (float(id)/float(size-1));
	
	vec4 distT = texelFetch(texDist, id); // using uvec4?
	vec4 ampT = texelFetch(texAmp, id);

	float dist = vec4ToInt(distT) / 10.0;
	float amp = vec4ToInt(ampT);
	
//	float dist = float(distT.x) / 1.0;
//	float amp = ampT.x;

	// calc position
	float x = sin(t) * dist;
	float y = cos(t) * dist;
	
	
	vec4 vPos = position;
	vPos.x += x;
	vPos.y += y;
	
	gl_Position = modelViewProjectionMatrix * vPos;

	if (amp < 32) {
		color = vec4(1.0, 0.0, 0.0, 1.0);
	} else {
		color = vec4(1.0, 1.0, 0.5, amp/2047.0);
//		color.a = amp/4095.0;
	}
}