#ifndef	__COMMDLGS_H
#define	__COMMDLGS_H

#include	"comfile.h"

#ifndef __CHARRAY_H
#include	"charray.h"
#endif

// CommFileDialogで取得したマルチファイル名 str をfilesにセットする
void GetCommFiles( const tchar *str, tnstr_vec  &files, int delim='|', int nFileOffset=-1 );

#endif	// __COMMDLGS_H

