#ifndef	__FILESTR_H
#define	__FILESTR_H

#define	DIR_DELIM	'\\'
#define	DIR_DELIM_S	_t("\\")

bool makename( tnstr &fullpath, const tchar *path, const tchar *filename );
bool makename2( tnstr &fullpath, const tchar *path, const tchar *filename );
void get_filename( const tchar *path, tchar *filename );
tchar *GetWildPtr( const tchar *path );
void addyen( tchar *path );
tnstr AddYen(const tchar *path);
void AddSlash(tnstrbuf &path);
tchar *GetFileName( const tchar *path );
inline tchar *GetBaseName(const tchar *path)
	{ return GetFileName(path); }
tchar *GetFileName1( const tchar *path );
// 拡張子の位置を得る
// . の次のアドレスを返す
// 見つからないと NULLを返す
// . で終わるファイルでもNULLでないので注意！
tchar *GetFileExtension( const tchar *filename );	// 拡張子の位置を得る
tchar *GetFileExt(const tchar *filename);
tchar *SkipDriveLetter(const tchar *path);
bool IsFullPath( const tchar *filename );
bool IsRootPath( const tchar *path );
bool IsSameFile( const tchar *file1, const tchar *file2 );
bool IsSameFile( tnstr_vec &name1, const tchar *name2 );
bool IsSameFileName(const tchar *file1, const tchar *file2);
bool HasFilePath(const tchar *filename);

#ifdef _Windows
void AbsToRel( tnstr &name );
void RelToAbs( tnstr &in, tnstr &out );
#else
#define	AbsToRel( name )
inline void RelToAbs( tnstr &in, tnstr &out ) { out = in; }
#endif

tnstr MakeRelativeDrive(const tchar *path1, const tchar *path2);
tnstr MakeAbsoluteDrive(const tchar *path1, const tchar *path2);
tnstr MakeRelativePath(const tchar *path1, const tchar *path2, bool inc_drive);
tnstr MakeAbsolutePath(const tchar *path1, const tchar *path2);

void ChangeFileExtension(tnstr &filename, const tchar *ext);	// obsolete
tnstr ChangeFileExt(const tchar *filename, const tchar *ext);

void ChangePath( tnstr &newfilename, const tchar *filename, const tchar *newpath );

tnstr _GetFilePath( const tchar *filename );
tnstr GetTempPath();
tnstr MakePath(const tchar *path, const tchar *filename);
bool MkDir(const tchar *dir);
bool is_dir(const tchar *path);
int ReadDir(tnstr_vec &files, const tchar *dir, const tchar *ext);

void SlashToBackSlash(tchar *name);
void BackSlashToSlash(tchar *name);

#endif	// __FILESTR_H

