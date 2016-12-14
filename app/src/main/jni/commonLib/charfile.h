#ifndef __CHARFILE_H
#define	__CHARFILE_H

// ñ¢äÆê¨
class TCharFile : public TIFile {
public:
	TCharFile(){}
	virtual int open( const tchar *filename ){ return -1; }	// filename is const tchar pointer
	virtual void close(){}
protected:
	virtual int get( );
private:
	void seek( long ){}		// cannot use
	__off_t tell( ){ return 0L; }	// cannot use
};

class TCharOFile : public TOFile {
protected:
	tnstr *str;
public:
	TCharOFile();
public:
	virtual int open( const tchar *filename );	// filename is pointer to tnstr object
	virtual int create( const tchar *_filename ); // filename is pointer to tnstr object
	virtual int flush( );
private:
	virtual int open( const tchar */*filename*/, int /*trunc*/ )
		{ return -1; }
	virtual void seek(long ){}	// cannot use
	virtual __off_t tell( ){ return 0L; }	// cannot use
	virtual void end( ){}	// cannot use
};

#endif
