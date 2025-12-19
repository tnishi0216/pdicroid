#include "tndefs.h"
#pragma	hdrstop
#include "str.h"

#include <windows.h>	// Windowsコードがあるため暫定措置

#define	MAX_FLEN	256

#if 0
Str::Str(int _maxlen)
{
	maxlen = _maxlen;
	buf = new char[1];
	buf[0] = '\0';
}

Str::~Str()
{
	delete[] buf;
}

void Str::set(const char *str)
{
	delete[] buf;
	buf = new char[strlen(str)+1];
	strcpy(buf, str);
}

Path::Path():Str(MAX_FLEN)
{
	fullpath = new char[MAX_FLEN+1];
	fullpath[MAX_FLEN] = '\0';
}

Path::~Path()
{
	delete fullpath;
}

const char *Path::make(const char *filename)
{
	if ( strchr(filename, CHR_DIRSEP) != 0 || strchr(filename, ':') != 0 || buf[0] == '\0'){
		strncpy(fullpath, filename, MAX_FLEN);
	} else {
		strcpy(fullpath, buf);
		char *p = fullpath + strlen(fullpath) - 1;
		if (*p != CHR_DIRSEP && *p != ':'){
			p++;
			*p++ = CHR_DIRSEP;
			*p = '\0';
		}
		strcat(fullpath, filename);
	}
//	cerr << "PATH:" << fullpath << "\n";
	return fullpath;
}
#endif


//TODO: obsolete
tchar *SkipTo( const tchar *str, int c )
{
	while ( *str ){
		if ( *str == c )
        	break;
		str = NEXT_CHAR( str );
	}
    return (tchar*)str;
}

