#ifndef __JLINK_H
#define	__JLINK_H

#include "pddefs.h"
#include "dicdef.h"

#define	DDN_DEFAULT	false	// defined in japa.h as well.

#ifdef USE_JLINK

#ifdef _Windows
#include "draw4.h"

#if !defined(__DIB_H) && defined(GUI)
#include "dib.h"
#endif

#include "pdcdefs.h"
#endif // _Windows

#define	JL_UFO		0x00
#define	JL_OLE		0x01
#define	JL_FILE		0x02
//#define	JL_VOICE	0x03	// 音声オブジェクト
#define	JL_IMAGE	0x04	// 画像データ -> JL_FILEIMAGE
#define	JL_FILEIMAGE 0x05	// embededd file image object.
//#define	JL_RTF		0x06	// RTF
#define	JL_EPWING	0x07	// EPWing link
//#define	JL_ICONFILE	0x08	// Icon File Object


/////////////// for GUI //////////////////////////////////////////

#define	CFS_COPY		0x01
#define	CFS_PASTE		0x02
#define	CFS_PASTELINK	0x04

#define	F_INPLACE		0x8000	// inplace-editing

// エラーコード表	///////////////////////////////////////////////////////////
#define	JLE_NODATA		256		// Getしようとしたが、データはない(プログラミングエラー)
#define	JLE_NOTENOUGH	257		// メモリが足りない

extern const tchar *StrLinkError;

#if defined(CPBTEST) || defined(CBUILER)
//#pragma	option	-a1
#endif
struct JLinkFormat {
	uint8_t type;		// リンクの種類
	uint8_t data[1];	// データ本体
};

typedef unsigned int t_id;

#ifdef _Windows
// ファイルリンクオブジェクトの辞書上のイメージ
struct FileLinkField {
	short	size;		// FileLinkFieldのサイズ
	short	nMag;
	short	nAspect;
//	uint8_t data[ 1 ];		// データ(可変長) データ部の位置は
						// FileLinkField of;
						// uint8_t *data = of.GetDataPtr( ); で求めること！
	uint8_t *GetDataPtr( )
		{ return (uint8_t*)(((uint8_t*)this) + size); }
		// この構造体の先頭を指しているポインタ(top)からdataの先頭アドレスを求める
};

// ファイルリンクオブジェクトの辞書上のイメージ
struct FileImageField {
	short	size;		// FileLinkFieldのサイズ
	short	nMag;
	short	nAspect;
//	uint8_t data[ 1 ];		// データ(可変長) データ部の位置は
						// FileLinkField of;
						// uint8_t *data = of.GetDataPtr( ); で求めること！
	uint8_t *GetDataPtr( )
		{ return (uint8_t*)(((uint8_t*)this) + size); }
		// この構造体の先頭を指しているポインタ(top)からdataの先頭アドレスを求める
};

struct ImageField {
	short	size;		// = sizeof(ImageField)
	short	imgtype;	// 
	int		headersize;	// header for image data
	short	nMag;
	short	nAspect;
//	uint8_t header[ 1 ];	// ヘッダー(可変長)
//	uint8_t data[ 1 ];		// データ(可変長) データ部の位置は
						// ImageField df;
						// uint8_t *data = df.GetDataPtr( ); で求めること！
	uint8_t *GetDataPtr( )
		{ return (uint8_t*)(((uint8_t*)this) + size + headersize); }
		// この構造体の先頭を指しているポインタ(top)からdataの先頭アドレスを求める
	uint8_t *GetHeaderPtr( )
		{ return (uint8_t*)(((uint8_t*)this) + size); }
};

#if 0	// to be deleted.
// RTFオブジェクトの辞書上のイメージ
struct RTFField {
	short	size;		// RTFFieldのサイズ
	short	nMag;
	short	nAspect;
//	uint8_t data[ 1 ];		// データ(可変長) データ部の位置は
						// RTFField of;
						// uint8_t *data = of.GetDataPtr( ); で求めること！
	uint8_t *GetDataPtr( )
		{ return (uint8_t*)(((uint8_t*)this) + size); }
		// この構造体の先頭を指しているポインタ(top)からdataの先頭アドレスを求める
};
#endif

struct HTMLField {
	short	size;	// sizeof(HTMLField)
};

// JLink error code table
// 0 ～ 255 までは PDIC辞書と同じ
// 256 : Clone, Getなどで転送元の辞書からデータを得る事ができなかった(内部エラー?)

