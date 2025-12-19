#include "stdafx.h"
#include "tndefs.h"
#include "tnstr.h"
#include <stdio.h>
#include <stdarg.h>
#include <algorithm>
#include <fcntl.h>
#include <sys/stat.h>
#include <wchar.h>
#include <unistd.h>

static tnstr TemporaryDirectory;

void InitAndroid(const tchar *tempdir)
{
	TemporaryDirectory = tempdir;
	tchar c = TemporaryDirectory[ _tcslen(TemporaryDirectory)-1 ];
	if (c != '/'){
		TemporaryDirectory += _t("/");
	}
}

wchar_t *wcsdup(const wchar_t *s)
{
	int len = wcslen(s);
	wchar_t *r = (wchar_t*)malloc((len+1)*sizeof(wchar_t));
	if (!r) return NULL;
	wcscpy(r, s);
	return r;
}

int wcsicmp(const wchar_t *s1, const wchar_t *s2)
{
	while (*s1 && *s2){
		int r = (int)towlower(*s1) - (int)towlower(*s2);
		if (r){
			return r;
		}
		s1++;
		s2++;
	}
	return (int)*s1 - (int)*s2;
}

int __wcsnicmp(const wchar_t *s1, const wchar_t *s2, int n)
{
	while (*s1 && *s2 && n>0){
		int r = (int)towlower(*s1) - (int)towlower(*s2);
		if (r){
			return r;
		}
		s1++;
		s2++;
		n--;
	}
	return (int)*s1 - (int)*s2;
}

char *itoa(int val, char *s, int radix)
{
	if (radix!=10){
		//TODO: 未対応
		s[0] = '\0';
		return NULL;
	}

	char *t;
	int mod;

	if (val < 0){
		*s++ = '-';
		val = -val;
	}
	t = s;

	while (val){
		mod = val % 10;
		*t++ = static_cast<char>(mod) + '0';
		val /= 10;
	}

	if (s == t) *t++ = '0';
	*t = '\0';
	std::reverse(s, t);
	return s;
}

tchar *_itow(int val, wchar_t *s, int radix)
{
	if (radix!=10){
		//TODO: 未対応
		s[0] = '\0';
		return NULL;
	}

	wchar_t *t;
	int mod;

	if (val < 0){
		*s++ = '-';
		val = -val;
	}
	t = s;

	while (val){
		mod = val % 10;
		*t++ = static_cast<wchar_t>(mod) + '0';
		val /= 10;
	}

	if (s == t) *t++ = '0';
	*t = '\0';
	std::reverse(s, t);
	return s;
}

int _wtoi(const wchar_t *str)
{
	int val = 0;
	while (*str){
		wchar_t c = *str++;
		if (c>='0' && c<='9'){
			val = val * 10 + c-'0';
		} else
			break;
	}
	return val;
}
wchar_t *wcstok(wchar_t *str, const wchar_t *token)	//TODO: multithread
{
	return str;	//TODO: not impled.
}
off_t filelength( int handle )
{
	struct stat st;
	if (fstat(handle, &st)!=0) return (off_t)-1;
	return st.st_size;
}

FILE *fopen(const wchar_t *filename, const char *mode)
{
	__cstr sfilename(filename);
	return ::fopen((const char*)sfilename, mode);
}

int wopen(const wchar_t *filename, int flags)
{
	__cstr sfilename(filename);
	return open((const char*)sfilename, flags);
}

int wopen(const wchar_t *filename, int flags, mode_t mode)
{
	__cstr sfilename(filename);
	return open((const char*)sfilename, flags, mode);
}

int _wmkdir(const wchar_t *dirname)
{
	__cstr sdirname(dirname);
	return mkdir(sdirname, 0777);
}

int wchmod(const wchar_t *filename, mode_t mode)
{
	__cstr sfilename(filename);
	return chmod((const char*)sfilename, mode);
}

int wremove(const wchar_t*pathname)
{
	__cstr spathname(pathname);
	return remove((const char*)spathname);
}

