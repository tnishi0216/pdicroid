#include "stdafx.h"
#ifdef _Windows
#include <windows.h>
#endif
#pragma	hdrstop
#include	"rexpgen.h"
#ifdef _UNICODE
#include "bocu1.h"
#include "utf.h"
#endif

GenericRexp::GenericRexp( )
{
	srchflag = 0;
#ifdef _UNICODE
	Buffer = NULL;
	BufferLen = 0;
#endif

	next = NULL;
	connection = 0;
	_not = false;
}
GenericRexp::~GenericRexp( )
{
#ifdef _UNICODE
	delete[] Buffer;
#endif
	delete next;
}
BOOL GenericRexp::Compile(const tchar *str)
{
	pattern = str;
	return Compile();
}
BOOL GenericRexp::Compile( )
{
	GenericRexp *rexp = this;
	for(;;){
		BOOL r = rexp->CompileImpl(rexp->pattern);
		if (!r)
			return FALSE;
		if (!rexp->next)
			return r;
		rexp = rexp->next;
	}
}
// If matches is not null, get the full match info. (speed slower)
BOOL GenericRexp::Compare( const tchar *str, TMatchInfos *matches, int user )
{
	GenericRexp *rexp = this;
	BOOL ret = FALSE;
	for(;;){
		rexp->matched = false;
		BOOL r = rexp->CompareImpl(str);
		if (rexp->_not)
			r = !r;
		if (matches && !rexp->_not){
			//Note: not include the match info of the not operation.
			if (r){
				int loc, len;
				int offs = 0;
				const tchar *p = str;
				do {
					rexp->GetMatchImpl(len, loc);
					matches->add(new TRexpMatchInfo(loc+offs, len));
					offs += loc+len;
					p += loc+len;
					if (!rexp->CompareImpl(p))
						break;
				} while (1);
			}
		}
		GenericRexp *rexp_next = rexp->GetNextForComp(user);
		if (!rexp_next){
			return rexp->matched = (r || ret);
		}
		if (rexp_next->connection==0){
			// AND
			if (!r){
				return FALSE;
			}
		} else {
			// OR
			if (r){
				rexp->matched = true;
				if (!matches){
					return TRUE;
				}
				ret = TRUE;
			}
		}
		rexp = rexp_next;
	}
}
BOOL GenericRexp::GetMatch( int &len, int &loc, int user )	// len : マッチ長 loc : マッチ位置
{
	GenericRexp *rexp = this;
	for(;;){
		if (rexp->matched){
			BOOL r = rexp->GetMatchImpl(len, loc);
			if (r)
				return TRUE;
		}
		GenericRexp *rexp_next = rexp->GetNextForComp(user);
		if (!rexp_next){
			return FALSE;
		}
		rexp = rexp_next;
	}
}

void GenericRexp::SetFlag( int flag, BOOL f )
{
	if ( f ){
		srchflag |= flag;
	} else {
		srchflag &= ~flag;
	}
}
#ifdef _UNICODE
bool GenericRexp::CompareBocu1( const char *s, FNPreCodeTranslate precode )
{
	int slen = strlen(s);
	int maxlength = (slen<<2)+1;
	if (BufferLen < maxlength ){
		BufferLen = maxlength;
		delete[] Buffer;
		Buffer = new wchar_t[BufferLen];
	}
	bocu1Decode( (const unsigned char**)&s, (const unsigned char*)(s)+slen, maxlength, Buffer, NULL, precode );
	return Compare(Buffer) ? true : false;
}
bool GenericRexp::CompareUTF8( const char *s )	// for UTF-8
{
	int slen = strlen(s);
	int maxlength = (slen<<1)+1;
	if (BufferLen < maxlength ){
		BufferLen = maxlength;
		delete[] Buffer;
		Buffer = new wchar_t[BufferLen];
	}
#if wchar_size==2
	UTF8toUTF16( s, slen, Buffer );
#else
	UTF8toUTF32( s, slen, (utf32_t*)Buffer );
#endif
	return Compare(Buffer) ? true : false;
}
#endif

GenericRexp *GenericRexp::Duplicate()
{
	GenericRexp *regs = Create();
	if (!regs->Open()){
		delete regs;
		return NULL;
	}
	regs->srchflag = srchflag;
	regs->SetPattern( GetPattern() );
	regs->connection = connection;
	regs->_not = _not;
	if (next){
		regs->next = next->Duplicate();
		if (!regs->next){
			delete regs;
			return NULL;
		}
	}
	regs->Compile();
	return regs;
}

