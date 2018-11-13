/*
* TPG4162 Project 2 framework
* Author: Einar Baumann
*
* Based on a Lighthouse3D tutorial:
*  http://www.lighthouse3d.com/tutorials/glut-tutorial/the-code-so-far-v/
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
using namespace std;

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include "GL/glut.h"
#include <GL/glext.h>
//#include "glew.c"
#endif


bool	keys[256];
bool	active=true;

GLfloat	xrot, yrot, xspeed, yspeed;
GLfloat	z=-5.0f;

GLfloat LightAmbient[]  =	{ 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat LightDiffuse[]  =	{ 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat LightPosition[] =	{ 0.0f, 0.0f, 2.0f, 1.0f };

// Split value
float	split = 0.5f;

// Texture data
GLuint	texture;
char	matrix [13483][1750];

// GLSL IDs and variables
GLuint	program, glsplit, gltex;


/***************************************************************************************
 * PURPOSE: Read text file (here for the purposes of reading shader code)
 ***************************************************************************************/
char *textFileRead(char *fn) {
	FILE *fp;
	char *content = NULL;

	int count=0;

	if (fn != NULL) {
		fp = fopen(fn, "rb");

		if (fp != NULL) {
			fseek(fp, 0, SEEK_END);
			count = ftell(fp);
			rewind(fp);
			if (count > 0) {
				content = (char *)malloc(sizeof(char) * (count+1));
				count = fread(content,sizeof(char),count,fp);
				content[count] = '\0';
			}
			fclose(fp);
		}
	}
	return content;
}


/********************************************************************************************
 * PURPOSE: Create shader program (by assigning shader code) and create handles to variables
 *******************************************************************************************/
void GLSLInit()
{	
	printf("Here!\n");
	GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);

	const char  *vs = textFileRead((char*)"glsl.vert");
	const char  *fs = textFileRead((char*)"glsl.frag");

	glShaderSource(vshader, 1, &vs, NULL);
	glShaderSource(fshader, 1, &fs, NULL);

	free((void *)vs);
	free((void *)fs);

	glCompileShader(vshader);
	glCompileShader(fshader);

	program = glCreateProgram();
	glAttachShader(program,fshader);
	glAttachShader(program,vshader);

	glLinkProgram(program);

	glsplit = glGetUniformLocation(program, "splitter");
	gltex = glGetUniformLocation(program, "tex");
}

/********************************************************************************************
 * PURPOSE: Assign program and GLSL variables
 *******************************************************************************************/
void setGLSLvariables()
{
	glUseProgram(program);

	glUniform1f(glsplit, split);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE0, texture);
	glUniform1i(gltex, 0);
}

/********************************************************************************************
 * PURPOSE: Disable GLSL
 *******************************************************************************************/
void disableGLSL()
{
	glUseProgram(0);
}


/***************************************************************************************
 * PURPOSE: Read data from SEGY file to global array 'matrix'
 ***************************************************************************************/
void ReadSEGYFile(char* sFileName, int iNumTraces, int iNumSamples)
{
    //printf(sFileName);
	FILE *f = fopen(sFileName, "rb");
	if (!f) printf("\n\n Missing SEGY data file: %s\n\n", sFileName);

	fseek(f, 3600, SEEK_SET);						// skip file header

	for(int i=0; i < iNumTraces; i++)
	{
		fseek(f, 240, SEEK_CUR);					// skip trace header
		fread(matrix[i], 1, iNumSamples, f);
	}
	fclose(f);
}

/***************************************************************************************
 * PURPOSE: Create textures from values of the global array 'matrix'
 * NOTE:
 *   Non-float textures (i.e. the traditional integer formats) are clamped to the
 *   domain [0.0, 1.0] by the texture pipeline (as traditional). GL_LUMINANCE is such
 *   a format thus ensuring that the fragment shader can operate on the domain [0.0, 1.0].
 *   GL_LUMINANCE is also beneficial with regard to memory and bandwith.
 *
 *   With regard to a need for submitting actual float values: The relatively new float
 *   textures may be used to convey the actual float values, e.g. GL_LUMINANCE_FLOAT.
 *   Howevever, check the formats and hw support for which formats that are available
 *   and their resolution (#bits per channel).
 ***************************************************************************************/
