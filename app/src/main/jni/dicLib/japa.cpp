#include	"tnlib.h"
#pragma	hdrstop
#include "pdconfig.h"
#include "dic.h"
#include "japa.h"

#ifndef SMALL
#include "jmerge.h"
#endif

#ifdef _Windows
#ifdef GUI
#include "UserMsg.h"
#endif
#endif	// _Windows

#include "prontbl.h"

// merge機能を使用するか？
#if defined(PDUTY) || (defined(PDICW) && !defined(SMALL) && !defined(__ANDROID__))
#define	USE_MERGE	1
#else
#define	USE_MERGE	0
#endif

#ifdef USE_ZLIBCOMP
#include "zlibif.h"
#  define	cmpEncode		zlibEncode
#  define	cmpDecode		zlibDecode
#  define	cmpGetOrgSize	zlibGetOrgSize
#  define	cmpSetWindow	zlibSetWindow
#  define	cmpHEADER_SIZE	ZLIB_HEADER_SIZE
#  define	ZLIB_HEADER_SIZE	sizeof(ZLIB_HEADER)
#elif defined(USE_RANGECODEC)
#include "RangeCodec.h"
#  define	cmpEncode		RCEncode
#  define	cmpDecode		RCDecode
#  define	cmpGetOrgSize	RCGetOrgSize
#  define	cmpSetWindow	RCSetWindow
#  define	cmpHEADER_SIZE	RC_HEADER_SIZE
#  define	RC_HEADER_SIZE	sizeof(RC_HEADER)
#else
#include "arif.h"
#  define	cmpEncode		AREncode
#  define	cmpDecode		ARDecode
#  define	cmpGetOrgSize	ARGetOrgSize
#  define	cmpSetWindow	ARSetWindow
#  define	cmpHEADER_SIZE	AR_HEADER_SIZE
#  define	AR_HEADER_SIZE	sizeof(AR_HEADER)
#endif

#ifdef NEWDIC4UNI
#define	L_ExtAttr	2	// 拡張属性+reserved
#else
#define	L_ExtAttr	1	// 拡張属性
#endif

// ANSI上のマルチバイト文字１文字分が英字で何文字分に相当するか？
#ifdef _UNICODE
#define	_MBLEN1		1	// 1 charactor(=2byte)
#else
#define	_MBLEN1		2	// 2 charactor(=2byte)
#endif

#if __JAPAMT
#define	__lock__	TAutoLock lock(Mutex)
#define	__islocked__	Mutex.IsLocked()
#else	// !__JAPAMT
#define	__lock__	
#define	__islocked__	true
#endif	// !__JAPAMT

/*------------------------------------------*/
/*		Global Variables					*/
/*------------------------------------------*/
const tchar *StrExpSepa = _T(" / ");
const tchar *StrQ= _T("?");

/*------------------------------------------*/
/*		Prototypes							*/
/*------------------------------------------*/
#ifdef USE_JLINK
static JLink *CreateJLink(Pdic *dic, int type, int &ola, bool preset);
#endif
void CatStr( _jMixChar &dest, const tchar *delim, const tchar *src );
_mchar *_tcscpy2( _mchar *dst, const _mchar *src );

int Japa::InstCnt = 0;

Japa::Japa()
{
	IncInstCnt();
	attr = WA_NORMAL;
#ifdef USE_REF
	refnum = 0;
	for ( int i=0;i<NMAXREF;i++ ){
		refdata[i] = -1;
	}
#endif
#if CHKJOBJ
	HasUndefObj = false;
#endif
}

#if defined(GUI)
HWND Japa::HWindow = NULL;
#endif

void Japa::operator = ( const Japa &j )
{
	attr = j.attr;
	japa.set( ((Japa&)j).japa );
	exp.set( ((Japa&)j).exp );
	pron.set( ((Japa&)j).pron );
#ifdef USE_REF
	refnum = j.refnum;
	if ( refnum ){
		for ( int i=0;i<NMAXREF;i++ ){
			refdata[i] = j.refdata[i];
		}
	}
#endif
#ifdef USE_JLINK
	jlinks.clear( );
	for ( int i=0;i<((Japa&)j).jlinks.get_num();i++ ){
		jlinks.add(  ((Japa&)j).jlinks[i].Clone( NULL ) );
	}
#endif
#if CHKJOBJ
	HasUndefObj = j.HasUndefObj;
#endif
}

Japa::~Japa()
{
	DecInstCnt();
#ifdef USE_BOCU1
	if (InstCnt==0)
	{
		FreeBuffers();
	}
#endif
}

void Japa::clear( )
{
	attr = WA_NORMAL;
	japa.clear();
	exp.clear();
	pron.clear();
#ifdef USE_REF
	refnum = 0;
	for ( int i=0;i<NMAXREF;i++ ){
		refdata[i] = -1;
	}
#endif
#ifdef USE_JLINK
	jlinks.clear();
#endif
}


bool Japa::IsEmpty( ) const
{
	return !japa[0] && IsEmptyEx();
}

bool Japa::IsEmptyEx( ) const
{
	return !exp[0] && !pron[0]
#ifdef USE_REF
&& !refnum
#endif
#ifdef USE_JLINK
&& !jlinks.get_num()
#endif
		;
}

#ifdef USE_JLINK
ulong Japa::GetJLinksLength( int fixedlen ) const
{
	ulong l = 0;
	for ( int i=0;i<jlinks.get_num();i++ ){
		ulong r = jlinks[i].GetLength( );
        if ( r ) l += r + fixedlen;
        else {
        	if ( jlinks[i].GetType() == JL_EPWING ){
				l += 1 + sizeof(EPWingField);
            }
        }
	}
	return l;
}
#endif

// NEWDIC3 : Field1のときの長さを返す
// したがって、この戻り値でバッファを確保する場合は、
// 余計に確保する必要がある
int Japa::GetAllLen( ) const
{
	if ( IsEmptyEx() ){
		return
#ifndef NEWDIC4
			sizeof(wa_t) +
#endif
				_jcsbyte( japa );
	} else {
		// 拡張部の長さ取得
		int l1 = _jcsbyte( exp );
		if ( l1 ){
			l1 += L_ExtAttr + sizeof(_jchar);
			// JT_EXP + 本体 + '\0'
		}
		int l2 = _jcsbyte( pron );
		if ( l2 ){
			l2 += L_ExtAttr + sizeof(_jchar);
			// JT_PRON + 本体 + '\0'
		}
#ifdef USE_JLINK
		l1 += l2 + GetJLinksLength( L_ExtAttr + sizeof(t_jtb) + sizeof(_jchar) );
									// JT_LINK + t_jtb + 本体 + '\0'
#else
		l1 += l2;
#endif
		;
		// 属性 + 訳 + 区切り('\0') + 拡張部の長さ + JT_END
		return
#ifndef NEWDIC4
			sizeof(wa_t) +
#endif
				_jcsbyte1( japa ) + l1 + L_ExtAttr;
#ifdef USE_REF
( refnum ? refnum * 2 + 2 : 0 )
#endif
		;
	}
}

