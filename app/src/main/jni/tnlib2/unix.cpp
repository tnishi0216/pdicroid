#include "tndefs.h"
#pragma hdrstop
#include <time.h>
#include <unistd.h>
#include <string.h>

#define	min(x,y)	((x)<(y)?(x):(y))

//////////////////////////////////////////////////////////////////////////////
// Windows compatible
//////////////////////////////////////////////////////////////////////////////

//Note: Androidではせいぜい１時間程度の時間しか計測できないので注意！(CLOCKS_PER_SEC==1000000のため)
unsigned GetTickCount()
{
	return clock() / (CLOCKS_PER_SEC/1000);
}

void Sleep(int msec)
{
	usleep((useconds_t)msec*1000);
}

#if 0
//TODO: これだけでいいのだろうか？
// 用途：
void CharLower(char *s)
{
	while (*s){
		char c = *s;
		if (c>='A' && c<='Z'){
			*s = c + 0x20;
		}
		s++;
	}
}
#endif

int strnicmp( const char *s1, const char *s2, int len )
{
	int l1 = min(strlen(s1),len);
	int l2 = min(strlen(s2),len);
	if ( l1 < l2 ) return -1;
	int r;
	while(l2){
		r = *s1 - *s2;
		if ( r != 0 ) return r;
		l2--;
	}
	return 0;
}

#if 0
void DBW( const char *format, ... )
{
	//TODO: not yet
}
#endif
