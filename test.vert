#version 120

varying vec3 v_texCoord3D;

void main( void )
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    v_texCoord3D = gl_Normal.xyz;
}