#if defined(USE_COMP)
// src はt_nocを指していること(＝t_jtb(2)の次)
// decodelenは戻り値であり、非圧縮部、解凍部の合計値である（t_noc部は含まない）
// 戻り値はデコードしたバッファ
//		従って、デコードデータには、非圧縮部 + 解凍部
//		デコードデータとdecodelenは一致する
// 必ず、デコードバッファは呼出側でdeleteすること！！
// エラー発生は、メモリ不足か、解凍失敗
byte *Japa::Decode( const byte *src, ulong jtblen, ulong &decodelen )
{
	uint nocomplen = *((t_noc*)src);	// 非圧縮部の長さ
	const byte *header = src + sizeof(t_noc) + nocomplen;	// 圧縮部の先頭（ヘッダー部を含む）
	ulong complen = jtblen - sizeof(t_noc) - nocomplen;	// 圧縮部の長さ（ヘッダーを含む）

	ulong orglen = cmpGetOrgSize( header );
	if (orglen>complen*100){
		DBW("orglen=%d complen=%d", orglen, complen);
		return NULL;	// corrupted?
	}
	byte *decode = new byte[ orglen + nocomplen ];	// 先頭のt_jtbはダミー
	if ( !decode )
		return NULL;

	memcpy( decode, src + sizeof(t_noc), nocomplen );	// 非圧縮部のコピー
	long destlen;
	if ( !cmpDecode( header, complen, decode + nocomplen, destlen ) ){
		delete[] decode;
		return NULL;
	}

	decodelen = orglen + nocomplen;
	return decode;
}
#endif

// 辞書上のtextを__Char objectへsetするmacro //////////////////////
#ifdef NEWDIC4
	#define	arg_l(l)	_MBYTETOLEN(l)
#else
	#define	arg_l(l)	_MBYTETOLEN(l-1)
#endif

// NULL終端が無い場合 //
#if !defined(USE_BOCU1)
#define	__SetText( dic, src, srclen, strcls, single ) \
	strcls.set( (_mchar*)src, arg_l(srclen) );
#else	// USE_BOCU1
#define	__SetText( dic, src, srclen, strcls, single ) \
	{ \
		__lock__; \
		tchar *out = GetBocuOutBuffer(LJALL); \
		if (out){ \
			bocu1DecodeT( &src, (const byte*)(src)+srclen, LJALL, out); \
			strcls.set( out ); \
		} \
		ReleaseBocuOutBuffer(); \
	}
#endif	// USE_BOCU1

#ifdef DIC_UTF16
#define	__ALIGN_MCHAR(src) (((int)(src)+2)&~1)
#else	// DIC_UTF8
#define	__ALIGN_MCHAR(src) (src)
#endif

// 文字列セット //
#if defined(NEED_SINGLEBYTE)
#define	__SET_STR(_src, src, strcls, single) \
	src = (byte*)(strcls.set( (const _mchar*)_src ) + 1);
#else
#define	__SET_STR(_src, src, strcls, single) \
	strcls.set( (_mchar*)_src ); \
	src = _src + _mcslen((_mchar*)_src)+1;
#endif

// 文字列セット(srcが無いバージョン) //
#if defined(NEED_SINGLEBYTE)
#define	__SET_STR2(_src, strcls, single) \
	strcls.set( (const _mchar*)_src, single );
#else
#define	__SET_STR2(_src, strcls, single) \
	strcls.set( (_mchar*)_src );
#endif


// NULL終端がある場合 //
#if !defined(USE_BOCU1)
#define	__SetText1( dic, _src, src, strcls, single, maxlength ) \
	__SET_STR(_src,src,strcls,single)
#else	// USE_BOCU1
#define	__SetText1( dic, _src, src, strcls, single, maxlength ) \
	{ \
		__lock__; \
		const byte *__src = _src; \
		tchar *out = GetBocuOutBuffer(maxlength); \
		if (out){ \
			bocu1DecodeT( &__src, (const byte*)0xFFFFFFFFu, maxlength, out ); \
			src = (const byte*)__ALIGN_MCHAR(__src); \
			strcls.set( out ); \
		} \
		ReleaseBocuOutBuffer(); \
	}
#endif	// !MIXDIC && USE_BOCU1

// NULL終端がある場合 : __SetText1()にsrcが無いバージョン //
#if !defined(USE_BOCU1)
#define	__SetText2( dic, _src, strcls, single, maxlength ) \
	__SET_STR2(_src,strcls,single)
#else	// USE_BOCU1
#define	__SetText2( dic, _src, strcls, single, maxlength ) \
	{ \
		__lock__; \
		const byte *__src = _src; \
		tchar *out = GetBocuOutBuffer(maxlength); \
		if (out){ \
			bocu1DecodeT( &__src, (const byte*)0xFFFFFFFFu, maxlength, out ); \
			strcls.set( out ); \
		} \
		ReleaseBocuOutBuffer(); \
	}
#endif	// !MIXDIC && USE_BOCU1

