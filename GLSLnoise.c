/* 
 * Testbed for GLSL fragment noise() functions.
 *
 * Shaders are loaded from two external files:
 * "GLSLnoise.vert" and "GLSLnoise.frag".
 * The program itself draws a spinning sphere
 * with a noise-generated fragment color.
 *
 * This program uses GLFW for convenience, to handle the OS-specific
 * window management stuff. Some Windows-specific stuff for extension
 * loading is still here, but the code should still compile and run
 * on other platforms - function pointer reloading should not hurt. 
 *
 * Author: Stefan Gustavson (stegu@itn.liu.se) 2004, 2005, 2010
 */

 /*
As the original author of this code, I hereby
release it irrevocably into the public domain.
Please feel free to use it for whatever you want.
Credit is appreciated where appropriate, and I also
appreciate being told where this code finds any use,
but you may do as you like. Alternatively, if you want
to have a familiar OSI-approved license, you may use
This code under the terms of the MIT license:

Copyright (C) 2004, 2005, 2010 by Stefan Gustavson. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <GL/glfw.h>

// The default include file for GL extensions might not be up to date.
// #include <GL/glext.h>
#include "glext.h"

// colour ramp image
#include "out_rgb.h"

/* Global variables for all the nice stuff we need above OpenGL 1.1 */
#ifdef WIN32
PFNGLACTIVETEXTUREPROC           glActiveTexture      = NULL;
#endif
PFNGLCREATEPROGRAMPROC           glCreateProgram      = NULL;
PFNGLDELETEPROGRAMPROC           glDeleteProgram      = NULL;
PFNGLUSEPROGRAMPROC              glUseProgram         = NULL;
PFNGLCREATESHADERPROC            glCreateShader       = NULL;
PFNGLDELETESHADERPROC            glDeleteShader       = NULL;
PFNGLSHADERSOURCEPROC            glShaderSource       = NULL;
PFNGLCOMPILESHADERPROC           glCompileShader      = NULL;
PFNGLGETSHADERIVPROC             glGetShaderiv        = NULL;
PFNGLGETPROGRAMIVPROC            glGetProgramiv       = NULL;
PFNGLATTACHSHADERPROC            glAttachShader       = NULL;
PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog   = NULL;
PFNGLGETPROGRAMINFOLOGPROC       glGetProgramInfoLog  = NULL;
PFNGLLINKPROGRAMPROC             glLinkProgram        = NULL;
PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation = NULL;
PFNGLUNIFORM3FPROC               glUniform3f          = NULL;
PFNGLUNIFORM4FPROC               glUniform4f          = NULL;
PFNGLUNIFORM1FPROC               glUniform1f          = NULL;
PFNGLUNIFORM1IPROC               glUniform1i          = NULL;

/* Some more global variables for convenience. This is C, and I'm lazy. */
double t0 = 0.0;
int frames = 0;
char titlestring[200];

GLuint permTextureID;
GLuint gradTextureID;
GLuint diffTextureID;
GLuint sphereList;
GLboolean updateTime = GL_TRUE;
GLboolean animateObject = GL_TRUE;

GLuint octaves = 8;

GLhandleARB programObj;
GLhandleARB vertexShader;
GLhandleARB fragmentShader;
GLint location_permTexture = -1; 
GLint location_gradTexture = -1; 
GLint location_diffTexture = -1; 
GLint location_time = -1;
GLint location_octavesIn = -1;
GLint location_frequency = -1;

const char *vertexShaderStrings[1];
const char *fragmentShaderStrings[1];
GLint vertexCompiled;
GLint fragmentCompiled;
GLint shadersLinked;
char str[4096]; // For error messages from the GLSL compiler and linker

int perm[256]= {151,160,137,91,90,15,
  131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
  190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
  88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
  77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
  102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
  135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
  5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
  223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
  129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
  251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
  49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
  138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};

