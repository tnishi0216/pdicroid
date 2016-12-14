#include "tnlib.h"
#pragma	hdrstop
#include "pddefs.h"
#include "pdconfig.h"
#include "PrimaryPool.h"
#include "kstr.h"

//
PrimaryPool::PrimaryPool( int preallocSize )
{
	KCodeTrans = &KCodeTranslateSetN;
	fw.allocate(preallocSize);
}
#if 0	// BSearchExactÇ∆ìØÇ∂Ç»ÇÃÇ≈ìùçá (2015.5.26)
int PrimaryPool::BSearch( const tchar *str )
{
	int left = 0;
	int right = get_num();
	if ( right == 0 ) return 0;
#if MIXDIC || defined(KMIXDIC)
	_kwstrdef( _str, str );
#else
	#define	_str	str
#endif
	do {
		int mid = ( left + right ) /2;
		int k = cmpword( _str, _kwstr(fw[mid], GetKCodeTrans()));
		if (k == 0){
			return mid;
		}
		if (k < 0){
			right = mid;
		} else {
			left = mid + 1;
		}
	} while (left < right);
	return left;
}
#endif

int PrimaryPool::BSearchExact( const tchar *str )
{
	int left = 0;
	int right = get_num();
	if ( right == 0 ) return 0;

#if MIXDIC || defined(KMIXDIC)
	_kwstrdef( _str, str );
#else
	#define	_str	str
#endif
	do {
		int mid = ( left + right ) /2;
		int k = cmpword( _str, _kwstr(fw[mid], GetKCodeTrans()));
		if (k == 0){
			return mid;
		}
		if (k < 0){
			right = mid;
		} else {
			left = mid + 1;
		}
	} while (left < right);
	return left;
}

int PrimaryPool::Find( const tchar *str )
{
	int r = BSearch( str );
	if ( r >= get_num() ) return -1;
	if ( _tcscmp( str, fw[r]) == 0 ) return r;
	return -1; 
}

WJPool::WJPool( int maxnum )
	:PrimaryPool( maxnum )
{
	fj.allocate(maxnum);
}
void WJPool::Add( const tchar *word, const tchar *japa )
{
	fw.add( word );
	fj.add( japa );
}