// note:
// NEWDIC4の場合、attrは、SetAll()の前か後ろに直接 japa.attr = attr;というようにsetする。
void Japa::SetAll( const byte *src, int l, IndexData *dic, bool field2, wa_t _attr)
{
#if defined(USE_JLINK)
	int linkattr = ((Pdic*)dic)->GetLinkAttr( );
#endif
	clear();
#ifdef NEWDIC4
	attr = _attr;
#else
	attr = *src++;
#endif
	if ( attr & WA_EX ){
#if !defined(NOFIELD2)
		int jtbsize;
		if ( dic && field2 ){	//TODO: AllSearchParamから取得する必要有り
			jtbsize = sizeof(t_jtb2);
		} else {
			jtbsize = sizeof(t_jtb);
		}
#else
		const int jtbsize = sizeof(t_jtb);
#endif
		__SetText1( dic, src, src, japa, _jSingleByte, LJALL );
		while ( 1 ){
			byte jt = *src++;
#ifdef NEWDIC4UNI
			src++;	// reserved
#endif
#if defined(USE_COMP)
			byte *decode = NULL;
#endif
			ulong jtb;
			ulong _srclen;
#if !defined(NOFIELD2)
			if ( jtbsize == sizeof(t_jtb2) ){
//				jtb = *(*(t_jtb2**)&src)++;
				jtb = _alGetULong( (const t_jtb2*)src );
				src += sizeof(t_jtb2);
			} else
#endif
			{
//				jtb = *(*(t_jtb**)&src)++;
				jtb = _alGetUShort( (const t_jtb*)src );
				src += sizeof(t_jtb);
			}
			// src : Compの場合:t_noc, Non-Compの場合:t_jlink
			const byte *_src = src;	// _srcからt_jtbの取得はしないこと！！（圧縮していた場合、t_jtbがオーバフローすることがあるため！）
			_srclen = jtb;	// _srclenは解凍すると解凍後の長さになるので注意！
#if RO_COMP
			if ( jt & JT_COMP ){
				HasUndefObj = true;
			}
#endif
#if defined(USE_COMP)
			if ( jt & JT_COMP ){
				if ( jt & JT_LINK ){	// 非圧縮部にID番号が必ずあると仮定
					(*(t_noc**)&_src)++;
#ifdef USE_JLINK
					t_jlink jltype = *(t_jlink*)_src;
					if (
#ifdef USE_OLE
						( jltype == JL_OLE && linkattr & OLA_NOTREADOLE ) ||
#endif
						( (jltype == JL_FILE  || jltype==JL_FILEIMAGE
#ifdef JL_ICONFILE
						|| jltype == JL_ICONFILE
#endif
						) && linkattr & OLA_NOTREADFILE )
#ifdef JL_VOICE
						|| (jltype == JL_VOICE && linkattr & OLA_NOTREADVOICE)
#endif
#ifdef USE_JBMP
						|| ( ( jltype == JL_IMAGE ) && linkattr & OLA_NOTREADBMP )
#endif
#ifdef EPWING
//						|| ( (jltype==JL_EPWING) && linkattr & OLA_NOTREADEPWING )
#endif
						)
					{
				nextbyte:
						src += jtb;
						goto jnext;
					}
					{
						int ola;
						JLink *jlink = CreateJLink((Pdic*)dic, jltype, ola, true);
						if ( jlink ){
							if (linkattr & ola){
								jlinks.add( jlink );
								goto nextbyte;
							}
							if ( jlink->PreSet( _src+sizeof(t_jlink) ) ){	// IDのptr
								// プリセット成功！
								src += jtb;
								jlinks.add( jlink );
								continue;
							}
							delete jlink;
						}
					}
#endif	// USE_JLINK
				}
				// 解凍
				decode = Decode( src, jtb, _srclen );
				if ( !decode ){
					// エラー !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					return;
				}
				_src = decode;
				jt &= ~JT_COMP;
			}
#endif	// USE_COMP
#ifdef USE_JLINK
			JLink *jlink;
#endif
			if ( jt & JT_BYTE ){
				src += jtb;
				switch ( jt )
				{
				case JT_PRON|JT_BYTE:	// w/comp
					__SetText2( dic, _src, pron, _pSingleByte, LPRON+1 );
//					pron.set( (_mchar*)_src );
					break;
				case JT_EXP|JT_BYTE:	// w/comp
					__SetText2( dic, _src, exp, _eSingleByte, LEXP+1 );
//					exp.set( (_mchar*)_src );
					break;
#if !defined(DICTEST) && defined(USE_JLINK)
				case JT_LINK|JT_BYTE:
					{
						JLinkFormat &jlf = *(JLinkFormat*)(_src);	// = &JLinkStruct.jlink
						int ola;
						jlink = CreateJLink((Pdic*)dic, jlf.type, ola, false);
						if (!(linkattr & ola)){
							if ( !jlink->Set( _src + sizeof(t_jlink), _srclen-sizeof(t_jlink) ) ){
//								error = jlink->GetErrorCode( );
//								r = false;
							}
						}
						jlinks.add( jlink );
					}
					break;
#endif
				}
			} else {
				_src -= jtbsize;
				switch ( jt ){
					case JT_PRON:
						__SetText1( dic, _src, src, pron, _pSingleByte, LPRON+1 );
						break;
					case JT_EXP:
						__SetText1( dic, _src, src, exp, _eSingleByte, LEXP+1 );
						break;
#if 0
					case JT_REF:
						{
#ifdef USE_REF
							refnum = ( (byte)*_src++ ) / sizeof( short );
							for ( i=0;i<refnum;i++ ){
								refdata[i] = ((short*)_src)++;
							}
							for ( ;i<MAXREFNUM;i++ ){
								refdata[i] = -1;
							}
#else
							int refnum = ( (uchar)*src++ ) / sizeof( short );
							for ( i=0;i<refnum;i++ ){
								(*(short**)&src)++;
							}
#endif
						}
						break;
#endif	// 0
#ifdef EPWING
					case JT_EPWING:
						if ( !(linkattr & OLA_NOTREADEPWING) ){
							jlink = new JLEPWing( (Pdic*)dic, 0, ((EPWingField*)_src)->bookno, ((EPWingField*)_src)->pos );
							jlinks.add( jlink );
						}
						src = _src + sizeof(EPWingField);
						break;
#endif
					default:
					case JT_END:
						goto jmp1;
				}
			}
		jnext:;
#if defined(USE_COMP)
			if ( decode )
				delete[] decode;
#endif
			continue;
jmp1:
			break;
		}
	} else {
		if ( l ){
			__SetText( dic, src, l, japa, _jSingleByte );
#if 0
			japa.set( _mstr2((_mchar*)src, arg_l, _jSingleByte) );
#endif
		}
	}
#ifdef DISPDICNAME
	if (DispDicName&&dic){
		if (japa[0]){
			tnstr name;
			AddDicName( name, ((Pdic*)dic)->GetDispName() );
			name += _T("\r\n");
			name += japa;
			japa = name;
		}
	}
#endif
}

#ifdef USE_JLINK
static JLink *CreateJLink(Pdic *dic, int type, int &ola, bool preset)
{
	ola = 0;
	switch ( type ){
#if defined(USE_FILELINK)
		case JL_FILE:
			if (preset)
				return NULL;	// no preset in JLFile.
			ola = OLA_NOTREADFILE;
			return new JLFile( dic,_T(""), 0 );
		case JL_FILEIMAGE:
			ola = OLA_NOTREADFILE;
			return new JLFileImage( dic,_T(""), 0 );
#endif
#if defined(USE_ICONFILE)
		case JL_ICONFILE:
			if (preset)
				return NULL;
			ola = OLA_NOTREADFILE;
			return new JLIconFile( dic,_T(""),_T(""), 0 );
#endif
#ifdef USE_JBMP
		case JL_IMAGE:
			ola = OLA_NOTREADBMP;
			return new JLImage( dic, 0, NULL, NULL, 0 );
#endif
#if defined(USE_OLE)
		case JL_OLE:
			ola = OLA_NOTREADOLE;
			return new JLOle( (Pdic*)dic, NULL );
#endif
#ifdef GUI
#ifdef JL_VOICE
		case JL_VOICE:
			ola = OLA_NOTREADVOICE;
			return new JLVoice( dic, 0, NULL, NULL, 0 );
#endif
#endif
#ifdef EPWING
#if 0
		case JL_EPWING:
			if (preset)
				return NULL;
			ola = OLA_NOTREADEPWING;
			return new JLEPWing( dic, 0, 0 );
#endif
#endif
		default:
			if (preset)
				return NULL;
#if CHKJOBJ
			HasUndefObj = true;
#endif
			return new JLUFO( dic, 0 );
	}
}
#endif	// USE_JLINK

// テキスト専用圧縮
// 返り値は次へのポインター
// NULLだとエラー
// pはJT_...の次のポインターであること！！
static char *TextComp( char *p, char *top, const tchar *src, int compflag, uint limitlen )
{
	bool fOver = false;
	uint len = _tcsbyte1( src );
	if ( FP_DIFF( p, top ) + len + L_ExtAttr > limitlen ){	// len + JT_END
#ifndef USE_COMP
		return NULL;
#else
		fOver = true;
		if ( !(compflag & CP_TEXT) )
			return NULL;	// limitlenオーバー
		goto jmp1;
#endif
	}
#ifdef USE_COMP
	if ( !(compflag & CP_TEXT) ){
		goto notcomp;
	}
	if ( ( compflag & CP_COMP2 ) == CP_COMP2 && len >= CP_MINLEN ){
jmp1:
		char *codbuf = new char[ len + sizeof(t_jtb) + sizeof(t_noc) + cmpHEADER_SIZE ];
		if ( !codbuf ){
			if ( fOver )
				return NULL;	// メモリ不足
			goto notcomp;
		}
		long destlen;
#ifdef GUI
		cmpSetWindow( NULL );
#endif
		if ( cmpEncode( (const byte*)src, len, (byte*)codbuf, destlen ) && destlen+sizeof(JLinkStructC)-sizeof(JLinkStruct)+10 < len ){
			// 圧縮後のほうが長い場合も考慮(1996.9.7) - +10はおまけ
			// 圧縮できた
			if ( fOver ){
#if !defined(NOFIELD2)
				if ( compflag & CP_USEFIELD2 ){
					limitlen -= sizeof(t_jtb2) - sizeof(t_jtb);	// limitlenをここでかえている！！
				}
#endif
				if ( FP_DIFF( p, top ) + sizeof(t_jtb) + sizeof(t_noc) + destlen + L_ExtAttr > limitlen ){
					// 圧縮しても足りない
					delete[] codbuf;
					return NULL;	// limitlenオーバー
				}
			}
			*(p-1) |= JT_COMP | JT_BYTE;
#ifdef NEWDIC4UNI
			*p++ = 0;
#endif
#if !defined(NOFIELD2)
			if ( compflag & CP_USEFIELD2 ){
				*(*(t_jtb2**)&p)++ = (t_jtb2)(destlen+sizeof(t_noc));
			} else
#endif
				*(*(t_jtb**)&p)++ = (t_jtb)(destlen + sizeof(t_noc));
			*(*(t_noc**)&p)++ = 0;	// 非圧縮部長
			memcpy( p, codbuf, destlen );
			delete[] codbuf;
			return p+destlen;
		} else {
			// 圧縮できない
			delete[] codbuf;
			if ( fOver ){
				return NULL;	// limitlenオーバー
			}
			goto notcomp;
		}

	}
#endif	// def USE_COMP
notcomp:
	memcpy( p, src, len );
	return p+len;
}

