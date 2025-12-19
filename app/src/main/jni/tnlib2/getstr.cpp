#include	<windows.h>
#include	"tndefs.h"
#pragma	hdrstop
#include	"tnlib.h"

#define	MAX_SIMGETSTRING	4	//	GetString()を同時に呼んで良い回数(2^)

template <class T>
class TStrArrayHolder {
protected:
	T **array;
	int size;
public:
	TStrArrayHolder(T **_array, int _size)
		:array(_array)
		,size(_size)
	{
	}
	~TStrArrayHolder();
};
template <class T>
TStrArrayHolder<T>::~TStrArrayHolder()
{
	for ( int i=0;i<size;i++){
		if (array[i]){
			delete[] array[i];
			array[i] = NULL;
		}
	}
}

static tchar *StrArray[ MAX_SIMGETSTRING ] = { NULL, NULL, NULL, NULL };
static TStrArrayHolder<tchar> StrArrayHolder(StrArray, MAX_SIMGETSTRING);
static int StrN = 0;
static tchar StrBuffer[ 256 ];	// 読取りバッファ

// 同時に複数回読んでも良い
tchar *GetString( UINT msgno )
{
	if (msgno==0)
		return _t("");

	StrN &= 3;
	delete[] StrArray[ StrN ];
	if ( !LoadString( hTNInstance, msgno, StrBuffer, 256 ) ){
		return StrArray[ StrN ] = NULL;
	}
	return StrArray[ StrN++ ] = newstr( StrBuffer );
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
