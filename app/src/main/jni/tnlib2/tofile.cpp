#include	"tnlib.h"
#pragma	hdrstop
#include	"file.h"

//
// TOFileBase class
//
TOFileBase::TOFileBase()
{
	curp = 0;
	textmode = TFM_DEFAULT;
}

int TOFileBase::open( const tchar *_filename )
{
	curp = 0;
	text = 1;
	if ( super::open( _filename ) ){
		return -1;
	}
#ifdef _Windows
//	if ( !strcmp( _filename, "CON" ) == 0 || strcmp( _filename, "PRN" ) == 0 ){
	if ( _tcscmp( _filename, _T("CON") ) && _tcscmp( _filename, _T("PRN") ) ){
		end();
	}
#endif
	return 0;
}

int TOFileBase::create( const tchar *_filename )
{
	curp = 0;
	text = 1;
	if ( super::create( _filename ) ){
		return -1;
	}
	return 0;
}

void TOFileBase::close( )
{
	flush();
	super::close();
}

void TOFileBase::seek(long l)
{
	flush();
	_tlseek(fd, l, SEEK_SET);
}

#if 0
void TOFileBase::seek( long l, int dir )
{
	flush();
	_tlseek( fd, l, dir );
}
#endif

__off_t TOFileBase::tell()
{
	return _ttell(fd) + curp;
}

void TOFileBase::end( )
{
	//わざわざ^Zの処理をしてやらなければならない
	_tlseek( fd, -1L, SEEK_END );
	tchar c = 0;
	if ( _tread( fd, &c, 1 ) == 1 && c == 0x1a ){
		_tlseek( fd, -1L, SEEK_CUR );
	} else {
		_tlseek( fd, 0L, SEEK_END );
	}
}


int TOFileBase::bom()
{
#ifdef _UNICODE
	if (flush()){
		return -1;
	}
	switch (textmode){
		case TFM_DEFAULT:
			if (write("\xFF\xFE", 2)<=0){
				return -1;
			}
			break;
		case TFM_UTF8:
			if (write("\xEF\xBB\xBF", 3)<=0){
				return -1;
			}
			break;
		case TFM_UTF16BE:
			if (write("\xFE\xFF", 2)<=0){
				return -1;
			}
			break;
		default:
			return -1;	// Undefined.
	}
#endif
	return 0;
}

void TOFileBase::settextmode(int _textmode)
{
	flush();
	textmode = _textmode;
}

int TOFileBase::putbin( const char *str, int len )
{
	flush();
	return write(str, len);
}

//
// TOFileW class
//
TOFileW::~TOFileW()
{
	close();
}

int TOFileW::flush()
{
	if (curp != 0){
		if ( write(writebuff, textmode==TFM_UTF8?curp:curp*sizeof(utf16_t)) <= 0 ){
			return -1;
		}
		curp = 0;
	}
	return 0;
}


