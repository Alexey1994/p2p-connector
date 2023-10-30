#ifndef WINAPI_MSVCRT_INCLUDED
#define WINAPI_MSVCRT_INCLUDED


#include <types.c>


import cdecl Number _getch();
import cdecl Number _getwch();
import cdecl void _putch(Number character);
import cdecl void _putwch(Number character);


#define PI 3.14159265358979323846


import cdecl Real_Number32 cosf(Real_Number32 angle);
import cdecl Real_Number32 sinf(Real_Number32 angle);
import cdecl Real_Number32 sqrtf(Real_Number32 angle);

import cdecl Real_Number64 cos(Real_Number64 angle);
import cdecl Real_Number64 sin(Real_Number64 angle);


import cdecl void printf(Byte* message, ...);

import cdecl void qsort(void* array, Number number_of_elements, Number size_of_element, Signed_Number32 (cdecl *compare)(void* a, void* b));


import cdecl Number strlen(Byte* s);


import cdecl Number system(Byte* command);


#endif //WINAPI_MSVCRT_INCLUDED