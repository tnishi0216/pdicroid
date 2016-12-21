#include "tnlib.h"
#pragma	hdrstop

#if defined(_Windows) && defined(_UNICODE)
#define	USE_ANSI	1
#else
#define	USE_ANSI	0
#endif

#include "uniansi.h"

#include	"utydic.h"
//#include	"pdcom.h"
#include	"jmerge.h"
#include	"japa.h"
#include	"jtype.h"

#ifdef	GUI
#include "UtyDicView.h"
#include "id.h"
#include "winmsg.h"
#include "edfile.h"
#else
#if defined(_Windows) && !defined(UNIX)
#include "cbfile.h"
#endif
#endif
#include "charfile.h"

#include "dictype.h"
#include "dictext.h"

#ifdef PDICW
#include "prontbl.h"
#endif

#include "LangProc.h"
#include "LangProcMan.h"
#if USE_ANSI
#include "PdicAnsi.h"
#endif

#include "pdprof.h"

#include "fileio.h"
#include "filestr.h"

//
// Dictionary class
//
Dictionary::Dictionary()
	:canmerge(false),
	mergemode(MRG_MERGE)
{
	app = mod = def = 0L;
	Frequency = 0;
	CFlags = TRS_ALL;
	CFlags2 = 0;
	KeyTransIndex = 0;	// = Use the default keyword translate table
	LangProc = LangProcMan.GetLangProc(KeyTransIndex);

#ifdef PDICW
	prontable = NULL;
#endif
	dictype = DT_PDIC;

	// code conversion table //
	cctWord = NULL;
	cctPron = NULL;
	cctJapa = NULL;
	cctExp = NULL;

#ifdef GUI
	View = new TUtyDicViewVcl;
#endif
	inqf = 0;
}

Dictionary::~Dictionary()
{
#ifdef GUI
	if (View)
		delete View;
#endif

	LangProcMan.Free(LangProc);
#ifdef PDICW
	if ( prontable ) delete prontable;
#endif

	if (cctWord) delete cctWord;
	if (cctPron) delete cctPron;
	if (cctJapa) delete cctJapa;
	if (cctExp) delete cctExp;
}

int Dictionary::Open( const tchar *fname, int /* mode */ )
{
#if DICDBG1
	readcount = 0;
	maxcount = 0;
#endif
	app = mod = def = 0L;
	error = ECNone;
#if defined(_Windows) && defined(PDICW) && !defined(DLL) && !defined(NOGUI)
	if ( IsWindow( (HWND)fname ) )
		name = _T("");
	else
#endif
		name = fname;
	return 0;
}

int Dictionary::readPare( tnstr &word, Japa &japa )
{
	while ( 1 ){
		if (getWord(word) == -1)
			return -1;
		if ( word[0] == '\0' ){
			continue;
		}
		if (getJapa( japa ) == -1)
			return -1;
		break;
	}
	return 0;
}

// メモリ節約のため
int Dictionary::GetErrorCode( )
{
	return _getError( );
}

#ifdef GUI
void Dictionary::SetParent(class TConvertingDialog *parent)
{
	View->Set(parent);
}
#endif

int Dictionary::over_open( const tchar *filename )
{
	overf.open( filename );
	return ( (!overf) == 0)?0:-1;
}

void Dictionary::over_close( void )
{
	overf.close();
}

void Dictionary::SetCCTables(TCCTable *_cctWord, TCCTable *_cctPron, TCCTable *_cctJapa, TCCTable *_cctExp)
{
	if (cctWord) delete cctWord;
	cctWord = _cctWord;
	if (cctPron) delete cctPron;
	cctPron = _cctPron;
	if (cctJapa) delete cctJapa;
	cctJapa = _cctJapa;
	if (cctExp) delete cctExp;
	cctExp = _cctExp;
}

#if USE_ANSI
// Convert Japa contents.
void Dictionary::ConvertCode( const CJapa &src, Japa &dst )
{
	dst.clear();

	dst.SetAttr(src.attr);
	ConvertCode(cctPron, src.pron, dst.pron);
	ConvertCode(cctJapa, src.japa, dst.japa);
	ConvertCode(cctExp, src.exp, dst.exp);
}
#endif

void Dictionary::ConvertCode( TCCTable *table, const char *src, tnstr &dst )
{
	if (!table){
#if USE_ANSI
		// Convert with default code page.
		wchar_t *buf = ansi_to_uni(src);
		if (!buf){
			return;
		}
		dst.setBuf(buf);
		return;
#else
		DBW("ConvertCode with null table is not supported");
		return;
#endif
	}

	wchar_t *buf = new wchar_t[ strlen(src) * 2 + 1 ];
	if (!buf){
		dst.clear();
		return;
	}
	wchar_t *dp = buf;
	//char msg[300];
//	bool dumped = false;
	for(;;){
		unsigned short c = (unsigned char)*src++;
		if (!c)
			break;
		if (table->IsMultiByte( (unsigned char)c )){
			if (*src=='\0'){
				HandleCCError(src);
#if 0	//TODO: エラーをどうやって表示する？
				wsprintf(msg,"%d : %s : MultiByte Error [0x%02X] : [%s]", LineCount, name, c, word);
				if (!dumped){
					edDump->Lines->BeginUpdate();
					dumped = true;
				}
				edDump->Lines->Add(msg);
				ErrorMultiByteCount++;
#endif
				break;	// end of word(illegal character)
			}
			c = (c<<8) | (unsigned char)*src++;
		}
		wchar_t d = table->Table[c];
		if (d){
			*dp++ = d;
		} else {
			HandleCCWarning(src);
#if 0	//TODO: エラーをどうやって表示する？
			snprintf(msg,sizeof(msg)-1,"%d : %s : UndefChar [0x%02X] : [%s]", LineCount, name, c, word );
			if (!dumped){
				edDump->Lines->BeginUpdate();
				dumped = true;
			}
			edDump->Lines->Add(msg);
			UndefCharCount++;
#endif
		}
	}
	*dp = '\0';
	dst.setBuf( buf );
}

///////////////////////////////////////////////////////////////////////////
//	PdicU class
///////////////////////////////////////////////////////////////////////////
PdicU::PdicU()
{
	bFix = FALSE;
	bFixed = FALSE;
	Debug = false;
}

///////////////////////////////////////////////////////////////////////////
//	Pdic2 class
///////////////////////////////////////////////////////////////////////////
#if USE_ANSI
#include "PdicUni.h"
#include "PdicAnsi.h"
#endif
Pdic2::Pdic2()
{
	dictype = DT_PDIC;
	canmerge = true;
#if defined(USE_USERDLL)
	srcdic = NULL;
#endif
	pdic = NULL;
	dlldic = NULL;
#if USEREFMODE
	fRefMode = false;
#endif
}

Pdic2::~Pdic2()
{
	Close();
}

#if defined(GUI)
int Pdic2::CanOpen( TWinControl *parent, const tchar *fname, int mode )
{
#if defined(PDICW) && !defined(DLL) && !defined(NOGUI)
	if ( mode & FOM_CLIPBOARD ) return 0;
#endif
	if ( ( mode & FOM_OPENMODEMASK ) == FOM_READONLY ){
		if ( fexist( fname ) ){
			return 0;
		} else {
			setError( 1 );
			return -1;
		}
	} else {
		if ( fexist( fname ) ){
			int c;
			if ( mode & FOM_MERGE ){
				c = ID_MERGE;
			} else {
				if ( parent )
					c = View->MessageDialog( parent, MAKEINTRESOURCE(MRGNEWCANDLG), fname );
				else
					c = ID_MERGE;
			}
			switch (c){
				case ID_MERGE:
					if ( Open(fname, FOM_WRITE) == -1){
						if ( parent )
							View->ErrorMessage( parent, GetErrorCode(), fname );
						return -1;
					}
					break;
				case ID_NEW:
					if ( parent ){
						if ( View->CreateNewDictionary( parent, fname, false ) == -1)
							return -1;
					} else return -1;
					break;
				default:
					return -2;
			}
		} else {
			if ( parent ){
				if ( View->CreateNewDictionary( parent, fname, false ) == -1)
					return -1;
			} else return -1;
		}
		Close( );
	}
	return 0;
}
#else
#endif

int Pdic2::Open( const tchar *fname, int mode )
{
	Close();

	pdic = Pdic::CreateInstance(fname);
	if (!pdic){
		setError(DICERR_NOMEMORY);
		return -1;
	}
#if USEREFMODE
	fRefMode = false;
#endif
	Dictionary::Open( fname, mode );
	int r;
	switch ( mode & FOM_OPENMODEMASK ){
		case FOM_READONLY:
			r = pdic->OpenReadOnly( fname );
			if (r==0){
				// OK
				//pdic->SetPercentExact();
			} else {
				// Error
#if USE_ANSI
				if (pdic->GetErrorCode()==DICERR_DICVERSION || pdic->GetErrorCode()==DICERR_NOTPDIC){
					pdic->Release();
					pdic = NULL;

					do {
						// Try to open PDIC for Win32
						dlldic = new PdicAnsi;
						if (!dlldic){
							setError(DICERR_NOMEMORY);
							return -1;
						}
						r = dlldic->Open(fname, FOM_READONLY);
						if (r==0){
							dictype = DT_PDIC_OLD_ANS;
							break;
						}

						delete dlldic;
						dlldic = new PdicUni(LITTLEENDIAN, BOCU);
						if (!dlldic){
							setError(DICERR_NOMEMORY);
							return -1;
						}
						r = dlldic->Open(fname, FOM_READONLY);
						if (r==0){
							dictype = DT_PDIC_OLD_UNI;
						}
						//TODO: dlldic->SetPercentExact();
						break;
					} while (0);
				}
#endif
			}
			break;
		case FOM_CREATE:
			// 新規作成不可能
			return -1;
		case FOM_WRITE:
#ifdef USE_ESTDB
			if (CFlags2&CF2_FASTDB_CREATE){
				pdic->SetExtDBMode(DICEXT_FASTDB_CREATE, true);
			}
#endif
			r = pdic->OpenRetry( fname );
			pdic->AllowOldUpdate( );	// 古い辞書形式の更新を許可
			break;
	}
	if ( r ){
		return r;
	}
	if (pdic){
		TLangProc *proc = pdic->GetLangProc();
		if (proc){
			if (LangProc){
				LangProcMan.Free(LangProc);
			}
			LangProc = proc;
			LangProc->IncRef();
		}
#if defined(USE_JLINK)
		pdic->SetLinkAttr( OLA_NOTLINK
			| ( mode & FOM_NOTREADOLE ? OLA_NOTREADOLE : 0 )
			| ( mode & FOM_NOTREADFILE ? OLA_NOTREADFILE : 0 )
		  );	// OLE再リンクはしない
#endif
		SetAllSearch( NULL, SRCH_ALL );
	}
	return r;
}

//#ifdef PDICW
#if 1	// TDicConverter uses this.
int Pdic2::Open( Pdic *_pdic, int mode )
{
	Close();

//	_mtimes_start(s1);
	pdic = _pdic;
#if USEREFMODE
	fRefMode = true;
#endif
	pdic->AddRef();
	Dictionary::Open( pdic->GetFileName(), mode );
	switch ( mode & FOM_OPENMODEMASK ){
		case FOM_READONLY:
			pdic->SetPercentExact();
			break;
		case FOM_CREATE:
			// 新規作成不可能
			pdic->Release();
			return -1;
		case FOM_WRITE:
			if ( pdic->IsReadOnly() ){
				pdic->Release();
				return -1;
			} else {
#ifdef NEWDIC2
				pdic->AllowOldUpdate( );	// 古い辞書形式の更新を許可
#endif
			}
			break;
	}
#if USEREFMODE && defined(USE_JLINK)
	linkattr = pdic->GetLinkAttr( );
#endif
#if defined(NEWDIC2) && defined(USE_JLINK)
#if 0	// どちらでもよい？
	pdic->SetLinkAttr( OLA_NOTLINK
		| ( mode & FOM_NOTREADOLE ? OLA_NOTREADOLE : 0 )
		| ( mode & FOM_NOTREADFILE ? OLA_NOTREADFILE : 0 )
	  );	// OLE再リンクはしない
#endif
#endif
	SetAllSearch( NULL, SRCH_ALL );

	return 0;
}
#endif