// ばらばらになっているtextを辞書上のbufferへcopyするmacro /////////////////

// 2byte alignmentにする（NULL終端なしの場合）
#ifdef DIC_UTF16
#define	__SHORT_PADDING(dst) \
	if ( (int)dst & 1 ){ /*1byte padding */ \
		*(byte*)dst = '\0'; \
		(byte*)dst += 1; \
	}
#else	// !DIC_UTF16
#define	__SHORT_PADDING(dst)	/* nothing */
#endif

// 終端NULLをbyte単位および2byte alignmentで付加する
#ifdef DIC_UTF16
#define	__MCHAR_PADDING(dst) \
	if ( (int)dst & 1 ){ /*1byte padding */ \
		*(byte*)dst = '\0'; \
		(byte*)dst += 1; \
	} \
	*(wchar_t*)dst = '\0'; \
	*(wchar_t**)&(dst) += 1;
#else	// !DIC_UTF16
#define	__MCHAR_PADDING(dst) \
	{ \
	byte *__dst = (byte*)dst; \
	*__dst = '\0'; \
	__dst++; \
	dst = __dst; \
	}
#endif

// lengthがわかっている場合の処理(NULL終端なし) //
#if !defined(USE_BOCU1)
#define	__GetText( dic, src, srclen, dst ) \
	memcpy( dst, (const _mchar*)src, srclen ); \
	dst += srclen;
#else	// USE_BOCU1
#define	__GetText( dic, src, srclen, dst ) \
	dst = bocu1EncodeT( src, (tchar*)((byte*)((wchar_t*)src)+srclen), dst ); \
	__SHORT_PADDING(dst);
#endif	// USE_BOCU1

// lengthがわからない場合で終端'\0'を付加する場合の処理 //
#if !defined(USE_BOCU1)
#define	__GetText1( dic, src, dst ) \
	dst = (byte*)_tcscpy2( (_mchar*)(dst), src );
#else	// USE_BOCU1
#define	__GetText1( dic, src, dst ) \
	dst = bocu1EncodeT( src, (tchar*)-1, dst ); \
	__MCHAR_PADDING(dst);
#endif	// USE_BOCU1

// lengthがわかって、終端'\0'を付加する場合の処理 //
#if !defined(USE_BOCU1)
#define	__GetText2( dic, src, srclen, dst ) \
	memcpy( dst, (const _mchar*)src, srclen+sizeof(_mchar) ); \
	dst += srclen+sizeof(_mchar);
#else	// USE_BOCU1
#define	__GetText2( dic, src, srclen, dst ) \
	dst = bocu1EncodeT( src, (tchar*)-1, dst ); \
	__MCHAR_PADDING(dst);
#endif	// USE_BOCU1

