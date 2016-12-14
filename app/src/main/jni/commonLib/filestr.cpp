#include	"tnlib.h"
#pragma	hdrstop
#include <stdlib.h>
#include <dirent.h>
#include	"defs.h"
#include	"filestr.h"

#ifdef UNIX
#define	DELIM_S	_t("/")
#else
#define	DELIM_S	_t("\\")
#endif

inline bool is_delim(const tchar c)
{
	return c=='\\' || c=='/';
}

// return:
//	0     : no need to add slash
//	delim : need to add slash
tchar FindLastSlash(const tchar *path)
{
	if (path[0] == '\0')
		return 0;
	static tchar delim[2] = _T("\\");
	static const tchar delim_alt[2] = _T("/");

	delim[0] = '\\';

	const tchar *p = _tcsrchr(path, delim[0]);
	if ( !p ){
		p = _tcsrchr( path, delim_alt[0] );
		delim[0] = delim_alt[0];
	}
	if (p){
		if (p[1]){
			return delim[0];
		}
	} else {
		int len = _tcslen(path);
		if (*(path+len-1) != ':'){
			return delim[0];
		}
	}
	return 0;
}

//pathの最後に'\'を追加
void addyen(tchar *path)
{
	tchar delim = FindLastSlash(path);
	if (!delim)
		return;

	tchar *p = path+_tcslen(path);
	p[0] = delim;
	p[1] = '\0';
}

tnstr AddYen(const tchar *path)
{
	tchar delim = FindLastSlash(path);
	if (!delim)
		return tnstr(path);

	tchar buf[2];
	buf[0] = delim;
	buf[1] = '\0';

	tnstr ret(path);
	ret += buf;
	return ret;
}

void AddSlash(tnstrbuf &path)
{
	tchar delim = FindLastSlash(path);
	if (!delim)
		return;

	tchar buf[2];
	buf[0] = delim;
	buf[1] = '\0';
	path += buf;
}

bool IsLastSlash(const tchar *path)
{
	while (*path) path++;
	return is_delim(path[-1]);
}

// pathからファイル名の先頭のアドレスを得る
tchar *GetFileName( const tchar *path )
{
	if (!path)
		return NULL;

	const tchar *p;
	const tchar *delim1 = NULL;
	const tchar *delim2 = NULL;

	for ( p=path; *p ; ){
		tchar c = *p;
		if ( is_delim(c) ){
			delim1 = ++p;
		} else if ( c == ':' ){
			delim2 = ++p;
		} else {
			NEXT_P( p );
		}
	}

	return (tchar*)( delim1 ? delim1 : ( delim2 ? delim2 : path ) );
}
// pathからファイル名より１つ上のパスを含めた先頭のアドレスを得る
tchar *GetFileName1( const tchar *path )
{
	const tchar *p;
	const tchar *delim1 = NULL;
	const tchar *delim2 = NULL;
	const tchar *delim3 = NULL;

	for ( p=path; *p ; ){
		tchar c = *p;
		if ( is_delim(c) ){
			delim3 = delim1;
			delim1 = ++p;
		} else if ( c == ':' ){
			delim2 = ++p;
		} else {
			NEXT_P( p );
		}
	}

	return (tchar*)( delim3 ? delim3 : (delim1 ? delim1 : ( delim2 ? delim2 : path )) );
}
// 拡張子の位置を得る
// . の次のアドレスを返す
// 見つからないと NULLを返す
// . で終わるファイルはNULLではなくfilenameのnull終端のアドレスを返す
tchar *GetFileExtension( const tchar *filename )
{
	const tchar *ext = NULL;
	while ( *filename ){
		if ( *filename == '.' ){
			ext = filename + 1;
		} else if ( *filename == '\\' ){
			ext = NULL;
		}
		NEXT_P( filename );
	}
	return (tchar*)ext;
}
// x: や \\xxx をskipする
// (\で始まるpathと連結させるため(など))
// drive letterが存在しない場合はpathそのもの
tchar *SkipDriveLetter(const tchar *path)
{
	if (path[1]==':')
		return (tchar*)path+2;
	tchar c = path[0];
	if ( is_delim(c) && c==path[1]){
		// \\xxx
		path += 2;
		while ( 1 ){
			c = *path;
			if (!c){
				return (tchar*)path;
			}
			if ( is_delim(c) ){
				return (tchar*)path-1;	// next to \\xxx
			}
			path++;
		}
	}
	// no drive letter
	return (tchar*)path;
}
bool IsFullPath( const tchar *filename )
{
#ifdef UNIX
	if ( filename[0] == '/' ){
		return true;
	}
#endif
#ifdef _Windows
	if ( GetFileName( filename ) != filename ){
		if ( filename[1] == ':' || (filename[0] == '\\' && filename[1] == '\\' ) )
		{
			return true;
		}
	}
#endif
	return false;
}

