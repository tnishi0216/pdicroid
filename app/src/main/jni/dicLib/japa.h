#ifndef	__JAPA_H
#define	__JAPA_H

#include "pdconfig.h"
#include "dicdef.h"
#include "mstr.h"
#include "jtype.h"

#if (defined(DIC_UTF8) && defined(USE_BOCU1)) || defined(DIC_BOCU)
#define	_JisT
#define	_jsize		_tsize
typedef tchar		_jchar;
#define	_jcslen		_tcslen
#define	_jcscpy		_tcscpy
#define	_JChar		tnstr
#define	_jnext		CharNext
#define	_jlower		A_lower
#define	_jcschr		_tcschr
#define	_jcscmp		_tcscmp
#define	_jcsnicmp	_tcsnicmp
#define	_jcsncmp	_tcsncmp
#define	_jmbsnicmp	_tcsnicmp
#define	_JLENTOBYTE	LENTOBYTE
#define	_jcsbyte	_tcsbyte
#define	_jcsbyte1	_tcsbyte1
#else
#define	_JisM
#define	_jsize		_msize
typedef _mchar		_jchar;
#define	_jcslen		_mcslen
#define	_jcscpy		_mcscpy
#define	_JChar		_MChar
#define	_jnext		_mnext
#define	_jlower		_mlower
#define	_jcscmp		_mcscmp
#define	_jcschr		_mcschr
#define	_jcsnicmp	_mcsnicmp
#define	_jcsncmp	_mcsncmp
#define	_jmbsnicmp	_mmbsnicmp
#define	_JLENTOBYTE	_MLENTOBYTE
#define	_jcsbyte	_mcsbyte
#define	_jcsbyte1	_mcsbyte1
#endif

class Pdic;

// 登録項目
// 基本的にSQM_と同じ
// 下位16bitは、henkanのHK_xxx フラグで使用する
// Do not change these values so as to be used in profile
#define	TRS_KEYWORD	0x00800000
#define	TRS_ATTR	0x00300000
#define	TRS_PRON	0x00080000
#define	TRS_JAPA	0x00010000
#define	TRS_EXP		0x00020000
#define	TRS_LEVEL	0x00040000
#define	TRS_OBJECT	0x01000000
#define	TRS_ALL		0x7fff0000

// 属性処理を含む日本語訳バッファ

class IndexData;

#if defined(USE_JLINK)
#include "jlink.h"
#else
// dummy class to avoid the compile error
class JLink {
};
#endif

#if RO_COMP && !defined(USE_JLINK)
#define	NO_COMPLINK		1
#else
#define	NO_COMPLINK		0
#endif

#if RO_COMP || !defined(USE_JLINK)
#define	CHKJOBJ		1		// 登録時、objectのcheckが必要
#else
#define	CHKJOBJ		0
#endif

#if defined(__MT__) && INETDIC
#define	__JAPAMT	1
#else
#define	__JAPAMT	0
#endif