#endif	// def GUI

#if defined(CPBTEST) || defined(CBUILER)
//#pragma	option	-a4
#endif

#if defined(NEWDIC2)

// JLinkオブジェクトデータベース //
// 辞書ID(?) + オブジェクトID で識別、検索するとJLinkポインタが得られる

class Pdic;
class JLink;
struct TNFONT;

class JLinkArray : public FlexObjectArray<JLink> {
	public:
		JLinkArray();
		~JLinkArray();
		int Search( int start, Pdic *dic, t_id id );
};

#ifdef GUI
void DrawSelection( HDC hdc, RECT &rc, COLORREF color );	// 選択矩形を描く
#endif

#if 1
#include "MemObj.h"
class JLinkObject : TMemoryObject {
typedef TMemoryObject super;
public:
	JLinkObject( const uint8_t *_data, int _len );
	JLinkObject( const uint8_t *_data, int _len, bool reference );	// 参照型の場合reference=true
	bool _Validate( );
	inline void Release( )
		{ deref(); }
	inline JLinkObject *Clone( )
		{ ref(); return this; }
	inline int GetLength() const
		{ return super::size(); }
	bool Set( const uint8_t *data, int len );
	bool Get( uint8_t *data );
	uint8_t *GetData( )
		{ return (uint8_t*)data; }
};
#else	// old
class JLinkObject {
private:
	uint8_t *data;
	DWORD len;
	int ref;		// 参照回数
protected:
	BOOL fActive;
public:
	JLinkObject( const uint8_t *_data, DWORD _len );
	JLinkObject( const uint8_t *_data, DWORD _len, bool reference );	// 参照型の場合reference=true
	virtual ~JLinkObject( );
	bool Validate( );
	bool IsDataValid() const
		{ return data ? true : false; }
	void Release( );
	JLinkObject *Clone( );
	BOOL Activate( );
	virtual DWORD GetLength()
		{ return len; }
	BOOL IsActive( )
		{ return fActive; }
	virtual BOOL Set( const uint8_t *data, DWORD len );
	virtual BOOL Get( uint8_t *data );
	virtual uint8_t *GetData( ) const
		{ return data; }
	virtual BOOL CopyToClipboard( )
		{ return FALSE; }
protected:
	virtual void DeleteObject( );
};
#endif

