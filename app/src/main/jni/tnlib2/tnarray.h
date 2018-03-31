#ifndef	__tnarray_h
#define	__tnarray_h

#include <stdlib.h>

template <class T>
class ArrayT {
protected:
	T *array;
	int num;
	int maxnum;
public:
	ArrayT( int _maxnum );
	virtual ~ArrayT();
	int add( T obj );
	void del( int i );
	void del( int bi, int ei );
	void clear( void )
	{
		num = 0;
	}
	int insert( int i, T obj );
	int get_num( void )	const {return num;}
	int GetCount( ) { return num; }
	void exchange( int inx1, int inx2 );
	int ChangeSize( int newsize );
	T &operator [](int i)	{return array[i];}
};

template <class T>
ArrayT<T>::ArrayT( int _maxnum )
{
	num = 0;
	maxnum = _maxnum;
	array = new T[ _maxnum ];
	array[0] = 0;
}

template <class T>
ArrayT<T>::~ArrayT()
{
	clear();
	delete[] array;
}

template <class T>
void ArrayT<T>::del( int i )
{
	for (;i<num-1;i++){
		array[i] = array[i+1];
	}
	num--;
}

template <class T>
void ArrayT<T>::del( int bi, int ei )
{
	ei++;
	int w = ei - bi;
	int n = num - ei;
	for (int j=0;j<n;j++){
		int i = bi + j;
		array[i] = array[i+w];
	}
	num -= w;
}


template <class T>
int ArrayT<T>::add( T obj )
{
	if ( num >= maxnum )
		return -1;
	array[num] = obj;
	num++;
	return 0;
}

template <class T>
int ArrayT<T>::insert( int i, T obj )
{
	if ( i > num || num >= maxnum )
		return -1;
	memmove( &array[i+1], &array[i], ( num-i ) * sizeof(T) );
	array[ i ] = obj;
	num++;
	return 0;
}

template <class T>
int ArrayT<T>::ChangeSize( int newsize )
{
	num = 0;
	maxnum = newsize;
	delete array;
	array = new T[ newsize ];
	if ( array == 0 ){
		return -1;
	}
	array[0] = 0;
	return 0;
}

template <class T>
void ArrayT<T>::exchange( int inx1, int inx2 )
{
	T v = array[ inx1 ];
	array[ inx1 ] = array[ inx2 ];
	array[ inx2 ] = v;
}

class FlexObjectArrayBase {
protected:
	void **array;
	void **prearray;
	int slot_size;
	int slot_num;
	int num;
	bool destruct;
	int max_num;
protected:
	int realloc( void );
	virtual void delete_object( void * ) = 0;
public:
	FlexObjectArrayBase( void **prearray, int slot_size=16, bool destruct=true );
	FlexObjectArrayBase( FlexObjectArrayBase &objs, void **prearray, int slot_size=16, bool destruct=true );
	virtual ~FlexObjectArrayBase();
protected:
	void destructor();
public:
	void set_maxnum(int _max_num)
		{ max_num = _max_num; }
	int get_maxnum() const
		{ return max_num; }
	int add( void *obj );
	void del( int i );
	void del(int bi, int ei);
	int insert( int i, void *obj );
	int replace( int i, void *obj );
	void clear( );
	int get_num( )	const {return num;}
	int size() const { return num; }
	int get_slotsize() const { return slot_size*slot_num; }
	int GetCount( ) { return num; }
	void sort( int (*comp)( void **, void ** ) );
	void discard( );
	void *discard( int i );
	void exchange( int i, int j );
	void move(int src, int dst);
protected:
	void **migrate( );
public:
	void move(FlexObjectArrayBase &o);	// move o to this
	int add(FlexObjectArrayBase &o);
	bool allocate(int size);

	// for debug
	void **__get_array( ) { return array; }
	void __set_array( void **_array ) { array = _array; }
};

