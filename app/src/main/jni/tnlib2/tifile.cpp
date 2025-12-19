#include "stdafx.h"
#include	"tnlib.h"
#pragma	hdrstop
#include	"file.h"
#include <string>
using namespace std;

#include "tfilebuf.h"

//TODO:
// まだ文字コードの抽象化レベルが低い

TIFile::TIFile()
{
	readbuff = NULL;
	textmode = TFM_DEFAULT;
	bom = 0;	// ANSI or not confirmed
	unicode = false;
}

TIFile::~TIFile()
{
	delete readbuff;
}

int TIFile::open(const tchar* _filename, int defcharcode )
{
	filename = _filename;
	text = 1;
	err = 0;
	if ( fd == -1 ){
		fd = ::_topen( filename, O_RDONLY|O_BINARY );
		if (fd == -1){
			err = 1;
			return -1;
		}
	}
	delete readbuff;
#ifdef _UNICODE
	bom = skipmark();
	if (!bom && defcharcode){
		switch (defcharcode){
			case TFM_UTF16LE: bom = 0xFEFF; break;
			case TFM_UTF16BE: bom = 0xFFFE; break;
			case TFM_UTF8: bom = 0xBF; break;
			case TFM_ASCII: bom = 20127; break;
		}
	}
	unicode = bom ? true : false;
	switch (bom){
		case 0xFEFF:
//jutf16le:;
			readbuff = new TTextFileBufferUTF16(fd, BUFFSIZE);
			textmode = TFM_DEFAULT;
			break;
		case 0xFFFE:
//jutf16be:;
			readbuff = new TTextFileBufferUTF16BE(fd, BUFFSIZE);
			textmode = TFM_UTF16BE;
			break;
		case 0xBF:
//jutf8:;
			readbuff = new TTextFileBufferUTF8(fd, BUFFSIZE);
			textmode = TFM_UTF8;
			break;
		case 20127:
jascii:;
			readbuff = new TTextFileBufferAsc(fd, BUFFSIZE);
			textmode = TFM_ASCII;
			break;
		default:
			if (cc){
				int code = cc->GetDetectedCode();
				if (code == 0){
					TCharCodeStreamFD stream(fd);
					code = cc->Detect(filename, &stream);
				}
				switch (code){
#if 0	//TODO: どっちが速いか後で調べる
					case CP_UTF8:
						goto jutf8;
					case CP_UTF16LE:
						goto jutf16le;
					case CP_UTF16BE:
						goto jutf16be;
#endif
					case CP_USASCII:
						goto jascii;
					default:
						textmode = code;
						break;
				}
				if (code>0){
					textmode = code;
					readbuff = new TTextFileBufferCC(cc, fd, BUFFSIZE, code);
					unicode = true;
					break;
				}
			}
			readbuff = new TTextFileBufferAnsi(fd, BUFFSIZE);
			textmode = !cc && defcharcode ? defcharcode : TFM_ANSI;
			break;
	}
#else
	readbuff = new TTextFileBufferAnsi(fd, BUFFSIZE);
#endif
	return 0;
}

void TIFile::flush()
{
	if (!readbuff)
		return;
	readbuff->flush();
}

#ifdef _UNICODE
void TIFile::home( )
{
	seek(0);
	skipmark();
}

// 先頭の0xFFFE or 0xFEFFをとばす
// open()後のみ有効
// return value : mark value
// 無い場合はskipせず、0を返す
unsigned short TIFile::skipmark()
{
	TTextFileBufferAsc buff(fd, 4);
	int c = buff.read();
	c |= buff.read()<<8;
	if ( c == 0xFEFF || c == 0xFFFE){
		seek(2);
		return (unsigned short)c;
	}
	if (c==0xBBEF){
		c = buff.read();
		if (c==0xBF){
			seek(3);
			return (unsigned short)c;	// UTF-8
		}
	}
	if ( c != -1 )
		seek(0);
	return 0;
}
#endif
int TIFile::get( )
{
	return readbuff->read();
}

void TIFile::seek(long l)
{
	if (readbuff){
		readbuff->flush();
	}
	_tlseek(fd, l, SEEK_SET);
}

__off_t TIFile::tell()
{
	return _ttell(fd) - (readbuff ? readbuff->remainBytes() : 0);
}

#ifdef WINCE
inline bool _ismbblead( unsigned char c )
{
	// for ShiftJIS
	return (c>=0x80&&c<=0x9F) || (c>=0xE0 /*&&c<=0xFF*/);
}
#endif	// WINCE

int TIFile::getline( tnstr &buf )
{
#if defined(_UNICODE) && defined(TNANSI)
	if (!unicode){
		return getlineA(buf);
	}
#endif
	tchar fbuf[256];
	tchar *p = fbuf;
	int len = 0;
	buf.clear();
	int l;
	while ( 1 ){
		int cc = get();
		if ( cc == -1 ){
jeof:
			l = STR_DIFF(p, fbuf);
			if (l==0){
				return -1;
			} else {
				buf.cat(fbuf, l);
				return len + l;
			}
		}
		tchar c = (tchar)cc;
		if ( c == '\x1a' || c == '\0' )
			continue;
		if ( c == '\r' ){
			cc = get();
			if ( cc != '\n' ){
				if (cc==-1) goto jeof;
				readbuff->push_back(cc);
			}
			goto j1;
		}
		if ( c == '\n' ){
j1:
			l = STR_DIFF(p,fbuf);
			buf.cat(fbuf, l);
			return len + l;
		}
		if ( (l=STR_DIFF(p,fbuf)) >= tsizeof(fbuf)-1 ){
			buf.cat(fbuf, l);
			len += l;
			p = fbuf;
		}
		*p++ = c;
	}
}