class JLink {
protected:
	t_jlink type;	// リンクの種類
	_jMixChar title;
	_jMixChar word;	// nMag,nAspect更新用英単語
private:
	Pdic *dic;
	t_id id;	// ID番号
protected:
	int error;
	SIZE orgsize;		// オリジナルのオブジェクトの大きさ
	SIZE objsize;		// 現在のオブジェクトの大きさ(MM_TEXT)(タイトルを含まない),
	int nMag;			// X倍率
	int nAspect;		// アスペクト比(Xに対するY倍率(x100))
	int orgX, orgY;		// 表示開始左上座標

protected:
	static JLinkArray jlarray;
	JLink *Search( Pdic *_dic, t_id _id );
	uint8_t *GetTitle( uint8_t *buf );
#ifdef USE_BOCU1
	void SetTitle( const uint8_t *buf );
#elif defined(_UNICODE)
	void SetTitle( const uint8_t *buf )
		{ SetTitle( (const tchar*)buf ); }
#endif
	int GetTitleLen()
#ifdef USE_BOCU1
		;
#else
		{ return _tcsbyte1( title ); }
#endif

public:
	JLink( int _type, Pdic *_dic, t_id _id );
	virtual ~JLink( );
	t_jlink GetType( ) const
		{ return type; }
	int GetCFType();
	int GetErrorCode( ) const
		{ return error; }
	void SetErrorCode(int err)
		{ error = err; }
	void ClearError( )
		{ error = 0; }
	Pdic *GetDic( ) const
		{ return dic; }
	const tchar *GetTitle( )
		{ return title; }
	void SetTitle( const tchar *_title )
		{ title.set( _title ); }
	t_id GetID( ) const
		{ return id; }
#ifdef _Windows
	void GetOrgPoint(POINT &pt)
		{ pt.x = orgX; pt.y = orgY; }
#endif
	int GetMag() const
		{ return nMag; }
	int GetAspect() const
		{ return nAspect; }
#ifdef DISPDICNAME
	static bool DispDicName;
#endif
protected:
	void _SetID( t_id _id )	{ id = _id; }	// 必ずダイレクトに設定したい場合のみ
	virtual int GetCommonHeaderOffset()
		{ return sizeof(short); }	// nMag,nAspectの先頭オフセット
public:
	virtual void SetID( t_id _id ) { id = _id; }
	void SetDic( Pdic *_dic ) { dic = _dic; }
	virtual int GetLength( ) = 0;
	virtual int GetHeaderLength( )			// 非圧縮長(タイトルの次から)
		{ return 0; }
	virtual BOOL Get( uint8_t *buf ) = 0;
	virtual BOOL Set( const uint8_t *buf, int len ) = 0;
	virtual BOOL PreSet( const uint8_t * )
		{ return FALSE; }
	virtual JLink *Clone( Pdic * ) = 0;
	void CopyMapMode( JLink *o );
	void Release( );
	virtual const tchar *GetClassName( tnstr &str ) = 0;
#ifdef GUI
	void SetMapMode( HDC hdc, int orgx, int orgy );
	void ResetMapMode( HDC hdc );
	virtual void SetParent(TWinControl *parent){}
	virtual int Draw( struct TNFONT &tnfont, RECT &rc, BOOL dispf, EnphTextVec *enph =NULL )
		{ tnstr str; return ErrorDraw( tnfont, rc, dispf, GetClassName( str ) ); }
	virtual bool Edit( TWinControl *, int=0 ){ return false; }
	virtual BOOL CanEdit(){ return false; }
	virtual BOOL Deactivate( ){ return FALSE; }
	virtual bool IsActivating( ){ return false; }
	virtual BOOL AddVerbMenu( HMENU hMenu, UINT uPos, UINT idFirst, UINT idConvert ) { return FALSE; }
	virtual BOOL CanClose( )
		{ return TRUE; }
	virtual BOOL CopyToClipboard( HWND ){ return FALSE; }
	virtual BOOL CanCopy( ) { return FALSE; }	// can copy to clipboard
#ifdef OLE2
	virtual bool DoDragDrop( bool & /* fCanMove */ ) { return false; }
	virtual BOOL Convert( HWND ) { return FALSE; }
#endif
	void DrawSelection( HDC hdc, RECT &rc );
	int HitTest( POINT &org, POINT &pt, UINT key, BOOL fRightButton );
protected:
	RECT rcCapture;		// キャプチャー矩形
	int capmode;			// キャプチャーモード
public:
	virtual int ButtonDown( HDC hdc, POINT org, POINT &pt, UINT key, BOOL fRightButton );
	virtual int ButtonUp( HDC hdc, POINT &pt, const tchar *word );
	virtual bool MouseMove( HDC hdc, POINT &pt, bool hover, class TMouseCaptureBase *cap );
	void SetSizeFromCapture( );
	SIZE &GetSize( ) { return objsize; }
	int ErrorDraw( TNFONT &tnfont, RECT &rc, BOOL dispf, const tchar *msg=NULL, bool fTitle=true );
	int DrawTitle( TNFONT &tnfont, int left, int top, int right, int dispf )
	// 戻り値 : 縦ドット数
	{
		RECT rc;
		rc.left = left;
		rc.top = top;
		rc.right = right;
		return DrawText2( tnfont, title, &rc, right, dispf );
	}
	int DrawTitle( TNFONT &tnfont, RECT &rc, int dispf )
		{ return DrawText2( tnfont, title, &rc, rc.right, dispf ); }
	virtual void Invalidate(){}	// 表示無効化
#endif	// GUI
	virtual BOOL IsLinked( ) = 0;
	virtual bool IsAudio() const { return false; }
};

// 未確認オブジェクト Undefined Format Object!
typedef unsigned int t_extra;
class JLUFO : public JLink {
protected:
	JLinkObject *jlobj;	// データバッファ
//protected:
	t_extra ExtraData;
public:
	JLUFO( Pdic *dic, t_id _id );
private:
	~JLUFO( );
public:
	virtual int GetLength( );
	virtual JLink *Clone( Pdic * );
	virtual BOOL Get( uint8_t *buf );
	uint8_t *GetData()
		{ return jlobj ? (uint8_t*)jlobj->GetData() : NULL; }
	virtual BOOL Set( const uint8_t *buf, int len );
	void Set(JLinkObject *jlobj);
	bool SetRef( const uint8_t *buf, int len );
	virtual const tchar *GetClassName( tnstr &str );
	virtual BOOL IsLinked( )
		{ return FALSE; }
	virtual void SetID( t_id _id )
		{ _SetID( _id ); }
	t_extra GetExtraData()
		{ return ExtraData; }
	void SetExtraData( t_extra data )
		{ ExtraData = data; }
};

