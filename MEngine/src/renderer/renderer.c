#include <stdio.h>
#include <time.h>
#include "emgl.h"
#include "common/common.h"
#include "renderer.h"

glstate_t glstate;

/*
* Function: Render_StartFrame
* Clears the screen and depth buffer, ready for rendering
*/
void Render_StartFrame(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen and depth buffer
}

/*
* Function: Render_Frame
* Renders the scene and all components both 2D and 3D
*/
void Render_Frame(void)
{
	// set up for 3D rendering
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLdouble)glstate.width / (GLdouble)glstate.height, 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();

	// TODO: add 3D rendering code here

	glPopMatrix();

	// switch to 2D orthographic projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();	// reset the current modelview matrix
	glPushMatrix();
	gluOrtho2D(0.0, 1.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();

	// TODO: add 2D rendering code here
	// testing code
	glBegin(GL_TRIANGLES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex2f(0.5f, 1.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex2f(0.0f, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex2f(1.0f, 0.0f);
	glEnd();
	// end testing code

	glPopMatrix();
	glPopMatrix();
}

/*
* Function: Render_EndFrame
* Swaps the buffers to display the rendered frame
*/
void Render_EndFrame(void)
{
	GLWnd_SwapBuffers();
}
