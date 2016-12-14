//---------------------------------------------------------------------------
// bregonig.dll control class
//	http://k-takata.o.oo7.jp/mysoft/bregonig.html

#include "tnlib.h"
#pragma hdrstop

#include "bregonigctrl.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

HINSTANCE TBregonig::hInst = NULL;
int TBregonig::Ref = 0;
FNBMatch TBregonig::BMatch = NULL;
FNBSubst TBregonig::BSubst = NULL;
FNBRegfree TBregonig::BRegfree = NULL;
FNBRegexpVersion TBregonig::BRegexpVersion = NULL;
FNBoMatch TBregonig::BoMatch = NULL;
FNBoSubst TBregonig::BoSubst = NULL;

TBregonig::TBregonig()
{
	Opened = false;
	Handle = NULL;
	ErrorCode = 0;
#if BREGONIG_THREADSAFE
	InitializeCriticalSection(&mutex);
#endif
}
TBregonig::~TBregonig()
{
	Close( );
#if BREGONIG_THREADSAFE
	DeleteCriticalSection(&mutex);
#endif
}

BOOL TBregonig::Open( )
{
	if (Opened)
		return TRUE;
	Opened = true;
	Lock();
	if (Ref++>0){
		Unlock();
		return TRUE;
	}
	hInst = LoadLibraryA("bregonig.dll");
	if ( !IsValidInstance(hInst) ){
		goto jerror2;
	}
	BMatch = (FNBMatch)GetProcAddress(hInst, "BMatchW");
	BSubst = (FNBSubst)GetProcAddress(hInst, "BSubstW");
	BRegfree = (FNBRegfree)GetProcAddress(hInst, "BRegfreeW");
	BRegexpVersion = (FNBRegexpVersion)GetProcAddress(hInst, "BRegexpVersion");
	BoMatch = (FNBoMatch)GetProcAddress(hInst, "BoMatchW");
	BoSubst = (FNBoSubst)GetProcAddress(hInst, "BoSubstW");
	if (!BMatch || !BSubst || !BRegfree || !BoMatch || !BoSubst){
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
void TBregonig::Close( )
{
	if (!Opened)
		return;
	Opened = false;
	if (Handle){
		BRegfree(Handle);
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
BOOL TBregonig::CompileImpl( const tchar *pattern )
{
	if (Handle){
		BRegfree(Handle);
		Handle = NULL;
	}

	const tchar *text = _t("");
	tchar msg[BREGEXP_MAX_ERROR_MESSAGE_LEN];
	const tchar *option = _t("");
	if (srchflag&GRXP_IGNORECASE)
		option = _t("i");
	return BoMatch(pattern, option, text, text, text, FALSE, &Handle, msg) >= 0;
}
BOOL TBregonig::CompareImpl( const tchar *str )
{
	tchar msg[BREGEXP_MAX_ERROR_MESSAGE_LEN];
	if (BoMatch(NULL, NULL, str, str, str+_tcslen(str), FALSE, &Handle, msg)<=0){
		return FALSE;
	}
	Start = (int)(Handle->startp[0] - str);
	End = (int)(Handle->endp[0] - str);
	return TRUE;
}
BOOL TBregonig::GetMatchImpl( int &len, int &loc )
{
	len = End-Start;
	loc = Start;
	return TRUE;
}
int TBregonig::GetErrorCode( )
{
	if ( !hInst )
		return GRE_NOTREADY;
	switch ( ErrorCode ){
#if 0
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
#endif
		default:
			return GRE_UNEXPECTED;
	}
}
WORD TBregonig::GetVersion( )
{
	if ( BRegexpVersion ){
		return _ttoi(BRegexpVersion());	//TODO: Ç«Ç§Ç∑ÇÈÅH
	} else {
		return 0;
	}
}

bool TBregonig::CompareEx(const tchar *pattern, const tchar *str, tnstr_vec &strs)
{
	tchar msg[BREGEXP_MAX_ERROR_MESSAGE_LEN];

	BREGEXP *handle = NULL;
	if (pattern){
		const tchar *text = _t("");
		const tchar *option = _t("");
		if (srchflag&GRXP_IGNORECASE)
			option = _t("i");
		if (BoMatch(pattern, option, text, text, text, FALSE, &handle, msg) <= 0){
			return false;
		}
	} else {
		if (BoMatch(NULL, NULL, str, str, str+_tcslen(str)-1, FALSE, &handle, msg)<=0){
			return false;
		}
	}

	for (int i=1;i<=handle->nparens;i++){
		strs.push_back(tnstr(handle->startp[i], (int)(handle->endp[i]-handle->startp[i])));
	}
	if (pattern){
		BRegfree(handle);
		handle = NULL;
	}
	return true;
}

