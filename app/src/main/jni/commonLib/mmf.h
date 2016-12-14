//---------------------------------------------------------------------------

#ifndef mmfH
#define mmfH
//---------------------------------------------------------------------------

#ifdef _Windows
class TMmfMap {
protected:
	HANDLE Handle;
	HANDLE hMem;
	void *TopAddr;
	void *Buffer;
	int Size;
public:
	TMmfMap(HANDLE handle)
	{
		Handle = handle;
		hMem = NULL;
		Size = 0;
	}
	virtual ~TMmfMap()
	{
		unmap();
	}
	void *map(unsigned offset, unsigned size, bool readonly);
	void unmap();
	void *changeOffset(unsigned offset);
	void *data()
	{
		return Buffer;
	}
	int size()
	{
		return Size;
	}
};

class TMmfFile : public TMmfMap {
typedef TMmfMap super;
public:
	TMmfFile()
		:super(INVALID_HANDLE_VALUE)
	{
	}
	virtual ~TMmfFile()
	{
		close();
	}
	bool create(const TCHAR *filename);
	void close();
};
#endif

#endif
 