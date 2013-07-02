/*
 * phrasea_clock_t.h
 *
 *  Created on: 4 mars 2010
 *      Author: gaulier
 */

#ifndef PHRASEA_CLOCK_T_H_
#define PHRASEA_CLOCK_T_H_

#include <stdio.h>

#pragma warning(push)
#pragma warning(disable:4005)		// disable warning about double def of _WIN32_WINNT
#include "php.h"
#pragma warning(pop)


#ifdef PHP_WIN32
	#include <sys/timeb.h>
	typedef DWORD CHRONO;
#else
    #include <sys/time.h>
	typedef struct timeval CHRONO;
#endif

void startChrono(CHRONO &chrono);
double stopChrono(CHRONO &chrono);

#endif /* PHRASEA_CLOCK_T_H_ */
