#ifndef TIME_INCLUDED
#define TIME_INCLUDED


#include <types.c>


Number get_number_of_ticks();


#ifdef __WIN32__

#include <WinAPI/kernel32.c>


Number get_number_of_ticks()
{
	return GetTickCount();
}

#endif//__WIN32__


#endif//TIME_INCLUDED