// 返り値をチェックすること！
// (ulong)-1 のときはメモリ不足またはlimitlenをオーバ
// totallenに0を与えるとこの関数内で計算(GetAllLen()の値を渡す)
// NEWDIC3では、japatotallen+LWORD+L_FieldHeaer+sizeof(tfield)+1が
// 圧縮してもMAX_FIELD1SIZEを超えると自動的にField2で処理
// その場合、戻り値の最上位ビットに1がセットされる
// ただし、呼ぶときにcompflagにCP_USEFIELD2フラグがセットされていなければならない
// Note:
// buf[return]にattrがセットされる(NEWDIC4)
// NEWDIC4の場合、Get()の後ろに、attr = buf[return];を追加すること
ulong Japa::_Get2( byte *buf, int
#if defined(USE_COMP) || !defined(NOFIELD2)
	compflag
#endif
	, ulong limitlen, ulong totallen, IndexData *dic ) const
{
	byte *p = (byte*)buf;
	byte *orgp;
//#if MIXDIC && !(defined(DIC_UTF8) && defined(USE_BOCU1))
#if defined(MIXMJ) || defined(MIXJAPA)	// 2015.2.22 あまり自信が無いが、japaがtchar, _jcharがtcharなら不要のはずなので↑行の条件を変更した
	_mstrdef( recjapa, (const tchar*)japa, _jSingleByte );
#else
	const _jchar *recjapa = japa;
#endif
	int ljapa = _jcsbyte( recjapa );
#ifdef NEWDIC4
	wa_t _attr;
#endif
	if ( IsEmptyEx() ){
#ifdef NEWDIC4
		_attr = attr & ~WA_EX;
#else
		*p++ = attr & ~WA_EX;
#endif
		if ( FP_DIFF( p, buf ) + ljapa > limitlen )
			return (ulong)-1;
		__GetText( dic, recjapa, ljapa, p );
#if !defined(NOFIELD2)
		compflag &= ~CP_USEFIELD2;
#endif
	} else {
#ifdef NEWDIC4
		_attr = attr | WA_EX;
#else
		*p++ = attr | WA_EX;
#endif
		__GetText2( dic, recjapa, ljapa, p );
#if 0	// __GetTextの前
		_mcscpy( (_mchar*)p, recjapa );
		p += ljapa + sizeof(_mchar);
#endif
		// トータルの長さで圧縮ポリシーを変える
		if ( !totallen ){
			totallen = GetAllLen( );
		}
#ifdef USE_COMP
		if ( (compflag & CP_COMP2) == CP_COMP ){	// 圧縮するがなるべく圧縮するでは無い場合
			if ( totallen >= limitlen )
				compflag |= CP_COMP2;
		}
#endif
#if !defined(NOFIELD2)
		// 本当は圧縮した後のサイズで判断すればいいのだが、
		// 圧縮しながら作成するためここで判断
		int jtbsize;
		if ( compflag & CP_USEFIELD2 ){
			if ( totallen >= MAX_FIELD1JAPA ){
				jtbsize = sizeof(t_jtb2);
//				totallen += jlinks.get_num() * (sizeof(t_jtb2)-sizeof(t_jtb));
			} else {
				jtbsize = sizeof(t_jtb);
				compflag &= ~CP_USEFIELD2;
			}
		} else jtbsize = sizeof(t_jtb);
#else
		const int jtbsize = sizeof(t_jtb);
#endif
		if ( exp[0] ){
			*p++ = JT_EXP;
#ifdef NEWDIC4UNI
			*p++ = 0;	// reserved
#endif
#if defined(USE_COMP) && !defined(USE_BOCU1)
			if ( (p=(byte*)TextComp( (char*)p, (char*)buf, exp, compflag, limitlen )) == NULL ){
				return (ulong)-1;
			}
#else
			orgp = p;
			__GetText1( dic, exp, p );
			if ((int)(p-orgp)>LEXP){
				return (ulong)-1;
			}
#if 0
			_mcscpy( (_mchar*)p, (_mchar*)exp );
			p += _jcsbyte1( exp );
#endif
#endif
		}
#ifdef USE_REF
		if ( refnum ){
			*p++ = JT_REF;
#ifdef NEWDIC4UNI
			*p++ = 0;	// reserved
#endif
			*p++ = (byte)(refnum * sizeof( short ));
			for ( int i =0;i<refnum;i++ ){
				*((short*)p)++ = refdata[i];
			}
		}
#endif
		if ( pron[0] ){
			*p++ = JT_PRON;
#ifdef NEWDIC4UNI
			*p++ = 0;	// reserved
#endif
#if defined(USE_COMP) && !defined(USE_BOCU1)
			if ( (p=(byte*)TextComp( (char*)p, (char*)buf, pron, compflag, limitlen )) == NULL ){
				return (ulong)-1;
			}
#else
			orgp = p;
			__GetText1( dic, pron, p );
			if ((int)(p-orgp)>LPRON){
				return (ulong)-1;
			}
#endif
		}
		if ( FP_DIFF( p, buf ) + 1 > limitlen ){
			return (ulong)-1;
		}
#ifdef USE_JLINK
		for ( int i=0;i<jlinks.get_num();i++ ){
			JLink &jl = jlinks[i];
			DWORD l = jl.GetLength();
			if ( !l ){
#ifdef EPWING
				if ( jl.GetType() == JL_EPWING ){
					*p++ = JT_EPWING;
#ifdef NEWDIC4UNI
					*p++ = 0;	// reserved
#endif
					((EPWingField*)p)->bookno = ((JLEPWing&)jl).GetBookNo();
					((EPWingField*)p)->pos = ((JLEPWing&)jl).GetPos();
					p += sizeof(EPWingField);
				}
#endif
				continue;	// データが無い
			}
			bool fOver = false;
			// + JT_LINK + t_jtb* + ID + type + l + JT_END
			if ( FP_DIFF( p, buf ) + L_ExtAttr + jtbsize + 1 + l + L_ExtAttr > limitlen ){
#ifndef USE_COMP
				return (ulong)-1;
#else
				fOver = true;
				if ( compflag & CP_COMP ){
					goto comp;
				} else {
					// limitlenオーバー
					return (ulong)-1;
				}
#endif
			}
#ifdef USE_COMP
			if ( ( compflag & CP_COMP2 ) == CP_COMP2 && l >= CP_MINLEN ){
				// なるべく圧縮する
comp:
				byte *srcbuf = new byte[ l ];	// l には nocomplenが含まれている
				if ( !srcbuf ){
					if ( fOver ){
						return (ulong)-1;	// メモリ不足
					}
					goto notcomp;
				}
				byte *codbuf = new byte[ l ];
				if ( !codbuf ){
					delete[] srcbuf;
					if ( fOver ){
						return (ulong)-1;	// メモリ不足
					}
					goto notcomp;
				}
				if ( !jl.Get( srcbuf ) ){	// 生データ
					/// エラー処理
					delete[] srcbuf;
					delete[] codbuf;
					return (ulong)-1;		// OLEエラー
				}
#ifdef GUI
				cmpSetWindow( HWindow );
				if ( HWindow ){
					SendMessage( HWindow, UM_COMP, MSGCOMP_START, 0 );
				}
#endif
				int nocomplen;
#ifdef USE_BOCU1
				{
					byte *p = srcbuf + 1 + sizeof(t_id);
					// skip compressed title
					for(;*p;) p++;
					p++;
					nocomplen = (int)(p-srcbuf) +  + jl.GetHeaderLength();	// JL_... + ID + title + '\0' + α
				}
#else
				nocomplen = 1 + sizeof(t_id) + _tcsbyte1( jl.GetTitle( ) ) + jl.GetHeaderLength();	// JL_... + ID + title + '\0' + α
#endif
				long destlen;
				int r = cmpEncode( srcbuf + ( nocomplen - 1 ), l - ( nocomplen - 1 ), codbuf, destlen );	// -1はリンクタイプ分(srcbufにはJL_が入っていない）
#ifdef GUI
				if ( HWindow ){
					SendMessage( HWindow, UM_COMP, MSGCOMP_END, 0 );
				}
				cmpSetWindow( NULL );
#endif
				if ( r ){
					// 圧縮できた
					if ( fOver ){
						if ( FP_DIFF( p, buf ) + jtbsize + sizeof(t_noc) + nocomplen + destlen > limitlen ){
							// 圧縮しても足りない
							delete[] srcbuf;
							delete[] codbuf;
							return (ulong)-1;	// limitlenオーバー
						}
					}
//					int titlelen = nocomplen - sizeof(t_id) - 1;	// -1はリンクタイプ分
#if !defined(NOFIELD2)
					if ( compflag & CP_USEFIELD2 ){
						*(*(t_exattr**)&p)++ = JT_LINK | JT_BYTE | JT_COMP;
#ifdef NEWDIC4UNI
						*p++ = 0;	// reserved
#endif
						*(*(t_jtb2**)&p)++ = sizeof(t_noc) + nocomplen + destlen;
						*(*(t_noc**)&p)++ = (t_noc)nocomplen;
						*(*(t_jlink**)&p)++ = (t_jlink)jl.GetType();
//						*(*(t_id**)&p)++ = jl.GetID();
//						memcpy( p, jl.GetTitle(), titlelen );
//						p += titlelen;
					} else
#endif
					{
						JLinkStructC &jlsc = *(JLinkStructC*)p;
						jlsc.jtype = JT_LINK | JT_BYTE | JT_COMP;
#ifdef NEWDIC4UNI
						jlsc.reserved = 0;
#endif
						jlsc.size = (t_jtb)(sizeof(t_noc) + nocomplen + destlen);
						jlsc.nocomplen = (t_noc)nocomplen;	// 非圧縮長
						jlsc.jlink = (char)jl.GetType( );				// リンクタイプ
//						jlsc.id = jl.GetID( );
//						memcpy( jlsc.title, jl.GetTitle( ), titlelen );
						p = (byte*)&jlsc.id;
					}
					memcpy( p, srcbuf, nocomplen - sizeof(t_jlink) );
					delete[] srcbuf;
					p += nocomplen - sizeof(t_jlink);
					memcpy( p, codbuf, destlen );
					p += destlen;
					delete[] codbuf;
					goto next;
				} else {
					// 圧縮できない
					delete[] srcbuf;
					delete[] codbuf;
					if ( fOver ){
						return (ulong)-1;	// limitlenオーバー
					}
					goto notcomp;
				}
			}
#endif	// def USE_COMP
notcomp:
			*p++ = JT_LINK|JT_BYTE;
#ifdef NEWDIC4UNI
			*p++ = 0;	// reserved
#endif
#if !defined(NOFIELD2)
			if ( compflag & CP_USEFIELD2 ){
//				*(*(t_jtb2**)&p)++ = l+1;
				_alSetULong( (t_jtb2*)p, l+1 );
				p += sizeof(t_jtb2);
			} else
#endif
			{
//				*(*(t_jtb**)&p)++ = (t_jtb)(l + 1);
				_alSetUShort( (t_jtb*)p, l+1 );
				p += sizeof(t_jtb);
			}
			*p++ = (byte)jlinks[i].GetType( );
			if ( !jlinks[i].Get( p ) ){
				return (ulong)-1;		// OLEエラー
				// エラー処理
//				error = jlinks[i].GetErrorCode( );
//				r = false;
			}
			p += l;
next:;
		}
#endif	// USE_JLINK
		*p++ = JT_END;
#ifdef NEWDIC4UNI
		*p++ = 0;	// reserved
#endif
	}
	ulong ret = FP_DIFF( p, buf );
#ifdef NEWDIC4
	buf[ret] = _attr;
#endif
	return ret
#if !defined(NOFIELD2)
		| (compflag & CP_USEFIELD2 ? 0x80000000L : 0);
#endif
}