class Japa {
protected:
#if __JAPAMT
	static TMutex Mutex;
#endif	// !__JAPAMT

#ifndef SMALL
	static tchar *MergeBuffer;
	static void FreeMergeBuffer();
#endif	// !SMALL
#ifdef USE_BOCU1
	static tchar *BocuOutBuffer;
#if !__JAPAMT && defined(_DEBUG)
	static bool BocuOutBufferIn;
#endif
	tchar *GetBocuOutBuffer(int reqlen);
	void ReleaseBocuOutBuffer()
#if !__JAPAMT && defined(_DEBUG)
		;
#else
		{}
#endif
	static void FreeBocuOutBuffer();
#endif	// USE_BOCU1
	static uint8_t *GetBuffer;
	static int GetBufferSize;
	static void FreeGetBuffer();
public:
	static void FreeBuffers();
	// メンバ変数
#ifdef USE_REF
	int refnum;
	short refdata[ NMAXREF ];
#endif
	wa_t attr;
public:
#if CHKJOBJ
	bool HasUndefObj;
#endif
#ifdef MIXJAPA
	TNMixChar exp;
	TNMixChar pron;
	TNMixChar japa;
#else
	tnstr exp;
	tnstr pron;
	tnstr japa;
#endif
#ifdef GUI
	static HWND HWindow;		// 圧縮中のダイアログの表示のための親ウィンドウ
	static void SetWindow( HWND hwnd = NULL )
		{ HWindow = hwnd; }
#endif
#ifdef USE_JLINK
	JLinks jlinks;
	uint GetJLinksLength( int fixedlen ) const;
	void AddObject( JLink *jlink )
		{ jlinks.add( jlink ); }
	void DeleteObject( int no )
		{ jlinks.del( no ); }
	void ChangeDic( Pdic &dic )
		{ jlinks.ChangeDic( dic ); }
#endif
	bool IsError( ) const;	// 未サポート
protected:
	static int InstCnt;
	void IncInstCnt(){ InstCnt++; }
	void DecInstCnt(){ InstCnt--; }
public:
	Japa( );
//	Japa( const tchar *str, int len );
	Japa( const Japa &_j )
	{
		IncInstCnt();
		operator = ( _j );
	}
	~Japa();
	void clear( );
	int GetLevel( ) const
		{ return attr & WA_LEVELMASK; }
	void SetLevel( int level )
		{ attr = (wa_t)((attr & ~WA_LEVELMASK) | level); }

	bool HasUndefObject() const
		{ return
#if CHKJOBJ
			HasUndefObj;
#else
			false;
#endif
		}

	void SetAll( const uint8_t *src, int l, IndexData *dic, bool field2, wa_t attr);	// 返り値はエラーかどうか
	void SetAll2( const tchar *src, bool dist=true);		// フォーマットテキスト形式から
													// dist : 訳 / 用例の区別


	bool IsEmpty( ) const;
	bool IsEmptyEx( ) const;
#ifdef SMALL
	void Cat( Japa &j, Pdic *dic, const tchar *delim, int flags );		// 辞書コンバート専用のcat
#else	// !SMALL
	// なるべくMerge()関数を使用すること！
	void Cat( Japa &j, Pdic *dic, const tchar *delim, int flags )
#ifdef USE_SINGLEBYTE
		{ Merge( j, delim, dic, 0, flags, NULL ); }
#else
		{ Merge( j, delim, dic, flags, NULL ); }
#endif

#endif	// !SMALL
	void cat( const tchar *str )
		{ japa.cat( str ); }
	int jcut( int len )
		{ return ::jcut( japa, len ); }

	uint _Get2( uint8_t *buf, int compflag, uint limitlen, uint totallen, IndexData *dic ) const;
	uint8_t *Get2( uint &length, int compflag, uint limitlen, IndexData *dic ) const;

	int GetAllLen( ) const;
	void operator = ( const Japa &j );
	operator const tchar *()	{return (const tchar *)japa;}
	tchar &operator [] ( int i )
		{ return japa[i]; }
	bool operator == ( const Japa &j ) const;
	bool operator != ( const Japa &j ) const;

	void SetPron( const tchar *str )
		{ pron.set( str ); }
	void SetExp( const tchar *str )
		{ exp.set( str ); }
	void SetJapa( const tchar *str )
		{ japa.set( str ); }
	inline void SetAttr( uchar _attr )
		{ attr = _attr; }