/* These are Ken Perlin's proposed gradients for 3D noise. I kept them for
   better consistency with the reference implementation, but there is really
   no need to pad this to 16 gradients for this particular implementation.
   If only the "proper" first 12 gradients are used, they can be extracted
   from the grad4[][] array: grad3[i][j] == grad4[i*2][j], 0<=i<=11, j=0,1,2
*/
int grad3[16][3] = {{0,1,1},{0,1,-1},{0,-1,1},{0,-1,-1},
                   {1,0,1},{1,0,-1},{-1,0,1},{-1,0,-1},
                   {1,1,0},{1,-1,0},{-1,1,0},{-1,-1,0}, // 12 cube edges
                   {1,0,-1},{-1,0,-1},{0,-1,1},{0,1,1}}; // 4 more to make 16

/* These are my own proposed gradients for 4D noise. They are the coordinates
   of the midpoints of each of the 32 edges of a tesseract, just like the 3D
   noise gradients are the midpoints of the 12 edges of a cube.
*/
int grad4[32][4]= {{0,1,1,1}, {0,1,1,-1}, {0,1,-1,1}, {0,1,-1,-1}, // 32 tesseract edges
                   {0,-1,1,1}, {0,-1,1,-1}, {0,-1,-1,1}, {0,-1,-1,-1},
                   {1,0,1,1}, {1,0,1,-1}, {1,0,-1,1}, {1,0,-1,-1},
                   {-1,0,1,1}, {-1,0,1,-1}, {-1,0,-1,1}, {-1,0,-1,-1},
                   {1,1,0,1}, {1,1,0,-1}, {1,-1,0,1}, {1,-1,0,-1},
                   {-1,1,0,1}, {-1,1,0,-1}, {-1,-1,0,1}, {-1,-1,0,-1},
                   {1,1,1,0}, {1,1,-1,0}, {1,-1,1,0}, {1,-1,-1,0},
                   {-1,1,1,0}, {-1,1,-1,0}, {-1,-1,1,0}, {-1,-1,-1,0}};

/*
 * printError() - Signal an error. MessageBox() is a Windows-specific function,
 * rewrite this to print the error message to the console to make it portable!
 */
void printError(const char *errtype, const char *errmsg) {
  fprintf(stderr, "%s: %s\n", errtype, errmsg);
}


/*
 * Override the Win32 filelength() function with
 * a version that takes a Unix-style file handle as
 * input instead of a file ID number, and which works
 * on platforms other than Windows.
 */
long filelength(FILE *file) {
    long numbytes;
    long savedpos = ftell(file);
    fseek(file, 0, SEEK_END);
    numbytes = ftell(file);
    fseek(file, savedpos, SEEK_SET);
    return numbytes;
}


/*
 * loadExtensions() - Load OpenGL extensions for anything above OpenGL
 * version 1.1. (This is a requirement from Windows, not from OpenGL.)
 */
