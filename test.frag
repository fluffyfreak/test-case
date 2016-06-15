/*
 * 4D simplex noise, in a GLSL fragment shader.
 *
 * Simplex noise is implemented by the functions:
 * float snoise(vec4 P)
 *
 * Author: Stefan Gustavson ITN-LiTH (stegu@itn.liu.se) 2004-12-05
 * Simplex indexing functions by Bill Licea-Kane, ATI
 */
 
/*
This code was irrevocably released into the public domain
by its original author, Stefan Gustavson, in January 2011.
Please feel free to use it for whatever you want.
Credit is appreciated where appropriate, and I also
appreciate being told where this code finds any use,
but you may do as you like. Alternatively, if you want
to have a familiar OSI-approved license, you may use
This code under the terms of the MIT license:

Copyright (C) 2004 by Stefan Gustavson. All rights reserved.
This code is licensed to you under the terms of the MIT license:

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#version 120

uniform sampler2D permTexture;
uniform sampler2D gradTexture;
uniform sampler2D diffuse; // the vertical colour gradient
uniform float time; // Used for texture animation

uniform int octavesIn;
uniform vec3 frequency;

varying vec3 v_texCoord3D;

/*
 * To create offsets of one texel and one half texel in the
 * texture lookup, we need to know the texture image size.
 */
#define ONE 0.00390625
#define ONEHALF 0.001953125
// The numbers above are 1/256 and 0.5/256, change accordingly
// if you change the code to use another perm/grad texture size.

void simplex( const in vec4 P, out vec4 offset1, out vec4 offset2, out vec4 offset3 )
{
  vec4 offset0;
 
  vec3 isX = step( P.yzw, P.xxx );        // See comments in 3D simplex function
  offset0.x = dot( isX, vec3( 1.0 ) );
  offset0.yzw = 1.0 - isX;

  vec2 isY = step( P.zw, P.yy );
  offset0.y += dot( isY, vec2( 1.0 ) );
  offset0.zw += 1.0 - isY;
 
  float isZ = step( P.w, P.z );
  offset0.z += isZ;
  offset0.w += 1.0 - isZ;

  // offset0 now contains the unique values 0,1,2,3 in each channel

  offset3 = clamp(   offset0, 0.0, 1.0 );
  offset2 = clamp( --offset0, 0.0, 1.0 );
  offset1 = clamp( --offset0, 0.0, 1.0 );
}


/*
 * 4D simplex noise. A lot faster than classic 4D noise, and better looking.
 */