// Prerequisite : s does not include CR/LF.
// If includes CR/LF, CR/LF is not processed properly.
// length must be right value.(bad ex. length > wcslen(s) is not allowed!)
int TOFileW::put(const wchar_t *s, int length)
{
	int cpylen;
	const wchar_t *endp;
	const wchar_t *sp;
	utf16_t *dp;
	switch (textmode){
		case TFM_UTF8:
			do {
				// UTF8で十分格納できるバッファ量とのmin
				cpylen = min((BUFFSIZE*2-curp)/4, length);
				if (cpylen==0){
					if (flush()){
						curp -= cpylen;
						return -1;
					}
					continue;
				}
#if wchar_size==2
				int rlen = UTF16toUTF8((utf16_t*)s, cpylen, &((char*)writebuff)[curp]);
#endif
#if wchar_size==4
				int rlen = UTF32toUTF8((utf32_t*)s, cpylen, &((char*)writebuff)[curp]);
#endif
				curp += rlen;
				length -= cpylen;
				s += cpylen;
			} while (length>0);
			if (curp >= BUFFSIZE*2){
				if ( flush( ) ){
					return -1;
				}
			}
			break;
#ifdef _Windows
		case TFM_ANSI:
			do {
				cpylen = min((BUFFSIZE*2-curp)/2, length);
				if (cpylen==0){
					if (flush()){
						curp -= cpylen;
						return -1;
					}
					continue;
				}
				int rlen = WideCharToMultiByte(CP_ACP, 0, s, cpylen, &((char*)writebuff)[curp], cpylen*2, NULL, NULL );
				if (rlen<=0){
					return -1;
				}
				curp += rlen;
				length -= cpylen;
				s += cpylen;
			} while (length>0);
			if (curp >= BUFFSIZE*2){
				if ( flush( ) ){
					return -1;
				}
			}
			break;
#endif
		case TFM_UTF16BE:
#if wchar_size==4
			DBW("not supported");
			return -1;
#else
			do {
				cpylen = min(BUFFSIZE-curp, length);
				endp = s+cpylen;
				sp = s;
				dp = &writebuff[curp];
				for(;sp<endp;){
					wchar_t c = *sp++;
					*dp++ = (c>>8) | (c<<8);
				}
				curp += cpylen;
				if (curp>=BUFFSIZE){
					if (flush()){
						curp -= cpylen;
						return -1;
					}
				}
				length -= cpylen;
				s = sp;
			} while (length>0);
			break;
#endif
		case TFM_DEFAULT:
#if wchar_size==4
			{
			uint16_t surrogate = 0;
			sp = s;
			do {
				cpylen = min(BUFFSIZE-curp, length);
				dp = &writebuff[curp];
				uint16_t *endp = dp+cpylen;
				for(;dp<endp;){
					wchar_t c;
					if (surrogate){
						c = surrogate;
						surrogate = 0;
					} else {
						c = *sp++;
						length--;
						if (c>=0x10000){
							surrogate = (uint16_t)((c & 0x3FF) + 0xDC00);
							c = (uint16_t)((c & 0x3FF) + 0xDC00);
						}
					}
					*dp++ = (uint16_t)((c>>8) | (c<<8));
				}
				curp += cpylen;
				if (curp>=BUFFSIZE){
					if (flush()){
						curp -= cpylen;
						return -1;
					}
				}
			} while (length>0 || surrogate!=0);
			}
#else
			do {
				cpylen = min(BUFFSIZE-curp, length);
				memcpy(&writebuff[curp], s, cpylen*sizeof(utf16_t));
				curp += cpylen;
				if (curp>=BUFFSIZE){
					if (flush()){
						curp -= cpylen;
						return -1;
					}
				}
				length -= cpylen;
				s += cpylen;
			} while (length>0);
#endif
			break;
		default:
			return -1;	// unsupported.
	}
	return 0;
}

int TOFileW::put(const wchar_t *str)
{
	return TOFileW::put(str, wcslen(str));
}

int TOFileW::putline(const wchar_t *str)
{
	if ( put( str ) )
		return -1;
	return putdir( '\n' );
}

int TOFileW::put(const wchar_t c)
{
	if (c<' '){
		return putdir(c);
	}
	static wchar_t buf[2] = {0,0};
	buf[0] = c;
	return put(buf, 1);
}

// put directly without code conversion such as control code.
int TOFileW::putdir(const wchar_t c)
{
	if ( text && c == '\n' )
		putdir( '\r' );
	switch (textmode){
		case TFM_UTF8:
			((char*)writebuff)[curp++] = c;
			break;
		case TFM_UTF16BE:
			writebuff[curp++] = (c>>8) | (c<<8);
			break;
		default:
			writebuff[curp++] = c;
			break;
	}
	if (curp >= BUFFSIZE){
		if ( flush( ) ){
			return -1;
		}
	}
	return 0;
}


//
// TOFileA class
//
TOFileA::~TOFileA()
{
	close();
}

int TOFileA::flush()
{
	if (curp != 0){
		if ( write(writebuff, curp) <= 0 ){
			return -1;
		}
		curp = 0;
	}
	return 0;
}

// Prerequisite : s does not include CR/LF.
// If includes CR/LF, CR/LF is not processed properly.
// length must be right value.(bad ex. length > wcslen(s) is not allowed!)
int TOFileA::put(const char *s, int length)
{
	int cpylen;
	do {
		cpylen = min(BUFFSIZE-curp, length);
		memcpy(&writebuff[curp], s, cpylen);
		curp += cpylen;
		if (curp>=BUFFSIZE){
			if (flush()){
				curp -= cpylen;
				return -1;
			}
		}
		length -= cpylen;
		s += cpylen;
	} while (length>0);
	return 0;
}

int TOFileA::put(const char *str)
{
	return TOFileA::put(str, strlen(str));
}

int TOFileA::putline(const char *str)
{
	if ( put( str ) )
		return -1;
	return putdir( '\n' );
}

int TOFileA::put(const char c)
{
	if ((unsigned char)c<' '){
		return putdir(c);
	}
	static char buf[2] = {0,0};
	buf[0] = c;
	return put(buf, 1);
}

