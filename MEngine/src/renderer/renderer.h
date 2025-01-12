#pragma once

#include <stdbool.h>
#include "mservices.h"

typedef struct
{
	bool initialized;
	int width;
	int height;
	double fov;
} glstate_t;

extern glstate_t glstate;

bool Render_Init(void);
void Render_Shutdown(void);
void Render_StartFrame(void);
void Render_Frame(void);
void Render_EndFrame(void);

#define R_DEF_WIN_WIDTH 1920
#define R_DEF_WIN_HEIGHT 1080
#define R_DEF_FULLSCREEN false
#define R_DEF_MULTISAMPLES 4
#define R_DEF_REFRESH_RATE 60
#define R_DEF_VSYNC 1
#define R_DEF_FOV 60.0
#define R_DEF_WIN_NAME "MEngine"

int Render_GetMinWidth(void);
int Render_GetMinHeight(void);

#define MAX_WIN_NAME SYS_MAX_PATH

typedef struct
{
	bool fullscreen;
	int width;
	int height;
	int multisamples;
	int refreshrate;
	char *wndname;		// propogated from gamename in gameservices, so always 256 chars/wchars
} glwndparams_t;

// systems specific to the renderer
bool GLWnd_Init(glwndparams_t params);
void GLWnd_Shutdown(void);
bool GLWnd_ChangeScreenParams(glwndparams_t params);
void GLWnd_SwapBuffers(void);
void GLWnd_SetVSync(int vsync);
int GLWnd_GetVSync(void);