void loadExtensions() {
    //These extension strings indicate that the OpenGL Shading Language
    // and GLSL shader objects are supported.
    if(!glfwExtensionSupported("GL_ARB_shading_language_100"))
    {
        printError("GL init error", "GL_ARB_shading_language_100 extension was not found");
        return;
    }
    if(!glfwExtensionSupported("GL_ARB_shader_objects"))
    {
        printError("GL init error", "GL_ARB_shader_objects extension was not found");
        return;
    }
    else
    {
#ifdef WIN32
        glActiveTexture           = (PFNGLACTIVETEXTUREPROC)glfwGetProcAddress("glActiveTexture");
#endif
        glCreateProgram           = (PFNGLCREATEPROGRAMPROC)glfwGetProcAddress("glCreateProgram");
        glDeleteProgram           = (PFNGLDELETEPROGRAMPROC)glfwGetProcAddress("glDeleteProgram");
        glUseProgram              = (PFNGLUSEPROGRAMPROC)glfwGetProcAddress("glUseProgram");
        glCreateShader            = (PFNGLCREATESHADERPROC)glfwGetProcAddress("glCreateShader");
        glDeleteShader            = (PFNGLDELETESHADERPROC)glfwGetProcAddress("glDeleteShader");
        glShaderSource            = (PFNGLSHADERSOURCEPROC)glfwGetProcAddress("glShaderSource");
        glCompileShader           = (PFNGLCOMPILESHADERPROC)glfwGetProcAddress("glCompileShader");
        glGetShaderiv             = (PFNGLGETSHADERIVPROC)glfwGetProcAddress("glGetShaderiv");
        glGetShaderInfoLog        = (PFNGLGETSHADERINFOLOGPROC)glfwGetProcAddress("glGetShaderInfoLog");
        glAttachShader            = (PFNGLATTACHSHADERPROC)glfwGetProcAddress("glAttachShader");
        glLinkProgram             = (PFNGLLINKPROGRAMPROC)glfwGetProcAddress("glLinkProgram");
        glGetProgramiv            = (PFNGLGETPROGRAMIVPROC)glfwGetProcAddress("glGetProgramiv");
        glGetProgramInfoLog       = (PFNGLGETPROGRAMINFOLOGPROC)glfwGetProcAddress("glGetProgramInfoLog");
        glGetUniformLocation      = (PFNGLGETUNIFORMLOCATIONPROC)glfwGetProcAddress("glGetUniformLocation");
        glUniform3f               = (PFNGLUNIFORM3FPROC)glfwGetProcAddress("glUniform3f");
        glUniform4f               = (PFNGLUNIFORM4FPROC)glfwGetProcAddress("glUniform4f");
        glUniform1f               = (PFNGLUNIFORM1FPROC)glfwGetProcAddress("glUniform1f");
        glUniform1i               = (PFNGLUNIFORM1IPROC)glfwGetProcAddress("glUniform1i");

        if( !glActiveTexture || !glCreateProgram || !glDeleteProgram || !glUseProgram ||
            !glCreateShader || !glDeleteShader || !glShaderSource || !glCompileShader || 
            !glGetShaderiv || !glGetShaderInfoLog || !glAttachShader || !glLinkProgram ||
            !glGetProgramiv || !glGetProgramInfoLog || !glGetUniformLocation ||
            !glUniform4f || !glUniform1f || !glUniform1i )
        {
            printError("GL init error", "One or more required OpenGL functions were not found");
            return;
        }
    }
}


/*
 * readShaderFile(filename) - read a shader source string from a file
 */
unsigned char* readShaderFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if(file == NULL)
    {
        printError("ERROR", "Cannot open shader file!");
  		  return 0;
    }
    int bytesinfile = filelength(file);
    unsigned char *buffer = (unsigned char*)malloc(bytesinfile+1);
    int bytesread = fread( buffer, 1, bytesinfile, file);
    buffer[bytesread] = 0; // Terminate the string with 0
    fclose(file);
    
    return buffer;
}


/*
 * createShaders() - create, load, compile and link the GLSL shader objects.
 */
