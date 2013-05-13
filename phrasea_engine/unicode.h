/*
 * File:   unicode.h
 * Author: gaulier
 *
 * Created on 22 avril 2013, 20:06
 */

#ifndef UNICODE_H
#define	UNICODE_H

#ifdef	__cplusplus
extern "C"
{
#endif

#define CFLAG_NORMALCHAR 0
#define CFLAG_ENDCHAR 1
#define CFLAG_SPACECHAR 2

#define UNICODE_LC 0
#define UNICODE_ND 1
#define UNICODE_LCND 2

typedef struct
{
	unsigned char flags;
	unsigned char c[3];
} CMAP1;

typedef struct
{
	unsigned char flags;
	unsigned char *s[3];
} CMAP2;


#ifdef	__cplusplus
}
#endif

#endif	/* UNICODE_H */

