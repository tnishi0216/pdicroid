#include "stdafx.h"
#include	"bc.h"
#pragma	hdrstop
#include	"tnarray.h"

#if 0
ObjectArrayBase::ObjectArrayBase( int _maxnum )
{
	num = 0;
	maxnum = _maxnum;
	array = new void*[ maxnum ];
	for ( int i=0;i<_maxnum;i++ ){
		array[i] = NULL;
	}
}

ObjectArrayBase::~ObjectArrayBase()
{
	clear();
	delete[] array;
}

void ObjectArrayBase::clear( void )
{
	for (int i=0;i<num;i++){
		if ( array[i] ){
			delete_object( array[i] );
		}
	}
	num = 0;
}

void ObjectArrayBase::del( int i )
{
	delete_object( array[i] );
	for (int j=i;j<num-1;j++){
		array[j] = array[j+1];
	}
	num--;
}
int ObjectArrayBase::add( void *obj )
{
	if ( num >= maxnum )
		return -1;
	array[num] = obj;
	num++;
	return 0;
}
int ObjectArrayBase::insert( int i, void *obj )
{
	if ( num >= maxnum )
		return -1;
	for ( int j=num;j>i;j-- ){
		array[ j ] = array[ j-1 ];
	}
	array[ i ] = obj;
	num++;
	return 0;
}
// オブジェクトを破棄する（オブジェクトはそのまま）
void ObjectArrayBase::discard( )
{
	num = 0;
	array[0] = 0;
}
#endif

FlexArrayBase::FlexArrayBase( char *_prearray, int _objsize, int _slot_size )
{
	objsize = _objsize;
//	destruct = _destruct;
	num = 0;
	slot_size = _slot_size;
	slot_num = 1;
	prearray = _prearray;
	if (prearray){
		array = _prearray;
	} else {
		array = (char*)malloc( _objsize * _slot_size );
	}
//	array[0] = 0;
}

FlexArrayBase::~FlexArrayBase()
{
	clear();
	if (array && array!=prearray) free(array);
}

void FlexArrayBase::reserve(int size)
{
	if ( size >= slot_size * slot_num ){
		slot_num = (size + slot_size - 1) / slot_size;
		realloc();
	}
}

void FlexArrayBase::clear( )
{
	slot_num = 1;
	realloc();
	num = 0;
}

void FlexArrayBase::del( )
{
	num--;
	if ( slot_num > 1 && num < slot_size * ( slot_num - 1 ) ){
		slot_num--;
		realloc();
	}
}

int FlexArrayBase::add( )
{
	if ( num >= slot_size * slot_num ){
		slot_num++;
		if ( realloc() == -1 ){
			return -1;
		}
	}
	return 0;
}

int FlexArrayBase::realloc( void )
{
	char *_array = (char*)::realloc( array!=prearray ? array : NULL, objsize * slot_size * slot_num );
	if ( _array == 0 ){
		return -1;
	}
	if (array==prearray && prearray){
		if (num)
			memcpy(&_array[0], &prearray[0], objsize * num);
	}
	array = _array;
	return 0;
}

#if 0
void *FlexArrayBase::migrate( )
{
	if (array==prearray) return NUULL;

	void *r = array;
	num = 0;
	slot_num = 1;
	array = (char*)malloc(objsize * slot_size);
	return r;
}
#endif

#if 0
void ObjectArrayBase::del( int bi, int ei )
{
	ei++;
	int w = ei - bi;
	int n = num - ei;
	int j;
	for ( j=0;j<w;j++ ){
		delete_object( array[bi+j] );
	}
	for ( j=0;j<n;j++){
		int i = bi + j;
		array[i] = array[i+w];
	}
	num -= w;
}

void ObjectArrayBase::exchange( int i, int j )
{
	void *obj = array[i];
	array[i] = array[j];
	array[j] = obj;
}
void ObjectArrayBase::discard( int i )
{
	for (int j=i;j<num-1;j++){
		array[j] = array[j+1];
	}
	num--;
}
#endif