void createShaders() {
	  // Create the vertex shader.
    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    unsigned char *vertexShaderAssembly = readShaderFile("test.vert");
    vertexShaderStrings[0] = (char*)vertexShaderAssembly;
    glShaderSource( vertexShader, 1, vertexShaderStrings, NULL );
    glCompileShader( vertexShader);
    free((void *)vertexShaderAssembly);

    glGetShaderiv( vertexShader, GL_COMPILE_STATUS,
                               &vertexCompiled );
    if(vertexCompiled  == GL_FALSE)
  	{
        glGetShaderInfoLog(vertexShader, sizeof(str), NULL, str);
        printError("Vertex shader compile error", str);
  	}

  	// Create the fragment shader.
    fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );

    unsigned char *fragmentShaderAssembly = readShaderFile( "test.frag" );
    fragmentShaderStrings[0] = (char*)fragmentShaderAssembly;
    glShaderSource( fragmentShader, 1, fragmentShaderStrings, NULL );
    glCompileShader( fragmentShader );
    free((void *)fragmentShaderAssembly);

    glGetProgramiv( fragmentShader, GL_COMPILE_STATUS, 
                               &fragmentCompiled );
    if(fragmentCompiled == GL_FALSE)
   	{
        glGetShaderInfoLog( fragmentShader, sizeof(str), NULL, str );
        printError("Fragment shader compile error", str);
    }

    // Create a program object and attach the two compiled shaders.
    programObj = glCreateProgram();
    glAttachShader( programObj, vertexShader );
    glAttachShader( programObj, fragmentShader );

    // Link the program object and print out the info log.
    glLinkProgram( programObj );
    glGetProgramiv( programObj, GL_LINK_STATUS, &shadersLinked );

    if( shadersLinked == GL_FALSE )
	{
		glGetProgramInfoLog( programObj, sizeof(str), NULL, str );
		printError("Program object linking error", str);
	}
	// Locate the uniform shader variables so we can set them later:
    // a texture ID ("permTexture") and a float ("time").
	location_permTexture = glGetUniformLocation( programObj, "permTexture" );
	if(location_permTexture == -1)
    printError("Binding error","Failed to locate uniform variable 'permTexture'.");
    // This is not needed for the 2D and 3D noise variants.
    location_gradTexture = glGetUniformLocation( programObj, "gradTexture" );
	/*
    if(location_gradTexture == -1)
      printError("Binding error","Failed to locate uniform variable 'gradTexture'.");
    */
	location_diffTexture = glGetUniformLocation( programObj, "diffuse" );
	location_octavesIn = glGetUniformLocation( programObj, "octavesIn" );
	location_frequency = glGetUniformLocation( programObj, "frequency" );
    // This is not used for the 2D noise demo.
    location_time = glGetUniformLocation( programObj, "time" );
    /*
	if(location_time == -1)
      printError("Binding error", "Failed to locate uniform variable 'time'.");
    */
}


/*
 * showFPS() - Calculate and report frames per second
 * (updated once per second) in the window title bar
 */
void showFPS() {

    double t, fps;
    
    // Get current time
    t = glfwGetTime();  // Gets number of seconds since glfwInit()
    // If one second has passed, or if this is the very first frame
    if( (t-t0) > 1.0 || frames == 0 )
    {
        fps = (double)frames / (t-t0);
        sprintf(titlestring, "GLSL Perlin noise (%.1f FPS), %u octaves", fps, octaves);
        glfwSetWindowTitle(titlestring);
        t0 = t;
        frames = 0;
    }
    frames ++;
}


/*
 * setupCamera() - set up the OpenGL projection and (model)view matrices
 */
void setupCamera() {

    int width, height;
    
    // Get window size. It may start out different from the requested
    // size, and will change if the user resizes the window.
    glfwGetWindowSize( &width, &height );
    if(height<=0) height=1; // Safeguard against iconified/closed window

    // Set viewport. This is the pixel rectangle we want to draw into.
    glViewport( 0, 0, width, height ); // The entire window

    // Select and setup the projection matrix.
    glMatrixMode(GL_PROJECTION); // "We want to edit the projection matrix"
    glLoadIdentity(); // Reset the matrix to identity
    // 45 degrees FOV, same aspect ratio as viewport, depth range 1 to 100
    gluPerspective( 45.0f, (GLfloat)width/(GLfloat)height, 1.0f, 100.0f );

    // Select and setup the modelview matrix.
    glMatrixMode( GL_MODELVIEW ); // "We want to edit the modelview matrix"
    glLoadIdentity(); // Reset the matrix to identity
    // Look from 0,-4,0 towards 0,0,0 with Z as "up" in the image
    gluLookAt( 0.0f, -4.0f, 0.0f,  // Eye position
               0.0f, 0.0f, 0.0f,   // View point
               0.0f, 0.0f, 1.0f ); // Up vector
}


/*
 * drawTexturedSphere(r, segs) - Draw a sphere centered on the local
 * origin, with radius "r" and approximated by "segs" polygon segments,
 * with texture coordinates in a latitude-longitude mapping.
 */