template <class T>
class FlexObjectArray : public FlexObjectArrayBase {
typedef FlexObjectArrayBase super;
private:
	T *prealloc[16];
protected:
	virtual void delete_object( void *obj );
public:
	FlexObjectArray( int slot_size=16, bool destruct=true )
		:super( slot_size==16 ? (void**)prealloc : NULL, slot_size, destruct ){}
	FlexObjectArray( FlexObjectArray &objs, int slot_size=16, bool destruct=true )
		:super( objs, slot_size==16 ? (void**)prealloc : NULL, slot_size, destruct ){}
	virtual ~FlexObjectArray();
	void assign(const FlexObjectArray<T> &o);
	T **migrate( )
		{ return (T**)super::migrate( ); }
	int insert( int i, T *obj )
		{ return super::insert( i, obj ); }
	int insert( int i, const FlexObjectArray<T> &o );
	int insert_discard( int i, FlexObjectArray<T> &o );
	int replace( int i, T *obj )
		{ return super::replace( i, obj ); }
	int add( T *obj )
		{ return super::add( obj ); }
	void add(const FlexObjectArray<T> &o);
	int push( T *obj )
		{ return super::add( obj ); }
	T &operator [](int i)	const {return *(T*)array[i];}
	operator T**()	{ return (T**)array; }
	FlexObjectArray<T> &operator = (const FlexObjectArray &o)
		{ assign(o); return *this; }
	FlexObjectArray<T> &operator += (const FlexObjectArray &o)
		{ super::add(o); return *this; }
	void sort( int (*comp)( T **, T ** ) )
		{ super::sort( ( int (*)( void **, void ** ) ) comp ); }
	T *discard( int i )
		{ return (T*)super::discard( i ); }
	void discard( )
		{ super::discard( ); }
};

template <class T>
void FlexObjectArray<T>::delete_object( void *obj )
{
	if (obj)
		delete ( T* )obj;
}

template <class T>
FlexObjectArray<T>::~FlexObjectArray()
{
	destructor();
}

template <class T>
void FlexObjectArray<T>::assign(const FlexObjectArray<T> &o)
{
	clear();
	add(o);
}

template <class T>
int FlexObjectArray<T>::insert( int i, const FlexObjectArray<T> &o )
{
	if (o.size() == 0)
		return 0;

	if ( i > num )
		return -1;

	if (max_num!=-1 && num + o.size() > max_num)
		return -1;

	allocate( num + o.size() );

	if (num-i>0)
		memmove( &array[i+o.size()], &array[i], ( num-i ) * sizeof(void *) );

	for (int j=0;j<o.size();j++){
		array[ i + j ] = new T(o[j]);
	}
	num += o.size();
	return 0;
}

// o.discard()してinsert
template <class T>
int FlexObjectArray<T>::insert_discard( int i, FlexObjectArray<T> &o )
{
	if (o.size() == 0)
		return 0;

	if ( i > num )
		return -1;

	if (max_num!=-1 && num + o.size() > max_num)
		return -1;

	allocate( num + o.size() );

	if (num-i>0)
		memmove( &array[i+o.size()], &array[i], ( num-i ) * sizeof(void *) );

	for (int j=0;j<o.size();j++){
		array[ i + j ] = &o[j];
	}
	num += o.size();
	o.discard();
	return 0;
}

template <class T>
void FlexObjectArray<T>::add( const FlexObjectArray<T> &o )
{
	for (int i=0;i<o.size();i++){
		add(new T(o[i]));
	}
}

template<class T>
class SortedFlexObjectArray : public FlexObjectArrayBase {
private:
	T *prealloc[16];
protected:
	virtual void delete_object( void *obj );
public:
	int (*compfunc)( const void *, const void * );	// 比較関数アドレス
protected:
public:
	SortedFlexObjectArray( int (*_compfunc)( const void *, const void * ), int slot_size=16, int destruct=1 );
	virtual ~SortedFlexObjectArray();
	int search( T *keyobj );
	int add( T *obj );				// 返り値は、追加された場所のインデックス番号
	T &operator [](int i) const	{return *(T*)array[i];}
	operator T**() const	{ return (T**)array; }
};

template< class T >
SortedFlexObjectArray<T>::SortedFlexObjectArray( int (*_compfunc)( const void *, const void * ), int slot_size, int destruct )
	:FlexObjectArrayBase( slot_size==16 ? (void**)prealloc : NULL, slot_size, destruct )
	,compfunc( _compfunc )
{
}

