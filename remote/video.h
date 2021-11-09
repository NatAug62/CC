/*
Header file for the video.c source file
*/

#pragma once

// include the catch-all header
#include "general.h"

/*
Take a screenshot using the GDI API
Returns a byte buffer with the screenshot data
Returned byte buffer will need to be freed later
*/
BYTE* GDITakeScreenshot();