void drawTexturedSphere(float r, int segs) {
  int i, j;
  float x, y, z, z1, z2, R, R1, R2;

  // Top cap
  glBegin(GL_TRIANGLE_FAN);
  glNormal3f(0,0,1);
  glTexCoord2f(0.5f,1.0f); // This is an ugly (u,v)-mapping singularity
  glVertex3f(0,0,r);
  z = cos(M_PI/segs);
  R = sin(M_PI/segs);
    for(i = 0; i <= 2*segs; i++) {
      x = R*cos(i*2.0*M_PI/(2*segs));
      y = R*sin(i*2.0*M_PI/(2*segs));
      glNormal3f(x, y, z);
      glTexCoord2f((float)i/(2*segs), 1.0f-1.0f/segs);
      glVertex3f(r*x, r*y, r*z);
    }
  glEnd();  

  // Height segments
  for(j = 1; j < segs-1; j++) {
    z1 = cos(j*M_PI/segs);
    R1 = sin(j*M_PI/segs);
    z2 = cos((j+1)*M_PI/segs);
    R2 = sin((j+1)*M_PI/segs);
    glBegin(GL_TRIANGLE_STRIP);
    for(i = 0; i <= 2*segs; i++) {
      x = R1*cos(i*2.0*M_PI/(2*segs));
      y = R1*sin(i*2.0*M_PI/(2*segs));
      glNormal3f(x, y, z1);
      glTexCoord2f((float)i/(2*segs), 1.0f-(float)j/segs);
      glVertex3f(r*x, r*y, r*z1);
      x = R2*cos(i*2.0*M_PI/(2*segs));
      y = R2*sin(i*2.0*M_PI/(2*segs));
      glNormal3f(x, y, z2);
      glTexCoord2f((float)i/(2*segs), 1.0f-(float)(j+1)/segs);
      glVertex3f(r*x, r*y, r*z2);
    }
    glEnd();
  }

  // Bottom cap
  glBegin(GL_TRIANGLE_FAN);
  glNormal3f(0,0,-1);
  glTexCoord2f(0.5f, 1.0f); // This is an ugly (u,v)-mapping singularity
  glVertex3f(0,0,-r);
  z = -cos(M_PI/segs);
  R = sin(M_PI/segs);
    for(i = 2*segs; i >= 0; i--) {
      x = R*cos(i*2.0*M_PI/(2*segs));
      y = R*sin(i*2.0*M_PI/(2*segs));
      glNormal3f(x, y, z);
      glTexCoord2f(1.0f-(float)i/(2*segs), 1.0f/segs);
      glVertex3f(r*x, r*y, r*z);
    }
  glEnd();
}


/*
 * initSphereList(GLuint *listID, GLdouble scale) - create a display list
 * to render the sphere more efficently than calling lots of trigonometric
 * functions for each frame.
 * (A vertex array could be even faster, but I'm a bit lazy here.)
 */
void initSphereList(GLuint *listID, GLdouble scale)
{
  *listID = glGenLists(1);
  
  glNewList(*listID, GL_COMPILE);
  drawTexturedSphere(scale, 20);
  glEndList();
}


/*
 * initPermTexture(GLuint *texID) - create and load a 2D texture for
 * a combined index permutation and gradient lookup table.
 * This texture is used for 2D and 3D noise, both classic and simplex.
 */
void initPermTexture(GLuint *texID)
{
  char *pixels;
  int i,j;
  
  glGenTextures(1, texID); // Generate a unique texture ID
  glBindTexture(GL_TEXTURE_2D, *texID); // Bind the texture to texture unit 0

  pixels = (char*)malloc( 256*256*4 );
  for(i = 0; i<256; i++)
    for(j = 0; j<256; j++) {
      int offset = (i*256+j)*4;
      char value = perm[(j+perm[i]) & 0xFF];
      pixels[offset] = grad3[value & 0x0F][0] * 64 + 64;   // Gradient x
      pixels[offset+1] = grad3[value & 0x0F][1] * 64 + 64; // Gradient y
      pixels[offset+2] = grad3[value & 0x0F][2] * 64 + 64; // Gradient z
      pixels[offset+3] = value;                     // Permuted index
    }
  
  // GLFW texture loading functions won't work here - we need GL_NEAREST lookup.
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
}

