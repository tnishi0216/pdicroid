#include "tnlib.h"
#pragma hdrstop
#include "regexppctrl.h"

HINSTANCE TRegexpp::hInst = NULL;
int TRegexpp::Ref = 0;
FNREGCOMP TRegexpp::regcomp;
FNREGCOMPEX TRegexpp::regcomp_ex;
FNREGEXEC TRegexpp::regexec;
FNREGERROR TRegexpp::regerror;
FNREGFREE TRegexpp::regfree;
FNREGVERSION TRegexpp::regversion;
FNREGMATCH TRegexpp::regmatch;
FNREGMATCHSTRING TRegexpp::regmatch_string;
//FNREGREPLACE TRegexpp::regreplace;

TRegexpp::TRegexpp()
{
	Opened = false;
	Handle = NULL;
	ErrorCode = 0;
#if REGEXPP_THREADSAFE
	InitializeCriticalSection(&mutex);
#endif
}
TRegexpp::~TRegexpp()
{
	Close( );
#if REGEXPP_THREADSAFE
	DeleteCriticalSection(&mutex);
#endif
}

BOOL TRegexpp::Open( )
{
	if (Opened)
		return TRUE;
	Opened = true;
	Lock();
	if (Ref++>0){
		Unlock();
		return TRUE;
	}
	hInst = LoadLibraryA("REGEXPP.DLL");
	if ( !IsValidInstance(hInst) ){
		DBW("REGEXPP.DLL not found");
		goto jerror2;
	}
	regcomp = (FNREGCOMP)GetProcAddress(hInst,"regex_comp");
	regcomp_ex = (FNREGCOMPEX)GetProcAddress(hInst,"regex_comp_ex");
	regexec = (FNREGEXEC)GetProcAddress(hInst,"regex_exec");
	regerror = (FNREGERROR)GetProcAddress(hInst,"regex_error");
	regfree = (FNREGFREE)GetProcAddress(hInst,"regex_free");
	regversion = (FNREGVERSION)GetProcAddress(hInst,"regex_version");
	regmatch = (FNREGMATCH)GetProcAddress(hInst,"regex_match");
	regmatch_string = (FNREGMATCHSTRING)GetProcAddress(hInst,"regex_match_string");
	//regreplace = (FNREGREPLACE)GetProcAddress(hInst,"regex_replace");
	if (!regcomp || !regexec || !regerror || !regfree || !regversion){
		goto jerror1;
	}
	Unlock();
	return TRUE;

jerror1:
	FreeLibrary(hInst);	
jerror2:
	Ref--;
	hInst = NULL;
	Unlock();
	return FALSE;
}
void TRegexpp::Close( )
{
	if (!Opened)
		return;
	Opened = false;
	if (Handle){
		regfree(Handle);
		Handle = NULL;
	}
	Lock();
	if (--Ref>0){
		Unlock();
		return;
	}
	FreeLibrary(hInst);
	hInst = NULL;
	Unlock();
}
BOOL TRegexpp::CompileImpl( const tchar *patern )
{
	Lock();
	Handle = regcomp( patern, (srchflag&GRXP_IGNORECASE?REG_ICASE:0)|REG_EXTENDED, &ErrorCode );
	Unlock();
	return Handle ? true : false;
}

BOOL TRegexpp::CompareImpl( const tchar *str )
{
	Lock();
	BOOL r = regexec( Handle, str, &Start, &End, 0 );
	Unlock();
	return r;
}
BOOL TRegexpp::GetMatchImpl( int &len, int &loc )
{
	len = End-Start;
	loc = Start;
	return TRUE;
}
int TRegexpp::GetErrorCode( )
{
	if ( !hInst )
		return GRE_NOTREADY;
	switch ( ErrorCode ){
		case REG_BADPAT:	// Invalid pattern.
		case REG_ECOLLATE:	// Undefined collating element.
			return GRE_SYNTAX;
		case REG_ECTYPE:	// Invalid character class name.
			return GRE_CHARCLASS;
		case REG_EESCAPE:	// Trailing backslash.
		case REG_ESUBREG:	// Invalid back reference.
			return GRE_SYNTAX;
		case REG_EBRACK:	// Unmatched left bracket.
		case REG_EPAREN:	// Parenthesis imbalance.
		case REG_EBRACE:	// Unmatched \{.
			return GRE_BRACKET;
		case REG_BADBR:		// Invalid contents of \{\}.
			return GRE_SYNTAX;
		case REG_ERANGE:	// Invalid range end.
			return GRE_CHARCLASS;
		case REG_ESPACE:	// Ran out of memory.
			return GRE_MEMORY;
		case REG_BADRPT:	// No preceding re for repetition op.
			return GRE_SYNTAX;
		case REG_EEND:		// unexpected end of expression
			return GRE_SYNTAX;
		case REG_ESIZE:		// expression too big
			return GRE_TOOLONG;
		case REG_ERPAREN:	// unmatched right parenthesis
			return GRE_BRACKET;
		case REG_EMPTY:		// empty expression
			return GRE_NOSTRINGS;
		default:
			return GRE_UNEXPECTED;
	}
}

bool TRegexpp::CompareEx(const tchar *pattern, const tchar *str, tnstr_vec &strs)
{
	int handle;
	if (pattern){
		handle = regcomp_ex(pattern);
		if (!handle)
			return false;
	} else {
		handle = Handle;
		if (!handle)
			return false;
	}
	bool ret = false;
	if (regmatch(handle, str)){
		for (int i=1;;i++){
			const wchar_t *s = regmatch_string(handle, i);
			if (!s || !s[0])
				break;
			strs.push_back(s);
			ret = true;
		}
	}
	if (pattern)
		regfree(handle);
	return ret;
}
#if 0
// pattern matchÇ…ÇÊÇÈàÍívï∂éöÇÃéÊìæÇ™â¬î\
int TRegexpp::CompileEx(const tchar *pattern)
{
	return regcomp_ex(pattern);
}
void TRegexpp::CloseHandle(int h)
{
	if (h)
		regfree(h);
}
// Ç∑Ç≈Ç…CompileExÇ≈compileçœÇ›ÇÃCompareEx
bool TRegexpp::CompareEx(int h, const tchar *str, tnstr_vec &strs)
{
	if (!h)
		return false;
	if (regmatch(h, str)){
		for (int i=1;;i++){
			const wchar_t *s = regmatch_string(h, i);
			if (!s || !s[0])
				break;
			strs.push_back(s);
		}
		return true;
	}
	return false;
}
#endif
WORD TRegexpp::GetVersion( )
{
	if ( regversion ){
		return regversion();
	} else {
		return 0;
	}
}