#if defined(USE_FILELINK)
#include "JLFile.h"
#endif	// USE_FILELINK

#ifdef USE_OLE
#include "jlole.h"
#endif	// USE_OLE

#ifdef JL_VOICE
#ifndef __MMSYSTEM_H
#include <mmsystem.h>
#endif

// PCMLINKは１つの辞書につき、１つの音声ファイルという制限付き
// PCMLINKはあくまでも一時的な利用にとどめるべき？
struct PCMLINK {
	WORD reserved1;	// 必ず 0x0000(pcmと区別するため)
	WORD reserved2;	// 必ず 0x0000
	DWORD fileid;	// FILE ID(現在のところ0x00000000)にする
	DWORD playloc;	// 再生開始位置
	DWORD playlen;	// 再生の長さ
};

class JLVoice : public JLink {
public:
	PCMWAVEFORMAT pcm;		// PCMWAVEとPCMLINK兼用
	JLinkObject *wavedata;	// PCMWAVE 専用
	tnstr filename;		// PCMLINK専用
public:
	JLVoice( Pdic *dic, t_id _id, PCMWAVEFORMAT *_pcm, uint8_t *_wavedata, DWORD _length );	// メモリから
	JLVoice( Pdic *dic, t_id _id, PCMLINK *_link, tchar *filename );

	JLVoice( Pdic *dic, t_id _id, class WaveIO *wio, int length = -1 );	// WAVﾌｧｲﾙから(wioはオープン済みであること)
		// このcontructorはPCMWAVEのみ

	~JLVoice( );
	virtual int GetLength( );
#ifdef GUI
	virtual int Draw( TNFONT &tnfont, RECT &rc, BOOL dispf, EnphTextVec * =NULL );
	virtual bool Edit( TWinControl *, int=0 );
	virtual BOOL CanEdit(){ return true; }
	virtual BOOL AddVerbMenu( HMENU hMenu, UINT uPos, UINT idFirst, UINT idConvert);
	static BOOL GetClipboardState( int cfsflag );
	virtual BOOL CopyToClipboard( HWND );
	virtual BOOL CanCopy( ) { return wavedata ? TRUE : FALSE; }
#endif
	virtual JLink *Clone( Pdic * );
	virtual BOOL Get( uint8_t *buf );
	BOOL PreSet( const uint8_t *buf );
	virtual BOOL Set( const uint8_t *buf, int len );
	virtual const tchar *GetClassName( tnstr &str );
	virtual BOOL IsLinked( );

	PCMLINK &GetPcmLink( )
		{ return *(PCMLINK*)&pcm; }
	__override bool IsAudio() const { return true; }
};
#endif	// JL_VOICE

#ifdef USE_JBMP
#include "JLImage.h"
#endif	// USE_JBMP

#ifdef USE_RTF

class JLRTF : public JLink {
protected:
	class JLRTFCont *rtf;		// RTFコントロール
private:
	JLRTF( JLRTF & )
		:JLink( JL_RTF, NULL, 0 ){}	// Do not use copy constructor
									// Generate an object by using Clone() function
public:
	JLRTF( Pdic *dic, JLRTFCont *rtf );
	~JLRTF( );
	virtual void SetID( t_id _id );
	virtual int GetLength( );
	virtual BOOL Get( uint8_t *buf );
	virtual BOOL Set( const uint8_t *buf, int len );
	virtual BOOL PreSet( const uint8_t *buf );
	virtual int Draw( TNFONT &tnfont, RECT &rc, BOOL dispf, EnphTextVec *=NULL );
	virtual JLink *Clone( Pdic * );
	virtual const tchar *GetClassName( tnstr &str );

	void SetItem( class OleItem * );
#if defined(_Windows)
	virtual bool Edit( TWinControl *, int=0 );
	virtual BOOL CanEdit(){ return true; }
	virtual BOOL AddVerbMenu( HMENU hMenu, UINT uPos, UINT idFirst, UINT idConvert);
	static BOOL GetClipboardState( int cfsflag );
	virtual BOOL CopyToClipboard( HWND );
	virtual BOOL CanCopy( ) { return obj ? TRUE : FALSE; }	// can copy to clipboard
#ifdef OLE2
	virtual bool DoDragDrop( bool &fMove );
	virtual BOOL Convert( HWND );
#endif
#endif