/*
 * initGradTexture(GLuint *texID) - create and load a 2D texture
 * for a 4D gradient lookup table. This is used for 4D noise only.
 */
void initGradTexture(GLuint *texID)
{
  char *pixels;
  int i,j;
  
  glActiveTexture( GL_TEXTURE1 ); // Activate a different texture unit (unit 1)

  glGenTextures(1, texID); // Generate a unique texture ID
  glBindTexture(GL_TEXTURE_2D, *texID); // Bind the texture to texture unit 2

  pixels = (char*)malloc( 256*256*4 );
  for(i = 0; i<256; i++)
    for(j = 0; j<256; j++) {
      int offset = (i*256+j)*4;
      char value = perm[(j+perm[i]) & 0xFF];
      pixels[offset] = grad4[value & 0x1F][0] * 64 + 64;   // Gradient x
      pixels[offset+1] = grad4[value & 0x1F][1] * 64 + 64; // Gradient y
      pixels[offset+2] = grad4[value & 0x1F][2] * 64 + 64; // Gradient z
      pixels[offset+3] = grad4[value & 0x1F][3] * 64 + 64; // Gradient w
    }
  
  // GLFW texture loading functions won't work here - we need GL_NEAREST lookup.
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

  glActiveTexture( GL_TEXTURE0 ); // Switch active texture unit back to 0 again
}

/*
 * initDiffTexture(GLuint *texID) - create and load a 2D texture for
 * a combined index permutation and gradient lookup table.
 * This texture is used for 2D and 3D noise, both classic and simplex.
 */
void initDiffTexture(GLuint *texID)
{
  glActiveTexture( GL_TEXTURE2 ); // Activate a different texture unit (unit 2)
  
  glGenTextures(1, texID); // Generate a unique texture ID
  glBindTexture(GL_TEXTURE_2D, *texID); // Bind the texture to texture unit 2
  
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, MagickImage+12 );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

  glActiveTexture( GL_TEXTURE0 ); // Switch active texture unit back to 0 again
}



/*
 * drawScene(float t) - the actual drawing commands to render our scene.
 */
void drawScene(float t) {
    glRotatef(30.0f, 1.0f, 0.0f, 0.0f); // Rotate the view somewhat
    glPushMatrix(); // Transforms to animate the light:
      glRotatef(30.0f*t, 0.0f, 0.0f, 1.0f);
      glTranslatef(5.0f, 0.0f, 0.0f); // Orbit around Z, 5 units from origin
      float lightpos0[4]={0.0f, 0.0f, 0.0f, 1.0f}; // Origin, in hom. coords
      glLightfv(GL_LIGHT0, GL_POSITION, lightpos0); // Set light position
    glPopMatrix(); // Revert to initial transform
    glPushMatrix(); // Transforms to animate the object:
      glRotatef(45.0f*t, 0.0f, 0.0f, 1.0f); // Spin around Z
      glColor3f(1.0f, 1.0f, 1.0f); // White base color
      // Enable lighting and the LIGHT0 we placed before
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);
      // We have now enabled lighting, so this object is lit.
      glCallList(sphereList); // Draw a sphere using the display list
    glPopMatrix(); // Revert to initial transform
    // Disable lighting again, to prepare for next frame.
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
}


/*
 * renderScene() - a wrapper to drawScene() to switch shaders on and off
 */