#if __JAPAMT
TMutex Japa::Mutex;
#endif

#if !defined(SMALL)
tchar *Japa::MergeBuffer = NULL;
void Japa::FreeMergeBuffer()
{
	if (!MergeBuffer)
		return;
	delete[] MergeBuffer;
	MergeBuffer = NULL;
}
#endif	// !SMALL

#ifdef USE_BOCU1
tchar *Japa::BocuOutBuffer = NULL;
#if !__JAPAMT && defined(_DEBUG)
bool Japa::BocuOutBufferIn = false;
#endif
tchar *Japa::GetBocuOutBuffer(int reqlen)
{
	__assert(__islocked__);
#if !__JAPAMT && defined(_DEBUG)
	__assert(!BocuOutBufferIn);		// Check reenter for the thread unsafe.
	BocuOutBufferIn = true;
#endif
	__assert(reqlen<=LJALL);
	if (!BocuOutBuffer)
		BocuOutBuffer = new tchar[LJALL+1];
	return BocuOutBuffer;
}
#if !__JAPAMT && defined(_DEBUG)
void Japa::ReleaseBocuOutBuffer()
{
	__assert(BocuOutBufferIn);
	BocuOutBufferIn = false;
}
#endif
void Japa::FreeBocuOutBuffer()
{
	__lock__;
	if (BocuOutBuffer){
		delete[] BocuOutBuffer;
		BocuOutBuffer = NULL;
	}
}

#endif	// USE_BOCU1

byte *Japa::GetBuffer = NULL;
int Japa::GetBufferSize = 0;
void Japa::FreeGetBuffer()
{
	__lock__;
	if (GetBuffer){
		delete[] GetBuffer;
		GetBuffer = NULL;
		GetBufferSize = 0;
	}
}

void Japa::FreeBuffers()
{
#ifndef SMALL
	FreeMergeBuffer();
#endif
#if defined(USE_BOCU1) && defined(_DEBUG)
	FreeBocuOutBuffer();
#endif
	FreeGetBuffer();
}