float snoise(const in vec4 P) {

// The skewing and unskewing factors are hairy again for the 4D case
// This is (sqrt(5.0)-1.0)/4.0
#define F4 0.309016994375
// This is (5.0-sqrt(5.0))/20.0
#define G4 0.138196601125

  // Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
 	float s = (P.x + P.y + P.z + P.w) * F4; // Factor for 4D skewing
  vec4 Pi = floor(P + s);
  float t = (Pi.x + Pi.y + Pi.z + Pi.w) * G4;
  vec4 P0 = Pi - t; // Unskew the cell origin back to (x,y,z,w) space
  Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

  vec4 Pf0 = P - P0;  // The x,y distances from the cell origin

  // For the 4D case, the simplex is a 4D shape I won't even try to describe.
  // To find out which of the 24 possible simplices we're in, we need to
  // determine the magnitude ordering of x, y, z and w components of Pf0.
  vec4 o1;
  vec4 o2;
  vec4 o3;
  simplex(Pf0, o1, o2, o3);  

  // Noise contribution from simplex origin
  float perm0xy = texture2D(permTexture, Pi.xy).a;
  float perm0zw = texture2D(permTexture, Pi.zw).a;
  vec4  grad0 = texture2D(gradTexture, vec2(perm0xy, perm0zw)).rgba * 4.0 - 1.0;
  float t0 = 0.6 - dot(Pf0, Pf0);
  float n0;
  if (t0 < 0.0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad0, Pf0);
  }

  // Noise contribution from second corner
  vec4 Pf1 = Pf0 - o1 + G4;
  o1 = o1 * ONE;
  float perm1xy = texture2D(permTexture, Pi.xy + o1.xy).a;
  float perm1zw = texture2D(permTexture, Pi.zw + o1.zw).a;
  vec4  grad1 = texture2D(gradTexture, vec2(perm1xy, perm1zw)).rgba * 4.0 - 1.0;
  float t1 = 0.6 - dot(Pf1, Pf1);
  float n1;
  if (t1 < 0.0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad1, Pf1);
  }
  
  // Noise contribution from third corner
  vec4 Pf2 = Pf0 - o2 + 2.0 * G4;
  o2 = o2 * ONE;
  float perm2xy = texture2D(permTexture, Pi.xy + o2.xy).a;
  float perm2zw = texture2D(permTexture, Pi.zw + o2.zw).a;
  vec4  grad2 = texture2D(gradTexture, vec2(perm2xy, perm2zw)).rgba * 4.0 - 1.0;
  float t2 = 0.6 - dot(Pf2, Pf2);
  float n2;
  if (t2 < 0.0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad2, Pf2);
  }
  
  // Noise contribution from fourth corner
  vec4 Pf3 = Pf0 - o3 + 3.0 * G4;
  o3 = o3 * ONE;
  float perm3xy = texture2D(permTexture, Pi.xy + o3.xy).a;
  float perm3zw = texture2D(permTexture, Pi.zw + o3.zw).a;
  vec4  grad3 = texture2D(gradTexture, vec2(perm3xy, perm3zw)).rgba * 4.0 - 1.0;
  float t3 = 0.6 - dot(Pf3, Pf3);
  float n3;
  if (t3 < 0.0) n3 = 0.0;
  else {
    t3 *= t3;
    n3 = t3 * t3 * dot(grad3, Pf3);
  }
  
  // Noise contribution from last corner
  vec4 Pf4 = Pf0 - vec4(1.0-4.0*G4);
  float perm4xy = texture2D(permTexture, Pi.xy + vec2(ONE, ONE)).a;
  float perm4zw = texture2D(permTexture, Pi.zw + vec2(ONE, ONE)).a;
  vec4  grad4 = texture2D(gradTexture, vec2(perm4xy, perm4zw)).rgba * 4.0 - 1.0;
  float t4 = 0.6 - dot(Pf4, Pf4);
  float n4;
  if(t4 < 0.0) n4 = 0.0;
  else {
    t4 *= t4;
    n4 = t4 * t4 * dot(grad4, Pf4);
  }

  // Sum up and scale the result to cover the range [-1,1]
  return 27.0 * (n0 + n1 + n2 + n3 + n4);
}

float fbm(vec3 position, int octaves, float frequency, float persistence) {
	float total = 0.0;
	float maxAmplitude = 0.0;
	float amplitude = 1.0;
	for (int i = 0; i < octaves; i++) {
		total += snoise(vec4(position * frequency, time)) * amplitude;
		frequency *= 2.0;
		maxAmplitude += amplitude;
		amplitude *= persistence;
	}
	return total / maxAmplitude;
}

vec4 GetColour(in vec3 p)
{	
	// octaves = 7;	// broken
	// octaves = 6;	// distorted
	// octaves = 5;	// working

	float n1 = fbm(p * 4.0, octavesIn, frequency.x, 0.5);
	float n2 = fbm(p * 3.14159, octavesIn, frequency.z, 0.5);
	vec4 color = vec4(texture2D(diffuse, vec2(0.0, (p.y + 1.0) * 0.5) + vec2(n1*0.075,n2*0.075)).xyz, 1.0);
	return color;
}

void main(void)
{
	// call the GetColour function implemented for this shader type
	vec4 colour = GetColour(v_texCoord3D);
	
	// Hue Shift the colour and store the final result
	gl_FragColor = colour;
}