void Pdic2::Close( )
{
	if (pdic){
//	_mtimes_stop(s1);
#if USEREFMODE
		if ( fRefMode ){
#if defined(USE_JLINK)
			pdic->SetLinkAttr( linkattr );
#endif
			pdic->Release();
			pdic = NULL;
		} else
#endif
		{
			pdic->Close();
			pdic->Release();
			pdic = NULL;
		}
	}
#if USE_ANSI
	if (dlldic){
		delete dlldic;
		dlldic = NULL;
	}
#endif
	Japa::FreeBuffers();
}

#define	OVERNOREC	1		//欠落単語は登録しない

#ifdef USE_SINGLEBYTE
  #define	JMERGE( destj, srcjp, delim, pdic, singlebyte, henkan, prontable ) \
	destj.Merge( srcjp, delim, pdic, singlebyte, henkan, prontable )
#else
  #define	JMERGE( destj, srcjp, delim, pdic, singlebyte, henkan, prontable ) \
	destj.Merge( srcjp, delim, pdic, henkan, prontable )
#endif

#define	SET_STRQ	0

// srcjpがCutで削られる可能性があるので注意！！！
int Pdic2::record(const tchar *_word, Japa &srcjp )
{
//	DBW("word=%ws",_word);
	if (!pdic){
		setError(DICERR_PARAMETER);
		return -1;
	}
//	const int SingleByte = 0;		// シングルバイト処理！！！

#ifndef	_WINDOWS
	int c;
#endif

	int r;
	int overflg = 0;
	int ret = 0;
#if OVERNOREC
	int force = 0;
#else
	int force = 1;			// 欠落単語があっても登録する
#endif

	bool newrecmode;

	int henkan = 0;
	if ( CFlags & CF_ZEN2HAN ){
		henkan |= HK_ZEN2HAN;
	}
	henkan |= ( CFlags & (CF_CONVDELIM|CF_CONVLEVEL|CF_CONVPRON|CF_CONVWAVE|CF_REMOVEATMARK|TRS_ALL) );

	Japa newj;
	Japa destj;
	Japa *recj = &newj;

	const tchar *word;
	tnstr word_translated;
	tnstr word_buf;

	//CFlags2 |= CF2_ELIM_ILLCHAR;	//TODO: for debug

	const tchar *cword = find_cword_pos(_word);
	if (cword!=_word){
		// Keyword included.
		if (!cword[0]) return 0;	// skip
		word = _word;
		if (henkan&TRS_KEYWORD){
			goto jkeyword_exist;
		}
		// remove the keyword.
		word = cword;
	} else {
		word = _word;
	}

	if (CFlags2 & CF2_ELIM_ILLCHAR){
		word_buf = word;
		elimchar(word_buf.c_str());
		if (word_buf.empty()) return 0;	// skip
		//elimcharspc(word_buf.c_str());
		word = word_buf;
	}

	// No keyword.
	if (KeyTransIndex>=0){
		// Create a keyword.
		word_translated = LangProc->CompositeWord(word);
		word = word_translated;
	} else {
		// No keyword.
	}

jkeyword_exist:;
	
#if MIXDIC
	__kstr __word(word, pdic->GetKCodeTrans());
#else
	#define	__word	word
#endif

	if ( mergemode == MRG_LEVEL ){
		if (pdic->BSearch(__word) < 0){
			setError( pdic->GetErrorCode() );
			return -1;
		}
		if (_kcscmp( pdic->getfword(), __word ) ){
			recj = &srcjp;	// newwordの場合
			newrecmode = true;
			goto jrecword;
		} else {
			// 単語レベルマージ //
			pdic->getfjapa( destj );
			int slev = (srcjp.GetLevel());
			int dlev = (destj.GetLevel());
			if ( slev == dlev || !slev ){	// 同じレベルか、転送元にレベルが無い場合
				return 0;
			}
			else
			{
#ifndef CMD
				// 転送元にレベルがある場合
				// 1995.11.18変更
				if ( inqf && dlev ){	// 転送先にレベルがある場合のみ問合わせ
					r = InquiryLevelMerge( word, slev, dlev );
					if ( r != 1 ){
						return r;
					}
				}
#endif	// !CMD
				return pdic->change_attr( word, (uchar)slev, WA_LEVELMASK ) == -1 ? -1 : 0;
			}
		}
	} else {
		if (pdic->BSearch(__word) < 0){
			setError( pdic->GetErrorCode() );
			return -1;
		}
		if (_kcscmp( pdic->getfword(), __word ) == 0){
			// 日本語訳マージ //
			pdic->getfjapa( destj );
			if ( srcjp == destj ) return 0;
			int fl = 0;
			switch (mergemode){
				case MRG_MERGE:
					newj = destj;
					fl = JMERGE( newj, srcjp, NULL, pdic, SingleByte, henkan, prontable );
#ifndef CMD
					if ( inqf /* && !(srcjp.japa == destj.japa) */ ){
						// 問い合わせあり //
						r = InquiryMerge( word, srcjp, destj, newj, &recj, overflg, pdic );
						if ( r <= 0 )
							return r;
						if ( r == 2 )
							force = 1;
					}
#endif	// !CMD
					break;
				case MRG_APP:
//					destj.Cat( srcjp, NULL, StrDelimiter, CFlags );		// new = src + dest
					fl = JMERGE( destj, srcjp, StrDelimiter, pdic, SingleByte, henkan, prontable );
					recj = &destj;
					break;
				case MRG_APPCR:
					fl = JMERGE( destj, srcjp, _T("\r\n"), pdic, SingleByte, henkan, prontable );
					recj = &destj;
					break;
				case MRG_LONG:		// 長いほうを取る
					if ( _tcslen(srcjp) < _tcslen(destj) )
						return 0;
					recj = &srcjp;
					break;
				case MRG_IGN:		// 無視する(転送先優先)
					return 0;
				case MRG_REP:		// 転送元に置き換える
#ifdef GUI
					if ( inqf ){
						newj = srcjp;
						r = InquiryMerge( word, srcjp, destj, newj, &recj, 0, pdic );
						if ( r <= 0 )
							return r;
						if ( r == 2 )
							force = 1;
					} else
#endif
						recj = &srcjp;
					break;
#if defined(USE_USERDLL)
				case MRG_USER:
					if ( !srcdic->Merge( word, srcjp, destj ) )
						return 0;
					recj = &destj;
					goto jmp1;
#endif
			}
#if defined(USE_USERDLL)
	jmp1:;
#endif
			if ( fl == -1 ){
				setError( 11 );	// メモリ不足
				return -1;
			}
			if ( fl )
				overflg |= 2;
			newrecmode = false;
		} else {
			// new word
			if ( henkan != TRS_ALL ){
				// henkan有りで、すべて転送でない場合のみ、jmergeを使用
				int r;
				r = JMERGE( destj, srcjp, mergemode == MRG_MERGE ? NULL : _T(""), pdic, SingleByte, henkan, prontable );
				if ( r == -1 ){
					setError( 11 );
					return -1;
				} else if ( r ){
					overflg |= 2;
				}
				recj = &destj;
			} else {	// henkan == 0
				recj = &srcjp;
			}
#if SET_STRQ
			// 空の場合は?を付加する
			if ( (*recj)[0] == '\0' ){
				recj->SetJapa( StrQ );
			}
#endif
			newrecmode = true;
		}

		// overrun check //
		if ( force ){
#ifdef NEWDIC2
#if !defined(ND3ONLY)
			if ( pdic->GetVersion() >= DIC_VERSION2 )
#endif
			{
				recj->Cut();
			}
#if !defined(ND3ONLY)
			else {
				recj->CutOld();
			}
#endif
#else
			recj->Cut();
#endif
		} else {
			if (!overflg){
				if ( recj->IsOverFlow() ){
					overflg |= 2;
				}
			}
		}
	jrecword:
		if ( !overflg || force ){
			if (newrecmode){
#ifdef USE_JLINK
				recj->ChangeDic( *pdic );
#endif
				if ( pdic->record( word, *recj, 0 ) == -1 ){		// 要注意！！ 第３パラメータを０にした1996.7.11
					if (pdic->GetErrorCode() == DICERR_LONGWORD){
						// too long word
						overflg |= 1;
						if ( force ){
#if MIXDIC
							__kstr wp(word, pdic->GetKCodeTrans());
#else
							tchar *wp = newstr( word );
#endif
#ifdef NEWDIC2
							unsigned int _LWORD = pdic->GetHeader( )->lword;
#else
							const int _LWORD = LWORD;
#endif
							_kstrcut( wp, _LWORD );
#ifdef USE_JLINK
							recj->ChangeDic( *pdic );
#endif
							if ( pdic->record( wp, *recj ) == -1){
								ret = -1;
							} else
								app++;
#if !MIXDIC
							delete wp;
#endif
						}	// !force
						goto jexit;
					}
					ret = -1;
				} else {
					app++;
				}
			} else {
				if ( pdic->update( word, *recj ) == -1 ){
					ret = -1;
				} else
					mod++;
			}
		}
	}
jexit:;
	if ( overflg ){
		over_flow( word, srcjp );
		def++;
	}
	return ret;
}
// return value
//	1 : normal
//	0 : next
//	-1 : stop
int Pdic2::InquiryLevelMerge( const tchar *word, int slev, int dlev )
{
#ifdef GUI
	switch ( View->SelectWord( word, slev, dlev ) ){
		case mrNext:
			return 0;
		case mrAuto:
			inqf = 0;
			// fall thru
		case mrReplace:
			break;
		default:
			return -1;
	}
#endif
	return 1;
}
// return value
//	2 : force record
//	1 : normal
//	0 : next
//	-1 : stop
int Pdic2::InquiryMerge( const tchar *word, Japa &srcjp, Japa &destj, Japa &newj, Japa **recj, int overflg, Pdic *pdic )
{
#ifdef	GUI
	switch ( View->SelectJapa( word, srcjp, destj, newj, overflg, pdic ) ){
		case mrMerge:
			return 2;
		case mrNext:
			return 0;
		case mrReplace:
			*recj = &srcjp;
			break;
		case mrAuto:
			inqf = 0;
			break;
		default:
			return -1;
	}
#endif
	return 1;
}

void Pdic2::over_flow( const tchar *word, const tchar *srcjapa )
{
	if ( overf.is_open() ){
		overf.putline( word );
		overf.putline( srcjapa );
	}
}

void Pdic2::SetAllSearch( const tchar *word, SrchMode mode )
{
	pdic->SetAllSearch( word, mode, NULL );
}