// delim!=NULLである場合は、mergeせずに結合する
// mul : 標準の最大文字数の何倍まで許容するか
// -1 : エラー（メモリ不足）
//  1 : オーバー
//  0 : 正常
// Merge()を使用した処理が完了した後は、FreeMergeBuffer()を呼ぶこと！
int Japa::Merge( Japa &j, const tchar *delim, Pdic *dic, int flags, PronTable *prontable )
{
	const int nmul = 3;

	int overf = 0;
#ifdef PDICW
	tchar *newpron = NULL;
	const tchar *sp;
#endif
#if USE_MERGE
	tchar *lp, *lp2, *_lp2;
	ushort c;
	static const tchar StrLevel[] = _T("【レベル】");
#ifdef PDICW
	static const tchar StrPron1[] = _T("【発音");
	static const tchar StrWave[] = _T("【音声");
#endif
	static const tchar StrAtmark[] = _T("【＠】");
	static const tchar StrAtmark2[] = _T("【@】");
#endif

	wa_t a = j.attr;

	int japalen1   = _tcslen1(japa);
	int explen1    = _tcslen1(exp);
	//int j_japalen1 = _tcslen1(j.japa);
	int j_explen1  = _tcslen1(j.exp);

	const int l = LJALL * 2 + 100;	// 用例の最大長にあわせる
	if (!MergeBuffer){
		MergeBuffer = new tchar[ l+1 ];
		if ( !MergeBuffer )
			return -1;
	}

	const tchar *dstp;
	int r;
	// 日本語訳 //
	if ( (flags & TRS_JAPA) || ((flags & (HK_CONVLEVEL|TRS_LEVEL))==(HK_CONVLEVEL|TRS_LEVEL)) ){
		if ((flags&0xFFFF)==0 && !this->japa[0]){
			// そのまま
			if (j.japa.exist()){
				japa = j.japa;
			}
			goto jattr;
		} else {
			if ( flags & TRS_JAPA ){
				memcpy(MergeBuffer,(const tchar*)japa,LENTOBYTE(japalen1));
				if (j.japa[0]){
					if ( delim ){
						// append mode
						if ( japalen1 == 2 && japa[0] == '?' ){
							japa[0] = '\0';	// とりあえず、日本語訳は空白に
							japalen1 = 1;
						}
						dstp = MergeBuffer + japalen1 - 1;
						if (delim[0] && japa[0]){
							_tcscpy((tchar*)dstp,delim);
							dstp += _tcslen(delim);
						}
#if USE_MERGE
						if ( flags & (HK_ZEN2HAN|HK_CONVDELIM) ){
							preMerge( (tchar*)dstp, j.japa, flags );
						} else
#endif
						{
							_tcscpy( (tchar*)dstp, j.japa );
						}
					} else {
#if USE_MERGE
						r = jmerge( MergeBuffer, j.japa, LJAPA*nmul, flags );
#else
						r = -1;	// not supported
#endif
						if ( r == -1 ){
							overf = -1;
							goto exit;
						}
						overf |= r;
					}
				}
				dstp = MergeBuffer;
			} else {
				dstp = j.japa;
			}
#ifdef PDUTY
			extern int oflag;
			if ( oflag ){
				tchar *p = MergeBuffer;
				MergeBuffer = new tchar[ _tcslen( p ) + 1 ];
				MergeBuffer[0] = '\0';
				r = jmerge(MergeBuffer, p, LJAPA*nmul, flags, false);
				delete[] p;
				if ( r == -1 ){
					overf = -1;
					goto exit;
				}
				overf |= r;
			}
#endif	// PDUTY

#if USE_MERGE
			// 【レベル】変換
			if ( flags & HK_CONVLEVEL ){
				lp = _tcsstr( (tchar*)dstp, StrLevel );
				if ( lp ){
					lp2 = lp + tsizeof(StrLevel)-1;
					int level = _ttoi(lp2);
					if (level>WA_MAXLEVEL)
						level = WA_MAXLEVEL;
					a = (wa_t)(( a & ~WA_LEVELMASK ) | level);	// 数値変換
					if ( flags & TRS_JAPA ){
						// レベルの削除
						// 数値の終わりか、区切り記号まで
						while ( 1 ){
							_lp2 = lp2;
							LD_CHAR( c, lp2 );
							if ( IS_ENDCHAR(c) ){
								lp2 = _lp2;
								break;
							}
							if ( !_istdigit( c ) ){
								if ( !isdelim( c ) ){
									// 次が区切り文字でない場合、１つ戻す
									lp2 = _lp2;
								}
								break;
							}
						}
						memmove( lp, lp2, _tcsbyte1( (tchar*)lp2 ) );
					}
				}
			}
#endif	// USE_MERGE

			if ( flags & TRS_LEVEL ){
				// 単語レベルはマージ元のレベルを優先？
				if ( attr & WA_LEVELMASK ){
					attr |= (wa_t)( a & ~WA_LEVELMASK );
				} else {
					attr |= a;
				}
			}

#if USE_MERGE
			if ( (flags & HK_CONVPRON) && prontable ){
				// 発音記号変換
				// Zen2Hanに対応するため、分ける必要がある
				sp = j.japa;
				ushort c;
				while ( 1 ){	// 複数対応
		//			pronflag = 1;
					// 【発音 のチェック
					lp = _tcsstr( (tchar*)sp, StrPron1 );
					if ( !lp ){
						break;
#if 0
						pronflag = 2;
						lp = _tcsstr( (tchar*)sp, StrPron2 );
						if ( !lp )
							break;
#endif
					}
					// 】 or !】 or ！】 のチェック
					lp2 = lp + tsizeof(StrPron1)-1;
					if ( *lp2 == '!' ) lp2++;
					else if ( *(ushort*)lp2 == _TW("！") ) lp2+=_MBLEN1;
					if ( *(ushort*)lp2 != _TW("】") ) break;	// not 発音記号
					lp2 += _MBLEN1;

					tchar *dp;
					if ( !newpron ){
						newpron = new tchar[ _tcslen( lp )<<1 ];
						dp = newpron;
					} else {
						dp = newpron + _tcslen(newpron);
						*dp++ = ',';
					}
					tchar *_dp = dp;	// 先頭にカンマが来るのを防ぐ
		//			lp2 = lp + (pronflag == 1 ? tsizeof(StrPron1)-1 : tsizeof(StrPron2)-1 );
					while ( 1 ){
						_lp2 = lp2;
						LD_CHAR( c, lp2 );
						if ( !((tuchar)c) ){
							lp2 = _lp2;
							break;
						}
						if ( c == ',' || c == (tuchar)_T('､') || c == (tuchar)_T('｡') || c == CODE_COMMA1 || c == CODE_COMMA ){
							break;
						}
						if ( c == CODE_LBR6 || c == CODE_SPACE){
							if ( dp != _dp ){
								*dp++ = ',';
								_dp = dp;	// カンマが連続するのを防ぐ
							}
							if ( c != CODE_LBR6 ) continue;
						} else
						if ( islbr2( c ) ){
							lp2 = _lp2;
							break;
						}
						const tchar *code = prontable->find( _lp2, &lp2 );
						if ( code ){
							_tcscpy( dp, code );
							dp += _tcslen( code );
						} else {
							ST_CHAR( c, dp );
						}
					}
					if ( dp>newpron && dp[-1] == ',' ) dp--;
					*dp = '\0';
					sp = lp2;
				}
				if ( flags & TRS_JAPA ){
					// 発音記号削除
					sp = MergeBuffer;
					while ( 1 ){	// 複数対応
			//			pronflag = 1;
						lp = _tcsstr( (tchar*)sp, StrPron1 );
						if ( !lp ){
							break;	// not 発音記号
#if 0
							pronflag = 2;
							lp = _tcsstr( (char*)sp, StrPron2 );
							if ( !lp )
								break;
#endif
						}
						// 】 or !】 or ！】 のチェック
						lp2 = lp + tsizeof(StrPron1)-1;
						if ( *lp2 == '!' ) lp2++;
						else if ( *(ushort*)lp2 == _TW("！") ) lp2+=_MBLEN1;
						if ( *(ushort*)lp2 != _TW("】") ) break;	// not 発音記号
						lp2 += _MBLEN1;
			//			lp2 = lp + (pronflag == 1 ? tsizeof(StrPron1)-1 : tsizeof(StrPron2)-1 );
						while ( 1 ){
							_lp2 = lp2;
							LD_CHAR( c, lp2 );
							if ( !((tuchar)c) ){
								lp2 = _lp2;
								break;
							}
							if ( /* c == ',' || */ c == (tuchar)_T('､') || c == (tuchar)_T('｡') || c == CODE_COMMA1 || c == CODE_COMMA ){
								break;
							}
							if ( islbr2( c ) && (c != CODE_LBR6) ){
								lp2 = _lp2;
								break;
							}
						}
						memmove( lp, lp2, _tcsbyte1( lp2 ) );
						sp = lp;
					}
				}
			}
			if ( flags & HK_CONVWAVE ){
				// 音声ファイルリンク変換
				sp = j.japa;
				ushort c;
				while ( 1 ){	// 複数対応
					// 【音声 のチェック
					lp = _tcsstr( (tchar*)sp, StrWave );
					if ( !lp ){
						break;
					}
					// 】を探す
					lp += tsizeof(StrWave)-1;
					if ( *(ushort*)lp != _TW("】") ) break;	// not 【音声】
					lp += _MBLEN1;
					for(;*lp;lp++){if (*lp!=' ') break;}	// 先頭のspaceは飛ばす

					lp2 = lp;
					while ( 1 ){
						_lp2 = lp2;
						LD_CHAR( c, lp2 );
						if ( !((tuchar)c) || islbr2(c) ){
							lp2 = _lp2;
							break;
						}
						if ( c == ',' || c == (tuchar)_T('､') || c == CODE_COMMA1 || c == CODE_COMMA ){
							break;
						}
					}
					if (lp!=lp2){
#ifdef USE_FILELINK
						tnstr filename( lp, STR_DIFF(_lp2,lp) );
						jlinks.add( new JLFile( dic, filename, dic ? dic->GetObjectNumber():0 ) );
#endif
					}
					sp = lp2;
				}
				if ( flags & TRS_JAPA ){
					// 【音声】削除
					sp = MergeBuffer;
					while ( 1 ){	// 複数対応
						lp = _tcsstr( (tchar*)sp, StrWave );
						if ( !lp ){
							break;	// not 【音声】
						}
						// 】を探す
						lp2 = lp + tsizeof(StrWave)-1;
						if ( *(ushort*)lp2 != _TW("】") ) break;	// not 【音声】
						lp2 += _MBLEN1;
						while ( 1 ){
							_lp2 = lp2;
							LD_CHAR( c, lp2 );
							if ( !((tuchar)c) ){
								lp2 = _lp2;
								break;
							}
							if ( c == ',' || c == (tuchar)_T('､') || c == CODE_COMMA1 || c == CODE_COMMA ){
								break;
							}
							if ( islbr2( c ) ){
								lp2 = _lp2;
								break;
							}
						}
						memmove( lp, lp2, _tcsbyte1( lp2 ) );
						sp = lp;
					}
				}
			} // 【音声】
#endif	// PDICW

#if USE_MERGE
			if ( (flags & TRS_JAPA) && (flags & HK_REMOVEATMARK) ){
				// ＠削除
				RemoveStr(MergeBuffer, StrAtmark, StrAtmark2, true);
			}
#endif
			if ( flags & TRS_JAPA ){
				japa.set( MergeBuffer );
			}
		}
	} else {
jattr:;
		if ( flags & TRS_ATTR )
			attr |= a & (WA_MEMORY | WA_JEDIT);
		if ( flags & TRS_LEVEL ){
			// 単語レベルは設定されている場合のみマージ先を優先
			if ( attr & WA_LEVELMASK ){
				// attr |= (wa_t)( a & ~WA_LEVELMASK );
			} else {
				attr |= a & WA_LEVELMASK;	// 設定がないため、マージ元を使用
			}
		}
	}

#ifdef PDICW
	// 発音記号 //
	if ( flags & TRS_PRON ){
		if ((flags&(HK_ZEN2HAN|HK_CONVDELIM|HK_CONVPRON))==0 && !this->pron[0]){
			// そのまま
			if (j.pron.exist()){
				this->pron = j.pron;
			}
		} else {
			if (delim){
				if ( newpron ){
					CatStr( pron, delim, newpron );
				}
				if (j.pron[0]){
					CatStr( pron, delim, j.pron );
				}
			} else {
#if USE_MERGE
				_tcscpy( MergeBuffer, pron );
			//	overf |= jmerge( MergeBuffer, j.pron, LPRON*nmul, 0, SingleByte & SRCH_PRON );
				overf |= expmerge( MergeBuffer, j.pron, LPRON*nmul, 0, DLM_COMMA );
				if ( newpron ){
					overf |= expmerge( MergeBuffer, newpron, LPRON*nmul, 0, DLM_COMMA );
				}
				pron.set( MergeBuffer );
#else
				overf |= -1;	// not supported
#endif
			}
		}
	}
	if (newpron){
		delete[] newpron;
	}
#endif	// PDICW

// 用例のマージはどうしよう？？
// 基本的に追加で、" / "を区切り文字とする
// また、重複チェックも行なう
	if ( flags & TRS_EXP ){
		if ( explen1 + j_explen1 - 1 >= LEXP * nmul ){
			overf = 1;
		} else {
			if ((flags&(HK_ZEN2HAN|HK_CONVDELIM))==0 && !this->exp[0]){
				// そのまま
				if (j.exp.exist()){
					this->exp = j.exp;
				}
			} else {
				memcpy( MergeBuffer, (const tchar*)exp, LENTOBYTE(explen1) );
				if (j.exp[0]){
					if (delim){
						// concatination
#if USE_MERGE
						if ( flags & (HK_ZEN2HAN|HK_CONVDELIM) ){
							dstp = MergeBuffer + explen1 - 1;
							if (exp[0]){
								_tcscpy((tchar*)dstp,STR_DLMEX);
								dstp += _tcslen(STR_DLMEX);
							}
							preMerge( (tchar*)dstp, j.exp, flags );
							exp.set( MergeBuffer );
						} else
#endif
						{
							if (exp[0]){
								exp.cat(delim, j.exp);
							} else {
								exp.set(j.exp);
							}
						}
					} else {
						// 用例マージ
#if USE_MERGE
						r = expmerge( MergeBuffer, j.exp, l, flags );
						if ( r == -1 ){
							overf = -1;
							goto exit;
						}
						exp.set( MergeBuffer );
#else
						overf |= -1;	// not supproted
#endif
					}
				}
			}
		}
	}

#ifdef USE_REF
	// 参照のマージ
	if ( refnum < j.refnum ){
		refnum = j.refnum;
	}
	for ( int i=0;i<refnum;i++ ){
		if ( refdata[i] == -1 )
			refdata[i] = j.refdata[i];	// 未参照の場合のみ変更（で良いかな？）
	}
#endif

#ifdef USE_JLINK
	if ( flags & TRS_OBJECT ){
		// リンクデータのマージ
		// 単純に追加？
		{
		for ( int i=0;i<j.jlinks.get_num();i++ ){
			jlinks.add( j.jlinks[i].Clone( dic ) );
		}
		}
	}
#endif

exit:
	return overf;
}