	virtual BOOL CanClose( );

	virtual BOOL IsLinked( )
		{ return rtf ? TRUE : FALSE; }

	// リンクをしないときのデータバッファ処理 /////////////////////
public:
	JLinkObject *jlobj;	// データバッファ
};

#endif	// USE_RTF

#endif	// NEWDIC2
#endif	// USE_JLINK

#pragma	pack(push,1)
// OLEオブジェクトの辞書上のイメージ
struct EPWingField {
//	short	size;		// EPWingFieldのサイズ
	short	bookno;
	int pos;
//	tchar data[ 1 ];	// データ(可変長) データ部の位置は
					// EPWingField of;
					// tchar *data = of.GetDataPtr( ); で求めること！
//	tchar *GetDataPtr( )
//		{ return (tchar*)(((tchar*)this) + size); }
		// この構造体の先頭を指しているポインタ(top)からdataの先頭アドレスを求める
};
#pragma pack(pop)
#ifdef EPWING
class JLEPWing : public JLink {
protected:
	short bookno;
	int pos;
public:
	JLEPWing( Pdic *dic, t_id _id, short bookno, int pos );
	~JLEPWing( );
	virtual int GetLength( );
	virtual int Draw( TNFONT &tnfont, RECT &rc, BOOL, EnphTextVec * =NULL );
#ifdef GUI
	virtual bool Edit( TWinControl *, int=0 ){return false;}
	virtual BOOL AddVerbMenu( HMENU hMenu, UINT uPos, UINT idFirst, UINT idConvert);
	static BOOL GetClipboardState( int cfsflag );
	virtual BOOL CopyToClipboard( HWND );
	virtual BOOL CanCopy( );
#endif
	virtual JLink *Clone( Pdic * );
	virtual BOOL Get( uint8_t *buf );
	BOOL PreSet( const uint8_t *buf );
	virtual BOOL Set( const uint8_t *buf, int len );
	virtual const tchar *GetClassName( tnstr &str );
	virtual BOOL IsLinked( ){return true;}
	int GetPos()
		{ return pos; }
	short GetBookNo()
		{ return bookno; }
	bool CopyToClipboardAppend( class GlobalString *str );
};
#endif

#ifdef USE_JLINK
#if 0
class JLUser : public JLink {
public:
	JLUser( Pdic *dic, t_id _id, int data );
	~JLUser( );
	virtual int GetLength( );
	virtual int Draw( TNFONT &tnfont, RECT &rc, BOOL, EnphTextVec *=NULL );
	virtual bool Edit( TWinControl *, int=0 );
	virtual BOOL AddVerbMenu( HMENU hMenu, UINT uPos, UINT idFirst, UINT idConvert );
	static BOOL GetClipboardState( int cfsflag );
	virtual BOOL CopyToClipboard( HWND );
	virtual BOOL CanCopy( );
	virtual JLink *Clone( Pdic * );
	virtual BOOL Get( uint8_t *buf );
	virtual BOOL Set( const uint8_t *buf, int len );
	virtual const tchar *GetClassName( tnstr &str );
	virtual BOOL IsLinked( );
};
#endif
#endif	// USE_JLINK

#ifdef USE_JLINK
class JLinks : public FlexObjectArray<JLink> {
public:
	JLinks( )
		:FlexObjectArray<JLink>( 1 )
	{}
	JLinks( JLinks &jls, int slot_num=10, int destruct=1 )
		:FlexObjectArray<JLink>( jls, slot_num, destruct )
	{}
	BOOL CanClose( );	//
	int SearchID( t_id id );
	int SearchObj( Pdic *dic, t_id id );	// マルチ辞書でも同一オブジェクトを探せる
	int SearchType( t_jlink type, int first = 0 );
	void ChangeDic( Pdic &dic );
	int SearchHyperLink( class THyperLinks &hls ) const;
	void Clone( JLinks &jlinks );
	int GetAudioIndex(int firstIndex=0);	// 音声再生用のobjectはあるか？
	JLink *FindPlayObj(bool ole=false);
};

// Helpers //

#endif	// USE_JLINK

#endif	// __JLINK_H

