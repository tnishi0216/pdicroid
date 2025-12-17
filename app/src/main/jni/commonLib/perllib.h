//---------------------------------------------------------------------------

#ifndef perllibH
#define perllibH
//---------------------------------------------------------------------------

#include <string>
#include <vector>
#include <map>
#include <time.h>

using namespace std;

#ifndef DEF_MAP_MACRO
#define	DEF_MAP_MACRO
typedef map<string,string> string_map;
typedef map<string,int> int_map;
typedef vector<string> string_vec;
typedef vector<int> int_vec;

#define	foreach(obj, it, type) \
	for (type::iterator it=(obj).begin();it!=(obj).end();it++)
#define	foreach_string_vec(obj, it) foreach(obj, it, string_vec)
#define	foreach_map(obj, it, type) \
	for (type::iterator it=(obj).begin();it!=(obj).end();it++)
#define	foreach_map_const(obj, it, type) \
	for (type::const_iterator it=(obj).begin();it!=(obj).end();it++)
#define	foreach_string_map(obj, it) foreach_map(obj, it, string_map)
#define	foreach_int_map(obj, it) foreach_map(obj, it, int_map)

inline int get_map(int_map &m, const string &key)
	{ return m.find(key) != m.end() ? m[key] : 0; }

#endif

#ifdef UNICODE
#define	tstring			wstring
#define	tstring_map		wstring_map
#define	tstring_vec		wstring_vec
#define	int_tmap		int_wmap
#define	foreach_tstring_map	foreach_wstring_map
#define	foreach_tstring_vec	foreach_wstring_vec
#else
#define	tstring_map		string_map
#define	int_tmap		int_map
#endif

typedef map<wstring,wstring> wstring_map;
typedef map<wstring,int> int_wmap;
typedef vector<wstring> wstring_vec;

#define	foreach_wstring_vec(obj, it) foreach(obj, it, wstring_vec)
#define	foreach_wstring_map(obj, it) foreach_map(obj, it, wstring_map)

// perl compatible library //
tstring_vec *split(const TCHAR *delim, const TCHAR *str);
inline tstring_vec *split(const TCHAR *delim, const tstring &s){ return split(delim, s.c_str()); }
void split(const TCHAR *str, tstring_vec &array, const TCHAR *delim);

void push(string_vec &strs, string_vec *s);
void keys(string_map &smap, string_vec &svec);

// Avoid conflict with sys/stat.h macro 'st_mtime' which some platforms define
#if defined(st_mtime)
#pragma push_macro("st_mtime")
#undef st_mtime
#define _PERLLIB_HAS_PUSHED_ST_MTIME
#endif

// file operations //
bool IsFile(const char *file);
bool IsDirectory(const char *dir);
inline bool IsDirectory(const string &dir) { return IsDirectory(dir.c_str()); }
int FileSize(const char *file);
inline int FileSize(string &file) { return FileSize(file.c_str()); }

// time_t st_mtime(const char *file);
// inline time_t st_mtime(string &s){ return st_mtime(s.c_str()); }

#ifdef _PERLLIB_HAS_PUSHED_ST_MTIME
#pragma pop_macro("st_mtime")
#undef _PERLLIB_HAS_PUSHED_ST_MTIME
#endif

bool utime(time_t atime, time_t mtime, const string &file);
int touch(const char *file, time_t date);
#ifdef _Windows
void time_t_to_filetime(time_t t, FILETIME *ft);
#endif

int compare(const char *file1, const char *file2);
inline int compare(const string &file1, const string &file2) { return compare(file1.c_str(), file2.c_str()); }

// time //
time_t timelocal(string &date);

// string library //
void trim(string &s);
void erase(string &s, const char *delim);
void trimleft(string &s);
void trimright(string &s);
void replace(string &s, const char *s1, const char *s2);
inline void replace(string &s, const char *s1, const string &s2) { replace(s, s1, s2.c_str()); }
string itoa(int integer);
void erase(string &s, const char *delim);

#endif