wchar_t *_wfullpath(wchar_t *absPath, const wchar_t *relPath, size_t maxLength)
{
	//TODO: まだ作っていない
	//DBW("not supported _wfullpath: %s %s", absPath ? __cstr(absPath).utf8() : "(null)", __cstr(relPath).utf8());
	if (!absPath){
		absPath = (wchar_t*)malloc(_MAX_PATH*sizeof(wchar_t));
	}
	wcscpy(absPath, relPath);
	return absPath;
}

//TODO: Supported only except strings.
int _vsnwprintf(wchar_t *wcs, size_t maxlen, const wchar_t *format, va_list args)
{
	char *cs = new char[maxlen];
	int ret = vsnprintf(cs, maxlen, __cstr(format).utf8(), args);
	if (ret>0){
		UTF8toUTF32(cs, ret, wcs);
	} else {
		wcs[0] = '\0';
	}
	delete[] cs;
	return ret;
}

DWORD GetTempPath(DWORD size, LPTSTR path)
{
	_tcscpy(path, TemporaryDirectory);
	return _tcslen(path);
}
wchar_t *CharNext(const wchar_t *s)
{
    return (wchar_t*)s+1;	//TODO: サロゲートペア
}

BOOL IsDBCSLeadByte(BYTE c)
{
	return _ismbblead(c);
}

BOOL DeleteFile(const tchar *filename)
{
	return unlink(__cstr(filename).utf8()) == 0;
}
BOOL MoveFile(const tchar *src, const tchar *dst)
{
	__cstr _src(src);
	__cstr _dst(dst);
	if (rename(_src, _dst)==0)
		return TRUE;
	DBW("Failed to rename: %s %s", _src.utf8(), _dst.utf8());
	return FALSE;
}

tchar *GetString( UINT msgno )
{
	return (tchar*)_t("");	//TODO: not yet
}

// Note:
//	thread unsafe
void LoadStringArray(tnstr_vec &array, int id, const tchar *delim)
{
	array.clear();
	tchar *str = GetString(id);
	if (!str)
		return;
	tchar *p = _tcstok(str, delim);
	while (p){
		array.add(p);
		p = _tcstok(NULL, delim);
	}
}

void Hourglass( int bOn )
{

}

#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "PDJ", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "PDJ", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "PDJ", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "PDJ", __VA_ARGS__)

void DBX( int prio, const char *format, ... )
{
	va_list ap;
	va_start( ap, format );

	char buf[ 2048 ];
	vsnprintf( buf, sizeof(buf), format, ap);
	__android_log_print(prio, "PDJ", "%s", buf);

	va_end( ap );
}

void DBW( const char *format, ... )
{
	va_list ap;
	va_start( ap, format );

#ifdef __BORLANDC__
	FILE *fp = fopen("/sdcard/log.txt", "a");
	if (fp){
		char buf[ 2048 ];
		clock_t t = clock();
		snprintf(buf, sizeof(buf), "%u.%03u:", (unsigned)t/1000000, ((unsigned)t/1000)%1000);
		fwrite(buf, strlen(buf), 1, fp);
		vsnprintf( buf, sizeof(buf), format, ap);
		fwrite(buf, strlen(buf), 1, fp);
		fwrite("\n", 1, 1, fp);
		fclose(fp);
	}
#else
	char buf[ 2048 ];
	vsnprintf( buf, sizeof(buf), format, ap);
	LOGD("%s", buf);
#endif

	va_end( ap );
}

void DBe( const char *format, ... )
{
	va_list ap;
	va_start( ap, format );

	char buf[ 2048 ];
	vsnprintf( buf, sizeof(buf), format, ap);
	LOGE("%s", buf);

	va_end( ap );
}
void DBw( const char *format, ... )
{
	va_list ap;
	va_start( ap, format );

	char buf[ 2048 ];
	vsnprintf( buf, sizeof(buf), format, ap);
	LOGW("%s", buf);

	va_end( ap );
}
void DBi( const char *format, ... )
{
	va_list ap;
	va_start( ap, format );

	char buf[ 2048 ];
	vsnprintf( buf, sizeof(buf), format, ap);
	LOGI("%s", buf);

	va_end( ap );
}
