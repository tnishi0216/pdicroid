#include "stdafx.h"
#include	"bc.h"
#include	"tnarray.h"

#define	INIT_ALLOC	1

FlexObjectArrayBase::FlexObjectArrayBase( void **_prearray, int _slot_size, bool _destruct )
{
	destruct = _destruct;
	num = 0;
	slot_size = _slot_size>0 ? _slot_size : 16;
	prearray = _prearray;
	slot_num = 1;
	if (prearray){
		array = prearray;
	} else {
#if INIT_ALLOC
		array = (void**)malloc(sizeof(void*) * slot_num * slot_size);
#else
		slot_num = 0;
		array = NULL;
#endif
	}
	max_num = -1;
}

FlexObjectArrayBase::FlexObjectArrayBase( FlexObjectArrayBase &objs, void **_prearray, int _slot_size, bool _destruct )
{
	destruct = _destruct;
	num = 0;
	slot_size = _slot_size;
	prearray = _prearray;
	slot_num = 1;
	if (prearray){
		array = prearray;
	} else {
#if INIT_ALLOC
		array = (void**)malloc(sizeof(void*) * slot_num * slot_size);
#else
		slot_num = 0;
		array = NULL;
#endif
	}

	for ( int i=0;i<objs.get_num();i++ ){
		add( objs.array[i] );
	}
}

FlexObjectArrayBase::~FlexObjectArrayBase()
{
	destructor();
}

void FlexObjectArrayBase::destructor()
{
	if ( array ){
		clear();
		if (prearray!=array) free( array );
		array = NULL;
	}
}
void FlexObjectArrayBase::clear( )
{
	if ( destruct ){
		for (int i=0;i<num;i++){
			if ( array[i] ){
				delete_object( array[i] );
			}
		}
	}
	slot_num = 1;
	realloc();
	num = 0;
}

int FlexObjectArrayBase::realloc( void )
{
	if (slot_num==0){
		if (array && array!=prearray){ free(array); array = NULL; }
		return 0;
	}
	void **_array = (void**)::realloc( array!=prearray ? array : NULL, sizeof( void* ) * slot_size * slot_num );
	if ( _array == 0 ){
		return -1;
	}
	if (array==prearray && prearray){
		if (num)
			memcpy(&_array[0], &prearray[0], sizeof(void*) * num);
	}
	array = _array;
	return 0;
}

int FlexObjectArrayBase::add( void *obj )
{
	if (max_num!=-1 && num>=max_num)
		return -1;
	if ( num >= slot_size * slot_num ){
		slot_num++;
		if ( realloc() == -1 ){
			return -1;
		}
	}
	array[num++] = obj;
	return 0;
}

void FlexObjectArrayBase::del( int i )
{
	if ( destruct ){
		delete_object( array[i] );
	}
	if (num-i-1>0)
		memmove(&array[i], &array[i+1], (num-i-1)*sizeof(void*));
	num--;
#if 0
	if ( slot_num > 1 && num < slot_size * ( slot_num - 1 ) ){
		slot_num--;
		realloc();
	}
#endif
}
void FlexObjectArrayBase::del(int bi, int ei)
{
	ei++;
	
	if ( destruct ){
		for (int i=bi;i<ei;i++){
			delete_object( array[i] );
		}
	}

	if (num-ei>0)
		memmove(&array[bi], &array[ei], (num-ei)*sizeof(void*));
	num -= ei-bi;
}
int FlexObjectArrayBase::insert( int i, void *obj )
{
	if ( i > num ){
		return -1;
	}
	if (max_num!=-1 && num>=max_num)
		return -1;

	if ( num >= slot_size * slot_num ){
		slot_num++;
		if ( realloc() == -1 ){
			return -1;
		}
	}
	if (num-i>0)
		memmove( &array[i+1], &array[i], ( num-i ) * sizeof( void *) );
	array[ i ] = obj;
	num++;
	return 0;
}
int FlexObjectArrayBase::replace( int i, void *obj )
{
	if ( i >= num ){
		return -1;
	}
	if ( destruct )
		delete_object( array[i] );
	array[i] = obj;
	return 0;
}
void FlexObjectArrayBase::exchange( int i, int j )
{
	void *obj = array[i];
	array[i] = array[j];
	array[j] = obj;
}