void renderScene( void )
{
  static float t;
  if (animateObject) t = (float)glfwGetTime(); // Get elapsed time
	if(GL_TRUE)
	{
  	  // Use vertex and fragment shaders.
	  glUseProgram( programObj );
	  // Update the uniform time variable.
      if(( updateTime ) && ( location_time != -1 ))
    		glUniform1f( location_time, (float)glfwGetTime() );
	  // Identify the textures to use.
 	  if( location_permTexture != -1 )
  		glUniform1i( location_permTexture, 0 ); // Texture unit 0
 	  if( location_gradTexture != -1 )
  		glUniform1i( location_gradTexture, 1 ); // Texture unit 1
	  if( location_diffTexture != -1 )
  		glUniform1i( location_diffTexture, 2 ); // Texture unit 2
	  
	  
	  if( location_octavesIn != -1 )
  		glUniform1i( location_octavesIn, octaves ); //
	  if( location_frequency != -1 )
  		glUniform3f( location_frequency, 0.5, 1.0, 2.0 ); // 
		
 		// Render with the shaders active.
	  drawScene(t);
	  // Deactivate the shaders.
      glUseProgram(0);
	}
	else
	{
		// Render without shaders.
		drawScene(t);
	}
}


/*
 * main(argc, argv) - the standard C entry point for the program
 */
int main(int argc, char *argv[]) {

    int running = GL_TRUE; // Main loop exits when this is set to GL_FALSE
    
    // Initialise GLFW
    glfwInit();

    // Open the OpenGL window
    if( !glfwOpenWindow(640, 480, 8,8,8,8, 32,0, GLFW_WINDOW) )
    {
        glfwTerminate(); // glfwOpenWindow failed, quit the program.
        return 1;
    }
    
    // Load the extensions for GLSL - note that this has to be done
    // *after* the window has been opened, or we won't have a GL context
    // to query for those extensions and connect to instances of them.
    loadExtensions();

    // Create the two shaders
    createShaders();

    // Enable back face culling and Z buffering
    glEnable(GL_CULL_FACE); // Cull away all back facing polygons
    glEnable(GL_DEPTH_TEST); // Use the Z buffer

    // Use a dark blue color with A=1 for the background color
    glClearColor(0.0f, 0.1f, 0.3f, 1.0f);

    glEnable(GL_TEXTURE_1D); // Enable 1D texturing
    glEnable(GL_TEXTURE_2D); // Enable 2D texturing

    // Create and load the textures (generated, not read from a file)
    initPermTexture(&permTextureID);
    initGradTexture(&gradTextureID);
	initDiffTexture(&diffTextureID);
    
    glfwSwapInterval(1); // Wait for screen refresh between frames

    // Compile a display list for the teapot, to render it more quickly
    initSphereList(&sphereList, 1.0);
    
    // Main loop
    while(running)
    {
        // Calculate and update the frames per second (FPS) display
        showFPS();

        // Clear the color buffer and the depth buffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
        // Set up the camera projection.
        setupCamera();
        
        // Draw the scene.
        renderScene();

        // Swap buffers, i.e. display the image and prepare for next frame.
        glfwSwapBuffers();

        // Decide whether to update the shader "time" variable or not
        if(glfwGetKey('A')) updateTime = GL_TRUE;
        if(glfwGetKey('S')) updateTime = GL_FALSE;
        // Decide whether to animate the rotation for the scene or not
        if(glfwGetKey('Z')) animateObject = GL_TRUE;
        if(glfwGetKey('X')) animateObject = GL_FALSE;
		
        // Increase / Decrease the number of octaves used in the shader fbm noise
		static float fLastTime = 0.0f;
		const float currTime = (float)glfwGetTime();
		if(currTime - fLastTime > 0.5f) {
			if(glfwGetKey('Q')) {
				++octaves;
				octaves = min(octaves, 32);
				fLastTime = currTime;
			}
			if(glfwGetKey('E')) {
				--octaves;
				octaves = max(octaves, 2);
				fLastTime = currTime;
			}
		}
		

        // Check if the ESC key was pressed or the window was closed.
        if(glfwGetKey(GLFW_KEY_ESC) || !glfwGetWindowParam(GLFW_OPENED))
          running = GL_FALSE;
    }

    // Close the OpenGL window and terminate GLFW.
    glfwTerminate();

    return 0;
}