	static inline wa_t MergeAttr( wa_t attr1, wa_t attr2 )
		{
			if ( attr1 & WA_LEVELMASK ){
				return attr1 | (wa_t)( attr2 & ~WA_LEVELMASK );
			} else {
				return attr1 | attr2;
			}
		}

#ifdef MIXJAPA
	TNMixChar &GetPron( )
		{ return pron; }
	TNMixChar &GetExp( )
		{ return exp; }
	TNMixChar &GetJapa( )
		{ return japa; }
	tnstr &GetPronEx( )
		{ return pron.GetChar(); }
	tnstr &GetExpEx( )
		{ return exp.GetChar(); }
	tnstr &GetJapaEx( )
		{ return japa.GetChar(); }
#else
	tnstr &GetPron( )
		{ return pron; }
	tnstr &GetExp( )
		{ return exp; }
	tnstr &GetJapa( )
		{ return japa; }
	tnstr &GetPronEx( )
		{ return pron; }
	tnstr &GetExpEx( )
		{ return exp; }
	tnstr &GetJapaEx( )
		{ return japa; }
#endif
	uchar GetAttr( ) const
		{ return attr; }
	bool IsMemory() const
		{ return TO_BOOL(attr & WA_MEMORY); }
	bool IsEdit() const
		{ return TO_BOOL(attr & WA_JEDIT); }
	void SetMemory(bool f)
		{ attr = (attr & ~WA_MEMORY) | (f?WA_MEMORY:0); }
	inline void SetMemory()
		{ attr |= WA_MEMORY; }
	inline void ClearMemory()
		{ attr &= ~WA_MEMORY; }
	void SetModify(bool f)
		{ attr = (attr & ~WA_JEDIT) | (f?WA_JEDIT:0); }
	void SetModify()
		{ attr |= WA_JEDIT; }

	void GetRef( tchar *buf, int i );
	void SetRef( const tchar *buf, int i );
	void TruncRef( );	// 最小のrefnumにセットする

#ifdef USE_SINGLEBYTE
	int Merge( Japa &j, const tchar *delim, class Pdic *dic, int SingleByte, int flags, class PronTable *prontable=NULL );	// dicはマージ先の辞書を指定する
#else	// !USE_SINGLEBYTE
	int Merge( Japa &j, const tchar *delim, class Pdic *dic, int flags, class PronTable *prontable=NULL );	// dicはマージ先の辞書を指定する
#endif	// !USE_SINGLEBYTE
	bool IsOverFlow( ) const;
	int Cut();		// cutされた場合は、1が返る
	int CutOld( );	// NEWDIC形式の長さに合わせ、登録できない発音記号などをカットする。英単語、日本語訳、用例がカットされた場合のみ１を返す
	bool IsOld( ) const;	// 古い辞書形式(NEWDIC)に登録可能？
public:
	static uint8_t *Decode( const uint8_t *src, uint jtblen, uint &decodelen );

	bool IsQWord() const;		// ? だけの日本語訳か？
public:
	static bool DispDicName;
	static tnstr DicNameTemplate;
	Japa &operator = (const class WJapa &j);	// Defined in PdicUni.cpp
	Japa &operator = (const class CJapa &j);	// Defined in PdicAnsi.cpp
};

void AddDicName( tnstr &s, const tchar *dicname );
tnstr GetAddDicName( const tchar *dicname );

extern const tchar *StrQ;

#define	DDN_DEFAULT	false

#ifdef DISPDICNAME

#ifdef USE_JLINK
#define	__SetJLinkDispDicName(flag)	JLink::DispDicName = flag
#else
#define	__SetJLinkDispDicName(flag)
#endif

#if DDN_DEFAULT
// DDN default is enabled.
#define	__EnableDDNBegin(flag)
#define	__DisableDDNBegin() \
	bool __bdispdicname = Japa::DispDicName; \
	do { \
		Japa::DispDicName = false; \
		__SetJLinkDispDicName(false); \
	} while(0)
#define	__EnableDDNEnd()
#define	__DisableDDNEnd() \
	do { \
		Japa::DispDicName = __bdispdicname; \
		__SetJLinkDispDicName(__bdispdicname); \
	} while(0)

#else	// DDN default is disabled.

#define	__EnableDDNBegin(flag) \
	bool __bdispdicname = Japa::DispDicName; \
	if (flag){ \
		Japa::DispDicName = true; \
		__SetJLinkDispDicName(true); \
	}
#define	__DisableDDNBegin()
#define	__EnableDDNEnd() \
	do { \
		Japa::DispDicName = __bdispdicname; \
		__SetJLinkDispDicName(__bdispdicname); \
	} while(0)
#define	__DisableDDNEnd()

#endif	// DDN_DEFAULT

#else	// !DISPDICNAME
#define	__EnableDDNBegin(flag)
#define	__DisableDDNBegin()
#define	__EnableDDNEnd()
#define	__DisableDDNEnd()
#endif

#endif	// __JAPA_H