// \で始まるpathか？
bool IsRootPath( const tchar *path )
{
	tchar c = *path;
	if ( is_delim(c) ){
		if (c!=path[1]){
			// \\ではない
			return true;
		}
	}
	return false;
}

#if !defined(DOS)
//pathからファイル名をxxxxxxxx.xxx形式で得る
void get_filename( const tchar *path, tchar *filename )
{
#if defined(_Windows) || defined(wsnprintf)
	wsnprintf( filename, _MAX_PATH, _T("%-12s"), (LPCTSTR)GetFileName( path ) );
#else
	sprintf( filename, _T("%-12s"), (LPCTSTR)GetFileName( path ) );
#endif
}
#else
//pathからファイル名をxxxxxxxx.xxx形式で得る
void get_filename( const tchar *path, tchar *filename )
{
	const tchar *p = GetFileName( path );
	int i;
	for ( i=0;i<8;i++ ){
		if ( *p == '.' || *p == '\0' ){
			break;
		}
#ifndef _UNICODE
		if ( _ismbblead( *p ) ){
			if ( i == 7 ){
				while ( *p ){
					if ( *p == '.' )
						break;
					if ( _ismbblead( *p ) )
						p++;
					p++;
				}
				break;
			}
			*filename++ = *p++;
			i++;
		}
#endif
		*filename++ = *p++;
	}
	for ( ;i<8;i++ ){
		*filename++ = ' ';
	}
	i = 0;
	if ( *p == '.' ){
		*filename++ = '.';
		p++;
		for ( ;i<3;i++ ){
			if ( *p == '\0' )
				break;
#ifndef _UNICODE
			if ( _ismbblead( *p ) ){
				if ( i == 2 ){
					break;
				}
				*filename++ = *p++;
				i++;
			}
#endif
			*filename++ = *p++;
		}
	}
	for ( ;i<3;i++ ){
		*filename++ = ' ';
	}
	*filename = '\0';
}
#endif

// 最大maxlen文字までのファイル名を作成する
//	maxlen > 20 以上であること
//	返り値は作成されたファイル名
const tchar *MakeShortFilename( const tchar *name, int maxlen )
{
	static tchar *buf = NULL;
	if ( buf )
		delete[] buf;

	if ( _tcslen( name ) <= (unsigned)maxlen ){
		buf = newstr( name );
		return buf;
	}

	buf = new tchar[ maxlen + 1 ];

	tchar *dp = buf;
	const tchar *sp = name;
	if ( name[1] == ':' ){
		// ドライブ名がある場合
		*dp++ = *sp++;
		*dp++ = *sp++;
		maxlen -= 2;
	}
	const tchar *base = GetBaseName( name );
	int baselen = _tcslen( base );
	maxlen -= 3;	// ..\<basename>の..\の長さの分
	int _maxlen = maxlen - baselen;
	if ( _maxlen < 0 ) _maxlen = 0;
	while ( sp < base ){
		const tchar *yp = sp;
		while ( *yp ){
			if ( *yp == '\\' ){
				break;
			}
			yp = GetNextPtr( yp );
		}
		if ( *yp ){
			yp++;
			int l = STR_DIFF( yp, sp );
			if ( l < _maxlen ){
				_tcsncpy( dp, sp, l );
				dp += l;
				_maxlen -= l;
				sp = yp;
			} else {
				break;
			}
		} else {
			// 短縮不可能
			_tcscpy( buf, _T("?") );
			return buf;
		}
	}
	_tcscpy( dp, _T("..\\") );
	dp += 3;
#ifdef WIN32
	if ( maxlen < baselen ){
		memcpy( dp, base, maxlen-3 );
		dp[maxlen-3] = '.';
		dp[maxlen-2] = '.';
		dp[maxlen-1] = '.';
		dp[maxlen] = '\0';
	} else
#endif
	{
		_tcscpy( dp, base );
	}
	return buf;
}