int CreateGLTextures()
{
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);

	unsigned char* c_texture = new unsigned char[1024*1024];   // had to use the heap (too large for the stack)

	for(int i=0;i<1024;i++)
	{
		for(int j=0;j<1024;j++) c_texture[i*1024+j] = matrix[i][j] + 128;  // create unsigned byte values from SEGY byte values (ranging from -128 to 127)
	}

	glBindTexture(GL_TEXTURE_2D, texture);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE, 1024, 1024, GL_LUMINANCE, GL_UNSIGNED_BYTE, c_texture);

	return true;
}

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0) height=1;							// Prevent A Divide By Zero By Making Height Equal One

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}


int InitGL()										// All Setup For OpenGL Goes Here
{
	glewInit();
	CreateGLTextures();


	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.5f, 0.5f, 0.5f, 0.5f);				// Grey Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations

	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);		// Setup The Ambient Light
	glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);		// Setup The Diffuse Light
	glLightfv(GL_LIGHT0, GL_POSITION,LightPosition);	// Position The Light
	glEnable(GL_LIGHT0);								// Enable Light One


	GLSLInit();
	
	return true;										// Initialization Went OK
}


int DrawGLScene()									// Here's Where We Do All The Drawing
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
	glLoadIdentity();									// Reset The View
	glTranslatef(0.0f,0.0f,z);

	glRotatef(xrot,1.0f,0.0f,0.0f);
	glRotatef(yrot,0.0f,1.0f,0.0f);
	glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
	setGLSLvariables();

	glBegin(GL_QUADS);
		// Front Face
		glNormal3f( 0.0f, 0.0f, 1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
		// Back Face
		glNormal3f( 0.0f, 0.0f,-1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
		// Top Face
		glNormal3f( 0.0f, 1.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
		// Bottom Face
		glNormal3f( 0.0f,-1.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
		// Right face
		glNormal3f( 1.0f, 0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
		// Left Face
		glNormal3f(-1.0f, 0.0f, 0.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
	glEnd();

	xrot+=xspeed;
	yrot+=yspeed;

	disableGLSL();

	return true;										// Keep Going
}


void processNormalKeys(unsigned char key, int xx, int yy) {
	switch (key) {
		case 113:
            split -= 0.01f;
            printf("Decrementing split value to %f.\n",split);
			break;
        case 119:
            printf("Incrementing split value to %f.\n",split);
            split += 0.01f;
			break;
	}
}

void pressKey(int key, int xx, int yy) {

	switch (key) {
		case GLUT_KEY_UP:
            xspeed-=0.01f;
            break;
		case GLUT_KEY_DOWN:
            xspeed+=0.01f;
            break;
        case GLUT_KEY_LEFT:
            yspeed-=0.01f;
            break;
        case GLUT_KEY_RIGHT:
            yspeed+=0.01f;
            break;
	}
}


void renderScene(void) {
	DrawGLScene();
	glutSwapBuffers();           // Display our matrix (i.e. the figure)
}







int main(int argc, char **argv) {
    ReadSEGYFile((char*)"Data/NVGT-88-06.sgy", 13483,1750);




	// init GLUT and create window
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

	glutInitWindowPosition(100,100);

	

	glutInitWindowSize(640,640);
	glutCreateWindow("Project 1");


	// register glut callbacks
	glutDisplayFunc(renderScene);

	

	glutReshapeFunc(ReSizeGLScene);


	glutIdleFunc(renderScene);

	// Register keyboard functions
    glutKeyboardFunc(processNormalKeys);
	glutSpecialFunc(pressKey);

	// OpenGL initialization

	InitGL();


	// enter GLUT event processing cycle
	glutMainLoop();

	return 1;
}