// put directly without code conversion such as control code.
int TOFileA::putdir(const char c)
{
	if ( text && c == '\n' )
		putdir( '\r' );
	writebuff[curp++] = c;
	if (curp >= BUFFSIZE){
		if ( flush( ) ){
			return -1;
		}
	}
	return 0;
}

#if 0	// old
//
// TOFile class
//
TOFile::TOFile()
{
	curp = 0;
	writebuff = new tchar[ BUFFSIZE ];
	textmode = TFM_DEFAULT;
}

TOFile::~TOFile()
{
	close( );
	if (writebuff)
		delete[] writebuff;
}

int TOFile::open( const tchar *_filename )
{
	curp = 0;
	text = 1;
	if ( OFile::open( _filename ) ){
		return -1;
	}
//	if ( !strcmp( _filename, "CON" ) == 0 || strcmp( _filename, "PRN" ) == 0 ){
	if ( _tcscmp( _filename, _T("CON") ) && _tcscmp( _filename, _T("PRN") ) ){
		end();
	}
	return 0;
}

int TOFile::create( const tchar *_filename )
{
	curp = 0;
	text = 1;
	if ( OFile::create( _filename ) ){
		return -1;
	}
	return 0;
}

void TOFile::close( void )
{
	flush();
	OFile::close();
}

int TOFile::flush(void)
{
	if (curp != 0){
		if ( write(writebuff, textmode==TFM_UTF8?curp:LENTOBYTE(curp)) <= 0 ){
			return -1;
		}
	}
	curp = 0;
	return 0;
}

int TOFile::bom()
{
#ifdef _UNICODE
	if (flush()){
		return -1;
	}
	switch (textmode){
		case TFM_DEFAULT:
			if (write("\xFF\xFE", 2)<=0){
				return -1;
			}
			break;
		case TFM_UTF8:
			if (write("\xEF\xBB\xBF", 3)<=0){
				return -1;
			}
			break;
		case TFM_UTF16BIG:
			if (write("\xFE\xFF", 2)<=0){
				return -1;
			}
			break;
		default:
			return -1;	// Undefined.
	}
#endif
	return 0;
}

void TOFile::settextmode(int _textmode)
{
	flush();
	textmode = _textmode;
}

int TOFile::putline(const tchar *str)
{
	if ( put( str ) )
		return -1;
	return put( '\n' );
}

int TOFile::put( const tchar *str )
{
	while ( *str != '\0' ){
		if ( put( *str++ ) )
			return -1;
	}
	return 0;
#if 0
	int l = strlen( str );
	if ( curp + l >= BUFFSIZE ){
		if ( flush() )
			return -1;
		if ( l >= BUFFSIZE ){
			return write( str, l );
		}
	}
	strcpy( &writebuff[ curp ], str );
	curp += l;
	return 0;
#endif
}

int TOFile::put(tchar c)
{
	if ( text && c == '\n' )
		put( '\r' );
#ifdef _UNICODE
	if (textmode!=TFM_DEFAULT){
		switch (textmode){
			case TFM_UTF8:
				{
					char dst[8];
					char *dp;
#if wchar_size==2
					UTF16toUTF8(&c, 1, dst);
#endif
#if wchar_size==4
					UTF32toUTF8(&c, 1, dst);
#endif
					for (dp=dst;*dp;dp++){
						((char*)writebuff)[curp++] = *dp;
						if (curp >= BUFFSIZE*2){
							if ( flush( ) ){
								curp--;
								return -1;
							}
						}
					}
				}
				return 0;
			case TFM_UTF16BIG:
				c = ((c>>8) | (c<<8));
				break;
			default:
				return -1;	// unsupported.
		}
	}
#endif
	writebuff[curp++] = c;
	if (curp >= BUFFSIZE){
		if ( flush( ) ){
			curp--;
			return -1;
		}
	}
	return 0;
}

void TOFile::seek(long l)
{
	flush();
	_tlseek(fd, l, SEEK_SET);
}

#if 0
void TOFile::seek( long l, int dir )
{
	flush();
	_tlseek( fd, l, dir );
}
#endif

long TOFile::tell(void)
{
	return _ttell(fd) + curp;
}

void TOFile::end( void )
{
	//わざわざ^Zの処理をしてやらなければならない
	_tlseek( fd, -1L, SEEK_END );
	tchar c = 0;
	if ( _tread( fd, &c, 1 ) == 1 && c == 0x1a ){
		_tlseek( fd, -1L, SEEK_CUR );
	} else {
		_tlseek( fd, 0L, SEEK_END );
	}
}
#endif