// tfile1 と tfile2 の比較
//	もし、文字列がなかったり、不正なファイル名だとfalseを返す。
//	完全に一致した場合のみtrueを返す
//Note:
//	パスがないとカレントディレクトを付加して比較するので要注意！
//	なるべくIsSameFileName()を使用すること！！！
bool IsSameFile( const tchar *name1, const tchar *name2 )
{
	tnstr n1;
	tnstr n2;
	makename( n1, name1, NULL );
	makename( n2, name2, NULL );
	if ( _tmbsicmp( n1, n2 ) == 0 ){
		return true;
	}
	return false;
}

// nameとname2を比較
// １つでも同じファイルがあると、trueを返す
bool IsSameFile( tnstr_vec &name1, const tchar *name2 )
{
	for ( int i=0;i<name1.get_num();i++ ){
		if ( IsSameFile( name1[i], name2 ) )
			return true;
	}
	return false;
}

bool IsSameFileName(const tchar *file1, const tchar *file2)
{
	return _tmbsicmp(file1, file2) == 0;
}

bool HasFilePath(const tchar *filename)
{
	return filename != GetFileName(filename);
}

// path1をpath2から見たrelative driveに変更
tnstr MakeRelativeDrive(const tchar *path1, const tchar *path2)
{
	const tchar *drive_end = _tcsstr(path2, _t(":"));
	if (!drive_end){
		// no drive??
		// use as is.
		return path1;
	}
	drive_end++;
	int len = STR_DIFF(drive_end, path2);
	if (_tcsnicmp(path1, path2, len)){
		// not same drive.
		return path1;
	}
	// same drive
	return tnstr(path1+len);
}

// [path1のdrive]+path2を作成
// path2の先頭に\が無い場合は追加する
// path2にすでにdriveがある場合はpath2をそのまま返す
// path1にdriveが無い場合はpath2をそのまま返す(要検討)
tnstr MakeAbsoluteDrive(const tchar *path1, const tchar *path2)
{
	const tchar *drive_end = _tcsstr(path2, _t(":"));
	if (drive_end){
		// drive includes
		return path2;
	}
	drive_end = _tcsstr(path1, _t(":"));
	if (!drive_end){
		// no drive
		return path2;
	}
	drive_end++;
	int len = STR_DIFF(drive_end, path1);
	tnstr ret(path1, len);
	if ( !is_delim(path2[0]) ){
		ret += DIR_DELIM_S;
	}
	return ret+path2;
}

// path1をpath2から見たrelative pathに変更
// path2と完全に一致する部分がpath1にある場合はpath1-path2を返す。
// それ以外は、
// inc_drive=true:
//	return MakeRelativePath + MakeRelativeDrive
// inc_drive=false:
//	return NULL
tnstr MakeRelativePath(const tchar *path1, const tchar *path2, bool inc_drive)
{
	const tchar *p1 = path1;
	const tchar *p2 = path2;
	static const tchar *delims = _t(":\\/");
	for (;*p1&&*p2;){
		size_t s1 = _tcscspn(p1, delims);
		const tchar *q1 = p1+s1;
		size_t s2 = _tcscspn(p2, delims);
		const tchar *q2 = p2+s2;
		int len1 = STR_DIFF(q1, p1);
		int len2 = STR_DIFF(q2, p2);
		if (len1!=len2)
			break;	// not matched length
		if (_tcsnicmp(p1, p2, len1)){
			// not same
			break;
		}
		p1 = q1;
		p2 = q2;
		if (!*p2){
			break;	// partly matched. (path1 + ... = path2)
		}
		p1++;
		p2++;
	}

	if (!*p2){	// complete match
		while (is_delim(*p1)) p1++;	// skip delimitor not to be a root directory.
		return tnstr(p1);
	}

	if (!inc_drive)
		return NULL;
	return MakeRelativeDrive(path1, path2);
}

tnstr MakeAbsolutePath(const tchar *path1, const tchar *path2)
{
	if (is_delim(path2[0]) && !is_delim(path2[1])){
		// only root directory
		return MakeAbsoluteDrive(path1, path2);
	}
	if (IsFullPath(path2)){
		return path2;
	}
	tnstr ret(AddYen(path1));
	return ret + path2;
}

// obsolete
// filenameの拡張子をextに変更する
void ChangeFileExtension( tnstr &filename, const tchar *ext )
{
	tchar *p = GetFileExtension( filename );
	if ( p ){
		p[0] = '\0';
		filename.cat( ext );
	} else {
		filename.cat( _T(".") );
		filename.cat( ext );
	}
}

