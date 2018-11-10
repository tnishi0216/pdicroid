//---------------------------------------------------------------------------

#ifndef perllibH
#define perllibH
//---------------------------------------------------------------------------

#include <string>
#include <vector>
#include <map>

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
#define	foreach_string_map(obj, it) foreach_map(obj, it, string_map)
#define	foreach_int_map(obj, it) foreach_map(obj, it, int_map)

inline int get_map(int_map &m, const string &key)
	{ return m.find(key) != m.end() ? m[key] : 0; }
inline int get(int_map &m, const string &key)
	{ return m.find(key) != m.end() ? m[key] : 0; }

#endif	/* DEF_MAP_MACRO */

extern const string string_null;

// Additional string operator //
inline const char *operator *(string &s)
	{ return s.c_str(); }
inline bool operator !(string &s)
	{ return s.empty(); }

inline string get(string_vec &vec, int index)
	{ return index>=0&&index<(int)vec.size()?vec[index]:string(""); }

typedef map<wstring,wstring> wstring_map;
typedef map<wstring,int> int_wmap;
typedef vector<wstring> wstring_vec;

#define	foreach_wstring_vec(obj, it) foreach(obj, it, wstring_vec)
#define	foreach_wstring_map(obj, it) foreach_map(obj, it, wstring_map)

// perl compatible library //
string_vec *split_new(const char *delim, const char *str);
inline string_vec *split_new(const char *delim, const string &s){ return split_new(delim, s.c_str()); }
string_vec split(const char *delim, const char *str);	//Note: Œø—¦‚ªˆ«‚¢
inline string_vec split(const char *delim, const string &s){ return split(delim, s.c_str()); }
void split(const char *str, string_vec &array, const char *delim);

inline void push(string_vec &strs, const char *s){strs.push_back(s);}
inline void push(string_vec &strs, const string &s){strs.push_back(s);}
void push(string_vec &strs, string_vec *s);
void keys(string_map &smap, string_vec &svec);

// file operations //
bool IsFile(const char *file);
bool IsDirectory(const char *dir);
inline bool IsDirectory(const string &dir) { return IsDirectory(dir.c_str()); }
__int64 FileSize(const char *file);
inline int FileSize(string &file) { return FileSize(file.c_str()); }

time_t st_mtime(const char *file);
inline time_t st_mtime(string &s){ return st_mtime(s.c_str()); }
bool utime(time_t atime, time_t mtime, const string &file);
int touch(const char *file, time_t date);
void time_t_to_filetime(time_t t, FILETIME *ft);

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
void chomp(string &s);

// helper library for STL //
string sprintf(const char *format, ...);

#endif