// lengthは返り値がNULLでないときのみ有効
// length[31] == 1 : Field2を使用した場合(NEWDIC3)
// 戻り値はdeleteしてはいけない
// return[length]にattr(NEWDIC4)
byte *Japa::Get2( uint &length, int compflag, uint limitlen, IndexData *dic ) const
{
	__lock__;
	uint len = GetAllLen( );
	if ( len >= limitlen && !compflag )
		return NULL;	// 制限超過
	int bufsize = len
#ifdef USE_JLINK
		+ jlinks.get_num() * (sizeof(t_jtb2)-sizeof(t_jtb))
#endif
#ifdef NEWDIC4
		+1	// for attr
#endif
#ifdef USE_BOCU1
		// bocuでは圧縮後のほうが長くなる場合があるため
		//+ 100 + (len>>2) // ←これでは小さすぎるときがあった（unicode random test) 2014.7.23
		+ 100 + (len>>1)
#endif
		;
	if (bufsize>GetBufferSize){
		if (GetBuffer)
			delete[] GetBuffer;
		GetBuffer = new byte[bufsize];
		if ( !GetBuffer ){
			GetBufferSize = 0;
			return NULL;	// メモリ不足
		}
		GetBufferSize = bufsize;
	}
	if ( (length = _Get2( GetBuffer, compflag, limitlen, len, dic )) == (ulong)-1 ){
		return NULL;
	}
	return GetBuffer;
}

#ifdef USE_REF
void Japa::GetRef( char *buf, int i )
{
	if ( refdata[i] == -1 ){
		buf[0] = '\0';
	} else {
#ifdef	_WINDOWS
		wsprintf( buf, "%04d", refdata[i] );
#else
		esprintf( buf, "%04d", refdata[i] );
#endif
	}
}

void Japa::SetRef( const char *buf, int i )
{
	if ( buf[0] ){
		refdata[i] = (short)atoi( buf );
		if ( refnum <= i ){
			refnum = i + 1;
		}
	} else {
		refdata[i] = -1;
		TruncRef( );
	}
}

void Japa::TruncRef( )
{
	while ( refnum && refdata[refnum-1] == -1 ) refnum--;
}
#endif

bool Japa::IsQWord() const
{
	if ( japa[0] == '?' && japa[1] == '\0' ){
		return true;
	}
	return false;
}
// 文字列の結合
// dest = dest + delim + src;
// dest[0] == '\0'である場合：
//	dest = src;
// src[0] == '\0'である場合：
//	dest = dest;
void CatStr( _jMixChar &dest, const tchar *delim, const tchar *src )
{
	if ( dest[0] )
		if ( delim[0] )
			dest.cat( delim, src );
		else
			dest.cat( src );
	else
		dest.set( src );
}

// _mcscpyのreturn valueが次のdestination pointer version
// return valueは終端'\0'の次へのpointer
// japaでしか使用しないのでここで宣言
_mchar *_tcscpy2( register _mchar *dst, register const _mchar *src )
{
	for(;;){
		register _mchar c = *src++;
		*dst++ = c;
		if ( c == '\0' ){
			break;
		}
	}
	return dst;
}

#ifdef DISPDICNAME
bool Japa::DispDicName = DDN_DEFAULT;
tnstr Japa::DicNameTemplate;
#include "filestr.h"
void AddDicName( tnstr &s, const tchar *dicname )
{
	tnstr temp = GetAddDicName(dicname);
	temp += ( s );
	s = temp;
}
tnstr GetAddDicName( const tchar *dicname )
{
	if (Japa::DicNameTemplate.empty()){
		tnstr temp;
		temp = _t("[");
		temp += GetFileName( dicname );
		temp += _t("] ");
		return temp;
	} else {
		return tnsprintf(Japa::DicNameTemplate.c_str(), GetFileName(dicname));
	}
}
#endif

