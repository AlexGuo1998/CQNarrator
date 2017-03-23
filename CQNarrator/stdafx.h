// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>



// reference additional headers your program requires here
#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include "cqp.h"

#ifdef _DEBUG
#define log(str) fprintf(pLogfile, str); fflush(pLogfile);
#define loga(str, arg) fprintf(pLogfile, str, arg); fflush(pLogfile);
#define logb(str, arga, argb) fprintf(pLogfile, str, arga, argb); fflush(pLogfile);
#else // _DEBUG
#define log(str)
#define loga(str, arg)
#define logb(str, arga, argb)
#endif // _DEBUG