#if 0	//TODO: not implemented yet
//テキストファイル専用行読み込みルーチン
//'\n'まで読み込み、'\r\n'は捨てられる
//maxlenを越える文字列は捨てられる（日本語の第２バイト識別）
//一番良い終了判定は返り値が-1であり、なおかつbuf[0] == '\0'である場合
// 1999.1.6 revised.
// 戻り値：読み込みバイト数, -1=error or EOF
// 高速化
// UNIX, Mac改行対応
int TIFile::getline( tchar *buf, int maxlen )
{
#ifdef _UNICODE
	if (!unicode){
		return getlineA(buf, maxlen);	//TODO: not implemented yet
	}
#endif
	tchar *p = buf;
	tchar *lastp = buf + maxlen - 1;
	for (;;){
		int cc = get();
		if ( cc == -1 ){
jeof:
			p[0] = '\0';
			return buf == p ? -1 : STR_DIFF(p,buf);
		}
		tchar c = (tchar)cc;
		if ( c == '\x1a' || c == '\0' )
			continue;
		if ( c == '\r' ){
			c = (tchar)get();
			if ( c != '\n' ){
				if (c==-1) goto jeof;
				readbuff->push_back(c);
			}
			goto j1;
		}
		if ( c == '\n' ){
	j1:
#ifndef _UNICODE
			if ( p >= lastp ){
				if ( _ismbblead( (unsigned char)*(p-1) ) ){
					if ( _mbsbtype( (const unsigned char*)buf, (int)(p-buf)-1 ) == _MBC_LEAD ){
						p--;
					}
				}
			}
#endif
			*p = '\0';
			return STR_DIFF(p,buf);
		}
		if ( p < lastp ){		//len以上の文字は捨てる
			*p++ = c;
		}
	}
}
#endif

int TIFile::skipline( )
{
	for (;;){
		int cc = get();
		if ( cc == -1 ){
			// EOF
			return -1;
		}
		tchar c = (tchar)cc;
		if ( c == '\x1a' || c == '\0' )
			continue;
		if ( c == '\r' ){
			c = (tchar)get();
			if ( c != '\n' ){
				readbuff->push_back(c);
			}
			// EOL
			return 0;
		}
		if ( c == '\n' ){
			// EOL
			return 0;
		}
	}
}

int TIFile::getA( )
{
	if (unicode){
		__off_t loc = tell();
		delete readbuff;
		unicode = false;
		readbuff = new TTextFileBufferAnsi(fd, BUFFSIZE);
		if (!readbuff)
			return -1;
		seek(loc);
	}
	return readbuff->read();
}

#ifdef _UNICODE
#ifndef UNIX
// ANSI textを読み込み、UNICODEで返す
int TIFile::getlineA( tnstr &buf )
{
	string s;
	int r = getline(s);
	if (r==-1)
		return r;
	wchar_t *wbuf = new wchar_t[r+1];
	r = _MultiByteToWideChar( CP_ACP, 0, s.c_str(), r, wbuf, r+1 );
	wbuf[r] = '\0';
	buf.setBuf(wbuf);
	return r;
}
#endif

#if 0
// ANSI textを読み込み、UNICODEで返す
int TIFile::getlineA( __Char &buf )
{
	string s;
	int r = getline(s);
	if (r==-1)
		return r;
	wchar_t *wbuf = new wchar_t[r+1];
	r = _MultiByteToWideChar( CP_ACP, 0, s.c_str(), r, wbuf, r+1 );
	wbuf[r] = '\0';
	buf.SetBuf(wbuf);
	return r;
}
#endif
#endif	// _UNICODE

// ASC/UTF-8 textを読み込み
// No code conversion.
int TIFile::getline( std::string &buf )
{
	char fbuf[256];
	char *p = fbuf;
	int len = 0;
	int r;
	buf = "";
	while ( 1 ){
		int cc = getA();
		switch ( cc ){
			case -1:
jeof:
				*p = '\0';
				r = (int)(p-fbuf);
				buf += fbuf;
				if ( buf.empty() ){
					return -1;
				} else {
					return len + r;
				}
			case '\x1a':
			case '\0':
				continue;
			case '\r':
				cc = getA();
				if ( cc != '\n' ){
					if (cc==-1) goto jeof;
					readbuff->push_back(cc);
				}
				goto j1;
			case '\n':
j1:
				*p = '\0';
				r = (int)(p-fbuf);
				buf += fbuf;
				return len + r;
			default:
				if ( (int)(p-fbuf) >= sizeof(fbuf)-1 ){
					*p = '\0';
					BYTE c = (BYTE)p[-1];
					if (IsDBCSLeadByte(c)){
						// ２バイト文字の１バイト目（らしい）
						char *q = fbuf;
						for(;q<p;){
							if ( _ismbblead(*q) ){
								q += 2;
							} else {
								q++;
							}
						}
						if ( q != p ){
							// やはりp[-1]は２バイト文字の１バイト目だった
							p--;
							p[0] = '\0';
						} else {
							c = 0;
						}
					} else {
						c = 0;
					}
					buf += fbuf;
					r = (int)(p-fbuf);
					len += r;
					p = fbuf;
					if (c)
						*p++ = (char)c;
				}
				*p++ = (char)cc;
				break;
		}
	}
}

bool TIFile::createFixedTextBuffer(int buffsize)
{
	delete readbuff;
	readbuff = new TTextFileBufferT<tchar>(0, buffsize);
	if (!readbuff)
		return false;
	return true;
}
