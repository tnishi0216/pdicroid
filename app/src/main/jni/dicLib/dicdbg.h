#ifndef __DICDBG_H
#define	__DICDBG_H

#ifdef USE_DD
#include "ddeobj.h"
#endif

#define	DD_OPEN				1
#define	DD_RECORD			2
#define	DD_RECORDEND		3
#define	DD_UPDATE			4
#define	DD_UPDATEEND		5
#define	DD_ERASE			6
#define	DD_ERASEEND			7
#define	DD_REGIST			10
#define	DD_SETNBLOCK		11
#define	DD_SETINDEXBLOCK	12
#define	DD_FREE				13
#ifdef USE_DD
void DD( int cmd, int p1=0, int p2=0, const tchar *str=NULL );
void DDOpen();
void DDClose();
#else
#define	DDOpen()
inline void DD(...) {}
#define	DDClose()
#endif

#endif	// __DICDBG_H
