#include "renderer/emgl.h"

/*
* Function: EMGL_Init
* Loads the OpenGL library, assigns the function pointers
* 
* 	dllname: The name of the OpenGL library to load
* 
* Returns: A boolean if the initialization was successful or not
*/
bool EMGL_Init(const char *dllname)
{
	return(true);
}

/*
* Function: EMGL_Shutdown
* Unloads the OpenGL library, and removes the function pointers
*/
void EMGL_Shutdown(void)
{
}