template <class T>
SortedFlexObjectArray<T>::~SortedFlexObjectArray()
{
	destructor();
}

template <class T>
void SortedFlexObjectArray<T>::delete_object( void *obj )
{
	delete ( T* )obj;
}

// 挿入点を探す
void *_bsearch( const void *key, const void *base, size_t nelem, size_t width, int (* fcmp)(const void *, const void *));

template< class T >
int SortedFlexObjectArray<T>::search( T *keyobj )
{
	T** o = (T**)_bsearch( &keyobj, array, num, sizeof( T* ), compfunc );
	return ( (unsigned int)(o) - (unsigned int)(array) ) / sizeof( T* );
}

template< class T >
int SortedFlexObjectArray<T>::add( T *o )
{
	if ( !num ){
		if ( insert( 0, o ) == -1 )
			return -1;
		return 0;
	}
	int i = search( o );
	if ( insert( i, o ) == -1 )
		return -1;
	return i;
}

class FlexArrayBase {
protected:
	char *array;
	char *prearray;
	int slot_size;
	int slot_num;
	int num;
//	int destruct;
	int objsize;
protected:
	int realloc( );
	int add( );
	void del( );
public:
	FlexArrayBase( char *prearray, int _objsize, int _slot_size=16 );
	virtual ~FlexArrayBase();
	void reserve(int size);
	void clear( );
	//void *_migrate( );
	int get_num( )	const {return num;}
	int size() const { return num; }
	int GetCount( ) { return num; }
};

template <class T>
class FlexArray : public FlexArrayBase {
private:
	T prealloc[16];
public:
	FlexArray( int slot_size=16 )
		:FlexArrayBase( (char*)prealloc, sizeof( T ), slot_size )
	{
		*((T*)array) = 0;
	}
	int add( T obj );
	int insert( int i, T obj );
	void del( int i );
	int replace( int i, T obj );
	void sort( int (*comp)( T *, T * ) );
	int search( T obj );
	void assign(const FlexArray<T> &o);
#if 0
	T *_migrate( )
		{ return (T*)FlexArrayBase::migrate( ); }
#endif
	T *get_array( void ) const
		{ return (T*)array; }
	T &operator [](int i)	const {return ((T*)array)[i];}
	FlexArray<T> &operator = (const FlexArray<T> &o)
		{ assign(o); return *this; }
};

template <class T>
void FlexArray<T>::del( int i )
{
#if 0
	for (int j=i;j<num-1;j++){
		((T*)array)[j] = ((T*)array)[j+1];
	}
#else
	memmove( &((T*)array)[i], &((T*)array)[i+1], (num-i-1)*sizeof(T) );
#endif
	FlexArrayBase::del( );
}

template <class T>
int FlexArray<T>::add( T obj )
{
	if ( FlexArrayBase::add( ) == -1 ){
		return -1;
	}
	((T*)array)[num++] = obj;
	return 0;
}

template <class T>
int FlexArray<T>::insert( int i, T obj )
{
	if ( i > num ){
		return -1;
	}
	if ( FlexArrayBase::add( ) == -1 ){
		return -1;
	}
	memmove( &((T*)array)[i+1], &((T*)array)[i], ( num-i ) * sizeof( T ) );
	((T*)array)[ i ] = obj;
	num++;
	return 0;
}

template <class T>
int FlexArray<T>::replace( int i, T obj )
{
	if ( i >= num ){
		return -1;
	}
	((T*)array)[i] = obj;
	return 0;
}

template <class T>
void FlexArray<T>::sort( int (*comp)( T *, T * ) )
{
	::qsort( array, num, sizeof( T ), 
		(int(*)(const void *, const void *))comp );
}

template <class T>
int FlexArray<T>::search( T obj )
{
	for ( int i=0;i<num;i++ ){
		if ( ((T*)array)[i] == obj )
			return i;
	}
	return -1;
}

template <class T>
void FlexArray<T>::assign(const FlexArray<T> &o)
{
	reserve(o.size());
	memcpy(get_array(), o.get_array(), o.size()*sizeof(T));
	num = o.size();
}

#endif	// __tnarray_h