int Pdic2::readPare( tnstr &word, Japa &japa )
{
	int r;
	if (pdic){
		while ( ( r = pdic->NextAllSearch_( word, &japa ) ) == 0 );
		if ( r != AS_FOUND )
			return -1;
#if 0
		// Remove the keyword if cword==kword.
		const tchar *cword = find_cword_pos(word);
		if (cword!=word.c_str()){
			// cword exists.
			if (!comp_word(cword, word)){
				// kword == cword
				word.set(word, STR_DIFF(cword-1,word.c_str()));
			}
		}
#endif
		return 0;
	}
#if USE_ANSI
	else
	if (dlldic){
		if (dlldic->sizeOfChar()==sizeof(char)){
			CChar cword;
			CJapa cjapa;
			r = ((PdicAnsi*)dlldic)->readPare(cword, cjapa);
			ConvertCode(cctWord, cword, word);
			ConvertCode(cjapa, japa);
			if (!((PdicAnsi*)dlldic)->ConvertObject(cjapa, japa)){
				setError(DICERR_NOMEMORY);	// 手抜き
				return -1;
			}
		} else
		if (dlldic->sizeOfChar()==sizeof(wchar_t)){
			r = ((PdicUni*)dlldic)->readPare(word, japa);
		}
		return r;
	}
#endif
	setError(DICERR_PARAMETER);
	return -1;
}
long Pdic2::length( )
{
	return pdic ? pdic->GetSize() :
#if USE_ANSI
		dlldic ? dlldic->length() :
#endif
		0;
}
int Pdic2::percent( )
{
	return pdic ? pdic->GetPercent() :
#if USE_ANSI
		dlldic ? dlldic->percent() :
#endif
		0;
}
int Pdic2::_getError( )
{
	return pdic ? pdic->GetErrorCode() :
#if USE_ANSI
		dlldic ? dlldic->GetErrorCode() :
#endif
		Dictionary::error;
}
void Pdic2::SetCompFlag(int flag)
{
	if (pdic){
		pdic->SetCompFlag(flag);
	}
	// dlldic can be used only in read-only.
}
#ifdef USE_JLINK
void Pdic2::SetLinkAttr(int attr)
{
	if (pdic){
		pdic->SetLinkAttr(attr);
	}
	// dlldic can be used only in read-only.
}
#endif

///////////////////////////////////////////////////////////////////////////
//	OtherDictionary class
///////////////////////////////////////////////////////////////////////////

OtherDictionary::OtherDictionary( )
	:iof( 0 )
{
	outflag = OF_WORD|OF_JAPA|OF_EXP;
	iof = NULL;
#ifdef _UNICODE
	TransCode = false;
	bom = prof.IsTextFileBOM();
	textmode = prof.GetTextFileCode();
#endif
}

OtherDictionary::~OtherDictionary()
{
	Close( );
}

void OtherDictionary::NonFileMode()
{
#ifdef _UNICODE
	bom = false;
	textmode = -1;
#endif
}

#ifdef GUI
int OtherDictionary::CanOpen( TWinControl *parent, const tchar *fname, int mode )
{
	if ( mode & (FOM_CLIPBOARD|FOM_EDITCONTROL) )
		return 0;

	int openmode = ( mode & FOM_OPENMODEMASK );
	if ( openmode == FOM_READONLY ){
		if ( fexist( fname ) ){
			return 0;
		} else {
			error = ECNoFile;
			return -1;
		}
	} else {
		if ( fexist(fname) ){
			int c;
			if ( parent ){
				c = View->MessageDialog( parent, MAKEINTRESOURCE(APPOVRCANDLG), fname );
			} else c = ID_APPEND;
			switch (c){
				case ID_APPEND:
					if (Open(fname, FOM_WRITE) == -1){
						if ( parent )
							View->ErrorMessage( parent, GetErrorCode(), fname );
						return -1;
					}
					break;
				case ID_NEW:
					if (Open(fname, FOM_CREATE) == -1){
						if ( parent )
							View->ErrorMessage( parent, GetErrorCode(), fname );
						return -1;
					}
					break;
				case ID_OVER:
					if (Open(fname, FOM_CREATE) == -1){
						if ( parent )
							View->ErrorMessage( parent, GetErrorCode(), fname );
						return -1;
					}
					break;
				default:
					return -2;
			}
		} else {
			if (Open(fname, FOM_CREATE) == -1){
				if ( parent )
					View->ErrorMessage( parent, GetErrorCode(), fname );
				return -1;
			}
		}
		Close();
	}
	return 0;
}
#else
#endif

// _filenameがWindow Handleになることがある(FOM_CLIPBOARDの場合
int OtherDictionary::Open( const tchar *_filename, int mode )
{
	Dictionary::Open( _filename, mode );

	int openmode = ( mode & FOM_OPENMODEMASK );
#if defined(_Windows) && defined(PDICW) && !defined(DLL) && !defined(NOGUI)
	if ( mode & FOM_CLIPBOARD ){
		if ( openmode == FOM_READONLY ){
			iof = new TCBIFile;
		} else {
			iof = new TCBOFile;
		}
	} else
#ifdef GUI
	if ( mode & FOM_EDITCONTROL ){
		if ( openmode != FOM_READONLY ){
			iof = new TEditOFile;
		} else {
			iof = new TEditIFile;
		}
	} else
	if (mode & FOM_RICHEDITCONTROL){
		if ( openmode != FOM_READONLY ){
			// unsupported.
			//iof = new TRichEditOFile;
			__assert__;
			return -1;
		} else {
			iof = new TRichEditIFile;
		}
	} else 
#endif
	if ( mode & FOM_CHAR ){
		if ( openmode != FOM_READONLY ){
			iof = new TCharOFile;
		} else {
			// iof = new TCharIFile;	// not yet supported
			return -1;
		}
	} else
#endif
	{
		if ( openmode != FOM_READONLY ){
			iof = new TOFile;
		} else {
			iof = new TIFile;
		}
	}
	if ( openmode & FOM_CREATE )
		iof->create( _filename );
	else
		iof->open( _filename );
	if ( !(*iof) ){
		error = ECOpenError;
		delete iof;
		iof = NULL;
		return -1;
	}
	switch ( openmode ){
		case FOM_READONLY:
			iof->home();
#if USE_ANSI
			// 文字コードの判定
			if ( !((TCBIFile*)iof)->isunicode() ){
				TransCode = true;
			}
#endif
			break;
		case FOM_WRITE:
			iof->end();
		case FOM_CREATE:
#ifdef _UNICODE
			if (!TransCode&&(((TOFile*)iof)->tell()==0)){
				// Unicode出力
				if ((mode & FOM_FILE) || !(mode & (FOM_CLIPBOARD|FOM_EDITCONTROL|FOM_CHAR|FOM_RICHEDITCONTROL)))
				{
					if (textmode>=0)
						((TOFile*)iof)->settextmode(textmode);
					if (bom)
						((TOFile*)iof)->bom();
				}
			}
#endif
			break;
	}
	return 0;
}

void OtherDictionary::Close( )
{
	if ( iof ){
		iof->close();
		delete iof;
		iof = NULL;
	}
}

// 拡張PDICテキスト形式(改行を許す）
// 最後の改行は付加されないので呼出側で出力すること！！
int OtherDictionary::PutMultiLine( const tchar *p )
{
	while ( *p ){
		if ( *p == CharCR ){
			p++;
			if ( *p == CharLF ){
				p++;
			}
	jmp1:
			((TOFile*)iof)->put( EXTCRCHAR );
			continue;
		}
		if ( *p == CharLF ){
			p++;
			goto jmp1;
		}
		if ( ((TOFile*)iof)->put( *p ) == -1 ){
			return -1;
		}
		p++;
	}
	return 0;
}

int OtherDictionary::PutText( Japa &japa, BOOL fExtPdicText )
{
	int r = 0;
	if ( (outflag & OF_PRON) && japa.pron[0] ){
		r |= tof().put( _T("[") );
		r |= tof().put( japa.pron );
		r |= tof().put( _T("]") );
	}
	if ( outflag & OF_LEVEL ){
		tchar s[WA_LEVELNUM+7];
		int i = 0;
		s[i++] = '<';
		for ( int j=0;j<japa.GetLevel();j++ ){
			s[i++] = '*';
		}
		s[i++] = '>';
		s[i++] = ' ';
		s[i] = '\0';
		r |= tof().put( s );
	}
	if ( fExtPdicText ){
		// 拡張PDICテキスト形式(改行を許す）
		r |= PutMultiLine( japa.japa );
	} else {
		if ( outflag & OF_JAPA )
			r |= ((TOFile*)iof)->put( japa );
	}
	if ( (outflag & OF_EXP) && japa.exp[0] ){
		if ( outflag & OF_JAPA )
			r |= ((TOFile*)iof)->put( StrExpSepa );
		if ( fExtPdicText ){
			// 拡張PDICテキスト形式(改行を許す）
			r |= PutMultiLine( japa.exp );
		} else {
			r |= ((TOFile*)iof)->put( japa.exp );
		}
	}
	r |= ((TOFile*)iof)->put( CharLF );
	if ( r ){
		return -1;
	}
	return 0;
}

int OtherDictionary::getlineT(tnstr &word, TCCTable *cct)
{
	string line;
	int ret = tif().getline(line);
	if (ret<0)
		return ret;
	ConvertCode(cct, line.c_str(), word);
	return 1;
}

//////////////////////////////////////////////////////////////////////////////
// Perd12 class (PDIC Text Format)
//////////////////////////////////////////////////////////////////////////////
Perd12::Perd12( )
{
	dictype = DT_PDICTEXT;
}

#define USEEXT_ONPDICTEXT	0	// 拡張PDICテキスト形式

int Perd12::record(const tchar *word, Japa &japa )
{
	if ( outflag & OF_WORD ){
		if ( ((TOFile*)iof)->putline( find_cword_pos(word) ) )
			return -1;
	} else {
		if ( ((TOFile*)iof)->put( CharLF ) )
			return -1;
	}
#if USEEXT_ONPDICTEXT	// 拡張PDICテキスト形式
	if ( japa.IsEmptyEx() ){
		if ( PutMultiLine( *(TOFile*)iof, japa.japa ) ){
			error = ECWriteError;
			return -1;
		}
	} else {
		if ( PutText( (TOFile*)iof, japa, FALSE ) == -1 ){
			error = ECWriteError;
			return -1;
		}
	}
#else
#if 0	// 1997.1.15
	if ( japa.IsEmptyEx() ){
		if ( ((TOFile*)iof)->putline( japa ) ){
			return -1;
		}
	} else
#endif
	{
		if ( PutText( japa, FALSE ) == -1 ){
			return -1;
		}
	}
#endif
	return 0;
}

int Perd12::getWord( tnstr &word )
{
#if USE_ANSI
	if ( TransCode ){
		return getlineT(word, cctWord);
	}
#endif
	return (int) tif().getline( word );
}

int Perd12::getJapa( Japa &japa )
{
#if USEEXT_ONPDICTEXT
	tnstr line;
	tnstr buf;
	int r = 0;
	for (;;){
#if USE_ANSI
		if ( TransCode ){
			r = getlineT( line, cctJapa );
		} else
#endif
		r = tif().getline( line );
		if ( r <= 0 ){
			break;
		}
#if 1 // "\"で改行
		if ( line[r-1] == '\\' && ( r == 1 || !_ismbblead( line[r-2] ) ) ){
#ifdef PDICW
			line[r-1] = '\r';	// Windows版
			buf.cat( line );
			buf.cat( "\n" );
#else
			line[r-1] = '\0';	// DOS版
			buf.cat( line );
#endif
		}
#else
		// " \ "で改行
		// 未完成
#endif
		else {
			buf.cat( line );
			break;
		}
	}
#else
	tnstr buf;
	int r;
#if USE_ANSI
	if ( TransCode ){
		r = getlineT( buf, cctJapa );
	} else
#endif
	r = tif().getline( buf );
#endif
	japa.SetAll2( buf, CFlags & CF_DISTINCT ? TRUE : FALSE );
	return r;
}