void FlexObjectArrayBase::move(int src, int dst)
{
	if (src<dst){
		if (src<0 || dst>=get_num())
			return;

		if (src+1==dst){
			exchange(src, src+1);
		} else {
			void *srcobj = array[src];
			if (dst-src>0)
				memmove(&array[src], &array[src+1], (dst-src)*sizeof(void*));
			array[dst] = srcobj;
		}
	} else
	if (src>dst){
		if (src>=get_num() || dst<0)
			return;

		if (src-1==dst){
			exchange(src, dst);
		} else {
			void *srcobj = array[src];
			if (src-dst>0)
				memmove(&array[dst+1], &array[dst], (src-dst)*sizeof(void*));
			array[dst] = srcobj;
		}
	} else {
		// no operation
	}
}

void **FlexObjectArrayBase::migrate( )
{
	if (array==prearray) return NULL;

	void **r = array;
	num = 0;
	slot_num = 0;
	array = NULL;
	return r;
}

// move o to this
void FlexObjectArrayBase::move(FlexObjectArrayBase &o)
{
	num = o.num;
	slot_num = o.slot_num;
	int sz = o.slot_size;
	destruct = o.destruct;
	if (array && array!=prearray) free(array);
	array = o.migrate();
	if (!array){
		if (num>0){
			if (prearray && slot_size==sz && slot_num==1){
				array = prearray;
			} else {
				array = (void**)malloc( slot_size * slot_num * sizeof(void*) );
			}
			memcpy(&array[0], &o.prearray[0], num*sizeof(void*));
		} else {
			slot_num = 0;
		}
	}
	slot_size = sz;
}

// max_num‚ð’´‚¦‚éê‡‚Ímax_num“à‚ÉŽû‚Ü‚é‚æ‚¤‚É’Ç‰Á
// o‚É‚ ‚éobject‚Ídiscard‚³‚ê‚é(clone‚Å‚Í‚È‚­move)
// return: ’Ç‰Á‚Å‚«‚½—v‘f”, -1=error
// “®ì–¢ŒŸØ at 2016.8.5
int FlexObjectArrayBase::add(FlexObjectArrayBase &o)
{
	int add_num = o.num;
	if (max_num!=-1 && num+add_num>max_num){
		add_num = max_num - num;
	}

	if (!allocate(num + add_num)) return -1;

#if 1
	memcpy(&array[num], &o.array[0], add_num * sizeof(void*));
#else
	for (int i=0;i<add_num;i++){
		array[num++] = o[i];
	}
#endif

	o.num = 0;

	return add_num;
}

bool FlexObjectArrayBase::allocate(int size)
{
	if ( size <= slot_size * slot_num ){
		return true;	// Not changed (enough space);
	}
	slot_num = (size+slot_size-1) / slot_size;
	if (realloc()!=0){
		return false;	// error (not enough memory)
	}
	return true;	// reallocated
}

void FlexObjectArrayBase::discard( )
{
	bool _destruct = destruct;
	destruct = false;
	clear();
	destruct = _destruct;
}

void *FlexObjectArrayBase::discard( int i )
{
	bool _destruct = destruct;
	destruct = false;
	void *obj = array[i];
	del( i );
	destruct = _destruct;
	return obj;
}

#if defined(__ANDROID__) && !defined(__BORLANDC__)
#define	__cdecl
#endif

void FlexObjectArrayBase::sort( int (*comp)( void **, void ** ) )
{
	::qsort( array, num, sizeof( void * ), 
		(int(__cdecl *)(const void *, const void *))comp );
}

void *_bsearch( const void *key, const void *base, size_t nelem,
size_t width, int (* fcmp)(const void *, const void *))
{
  char  *kmin, *probe;
  int i, j;

  kmin = (char *) base;
  while (nelem > 0){
    i = nelem >> 1;
    probe = kmin + i * width;
    j = (*fcmp)(key, probe);
    if (j == 0)
      return(probe);
    else if (j < 0)
      nelem = i;
    else  {
      kmin = probe + width;
      nelem = nelem - i - 1;
    }
  }
  return kmin;
}