// filenameの拡張子をextに変更する
tnstr ChangeFileExt( const tchar *filename, const tchar *ext )
{
	tnstr ret(filename);
	tchar *p = GetFileExtension( ret );
	if ( p ){
		p[0] = '\0';
		if (ext && ext[0]){
			ret += ext;
		} else {
			p[-1] = '\0';
		}
	} else {
		if (ext && ext[0]){
			ret += _t(".");
			ret += ext;
		}
	}
	return ret;
}

// filenameのpathをnewpathのpathに変更する
// newpathは、ファイル名が含まれていてもOK.
// newpathにファイル名が無い場合、最後に \ が必要
// newpathがファイル名のみのときは変更されない
void ChangePath( tnstr &newfilename, const tchar *filename, const tchar *newpath )
{
	const tchar *fn = GetFileName( newpath );
	if ( fn == newpath )
		return;
	newfilename.set( newpath, STR_DIFF(fn,newpath) );
	newfilename.cat( GetFileName( filename ) );
}

// 最後の \ を付加したpathを返す
// ファイルにpathが無い場合は、""を返す(\はつかない)
tnstr _GetFilePath( const tchar *filename )
{
	const tchar *lp = GetFileName( filename );
	return tnstr(filename, STR_DIFF(lp,filename));
}

tnstr GetTempPath()
{
	TCHAR buf[MAX_PATH+1];
	buf[0] = '\0';
	GetTempPath(MAX_PATH, buf);
	return tnstr(buf);
}

// pathとfilenameを結合する
// pathの最後に'\'が無い場合は付加する
// filenameにpathが存在する場合はfilenameをそのまま返す 2008.4.14
tnstr MakePath(const tchar *path, const tchar *filename)
{
	if (!path || !path[0] || filename!=GetFileName(filename)){
		return tnstr(filename);
	}
	const tchar *p = path;
	const tchar *last = NULL;
	for (;*p;){
		if ( is_delim(*p) ){
			last = p;
		} else {
			last = NULL;
		}
		p = CharNext(p);
	}
	if (!last){
		return tnstr(path) + DELIM_S + filename;
	}
	return tnstr(path) + filename;
}

bool MkDir(const tchar *dir)
{
	const tchar *p = dir;
	for (;;){
		if (*p=='/' || *p=='\\' || !*p){
			// separator
			tnstr path(dir, STR_DIFF(p,dir));
			if (!fexist(path)){
				if (_tmkdir(path)!=0){
					return false;	// error
				}
			}
			if (!*p)
				break;
		}
		p = CharNext(p);
	}
	return true;
}

int ReadDir(tnstr_vec &files, const tchar *dir, const tchar *ext)
{
#ifdef UNIX
	__cstr sdir(dir);
	DIR *d = opendir( sdir );
	if (!d)
		return 0;
	struct dirent *ent;
	while ((ent = readdir(d)) != NULL){
		tnstr path(AddYen(dir));
#ifdef __UTF8
		const tchar *dname = ent->d_name;
#else
		tnstr dname; dname.setUTF8(ent->d_name);
#endif
		path += dname;
		if (is_dir(path)){
			// directory
			continue;
		}
		if (ext){
			tchar *cext = GetFileExtension(dname);
			if (!cext || !IsSameFileName(ext,cext)){
				continue;
			}
		}
		files.push(dname);
	}
	closedir( d );
	return files.size();
#else
	_tDIR *d = _topendir( dir );
	if (!d)
		return 0;
	struct _tdirent *ent;
	while ((ent = _treaddir(d)) != NULL){
		tnstr path(AddYen(dir));
		path += ent->d_name;
		if (is_dir(path)){
			// directory
			continue;
		}
		if (ext){
			tchar *cext = GetFileExtension(ent->d_name);
			if (!cext || !IsSameFileName(ext,cext)){
				continue;
			}
		}
		files.push(ent->d_name);
	}
	_tclosedir( d );
	return files.size();
#endif
}

void SlashToBackSlash(tchar *name)
{
	while (*name){
		if (*name == '/'){
			*name = '\\';
		}
		name++;
	}
}
void BackSlashToSlash(tchar *name)
{
	while (*name){
		if (*name == '\\'){
			*name = '/';
		}
		name++;
	}
}