//////////////////////////////////////////////////////////////////////////////
// １行テキスト形式
//////////////////////////////////////////////////////////////////////////////
ExtPdicText::ExtPdicText( )
{
	dictype = DT_EXTPDICTEXT;
}

ExtPdicText::~ExtPdicText( )
{
}

int ExtPdicText::record( const tchar *word, Japa &japa )
{
	if ( ((TOFile*)iof)->put(find_cword_pos(word)) == -1){
		return -1;
	}
	((TOFile*)iof)->put( StrOneLineDelim );
#if 0	// これがあるのは高速化のため？
		// レベルがあった場合、正しく出力されないので削除(1997.7.8)
	if ( japa.IsEmptyEx() ){
		if ( PutMultiLine( japa.japa ) ){
			error = ECWriteError;
			return -1;
		}
		if ( ((TOFile*)iof)->put( CharLF ) ){
			error = ECWriteError;
			return -1;
		}
	} else
#endif
	{
		if ( PutText( japa, TRUE ) == -1 ){
			error = ECWriteError;
			return -1;
		}
	}
	return 0;
}

int ExtPdicText::readPare( tnstr &word, Japa &japa )
{
	tnstr line;
    // 空行を飛ばす
	while ( 1 ){
#if USE_ANSI
		if ( TransCode ){
			//TODO: It's too difficult...
			if ( tif().getlineA( line ) < 0 )
				return -1;
		} else
#endif
		if ( tif().getline( line ) < 0 ){
			return -1;
		}
		if ( line[0] )
			break;
	}
	japa.clear();
	const tchar *p = _tcsstr( line, StrOneLineDelim );
	if ( p ){
		word.set( line, STR_DIFF( p, (const tchar *)line ) );
		GetMultiPdicText( (tchar*)p + _tcslen(StrOneLineDelim), japa, CFlags & CF_DISTINCT ? TRUE : FALSE );
	} else {
		word.set( line );
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////////
// １行テキスト形式（Japaが空の場合はwordのみ）
//////////////////////////////////////////////////////////////////////////////
int ExtPdicText2::record( const tchar *word, Japa &japa )
{
	if (!japa.IsEmpty()) return super::record(word, japa);
	return tof().putline( find_cword_pos(word) );
}

//////////////////////////////////////////////////////////////////////////////
// WX2形式
//////////////////////////////////////////////////////////////////////////////
WX2::WX2( )
{
	dictype = DT_WX2TEXT;
}


int WX2::getWord( tnstr &word )
{
	unsigned short c;
	unsigned short code;
	VarBuffer _word;
	word.clear();

	tnstr buf;
	// 空行を飛ばす
	for (;;){
#if USE_ANSI
		if ( TransCode ){
			if ( tif().getlineA( buf ) < 0 )
				return -1;
		} else
#endif
		if ( tif().getline( buf ) < 0 )
			return -1;
		if ( buf[0] )
			break;
	}
	const tuchar *p = buf;
	int n = LWORD;
	for (;;){
		c = *p++;
		if (c == '\0' || c == '\t' || c == '"'){
			jp = p;
			_word.add( (tchar)'\0' );
			word.set( _word );
			return 0;
		}
		if (_ismbblead(c)){
			code = (ushort)((c<<8) + *p++);
			if (n <= 2) continue;
			if (code >= CODE_L_A && code <= CODE_L_Z )
				code = (ushort)(code - CODE_L_A + 'a');
			else if (code >= CODE_S_A && code <= CODE_S_Z )
				code = (ushort)(code - CODE_S_A + 'a');
			else if (code == CODE_APOSTROPHE )
				code = 0x27;		// '
			else if (code == CODE_PERIOD)
				code = '.';
			else if (code == CODE_SPACE )	// 全角スペース
				code = ' ';

			if (code > 0x100){
				_word.add( (char)(code>>8) );
				_word.add( (char)(code&0xff) );
				n -= 2;
				continue;
			}
			c = code;
		}
		if (n <= 1) continue;
		_word.add( c );
		n--;
	}
}

int WX2::getJapa( Japa &japa )
{
	unsigned short c;
	VarBuffer _japa;

	const tuchar *p = jp;
	int n = LJAPA;
	for (;;){
		c = *p++;
		switch (c){
			case '\0':
			case ':':
				_japa.add( (tchar)'\0' );
				japa.SetJapa( _japa );
				return 0;
			case '\t':
			case '"':
				continue;
			case ' ':
				while (*p == ' ') p++;
				if (n <= 1) continue;
				_japa.add( ',' );
				n--;
				continue;
			case 0x81:
				if ( *p == 0x40 ){	// 全角スペース
					p++;
					if ( n <= 1 ) continue;
					_japa.add( ',' );
					n--;
					continue;
				}
                break;
		}
		if (n <= 1) continue;
		if ( _ismbblead(c) ){
			_japa.add( c );
            c = *p++;
			n--;
		}
		_japa.add( c );
		n--;
	}
}

#ifdef	_WINDOWS
// 取敢えず、WX2形式出力は_WINDOWS版のみ対応
//	WX2テキスト形式では、用例は出力されない！！
//	日本語訳がない場合は出力されない
int WX2::record( const tchar *word, Japa &japa )
{
	int l = _tcslen( japa.GetJapa() );
	if ( l == 0 )
		return 0;
	l += _tcslen( word );
	tchar *buf = new tchar[ l + 10 ];
#ifdef	_WINDOWS
	wsprintf( 
#else
	esprintf( 
#endif
		buf, _T("%s\t\"%s\":名詞"), (const tchar *)word, (const tchar *)japa.GetJapa()
	);
	if ( tof().putline( buf ) == -1){
		delete[] buf;
		error = ECWriteError;
		return -1;
	}
	delete[] buf;
	return 0;
}

#endif

//////////////////////////////////////////////////////////////////////////////
// WLevel class
//////////////////////////////////////////////////////////////////////////////
WLevel::WLevel( )
{
	dictype = DT_LEVEL;
}

int WLevel::readPare( tnstr &word, Japa &japa )
{
	tnstr buf;
	const tchar *np;	// 数値のある位置
	const tchar *wp;	// 単語のある位置
	int l;
	while ( 1 ){
#if USE_ANSI
		if ( TransCode ){
			if ( getlineT( buf, cctWord ) < 0 )
				return -1;
		} else
#endif
		if ( tif().getline( buf ) < 0 )
			return -1;
		l = 0;
		np = &buf[0];
		while ( *np ){
			if ( isdigit(*np) ){
				l = l * 10 + *np - '0';
				np++;
				continue;
			}
			break;
		}
		if ( np != &buf[0] && ( np[0] == ' ' || np[0] == '\t' ) ){
			if ( l > WA_MAXLEVEL )
				continue;
			// 赤瀬川さん形式
			wp = np+1;
			while ( *wp ){
				if ( *wp == ' ' || *wp == '\t' ){
					wp++;
					continue;
				}
				break;
			}
			if ( *wp ){
				break;
			}
		} else {
			// ANET形式
			tchar *sp = buf.c_str();
			tchar *dlm = NULL;	// ,記号
			tchar *spc = NULL;	// 最初のスペース,TAB記号
			np = NULL;
			while ( *sp ){
				if ( isdigit(*sp) ){
					np = sp;
					if ( spc || dlm )
						break;
				} else if ( *sp == ',' ){
					dlm = sp;
				} else if ( !spc && ( *sp == ' ' || *sp == '\t' ) ){
					spc = sp;
				} else {
					// 上記以外の文字ではspcをキャンセル
					spc = NULL;
				}
				sp = GetNextPtr( sp );
			}
			if ( !np )
				continue;
			l = _ttoi( np );
			if ( l > WA_MAXLEVEL )
				continue;
			if ( dlm ){
				*dlm = '\0';
			} else if ( spc ){
				*spc = '\0';
			} else {
				*((tchar*)np-1) = '\0';	// ここにくることはまず無いはずだが
			}
			wp = buf;
			break;
		}
	}
	word.set( wp );
	japa.clear();
	japa.SetLevel(l);
	japa.japa.set( StrQ );
	return 0;
}

int WLevel::record( const tchar *word, Japa &japa )
{
	int level = japa.GetLevel();
	if ( level == 0 )
		return 0;
	tnstr buf;
	tchar s[4];
	_itot( level, s, 10 );
	if ( mergemode == MRG_ANET ){
		buf.set( word );
		buf.cat( _T(",") );
		buf.cat( s );
	} else {
		buf.set( s );
		buf.cat( _T("\t") );
		buf.cat( word );
	}
	if ( tof().putline( buf ) == -1){
		error = ECWriteError;
		return -1;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
// TextFile class
//////////////////////////////////////////////////////////////////////////////
TextFile::TextFile( )
{
	LineCount = 0;
}

#ifdef	PDICW
//////////////////////////////////////////////////////////////////////////////
// TSVFile class
//////////////////////////////////////////////////////////////////////////////
void TSVtoStr( const tchar *line, tnstr_vec &item, int maxitem, bool &openbrace, tchar delim );
tchar *StrtoTSV( const tchar *str, tchar *csv, bool ini_quote, tchar delim);
TSVFile::TSVFile()
{
	dictype = DT_TSV;
	Delim = '\t';
	IniQuote = 0;
	LineBufLen = max(max(max(LWORD,LJAPA),LEXP),LPRON)*2;
	LineBuf = new tchar[ LineBufLen ];
}
TSVFile::~TSVFile()
{
	if (LineBuf)
		delete[] LineBuf;
}
int TSVFile::Open( const tchar *filename, int mode )
{
	int r = super::Open(filename, mode);
	if (r)
		return r;	// error
	FirstLine = true;
	NumTrs = 0;
	return r;
}

// item name list
struct TItemNameInfo {
	int trs;
	const tchar *name;
};

// temporary TRS IDs
#define	__TRS_WORD		0x40000000
#define	__TRS_MEMORY	0x10000000
#define	__TRS_MODIFY	0x08000000
#define	__TRS_FILELINK	0x04000000
//Note:
// 項目の順番はdeafult headerとなる。
static const TItemNameInfo ItemNameInfo[] = {
	{TRS_KEYWORD, _t("keyword")},
	{__TRS_WORD, _t("word")},
	{TRS_JAPA, _t("trans")},
	{TRS_EXP, _t("exp")},
	{TRS_LEVEL, _t("level")},
	{__TRS_MEMORY, _t("memory")},
	{__TRS_MODIFY, _t("modify")},
	{TRS_PRON, _t("pron")},
	{__TRS_FILELINK, _t("filelink")},
	{0}
};

int TSVFile::readPare( tnstr &word, Japa &japa )
{
	tnstr line;
	bool openbrace = false;
	tnstr_vec item;
	bool add_space = false;
	while ( 1 ){
		int r;
#if USE_ANSI
		if ( TransCode ){
			r = tif().getlineA( line );
		} else
#endif
		{
			r = GetLine( line );
		}
		LineCount++;
		if ( r < 0 )
			return -1;
		if (openbrace){
			// 前の行の最後のitemと現在行を結合する
			int last = item.get_num()-1;
			__assert(item.get_num()>0);
			if (last==WordCol || last==KeywordCol){
				//item[item.get_num()-1] += _T(" ");	// alternative char.
				add_space = true;
			} else {
				item[item.get_num()-1] += _T("\r\n");	// 見出語以外であれば改行コードOK
			}
		} else {
			if ( r == 0 )
				continue;
		}
#ifdef _DEBUG
//		DBW("count=%d line=%ws", LineCount, line.c_str());
#endif
		const int MaxReadItems = 32;	// 読み込む最大項目数
		TSVtoStr( line, item, MaxReadItems, openbrace, Delim );
		if (item.get_num() == 0)
			continue;
		if (openbrace){
			if (item.get_num()>=MaxReadItems){
				// over max item and open brace -> not supported.
				error = ECReadFormat;
				return -1;
			}
			// ""が閉じていない→継続行ありと判断
			continue;
		}

		if (FirstLine){
			FirstLine = false;
			if (readHeader(item)){
				// header exists.
				item.clear();
				continue;
			}
		}
		tnstr keyword;
		for (int i=0;i<NumTrs;i++){
			if (ItemCol[i]<item.get_num()){
				// column exists.
				const tchar *s = item[ItemCol[i]];
				switch (ItemTrs[i]){
					case __TRS_WORD:
						word = s;
						if (word.empty()){
							goto jnext;	// skip the line
						}
						break;
					case TRS_KEYWORD:
						keyword = s;
						break;
					case TRS_JAPA:
						japa.SetJapa(s);
						break;
					case TRS_EXP:
						japa.SetExp(s);
						break;
					case TRS_LEVEL:
						japa.SetLevel(_ttoi(s)&WA_LEVELMASK);
						break;
					case __TRS_MEMORY:
						if (_ttoi(s)){
							japa.SetMemory();
						}
						break;
					case __TRS_MODIFY:
						if (_ttoi(s)){
							japa.SetModify();
						}
						break;
					case TRS_PRON:
						japa.SetPron(s);
						break;
#ifdef USE_FILELINK
					case __TRS_FILELINK:
						{
							tnstr_vec objects;
							const int MaxObjects = 256;	// 最大ファイルリンク数
							const int MaxFileSize = 0x100000*32;	// 最大ファイルサイズを32MBに制限
							bool openbrace = false;
							const tchar delim_filename = ';';
							const tchar delim_object = ',';
							TSVtoStr( s, objects, MaxObjects, openbrace, delim_object );
							//__assert(!openbrace);
							if (openbrace){
								error = ECReadFormat;
								return -1;
							}
							for (int i=0;i<objects.get_num();i++){
								tnstr_vec objitems;
								const int MaxObjItems = 2;
								TSVtoStr(objects[i], objitems, MaxObjItems, openbrace, delim_filename);
								//__assert(!openbrace);
								if (openbrace){
									error = ECReadFormat;
									return -1;
								}
								if (objitems[0].exist()){
									const tchar *filename = objitems[0];
									JLFileCommon *jl;
									if (filename[0]=='+'){
										const tchar *rfilename = filename+1;
										tnstr filepath;
										if (!IsFullPath(rfilename)){
											filepath = View->GetFLinkCPath();
										}
										tnstr fullname;
										bool notfound = false;
										while (1){
											if (!notfound && IsFullPath(rfilename)){
												fullname = rfilename;
											} else {
												fullname = AddYen(filepath) + rfilename;
											}
											if (fexist(fullname)){
												break;
											}
											notfound = true;
											filepath = View->GetFLinkCPath(rfilename, false);
											if (filepath.empty())
												return -1;	// cancel
										}
										lsize_t fsize = filesize(fullname);
										if (fsize>=MaxFileSize){
											error = ECTooLong;
											return -1;
										}
										jl = new JLFileImage(NULL, fullname, 0);
									} else {
										jl = new JLFile(NULL, filename, 0);
									}
									if (objitems.get_num()>=2 && objitems[1].exist()){
										jl->SetTitle(objitems[1]);
									}
									japa.jlinks.add(jl);
								}
							}
							
						}
						break;
#endif
#ifdef _DEBUG
					default:
						__assert__;
						break;
#endif
				}
			}
		}
		if (keyword.exist()){
			word = join_word(word, keyword);
		}
		return 0;
jnext:;
	}
}
#ifdef USE_JLINK
static bool JLinkToStr(JLinks &jlinks, tchar *LineBuf, int LineBufLen, tchar Delim, TUtyDicView *view);
#endif

int TSVFile::record(const tchar *word, Japa &japa )
{
	if (FirstLine){
		FirstLine = false;
		if (!writeHeader()){
			error = ECWriteError;
			super::Close();
			return -1;
		}
	}
	tchar *dp;
	bool output = false;	// output one or more items.
	if (CFlags & TRS_KEYWORD){
		tnstr kword;
		split_word(word, NULL, &kword);
		dp = StrtoTSV(kword, LineBuf, IniQuote&TRS_KEYWORD, Delim);
		*dp = '\0';
		output = true;
		if (tof().put(LineBuf)==-1){
			goto jerror;
		}
	}
	for(;;){
		const tchar *cword = find_cword_pos(word);
		dp = LineBuf;
		if (output){
			*dp++ = Delim;
		}
		dp = StrtoTSV(cword, dp, IniQuote&__TRS_WORD, Delim);
		*dp = '\0';
		if ( tof().put( LineBuf ) == -1 ){
			goto jerror;
		}
		if (CFlags & TRS_JAPA){
			dp = LineBuf;
			*dp++ = Delim;
			dp = StrtoTSV(japa.japa, dp, IniQuote&TRS_JAPA, Delim);
			*dp = '\0';
			if ( tof().put( LineBuf ) == -1 ){
				goto jerror;
			}
		}
		if (CFlags & TRS_EXP){
			dp = LineBuf;
			*dp++ = Delim;
			dp = StrtoTSV(japa.exp, dp, IniQuote&TRS_EXP, Delim);
			*dp = '\0';
			if ( tof().put( LineBuf ) == -1 ){
				goto jerror;
			}
		}
		if (CFlags & TRS_LEVEL){
			dp = LineBuf;
			*dp++ = Delim;
			_itot( japa.GetLevel(), dp, 10 );
			dp += _tcslen(dp);
			*dp = '\0';
			if ( tof().put( LineBuf ) == -1 ){
				goto jerror;
			}
		}
		if (CFlags & TRS_ATTR){
			dp = LineBuf;
			*dp++ = Delim;
			*dp++ = japa.IsMemory() ? '1' : '0';
			*dp++ = Delim;
			*dp++ = japa.IsEdit() ? '1' : '0';
			*dp = '\0';
			if ( tof().put( LineBuf ) == -1 ){
				goto jerror;
			}
		}
		if (CFlags & TRS_PRON){
			dp = LineBuf;
			*dp++ = Delim;
			dp = StrtoTSV(japa.pron, dp, IniQuote&TRS_PRON, Delim);
			*dp = '\0';
			if ( tof().put( LineBuf ) == -1){
				goto jerror;
			}
		}
#ifdef USE_JLINK
		if (CFlags & TRS_OBJECT){
			LineBuf[0] = '\0';
			if (!JLinkToStr(japa.jlinks, LineBuf, LineBufLen, Delim, View)){
				// cancel
				return -1;
			}
			if (LineBuf[0]){
				if ( tof().put( LineBuf ) == -1 ){
					goto jerror;
				}
			}
		}
#endif
		if (tof().putline(_t(""))==-1){
			goto jerror;
		}
		break;
	}
	return 0;
jerror:
	error = ECWriteError;
	return -1;
}

#ifdef USE_JLINK
static
bool JLinkToStr(JLinks &jlinks, tchar *LineBuf, int LineBufLen, tchar Delim, TUtyDicView *view)
{
	tchar *dp = LineBuf;
	tchar *dp_top;
//	tchar *dp_end = LineBuf + LineBufLen;
	const tchar delim_filename = ';';
	const tchar delim_object = ',';
	tnstr s;
	for (int i=0;i<jlinks.get_num();i++){
		JLink &jl = jlinks[i];
		switch (jl.GetType()){
#ifdef USE_FILELINK
			case JL_FILE:
			case JL_FILEIMAGE:
				if (dp!=LineBuf){
					*dp++ = delim_object;
				}
				dp_top = dp;
				if (jl.GetType()==JL_FILEIMAGE){
					*dp++ = '+';	// file image mark
				}
				s = ((JLFile&)jl).GetFileName();
				dp = StrtoTSV(s, dp, false, delim_filename);
				s = jl.GetTitle();
				if (s.exist()){
					*dp++ = delim_filename;
					dp = StrtoTSV(s, dp, false, delim_filename);
				}
				*dp = '\0';
				s = dp_top;
				dp = StrtoTSV(s, dp_top, false, delim_object);
				*dp = '\0';
				if (jl.GetType()==JL_FILEIMAGE){
					// save file image
					JLFileImage &jlf = (JLFileImage&)jl;
					tnstr fullname;
					tnstr basename = GetFileName(jlf.GetFileName());
					int counter = -1;
					tnstr filepath = view->GetFLinkCPath();
					if (filepath.empty())
						return false;	// cancel
					_tmkdir(filepath);
					for (;;){
						tnstr filename = basename;
						if (counter>=0){
							const tchar *divp = _tcschr(basename, '.');
							if (divp){
								tnstr base(basename, STR_DIFF(divp,basename.c_str()));
								filename = tnsprintf(_t("%s_%03d.%s"), base.c_str(), counter, divp+1);
							} else {
								filename = tnsprintf(_t("%s_%03d"), basename.c_str(), counter);
							}
						}
						fullname = MakePath(filepath, filename);
						if (!fexist(fullname)){
							// no file exists -> OK.
							if (!jlf.SaveToFile(fullname)){
								filepath = view->GetFLinkCPath(filename, true);
								_tmkdir(filepath);
								if (filepath.empty())
									return false;	// cancel
								continue;	// retry
							}
							break;	// save OK
						}
						counter++;
					}
				}
				break;
#endif
			default:
				continue;
		}
	}
	if (dp!=LineBuf){
		s = LineBuf;
		dp = LineBuf;
		*dp++ = Delim;
		dp = StrtoTSV(s, dp, true, Delim);
		*dp = '\0';
	}
	return true;
}
#endif	// USE_JLINK

bool TSVFile::readHeader(tnstr_vec &items)
{
	// Scan header.
	NumTrs = 0;
	WordCol = -1;
	KeywordCol = -1;
	int trs = 0;
	for (int i=0;i<items.get_num();i++){
		if (NumTrs>=MaxTrs){
			break;
		}
		for (int j=0;;j++){
			const TItemNameInfo &info = ItemNameInfo[j];
			if (!info.trs)
				break;
			if (!_tcscmp(items[i], info.name)){
				// find an item.
				ItemTrs[NumTrs] = info.trs;
				ItemCol[NumTrs] = i;
				NumTrs++;
				trs |= info.trs;
				switch (info.trs){
					case __TRS_WORD:
						WordCol = i;
						break;
					case TRS_KEYWORD:
						KeywordCol = i;
						break;
				}
			}
		}
	}
	// check the must item.
	if (trs & __TRS_WORD){
		return true;
	}
	// Use the default header.
	tnstr_vec defitems;
	for (int i=0;;i++){
		const TItemNameInfo &info = ItemNameInfo[i];
		if (!info.trs)
			break;
		if (info.trs & TRS_KEYWORD)
			continue;	// keywordは除外
		defitems.add(info.name);
	}
	readHeader(defitems);
	return false;	// no header in items.
}
bool TSVFile::writeHeader()
{
	int cflags = CFlags | __TRS_WORD;	// word is always output.
	if (CFlags & TRS_ATTR){
		cflags |= __TRS_MEMORY|__TRS_MODIFY;
	} else {
		cflags &= ~(__TRS_MEMORY|__TRS_MODIFY);
	}
	if (CFlags & TRS_OBJECT){
		cflags |= __TRS_FILELINK;
	} else {
		cflags &= ~(__TRS_FILELINK);
	}
	tchar item_name[30];
	bool first = true;
	for (int i=0;;i++){
		const TItemNameInfo &info = ItemNameInfo[i];
		if (!info.trs)
			break;
		if (cflags & info.trs){
			if (first){
				first = false;
			} else {
				if (tof().put(Delim)<0){
					return false;
				}
			}
			tchar *dp = StrtoTSV(info.name, item_name, false, Delim);
			*dp = '\0';
			if (tof().put(item_name)<0){
				return false;
			}
		}
	}
	return tof().putline(_t(""))>=0;
}

// TSV/CSVファイル

// TSV/CSV形式をitemへ
// maxitem : 最大取得項目数（それ以降は無視)
// 返り値:取得項目数 -> deleted.
// openbrace : 継続行あり（""が閉じていない）
void TSVtoStr( const tchar *line, tnstr_vec &item, int maxitem, bool &openbrace, tchar delim )
{
	const tchar *p = line;
	tchar *buf = new tchar[ _tcslen( p ) + 1 ];
	ushort c;
	bool cont_open = false;
	if (openbrace){
		cont_open = true;	// continuous open brace.
		goto jopen;
	}
	for ( ;item.get_num()<maxitem; ){
		tchar *q;
		LD_CHAR( c, p );
		if ( !((tuchar)c) )
			break;
		if (delim==','){
			if ( (tuchar)c == (tuchar)' ' || (tuchar)c == (tuchar)'\t' ){
				continue;
			}
		}
		if ( c == '"' ){
jopen:;
			q = buf;
			for ( ;; ){
				LD_CHAR( c, p );
				if ( !((tuchar)c) ){
					openbrace = true;	// 対応する右括弧が無い
					break;
				}
				if ( c == '"' ){
					if ( *p == delim ){
						p++;
					} else if ( (tuchar)*p == (tuchar)'"' ){
						p++;
						goto j1;
					}
					openbrace = false;
					break;
				}
		j1:
				ST_CHAR( c, q );
			}
			*q = '\0';
			if (cont_open){
				item[item.get_num()-1] += buf;
				cont_open = false;
			} else {
				item.add( buf );
			}
		} else if ( c == delim ){
			item.add( new tnstr );
		} else {
			// ""で括られていない場合、次のdelimまで
			q = buf;
			ST_CHAR( c, q );
			for ( ;; ){
				LD_CHAR( c, p );
				if ( !((tuchar)c) || c == delim )
					break;
				ST_CHAR( c, q );
			}
			*q = '\0';
			item.add( buf );
		}
		if ( !((tuchar)c) ){
			break;
		}
	}
	delete[] buf;
	//return item.get_num();
}

tchar *StrtoTSV( const tchar *str, tchar *csv, bool ini_quote, tchar delim)
{
	const tchar *str_org = str;
	tchar *csv_org = csv;
	bool quoted = ini_quote;
	if (quoted){
		*csv++ = '"';
	}
	for (;;){
		while ( *str ){
			tchar c = *str++;
			if (c=='"'){
				if (!quoted)
					goto jquote;
				*csv++ = '"';
			} else
			if (c=='\r' || c=='\n' || c=='\t'){
				if (!quoted)
					goto jquote;
#if 0
				if (c=='\r'){	// drop CR
					continue;
				}
#endif
			}
			if (!quoted){
				if (c==delim)
					goto jquote;
			}
			*csv++ = c;
			const tchar *_str = NEXT_CHAR( str-1 );
			while ( _str != str ) *csv++ = *str++;
		}
		break;
jquote:
		quoted = true;
		str = str_org;
		csv = csv_org;
		*csv++ = '"';
	}
	if (quoted)
		*csv++ = '"';
	return csv;
}

//////////////////////////////////////////////////////////////////////////////
// CSVFile class
//////////////////////////////////////////////////////////////////////////////
CSVFile::CSVFile( )
{
	dictype = DT_CSV;
	Delim = ',';
	IniQuote = ~(TRS_LEVEL|__TRS_MEMORY|__TRS_MODIFY);	// 数値項目のみ列挙
}
CSVFile::~CSVFile( )
{
}
#endif	// PDICW

//////////////////////////////////////////////////////////////////////////////
// Utf8File class
//////////////////////////////////////////////////////////////////////////////
int Utf8File::Open( const tchar *filename, int mode )
{
	bom = false;
	textmode = -1;
	return super::Open(filename, mode);
}
bool Utf8File::putline(const char *text)
{
	//TODO: 効率が悪い
	if (tof().putbin(text, strlen(text))<0)
		return false;
	return tof().putbin("\n", 1)>=0;
}

bool Utf8File::put(const char *text, int len)
{
	//TODO: 効率が悪い
	return tof().putbin(text, strlen(text))>=0;
}

bool Utf8File::putxml(const char *text)
{
	__tnstrbufA buf;
	const char *p = text;
	const char *rep = NULL;
	while (1){
		char c = *p;
		if (!c)
			break;
		if (c=='<'){
			rep = "&lt;";
		} else
		if (c=='>'){
			rep = "&gt;";
		} else
		if (c=='&'){
			rep = "&amp;";
#if 0
		} else
		if (c=='"'){
			rep = "&quot;";
		} else
		if (c=='\''){
			rep = "&apos;";
#endif
		} else {
			p++;
			continue;
		}
		if (p!=text)
			buf.cat(text, (int)(p-text));
		buf.cat(rep);
		text = p = p+1;
	}
	if (rep){
		if (p!=text)
			buf.cat(text, (int)(p-text));
		return put(buf, buf.length());
	} else {
		return put(text);
	}
}

bool Utf8File::putxml(const tchar *text)
{
	__tnstrA buf(text, true);
	return putxml(buf);
}

//////////////////////////////////////////////////////////////////////////////
// PdicXmlFile class
//////////////////////////////////////////////////////////////////////////////
#ifdef _Windows
#include "inetdicif.h"
PdicXmlFile::PdicXmlFile()
{
	InetInfo = NULL;
}
PdicXmlFile::~PdicXmlFile()
{
	if (InetInfo)
		delete InetInfo;
}
int PdicXmlFile::Open( const tchar *filename, int mode )
{
	int r = super::Open(filename, mode);
	if (r)
		return r;	// error
	FirstLine = true;
	return 0;
}
void PdicXmlFile::Close( )
{
	writeFooter();
	super::Close();
}
int PdicXmlFile::record(const tchar *word, Japa &japa )
{
	if (FirstLine){
		FirstLine = false;
		if (!writeHeader()){
			error = ECWriteError;
			return -1;
		}
	}
	char buf[10];
	putline("<record>");
	put("  <word>"); putxml(find_cword_pos(word)); putline("</word>");
	if ((CFlags & TRS_JAPA) && japa.japa.exist()){
		put("  <trans>"); putxml(japa.japa); putline("</trans>");
	}
	if ((CFlags & TRS_EXP) && japa.exp.exist()){
		put("  <ex>"); putxml(japa.exp); putline("</ex>");
	}
	if ((CFlags & TRS_PRON) && japa.pron.exist()){
		put("  <pron>"); putxml(japa.pron); putline("</pron>");
	}
	if ((CFlags & TRS_LEVEL) && japa.GetLevel()>0){
		itoa(japa.GetLevel(), buf, 10);
		put("  <level>"); super::putxml(buf); putline("</level>");
	}
	if ((CFlags & TRS_ATTR) && japa.IsMemory()){
		put("  <memory>1</memory>");
	}
	return putline("</record>");
}
void PdicXmlFile::SetCryptKey(const char *key)
{
	if (InetInfo){
		delete InetInfo;
		InetInfo = NULL;
	}
	if (key && key[0]){
		InetInfo = new TInetDicInfo;
		InetInfo->CryptKey = key;
	}
}
bool PdicXmlFile::writeHeader()
{
	putline("<?xml version='1.0' encoding='UTF-8'?>");
	return putline("<pdic>");
}
bool PdicXmlFile::writeFooter()
{
	return putline("</pdic>");
}
bool PdicXmlFile::putxml(const tchar *text)
{
	if (InetInfo){
		__tnstrA outstr;
		if (!InetInfo->EncryptString(text, NULL, 0, &outstr)){
			return false;
		}
		return super::putxml(outstr);
	} else {
		return super::putxml(text);
	}
}

//////////////////////////////////////////////////////////////////////////////
// FENGFile class
//////////////////////////////////////////////////////////////////////////////
FENGFile::FENGFile( )
	:lbr( _T("《") )
	,rbr( _T("》") )
{
	dictype = DT_FENG5;
}

#ifdef GUI
int FENGFile::CanOpen( TWinControl *parent, const tchar *fname, int mode )
{
	if ( ( mode & FOM_OPENMODEMASK ) != FOM_READONLY ){
#if 0
		if ( parent )
			MessageBox( parent->Handle, _T("まだこの形式の出力はｻﾎﾟｰﾄしていません"), _T("辞郎形式出力"), MB_OK | MB_ICONEXCLAMATION );
#endif
		return -1;
	}
	return 0;
}
#endif

int FENGFile::Open( const tchar *fname, int mode )
{
	fLine = FALSE;
//	pword = NULL;
	swords.clear();
	return TextFile::Open( fname, mode );
}

int FENGFile::record( const tchar *word, Japa &japa )
{
	return -1;
}

void FENGFile::SetLabel( const tchar *_lbr, const tchar *_rbr )
{
	lbr.set( _lbr );
	rbr.set( _rbr );
}

int FENGFile::readPare( tnstr &word, Japa &japa )
{
#if DICDBG1
	if ( readcount > maxcount ){
		return -1;	// 終了
	}
	readcount++;
#endif
	japa.clear();

	// 複数単語の処理 //
	if ( swords.get_num() ){
		word.set( swords[0] );
		swords.del( 0 );
		japa.japa = sjapa;
		return 0;
	}

	// 見出し検索
	while ( 1 ){
		const tchar *sp;
		while ( 1 ){
			if ( !fLine ){
				if ( GetLineA( line ) < 0 ){
					return -1;	// 終了
				}
			} else {
				fLine = FALSE;
			}
			if ( *((ushort*)&line[0]) == _TW("■") )
//			if ( (tuchar)line[0] == (tuchar)0x81 && (tuchar)line[1] == (tuchar)0xa1 )	// ■
				break;
		}
		sp = &line[BYTETOLEN(2)];	// ■の次へのpointer
		while ( (tuchar)*sp == (tuchar)' ' ) sp++;
		const tchar *top = sp;
		const tchar *pjapa;
		const tchar *spc = NULL;
		const tchar *partp = NULL;
		int bracket = 0;
		while ( 1 ){
			if ( (tuchar)*sp == 0 ){
				pjapa = StrQ;
				break;
			}
			if ( (CFlags & CF_DIVWORD) && ((tuchar)*sp == (tuchar)';') && bracket == 0 ){
				// 複数単語の中の最初の単語の場合
				const tchar *p = _tcsstr( sp, _T(" : ") );
				if ( !p ){
					// 日本語訳が無い場合
					sjapa.set( StrQ );
				} else {
					p += 3;
					while ( (tuchar)*p == (tuchar)' ' ) p++;
					sjapa.set( p );
				}
				pjapa = sjapa;
				int len = STR_DIFF( sp, top );	// 見出語の長さ
				tchar *wd = new tchar[ len + 1 ];
				*(tchar*)sp = '\0';
				preMergeWord( wd, top );	// Zen to Han
				sp++;
				while ( (tuchar)*sp == (tuchar)' ' ) sp++;
				top = sp;
				swords.add( wd );
				delete[] wd;
				continue;
//				break;
			} else if ( (tuchar)*sp == (tuchar)' ' ){
				if ( !spc )
					spc = sp;
				// 見出語の最後であるかどうかのチェック
				if ( (tuchar)*(sp+1) == (tuchar)':' && (tuchar)*(sp+2) == (tuchar)' ' ){
//					pword = NULL;
					pjapa = sp+3;
					while ( (tuchar)*pjapa == (tuchar)' ' ) pjapa++;
					break;
				}
			} else if ( (tuchar)*sp == (tuchar)'{' ){
				// 品詞
				bracket++;
				if ( spc )
					partp = spc;
				else
					partp = sp;
			} else if ( (tuchar)*sp == (tuchar)'}' ){
				if ( bracket > 0 )
					bracket--;
			} else {
				spc = NULL;
			}
			sp = NEXT_CHAR(sp);
		}
		// sp : 単語の最後+1
		// top : 単語の先頭
		// pjapa : 日本語訳の先頭
		tnstr label;
		if ( partp ){
			if ( CFlags & CF_PARTLABELING ){
				// 品詞はラベリング
				const tchar *top = partp;
				while ( *top ){
					if ( *top != ' ' && *top != '{' ) break;
					top = NEXT_CHAR(top);
				}
				const tchar *p = top;
				while ( *p ){
					if ( *p == '}' ) break;
					p = NEXT_CHAR(p);
				}
				if ( STR_DIFF(p,top) > 0 ){
					label.set( lbr );
					label.cat( top, STR_DIFF(p,top) );
					label.cat( rbr );
				}
				*(tchar*)partp = '\0';
			} else
			if (CFlags & CF_REMOVEPART){
				// 品詞の削除
				*(tchar*)partp = '\0';
			}
		}
		int len = STR_DIFF( sp, top );	// 見出語の長さ
		tchar *wd = new tchar[ len + 1 ];
		*(tchar*)sp = '\0';
		preMergeWord( wd, top );	// Zen to Han
		// 複数見出語対策 //
		if ( swords.get_num() ){
			swords.add( wd );
			if ( partp ){
				if ( !(CFlags & (CF_PARTLABELING|CF_REMOVEPART)) ){
					for ( int i=0;i<swords.get_num()-1;i++ ){
						swords[i].cat( partp );
					}
				}
			}
			word.set( swords[0] );
			delete[] wd;
			swords.del( 0 );
		} else {
			word.setBuf( wd );
		}
		// 日本語訳
		tchar *newjapa = new tchar[ _tcslen( pjapa ) + label.length() + 20 ];
		tchar *dp = newjapa;
		if ( label[0] ){
			_tcscpy( dp, label );
			dp += _tcslen(label);
		}
		sp = pjapa;
		ushort c;
		BOOL fDistinct = (( CFlags & CF_DISTINCT ) == CF_DISTINCT);	// 用例であるかどうかを区別する必要があるフラグ
		while ( 1 ){
			LD_CHAR( c, sp );
			if ( !((tuchar)c) )
				break;
			bool fDelim = false;
			if ( CFlags & CF_ELIMKANA ){
				// {ひらがな}
				if ( c == CODE_LBR4 ){
					ushort _c = c;
					const tchar *_sp = sp;
					LD_CHAR( c, sp );
					if (ishiragana(c)){
						// 念のためひらがなチェック
						while ( 1 ){
							LD_CHAR(c,sp);
							if ( !((tuchar)c) ) break;
							if ( c == CODE_RBR4 ) break;
						}
						if ( !((tuchar)c) ) break;
						continue;
					} else {
						sp = _sp;
						c = _c;
					}
				}
			}
			if ( CFlags & CF_CONVCRLF ){
				// ■による改行
				if ( c == CODE_BLACK_SQUARE ){	// ■
					*dp++ = '\r';
					*dp++ = '\n';
					continue;
				}
			}
			if ( fDistinct ){
#if 0	// 2000.10.9
				// 用例か？
				if ( (c == CODE_BLACK_DIAMOND || c == CODE_BLACK_SQUARE) && !_ismbblead((uchar)*sp) && (uchar)*sp ){	// ◆
					// 「◆英文～」の場合
					goto jmp1;
				}
#endif
				if ( c == (tuchar)StrExpSepa[0] && (tuchar)*sp == (tuchar)StrExpSepa[1] && (tuchar)*(sp+1) == (tuchar)StrExpSepa[2] ){
					// 用例があった場合は、訳語部をセットし、新たにバッファを確保
					sp += 2;
//jmp1:
					*dp = '\0';
					japa.japa.setBuf( newjapa );
					dp = newjapa = new tchar[ _tcslen( sp ) + 20 ];
					fDistinct = FALSE;
					continue;
				}
			}
			if ( c == CODE_BLACK_DIAMOND || c == CODE_BLACK_CIRCLE )
			{
				fDelim = true;
#if 1	// Pdic2::record()へ移動と思ったがやっぱり止めた(1997.3.15)
				if ( CFlags & CF_CONVDELIM ){
					// ◆●の場合
					if ( CFlags & CF_ZEN2HAN ){
						c = ',';
					} else {
						c = CODE_COMMA1;	// 、
					}
				}
			}
#endif
			if ( (CFlags & CF_LINEFEED) && (fDelim || c == ',' || c == CODE_COMMA1) ){
				const tchar *__sp = sp;
				ushort cc;
				LD_CHAR( cc, __sp );
				if ( islbr( cc ) ){
					*dp++ = '\r';
					*dp++ = '\n';
					continue;
				}
			}
			ST_CHAR( c, dp );
		}
		*dp = '\0';
		if ( !fDistinct && (CFlags & CF_DISTINCT) ){
			// 用例部がある
			japa.exp.setBuf( newjapa );
		} else {
			japa.japa.setBuf( newjapa );
		}
		// 用例
		_jMixChar &exp = japa.exp;
		while ( 1 ){
			if ( !fLine ){	// 1997.1.12
				if ( GetLineA( line ) < 0 ){
					if ( swords.get_num() ){
						sjapa = japa.japa;
					}
					return 0;
				}
			} else {
				fLine = FALSE;
			}
			if ( (tuchar)line[0] == (tuchar)'*' && (tuchar)line[1] == (tuchar)' ' && (tuchar)line[2] ){
				// 用例
				if ( exp[0] ){
					exp.cat( _T(" / ") );
				}
				exp.cat( &line[2] );
			}
			if ( *((ushort*)&line[0]) == _TW("■"))
			{
				// 先頭の■
				fLine = TRUE;
				if ( swords.get_num() ){
					sjapa = japa.japa;
				}
				return 0;
			}
			// 最近の英辞郎はここに来ることは無い
		}
	}
}
#endif	// _Windows

#if USE_BM
// export only
#include "BookmarkMan.h"
TBookmarkStream::TBookmarkStream( )
{
	TopNode = NULL;
}

// filename is to be a tree node name.
int TBookmarkStream::Open( const tchar *filename, int mode )
{
	super::Open( filename, mode );

	if (mode & (FOM_CLIPBOARD|FOM_EDITCONTROL|FOM_RICHEDITCONTROL|FOM_CHAR)){
		error = ECOpenError;
		return -1;
	}

	switch ( mode & FOM_OPENMODEMASK ){
		case FOM_READONLY:
			return -1;
		case FOM_WRITE:
		case FOM_CREATE:
			break;
	}
	TopNode = NULL;
	xtime(&TimeStamp);
	return 0;
}
int TBookmarkStream::record(const tchar *word, Japa &japa )
{
	if (!japa.IsMemory())
		return 0;

	word = find_cword_pos(word);

	if (!TopNode){
		Bookmark.Load();
		TopNode = Bookmark.Add(name, false);
		Child = TopNode->CreateChild();
	}
	Child->add(new TBookmarkItem(TopNode, word, TimeStamp, BMF_MARK, NULL));
	app++;
	return 0;
}
void TBookmarkStream::Close()
{
	if (TopNode){
		Bookmark.Save();
		TopNode = NULL;
	}
}
#endif	// USE_BM

#if defined(PDICW) && !defined(UNIX)
//////////////////////////////////////////////////////////////////////////////
// UserFile class
//////////////////////////////////////////////////////////////////////////////
UserFile::UserFile()
{
	dictype = DT_USERTEMP;
	etp.Buffer = NULL;
	etp.BufferLength = 0;
}
UserFile::~UserFile()
{
	if ( etp.Buffer )
		delete[] etp.Buffer;
}

int UserFile::Open(const tchar *fname, int mode)
{
	if ( inherited::Open( fname, mode ) == -1 ) return -1;
	etp.ClearVar();
	return 0;
}
int UserFile::record( const tchar *word, Japa &japa )
{
	etp.temp = optionstr;
	etp.num = app;
	etp.srchstr = _T("");
	etp.fword = word;
	etp.fjapa = &japa;
	etp.crlfchar = '\n';
	etp.LastLine = LastLine;
	etp.Frequency = Frequency;
	if ( !ExpandTemplate( etp ) )
		return -1;	// out of memory
	app++;
	return tof().put( etp.Buffer );
}

#endif

#ifdef USE_ESTDB
//////////////////////////////////////////////////////////////////////////////
// QDBM classes
//////////////////////////////////////////////////////////////////////////////
#include "est/estdbdll.h"

//////////////////////////////////////////////////////////////////////////////
// QDBM base class
//////////////////////////////////////////////////////////////////////////////
TQDBMDict::TQDBMDict()
{
	Dll = NULL;
}

bool TQDBMDict::Load()
{
	if (Dll){
		return false;	// Already loaded
	}
	Dll = new TEstraierDllLoader;
	if (!Dll)
		return false;
	if (!Dll->Load()){
		delete Dll;
		Dll = NULL;
		return false;
	}
	return true;
}
void TQDBMDict::Unload()
{
	if (!Dll)
		return;
	delete Dll;
	Dll = NULL;
}

int TQDBMDict::CanOpen( TWinControl *parent, const tchar *fname, int mode )
{
	if ( mode & (FOM_CLIPBOARD|FOM_EDITCONTROL) )
		return 0;

	int openmode = ( mode & FOM_OPENMODEMASK );
	if ( openmode == FOM_READONLY ){
		if ( fexist( fname ) ){
			return 0;
		} else {
			error = ECNoFile;
			return -1;
		}
	} else {
		if ( fexist(fname) ){
			int c;
			if ( parent ){
				c = MessageDialog( parent->Handle, MAKEINTRESOURCE(APPOVRCANDLG), fname );
			} else c = ID_APPEND;
			switch (c){
				case ID_APPEND:
					if (Open(fname, FOM_WRITE) == -1){
						if ( parent )
							ErrorMessage( parent->Handle, GetErrorCode(), fname );
						return -1;
					}
					break;
				case ID_NEW:
					if (Open(fname, FOM_CREATE) == -1){
						if ( parent )
							ErrorMessage( parent->Handle, GetErrorCode(), fname );
						return -1;
					}
					break;
				case ID_OVER:
					if (Open(fname, FOM_CREATE) == -1){
						if ( parent )
							ErrorMessage( parent->Handle, GetErrorCode(), fname );
						return -1;
					}
					break;
				default:
					return -2;
			}
		} else {
			if (Open(fname, FOM_CREATE) == -1){
				if ( parent )
					ErrorMessage( parent->Handle, GetErrorCode(), fname );
				return -1;
			}
		}
		Close();
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
// QDBM Depot class
//////////////////////////////////////////////////////////////////////////////
TQDBMDepot::TQDBMDepot()
{
	db = NULL;
}
TQDBMDepot::~TQDBMDepot()
{
	Close();
}
int TQDBMDepot::Open( const tchar *fname, int mode )
{
	int omode;
	switch ( mode & FOM_OPENMODEMASK ){
		case FOM_READONLY:
			omode = DP_OREADER; 
			break;
		case FOM_CREATE:
			omode = DP_OWRITER | DP_OCREAT | DP_OTRUNC;
			break;
		case FOM_WRITE:
			omode = DP_OWRITER;
			break;
		default:
			return -1;	// parameter error
	}
	if (!Load()){
		return -1;
	}
	tnstrA fname8;
	fname8.setUTF8(fname);
	db = Dll->dpopen(fname8, omode, -1);
	if (!db){
		return -1;
	}
	super::Open(fname, mode);
	return 0;
}
void TQDBMDepot::Close( )
{
	if (db){
		Dll->dpclose(db);
		db = NULL;
		Unload();
	}
//	super::Close();
}

int TQDBMDepot::record(const tchar *word, Japa &japa )
{
	if (!db)
		return -1;

	uint size;
	const int limitsize = 0x100000;	//TODO: とりあえず適当な値
	char *buf = japa.Get(size, 0, limitsize, NULL);
	if (!buf){
		return -1;
	}
	int ret = 0;

	const int dmode = DP_DOVER;	//TODO: とりあえず上書きのみ
	__kstr kword(word);
	if (!Dll->dpput(db, kword, _tcsbyte(word), buf, size, dmode)){
		ret = -1;
	}

	delete[] buf;
	return ret;
}

//ファイル情報
int TQDBMDepot::percent()
{
	return 0;
}

int TQDBMDepot::getWord( tnstr &word)
{
	return -1;
}
int TQDBMDepot::getJapa( Japa &japa )
{
	return -1;
}

//////////////////////////////////////////////////////////////////////////////
// QDBM Villa class
//////////////////////////////////////////////////////////////////////////////
TQDBMVilla::TQDBMVilla()
{
	db = NULL;
}
TQDBMVilla::~TQDBMVilla()
{
	Close();
}
int TQDBMVilla::Open( const tchar *fname, int mode )
{
	int omode;
	switch ( mode & FOM_OPENMODEMASK ){
		case FOM_READONLY:
			omode = DP_OREADER; 
			break;
		case FOM_CREATE:
			omode = DP_OCREAT | DP_OTRUNC;
			break;
		case FOM_WRITE:
			omode = DP_OWRITER;
			break;
		default:
			return -1;	// parameter error
	}
	if (!Load()){
		return -1;
	}
	tnstrA fname8;
	fname8.setUTF8(fname);
#if 0	//TODO d:
	db = Dll->vlopen(fname8, omode, -1);
	if (!db){
		return -1;
	}
	super::Open(fname, mode);
	return 0;
#else
	return -1;
#endif
}
#ifdef GUI
int TQDBMVilla::CanOpen( TWinControl *, const tchar *fname, int mode )
{
	return 0;
}
#endif
void TQDBMVilla::Close( )
{
	if (!db)
		return;
	Dll->vlclose(db);
	db = NULL;
//	super::Close();
}

int TQDBMVilla::record(const tchar *word, Japa &japa )
{
	return -1;
}

//ファイル情報
int TQDBMVilla::percent()
{
	return 0;
}

int TQDBMVilla::getWord( tnstr &word)
{
	return -1;
}
int TQDBMVilla::getJapa( Japa &japa )
{
	return -1;
}
#endif	// USE_ESTDB

#ifdef USE_USERDLL
//////////////////////////////////////////////////////////////////////////////
// UserDic class
//////////////////////////////////////////////////////////////////////////////
int UserDic::count = 0;
LPFNSETUP UserDic::lpfnSetup = NULL;
LPFNCLEANUP UserDic::lpfnCleanup = NULL;
LPFNOPEN UserDic::lpfnOpen;
LPFNCANOPEN UserDic::lpfnCanOpen;
LPFNCLOSE UserDic::lpfnClose;
LPFNGETWORD UserDic::lpfnGetWord;
LPFNMERGE UserDic::lpfnMerge;
LPFNRECORD UserDic::lpfnRecord;
LPFNGETPERCENT UserDic::lpfnGetPercent;
HINSTANCE UserDic::hDll = NULL;

UserDic::UserDic( )
{
	memset( &data, 0, sizeof(PDICDATA) );
	data.size = sizeof( PDICDATA );
	count++;
}

UserDic::~UserDic()
{
	--count;
	if ( hDll && count==0 ){
		FreeLibrary( hDll );
		hDll = NULL;
	}
}

static const char *IDUSER_OPEN = "Open";
static const char *IDUSER_CANOPEN = "CanOpen";
static const char *IDUSER_CLOSE = "Close";
static const char *IDUSER_GETWORD = "GetWord";
static const char *IDUSER_MERGE = "Merge";
static const char *IDUSER_RECORD = "Record";
static const char *IDUSER_GETPERCENT = "GetPercent";
static const char *IDUSER_SETUP = "Setup";
static const char *IDUSER_CLEANUP = "Cleanup";

// 何度でも呼べる
int UserDic::Setup( const tchar *dllname )
{
	if ( !hDll ){
		SetErrorMode( SEM_NOOPENFILEERRORBOX );
		hDll = LoadLibrary( dllname );
		SetErrorMode( 0 );
		if ( !IsValidInstance( hDll ) ){
			hDll = NULL;
			return FALSE;
		}
		lpfnOpen	= (LPFNOPEN)	GetProcAddress( hDll, IDUSER_OPEN );
		lpfnCanOpen	= (LPFNCANOPEN)	GetProcAddress( hDll, IDUSER_CANOPEN );
		lpfnClose	= (LPFNCLOSE)	GetProcAddress( hDll, IDUSER_CLOSE );
		lpfnGetWord	= (LPFNGETWORD)	GetProcAddress( hDll, IDUSER_GETWORD  );
		lpfnMerge	= (LPFNMERGE)	GetProcAddress( hDll, IDUSER_MERGE );
		lpfnRecord	= (LPFNRECORD)	GetProcAddress( hDll, IDUSER_RECORD );
		if ( !lpfnOpen || !lpfnCanOpen || !lpfnClose || !lpfnGetWord || !lpfnMerge || !lpfnRecord ){
			FreeLibrary( hDll );
			hDll = NULL;
			return -1;
		}
		lpfnGetPercent = (LPFNGETPERCENT)GetProcAddress( hDll, GetString( IDUSER_GETPERCENT ) );
		lpfnSetup = (LPFNSETUP)GetProcAddress( hDll, GetString( IDUSER_SETUP ) );
		lpfnCleanup = (LPFNCLEANUP)GetProcAddress( hDll, GetString( IDUSER_CLEANUP ) );
	}
	if ( lpfnSetup ){
		return lpfnSetup( &data );
	}
	return TRUE;
}

void UserDic::Cleanup( )
{
	if ( lpfnCleanup ){
		lpfnCleanup( &data );
	}
}


int UserDic::record(const char *word, Japa &japa )
{
	_tcscpy( data.word, word );
	_tcscpy( data.japa, japa.GetJapa() );
	_tcscpy( data.exp, japa.GetExp() );
	data.attr = japa.GetAttr();
	data.hWnd = cvtdlg->HWindow;
	if ( !lpfnRecord( &data ) ){
		error = ECWriteError;
		return -1;
	}
	return 0;
}

int UserDic::readPare( tnstr &word, Japa &japa )
{
	data.word[0] = '\0';
	data.japa[0] = '\0';
	data.exp[0] = '\0';
	data.attr = WA_NORMAL;
	data.hWnd = cvtdlg->HWindow;
	if ( data.flag & AS_TEXTFILE ){
		while ( 1 ){
			tnstr line;
#if USE_ANSI
			if ( TransCode ){
				if ( tif().getlineA( line ) < 0 )
					return -1;
			} else
#endif
			if ( tif().getline( line ) < 0 ){
				// 終了
				return -1;
			}
			data.text = line;
			if ( !lpfnGetWord( &data ) ){
				if ( data.errorcode ){
					error = data.errorcode;
					return -1;
				}
				continue;
			}
			break;
		}
	} else {
		if ( !lpfnGetWord( &data ) ){
			// 終了
			return -1;
		}
	}
	word.set( data.word );
	japa.japa.set( data.japa );
	japa.exp.set( data.exp );
	japa.attr = data.attr | WA_NORMAL;
	return 0;
}

int UserDic::Open( const char *fname, int mode )
{
	data.filename = fname;
	if ( data.flag & AS_TEXTFILE ){
		int r = Perd12::Open( fname, mode );
		if ( r == 0 ){
			data.errorcode = 0;
			data.hFile = tif().get_fd();
			lpfnOpen( &data, mode );
		}
		return r;
	} else {
		if ( !lpfnOpen( &data, mode ) || data.hFile == HFILE_ERROR ){
			error = ECOpenError;	// オープンエラー
			return -1;
		}
		Dictionary::Open( fname, mode );
	}
	return 0;
}

int UserDic::CanOpen( TWinControl *parent, const char *fname, int mode )
{
	data.hWnd = hwnd;
	data.filename = fname;
	if ( !lpfnCanOpen( &data, mode ) ){
		error = ECOpenError;
		return -1;
	}
	if ( data.flag & AS_TEXTFILE ){
		return Perd12::CanOpen( parent, fname, mode );
	}
	return 0;
}

void UserDic::Close( )
{
	if ( data.flag & AS_TEXTFILE ){
		Perd12::Close( );
		lpfnClose( &data );
	} else {
		lpfnClose( &data );
//		data.hFile = NULL;
	}
}

int UserDic::percent( )
{
	if ( lpfnGetPercent ){
		return lpfnGetPercent( &data );
	} else if ( data.flag & AS_TEXTFILE ){
		return Perd12::percent( );
	} else {
		return 0;
	}
}

BOOL UserDic::Merge( const char *word, Japa&src, Japa&dest )
{
	_tcscpy( data.word, word );

	PDICDATA *srcdata = new PDICDATA;
	memcpy( srcdata, &data, sizeof( PDICDATA ) );
	_tcscpy( srcdata->japa, src.japa );
	_tcscpy( srcdata->exp, src.exp );
	srcdata->attr = src.GetAttr();

	_tcscpy( data.japa, dest.japa );
	_tcscpy( data.exp, dest.exp );
	data.attr = dest.GetAttr();

	BOOL r = lpfnMerge( srcdata, &data );
	delete srcdata;
	if ( r ){
		dest.japa.set( data.japa );
		dest.exp.set( data.exp );
		dest.attr = data.attr | WA_NORMAL;
	}
	return r;
}

#endif


Dictionary *MakeDic( int type )
{
	switch ( type ){
		case DT_PDICTEXT:
			return new Perd12;
		case DT_WX2TEXT:
			return new WX2;
		case DT_PDIC:
		case DT_PDIC_OLD_ANS:
		case DT_PDIC_OLD_UNI:
			return new Pdic2;
		case DT_LEVEL:
			return new WLevel;
		case DT_TSV:
			return new TSVFile;
#ifdef PDICW
		case DT_CSV:
			return new CSVFile;
#endif
#ifdef USE_USERDLL
		case DT_USER:
			return new UserDic;
#endif
		case DT_TEXT:
			return new ::TextFile;
		case DT_EXTPDICTEXT:
			return new ExtPdicText;
#ifdef _Windows
		case DT_FENG5:
			return new FENGFile;
#endif
#ifdef USE_ESTDB
		case DT_QDBM_DEPOT:
			return new TQDBMDepot;
		case DT_QDBM_VILLA:
			return new TQDBMVilla;
#endif	// USE_ESTDB
#if defined(PDICW) && !defined(UNIX)
		case DT_USERTEMP:
			return new UserFile;
#endif
#ifdef _Windows
		case DT_XML:
			return new PdicXmlFile;
#endif
#if USE_BM
		case DT_BOOKMARK:
			return new TBookmarkStream;
#endif
		default:
			return NULL;
	}
}


