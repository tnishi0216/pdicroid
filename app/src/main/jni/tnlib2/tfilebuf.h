#ifndef __tfilebuf_h
#define	__tfilebuf_h

#define	CP_UTF16LE		1200
#define	CP_UTF16BE		1201
#define	CP_USASCII		20127
#define	CP_SJIS			932
#define	CP_EUCJP		51932
#define	CP_JIS			50222	//TODO: 50221?

class TTextFileBuffer {
protected:
	int fd;
	int curp;
	int Size;
public:
	TTextFileBuffer(int _fd, int size)
		:fd(_fd)
		,Size(size)
	{
		curp = Size;
	}
	virtual ~TTextFileBuffer(){}
	void flush()
	{
		curp = Size;
	}
	virtual int read() = 0;
	virtual void push_back(int c) = 0;
	virtual int remainBytes() = 0;
	virtual void *_get() = 0;
	void full()
	{
		curp = 0;
	}
protected:
	int read(void *buf, int len);
};

template <class T>
class TTextFileBufferT : public TTextFileBuffer {
typedef TTextFileBuffer super;
protected:
	T *Buffer;
public:
	TTextFileBufferT(int fd, int size)
		:super(fd, size)
	{
		Buffer = new T[size];
		Size = size;
	}
	virtual ~TTextFileBufferT()
	{
		delete[] Buffer;
	}
	T &operator[](int index)
	{
		return Buffer[index];
	}
	T *get() const
		{ return Buffer; }
	virtual void *_get()
		{ return Buffer; }
	virtual void push_back(int c)
	{
		Buffer[ --curp ] = (T)c;
	}
	int remainBytes()
	{
		return (Size - curp)*sizeof(T);
	}
	virtual int read()
	{
		int d;
		if ( curp >= Size ){
			if ((d = (super::read(Buffer, Size*sizeof(T)))) <= 0)
				return -1;
			d = d/sizeof(T);
			if (d < Size){
				curp = Size - d;
				memmove(Buffer+curp, Buffer, d*sizeof(T));
			} else {
				curp = 0;
			}
			changeEndian();
		}
		return Buffer[curp++];
	}
protected:
	virtual void changeEndian(){}
	inline int read(void *buf, int len)
	{
		return super::read(buf, len);
	}
};

class TTextFileBufferUTF8 : public TTextFileBufferT<unsigned char> {
typedef TTextFileBufferT<unsigned char> super;
protected:
#if wchar_size==2
	wchar_t surrogate;
#endif
public:
	TTextFileBufferUTF8(int fd, int size)
		:super(fd, size)
	{
#if wchar_size==2
		surrogate = 0;
#endif
	}
	virtual int read();
protected:
	inline int read(void *buf, int len)
	{
		return super::read(buf, len);
	}
};

class TTextFileBufferUTF16 : public TTextFileBufferT<utf16_t> {
typedef TTextFileBufferT<utf16_t> super;
public:
	TTextFileBufferUTF16(int fd, int size)
		:super(fd, size)
	{
	}
#if wchar_size==4
	virtual int read()
	{
		int c = super::read();
		if ( c >= 0xD800 && c <= 0xDBFF ){
			// surrogate
			utf16_t wc = super::read();
			if ( wc >= 0xDC00 && wc <= 0xDFFF ){
				return (((c-0xD800)<<10) | (wc - 0xDC00)) + 0x10000;
			}
			DBW("TTextFileBufferUTF16::illegal char: %04X %04X", c, wc);
			push_back(wc);
		}
		return c;
	}
protected:
	inline int read(void *buf, int len)
	{
		return super::read(buf, len);
	}
#endif
};

class TTextFileBufferUTF16BE : public TTextFileBufferUTF16 {
typedef TTextFileBufferUTF16 super;
public:
	TTextFileBufferUTF16BE(int fd, int size)
		:super(fd, size)
	{
	}
protected:
	virtual void changeEndian();
};

class TTextFileBufferAnsi : public TTextFileBufferT<unsigned char> {
typedef TTextFileBufferT<unsigned char> super;
protected:
public:
	TTextFileBufferAnsi(int fd, int size)
		:super(fd, size)
	{
	}
};

// Assumes the all code is Ascii text, not need the code conversion for ANSI/UTF-8
class TTextFileBufferAsc : public TTextFileBufferT<unsigned char> {
typedef TTextFileBufferT<unsigned char> super;
protected:
public:
	TTextFileBufferAsc(int fd, int size)
		:super(fd, size)
	{
	}
};

// text buffer for TCharCode class
class TTextFileBufferCC : public TTextFileBufferUTF16 {
typedef TTextFileBufferUTF16 super;
protected:
	class TCharCodeBase *cc;
	int cp_src;	// code page of src
	char *readbuf;
	int readbuf_size;
	int readbuf_len;
	int readbuf_cur;
public:
	TTextFileBufferCC(TCharCodeBase *_cc, int fd, int size, int _cp_src)
		:super(fd, size)
		,cc(_cc)
		,cp_src(_cp_src)
	{
		readbuf_size = size;
		readbuf = new char[readbuf_size];
		readbuf_len = 0;
		readbuf_cur = 0;
	}
	~TTextFileBufferCC()
	{
		delete[] readbuf;
	}
	virtual int read();
};


#endif	// __tfilebuf_h
