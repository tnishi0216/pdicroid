#include "tnlib.h"
#pragma	hdrstop
#include	"dic.h"
#include "jlink.h"
#include "pdstrlib.h"
#include "cftype.h"		// for JLink class
#ifdef EPWING
#include "pdepwing.h"
#endif

#if defined(GUI)
#include "hyplink.h"
#endif

//#include "arif.h"

// オブジェクトサイズ変更
#define	USE_EXT	0
#ifdef USE_JBMP
#undef USE_EXT
#define	USE_EXT	1
#endif
// オブジェクトサイズ変更の残す課題
// ・サイズ変更後の保存
// ・ファイルリンクオブジェクトの表示処理
// ・左上ハンドルによる、拡大縮小

const tchar *StrLinkError = _T("<<Link failure>>");

// ヒットテストコード ////////////////////////////////////////////////////////
#define	JLHIT_SIZE		1	// サイズ変更
#define	JLHIT_RIGHT		2	// 左右
#define	JLHIT_BOTTOM	3	// 上下
#define	JLHIT_MAGNIFY1	4	// 拡大
#define	JLHIT_MAGNIFY2	5	// 縮小
#define	JLHIT_MOVE		6	// 移動

// JLinkArray //
JLinkArray::JLinkArray()
	:FlexObjectArray<JLink>( 10, 0 )
{
}

JLinkArray::~JLinkArray()
{
}

int JLinkArray::Search( int i, Pdic *dic, t_id id )
{
	for ( ;i<get_num();i++ ){
		if ( (*this)[i].GetID() == id && (*this)[i].GetDic() == dic ){
			return i;
		}
	}
	return -1;
#if 0
	JLink **a = (JLink**)array;
	int n = get_num();
	while ( n ){
		if ( (*a)->GetID() == id && (*a)->GetDic() == dic ){
			return *a;
		}
		n--;
		a++;
	}
	return NULL;
#endif
}

///////////////////// JLinkObject /////////////////////////////////////////////
#if 1
//Note:
// super::datarefは使用しない
//
// _dataはNULLでも可能:その場合はﾊﾞｯﾌｧを新規確保するだけ
JLinkObject::JLinkObject( const uint8_t *_data, int _len )
{
	Set( _data, _len );
}
// reference = true:
// _dataは、JLinkObjectの参照となる
// したがって、JLinkObject objectがdeleteされるまでは有効でなければならない
// ただし、１回以上誤って余計にやると data は削除されるので要注意！！
JLinkObject::JLinkObject( const uint8_t *_data, int _len, bool /* reference */ )
	:super((uint8_t*)_data, _len, true)
{
	//TODO: refcntは1で大丈夫？(originalは2だった)
}

// constructorのSet()が成功したかどうかのvalidation以外はなるべく使用しないこと！
// なぜなら、何に対するvalidationなのか、classの中身を知らないと理解できないため
bool JLinkObject::_Validate( )
{
	return data ? true : false;
}

bool JLinkObject::Set( const uint8_t *_data, int _len )
{
	if ( super::data && (uint8_t*)super::data == _data ) return true;
	uint8_t *p = new uint8_t[ _len ];
	if ( !p )
		return false;
	if (_data)
		memcpy(p, _data, _len);
	super::set(p, _len);
	return true;
}

bool JLinkObject::Get( uint8_t *buf )
{
	if ( (uint8_t*)super::data == buf ) return true;
	if ( super::data ){
		memcpy( buf, super::data, datalen );
		return true;
	}
	return false;
}

#else	// old : to be deleted.
// _dataはNULLでも可能:その場合はﾊﾞｯﾌｧを新規確保するだけ
JLinkObject::JLinkObject( const uint8_t *_data, DWORD _len )
{
	data = NULL;
	if ( Set( _data, _len ) ){
		ref = 1;
	} else {
		ref = 0;
		len = 0;
	}
}
// reference = true:
// _dataは、JLinkObjectの参照となる
// したがって、JLinkObject objectがdeleteされるまでは有効でなければならない
// ただし、１回以上誤って余計にやると data は削除されるので要注意！！
JLinkObject::JLinkObject( const uint8_t *_data, DWORD _len, bool /* reference */ )
{
	data = (uint8_t*)_data;
	len = _len;
	ref = 2;	// １つ余計に
}

JLinkObject::~JLinkObject( )
{
	if ( --ref <= 0 )		// 条件追加 1999.3.5
	{
		DeleteObject( );
	}
}

bool JLinkObject::Validate( )
{
	if ( ref <= 0 ){
		DeleteObject( );
		delete this;
		return false;
	}
	return true;
}

void JLinkObject::Release( )
{
	ref--;
	Validate( );
}

// dataは有効であると仮定している
JLinkObject *JLinkObject::Clone( )
{
	ref++;
	return this;
}

BOOL JLinkObject::Activate( )
{
	if ( fActive ) return TRUE;
	if ( Set( data, len ) )
		fActive = TRUE;
	return fActive;
}

BOOL JLinkObject::Set( const uint8_t *_data, DWORD _len )
{
	if ( data && data == _data ) return TRUE;
	uint8_t *p = new uint8_t[ _len ];
	if ( !p )
		return FALSE;
	DeleteObject();
	data = p;
	len = _len;
	if ( _data )
		memcpy( data, _data, _len );
	return TRUE;
}

BOOL JLinkObject::Get( uint8_t *buf )
{
	if ( data == buf ) return TRUE;
	if ( data ){
		memcpy( buf, data, len );
		return TRUE;
	}
	return FALSE;
}

void JLinkObject::DeleteObject( )
{
	if ( data ){
		delete[] data;
		data = NULL;
		len = 0;
		ref = 0;
	}
}
#endif	// 0

#define	NMAG_HANDLE	5		// ハンドルの大きさ

///////////////////// JLink /////////////////////////////////////////////

#ifdef GUI
#ifdef DISPDICNAME
bool JLink::DispDicName = DDN_DEFAULT;
#endif

void JLink::DrawSelection( HDC hdc, RECT &rc )
{
	int line = GetSystemMetrics( SM_CXBORDER );
	RECT r;

	// 枠の描画
	r = rc;
	// 上
	r.bottom = r.top + line;
	InvertRect( hdc, &r );
	r.bottom = rc.bottom;
	// 左
	r.right = r.left + line;
	InvertRect( hdc, &r );
	r.right = rc.right;
	// 右
	r.left = r.right - line;
	InvertRect( hdc, &r );
	r.left = rc.left;
	// 下
	r.top = r.bottom - line;
	InvertRect( hdc, &r );

	objsize.cx = rc.right - rc.left;	// ここでオブジェクトのサイズをセット
	objsize.cy = rc.bottom - rc.top;

	// ハンドルの描画
	int box = line * NMAG_HANDLE;
	r = rc;
	// 左上
	r.left++;
	r.top++;
	r.right = r.left + box;
	r.bottom = r.top + box;
	InvertRect( hdc, &r );
	// 右上
	r.left = rc.right - box;
	r.right = rc.right - 1;
	InvertRect( hdc, &r );

	// 右下
	r.top = rc.bottom - box;
	r.bottom = rc.bottom - 1;
	InvertRect( hdc, &r );

	// 左下
	r.left = rc.left + 1;
	r.right = r.left + box;
	InvertRect( hdc, &r );
}
int JLink::HitTest( POINT &org, POINT &pt, UINT key, BOOL fRightButton )
{
	if ( (key & (MK_CONTROL|MK_SHIFT)) == (MK_CONTROL|MK_SHIFT) ){
		// 拡大 or 縮小
		return fRightButton ? JLHIT_MAGNIFY2 : JLHIT_MAGNIFY1;
	}
	RECT objrect;
	SetRect( &objrect, org.x, org.y, org.x+objsize.cx*nMag/100, org.y+objsize.cy*nMag/100*nAspect/100 );
	int line = GetSystemMetrics( SM_CXBORDER ) * NMAG_HANDLE;
	RECT rc;
	// サイズ判定
	rc = objrect;
	rc.left = rc.right - line;
	rc.top = rc.bottom - line;
	if ( PtInRect( &rc, pt ) )
		return JLHIT_SIZE;
	// 右側判定
	rc = objrect;
	rc.left = rc.right - line;
	if ( PtInRect( &rc, pt ) )
		return JLHIT_RIGHT;
	// 下側判定
	rc = objrect;
	rc.top = rc.bottom - line;
	if ( PtInRect( &rc, pt ) )
		return JLHIT_BOTTOM;
	return 0;
}
#endif	// GUI

///////////////// JLink /////////////////////////////////
JLinkArray JLink::jlarray;

JLink::JLink( int _type, Pdic *_dic, t_id _id )
{
	type = _type;
	dic = _dic;
	id = _id;

#ifdef GUI
	capmode = 0;
#endif
//	mapmode = MM_TEXT;
	nMag = 100;	// %
	nAspect = 100;	// %
	orgsize.cx = 0;
	orgsize.cy = 0;
	orgX = orgY = 0;

	jlarray.add( this );
}

JLink::~JLink( )
{
	for ( int i=0;i<jlarray.get_num();i++ ){
		if ( &jlarray[i] == this ){
			jlarray.del( i );
			return;
		}
	}
}

JLink *JLink::Search( Pdic *_dic, t_id _id )
{
	int i = 0;
	while ( 1 ){
		i = jlarray.Search( i, _dic, _id );
		if ( i == -1 )
			return NULL;
		if ( &jlarray[i] != this )
			return &jlarray[i];
		i++;
	}
}

int JLink::GetCFType( )
{
	switch ( type )
	{
		case JL_UFO:	return 0;
#ifdef USE_OLE
		case JL_OLE:	return CFT_OLEOBJ;
#endif
#ifdef USE_ICONFILE
		case JL_ICONFILE:
#endif
		case JL_FILE:	return CFT_FILELINKOBJ;
#ifdef JL_VOICE
		case JL_VOICE:	return CFT_VOICE;
#endif
#ifdef USE_JBMP
		case JL_IMAGE:	return CFT_IMAGE;
#endif
		default:
			return 0;
	}
}
// 辞書のdata bufferへtitleをcopyする
uint8_t *JLink::GetTitle( uint8_t *buf )
{
#ifdef USE_BOCU1
	buf = bocu1EncodeT( title, (tchar*)-1, buf );
	*buf = '\0';
	return buf+1;
#else
#if defined(DIC_UTF8)
#error	not yet supported
#endif
	return (uint8_t*)nstrcpy( (tchar*)buf, title );
#endif
}
#ifdef USE_BOCU1
void JLink::SetTitle( const uint8_t *buf )
{
	tchar *s = bocu1DecodeStr( buf );
	title.set( s );
	delete[] s;
}
#endif
#ifdef USE_BOCU1
int JLink::GetTitleLen()
{
	return _tcslen( title ) * 2+1;	//*+++ 暫定
}
#endif

#if 0
void JLink::SetID( t_id _id )
{
	Search( dic, id )->id = _id;
}

void JLink::SetDic( Pdic *_dic )
{
	Search( dic, id )->dic = _dic;
}
#endif

#ifdef GUI
void JLink::SetMapMode( HDC hdc, int orgx, int orgy )
{
	if ( nMag == 100 && nAspect == 100 ) return;
	if ( nAspect != 100 ){
		::SetMapMode( hdc, MM_ANISOTROPIC );
		SetWindowOrgEx( hdc, orgx, orgy, NULL );
		SetWindowExtEx( hdc, 100, 100, NULL );
		SetViewportExtEx( hdc, nMag, nMag*nAspect/100, NULL );
		SetViewportOrgEx( hdc, orgx, orgy, NULL );
	} else {
		::SetMapMode( hdc, MM_ISOTROPIC );
		SetWindowOrgEx( hdc, orgx, orgy, NULL );
		SetWindowExtEx( hdc, 100, 100, NULL );
		SetViewportExtEx( hdc, nMag, nMag, NULL );
		SetViewportOrgEx( hdc, orgx, orgy, NULL );
	}
}

void JLink::ResetMapMode( HDC hdc )
{
	if ( nMag != 100 || nAspect != 100 ){
		::SetMapMode( hdc, MM_TEXT );
		SetWindowExtEx( hdc, 10000, 10000, NULL );
		SetViewportExtEx( hdc, 10000, -10000, NULL );
	}
}

int JLink::ErrorDraw( TNFONT &tnfont, RECT &rc, BOOL dispf, const tchar *msg, bool fTitle )
{
	ResetMapMode( tnfont );
	const tchar *txt = msg ? msg : StrLinkError;
	int cy;
	int cytitle = 0;
	RECT _rc = rc;
	if ( dispf & DF_DISP && !(dispf & DF_UNREDRAW) ){
		cy = DrawText2( tnfont, txt, &rc, rc.right, DF_DISP );
		if ( fTitle && title[0] ){
			_rc.top += cy;
			cytitle = DrawTitle( tnfont, _rc, DF_DISP );
			rc.right = max( rc.right, _rc.right );
		}
	} else {
		cy = DrawText2( tnfont, txt, &rc, rc.right, DF_CALCRECT );
		if ( fTitle && title[0] ){
			_rc.top += cy;
			cytitle = DrawTitle( tnfont, _rc, DF_CALCRECT );
			rc.right = max( rc.right, _rc.right );
		}
	}
	rc.bottom = rc.top + cy + cytitle;
	if ( dispf & DF_DISP && dispf & (DF_SELECTED|DF_UNSELECTED)
		&& fTitle){
			// title無し=すべて自前で表示する
			// selectionは別の場所で描画するため
		_rc = rc;
		_rc.bottom -= cytitle;
		DrawSelection( tnfont, _rc );
	}
	return cy + cytitle;
}

#pragma	warn -param
// return: (see wsobj.cpp for details)
//	0 : no status change.
//	1 : D&D
//	2 : Normal capturing
//	3 : normal capturing and need to redraw.
// -1 : need to redraw
int JLink::ButtonDown( HDC hdc, POINT org, POINT &pt, UINT key, BOOL fRightButton )
{
#if USE_EXT
	if ( !dic || dic->IsReadOnly() )
		return FALSE;
	switch ( GetType() ){
		case JL_OLE:
		case JL_IMAGE:
		case JL_FILE:
#ifdef USE_ICONFILE
		case JL_ICONFILE:
#endif
			break;
		default:
			return FALSE;
	}
	if ( ( capmode = HitTest( org, pt, key, fRightButton ) ) == 0 ){
		return FALSE;
	}
	if ( !objsize.cx || !objsize.cy ){
		capmode = 0;
		return FALSE;
	}
//	SetRect( &rcCapture, org.x, org.y, org.x+objsize.cx*nMag/100, org.y+objsize.cy*nMag/100*nAspect/100 );
	SetRect( &rcCapture, pt.x, pt.y, pt.x+objsize.cx*nMag/100, pt.y+objsize.cy*nMag/100*nAspect/100 );
	switch ( capmode ){
		case JLHIT_MAGNIFY1:
		case JLHIT_MAGNIFY2:
			return 2;
	}
//	DrawFocusRect( hdc, &rcCapture );
	return TRUE;
#else
	return 1;
#endif
}

// hovering:
//	true - mouse moving in the object area.
// capturing:
//	true - mouse moving while mouse dragging.
// return:
//	true : Need to redraw. if hit and change the status.
bool JLink::MouseMove( HDC hdc, POINT &pt, bool hovering, class TMouseCaptureBase *cap )
{
#if USE_EXT
	if ( !capmode )
		return false;
	int minsize = GetSystemMetrics( SM_CXBORDER ) * NMAG_HANDLE;
//	DrawFocusRect( hdc, &rcCapture );
	switch ( capmode ){
		case JLHIT_SIZE:
			{
				double kx = (double)( pt.x - rcCapture.left ) / objsize.cx;
				double ky = (double)( pt.y - rcCapture.top ) / objsize.cy;
				if ( kx < 0 )
					kx = 0;
				if ( ky < 0 )
					ky = 0;
				if ( kx < ky )
					kx = ky;
				rcCapture.right = rcCapture.left + kx * objsize.cx;
				rcCapture.bottom = rcCapture.top + kx * objsize.cy;
			}
			break;
		case JLHIT_BOTTOM:
			rcCapture.bottom = pt.y;
			break;
		case JLHIT_RIGHT:
			rcCapture.right = pt.x;
			break;
		case JLHIT_MAGNIFY1:	// left button
			capmode = 0;
			return false;
		case JLHIT_MAGNIFY2:	// right button
		case JLHIT_MOVE:
			capmode = JLHIT_MOVE;
			orgX += rcCapture.left - pt.x;
			if ( orgX < 0 ) orgX = 0;
//			else if ( orgX >= orgsize.cx - 32 ) orgX = orgsize.cx - 32;
			orgY += rcCapture.top - pt.y;
			if ( orgY < 0 ) orgY = 0;
//			else if ( orgY >= orgsize.cy - 32 ) orgY = orgsize.cy - 32;
			return false;
	}
	if ( rcCapture.right - rcCapture.left - minsize < 0 )
		rcCapture.right = rcCapture.left + minsize;
	if ( rcCapture.bottom - rcCapture.top - minsize < 0 )
		rcCapture.bottom = rcCapture.top + minsize;
	DrawFocusRect( hdc, &rcCapture );
#endif
	return false;
}

// 
// 1 : Need to redraw.
int JLink::ButtonUp( HDC hdc, POINT &/* pt */, const tchar *_word )
{
#if USE_EXT
	if ( !capmode )
		return FALSE;
	switch ( capmode ){
		case JLHIT_MAGNIFY1:
			if ( nMag <= 25 ){
				nMag = (nMag+3) * 4 / 3;
			} else {
				nMag += 25;
			}
			if ( nMag > 25 ){
				nMag = (nMag+24) / 25 * 25;
			}
			if ( dic && _word ){
				word.set( _word );
			}
			if ( GetDic() && word[0] ){
				short t = (short)nMag;
				GetDic()->UpdateObjectHeader( _kstr(word, _wSingleByte), GetID(), GetCommonHeaderOffset(), sizeof(short), (char*)&t );
				word.clear();
			}
			capmode = 0;
			return -1;
		case JLHIT_MAGNIFY2:
			if ( nMag <= 25 ){
				nMag = nMag * 3 / 4;
				if ( nMag == 0 ) nMag = 1;
			} else {
				nMag -= 25;
			}
			if ( dic && _word ){
				word.set( _word );
			}
			if ( GetDic() && word[0] ){
				short t = (short)nMag;
				GetDic()->UpdateObjectHeader( _kstr(word, _wSingleByte), GetID(), GetCommonHeaderOffset(), sizeof(short), (char*)&t );
				word.clear();
			}
			capmode = 0;
			return -1;
	}
	capmode = 0;
//	DrawFocusRect( hdc, &rcCapture );
	if ( rcCapture.top >= rcCapture.bottom || rcCapture.left >= rcCapture.right )	// 不正な大きさはFALSE
		return FALSE;
	if ( rcCapture.right - rcCapture.left == objsize.cx		// 同じ大きさはFALSE
		&& rcCapture.bottom - rcCapture.top == objsize.cy )
		return FALSE;
	return TRUE;
#else
	return FALSE;
#endif
}

void JLink::SetSizeFromCapture( )
{
	if ( !orgsize.cx || !orgsize.cy )
		return;
	nMag = ( rcCapture.right - rcCapture.left ) * 100 / orgsize.cx;
	if ( nMag <= 0 ) nMag = 1;
	nAspect = ( rcCapture.bottom - rcCapture.top ) * 100 / orgsize.cy / nMag;
}
#pragma	warn .param
#endif	// GUI

void JLink::CopyMapMode( JLink *o )
{
//	o->mapmode = mapmode;
	o->nMag = nMag;
	o->nAspect = nAspect;
}

#if 0
BOOL JLink::UpdatePalette( HDC hdc )
{
	if ( !hPalette )
		return FALSE;
	SelectPalette( hdc, hPalette, FALSE );
	if ( RealizePalette( hdc ) > 0 ){
		if ( alwaysRepaint )
			Invalidate( );
		else
			UpdateColors( hdc );
	}
	return TRUE;
}
#endif

int JLinks::SearchHyperLink( class THyperLinks &hls ) const
{
#ifdef EPWING	// 現在のところEPWINGのみ
	int n = 0;
	for ( int i=0;i<get_num();i++ ){
		if ( (*this)[i].GetType() != JL_EPWING ) continue;
		// EPWING
		PDEPWing *dic = (PDEPWing*)(*this)[i].GetDic();
		if ( dic->GetDicType() == 1 )
		{
			HypLinkCurrentDic = dic;
			HypLinkCurrentItem = HLI_EPWING + i;
			n += dic->SearchAutoLink( ((JLEPWing&)(*this)[i]).GetBookNo(), ((JLEPWing&)(*this)[i]).GetPos(), &hls );
		}
	}
	return n;
#else
	return 0;
#endif
}

///////////////////// JL-UFO //////////////////////////////////////////////////
JLUFO::JLUFO( Pdic *_dic, t_id _id )
	:JLink( JL_UFO, _dic, _id )
{
	jlobj = NULL;
	ExtraData = 0;
}

JLUFO::~JLUFO( )
{
	if ( jlobj )
		jlobj->Release( );
}

const tchar *JLUFO::GetClassName( tnstr &str )
{
	str.set( _T("<<Unknown Object>>") );
	return str;
}

JLink *JLUFO::Clone( Pdic *_dic )
{
	JLUFO *o;
	if ( GetDic() == _dic || !_dic ){
		// 同じ辞書へのコピー
		o = new JLUFO( GetDic(), GetID() );
	} else {
		// 異なる辞書へのコピー
		o = new JLUFO( _dic, _dic->GetObjectNumber() );
	}
	if ( jlobj ){
		o->jlobj = jlobj->Clone( );
	}
	o->SetID( GetID() );
	o->SetExtraData( ExtraData );
	o->title = title;
	CopyMapMode( o );
	return o;
}

int JLUFO::GetLength( )
{
	if ( jlobj ){
		return sizeof( t_id ) + sizeof(t_extra) + jlobj->GetLength( );
	}
	return 0;
}

BOOL JLUFO::Get( uint8_t *buf )
{
	*(t_id*)buf = GetID();
	*(t_extra *)(buf+sizeof(t_id)) = ExtraData;
	if ( jlobj ){
		jlobj->Get( buf+sizeof(t_id)+sizeof(t_extra) );
	}
	return TRUE;
}

BOOL JLUFO::Set( const uint8_t *buf, int len )
{
	if ( jlobj ){
		jlobj->Release( );
		jlobj = NULL;
	}
	_SetID( *(t_id*)buf );
	ExtraData = *(t_extra*)(buf+sizeof(t_id));
	len -= sizeof(t_id)+sizeof(t_extra);
	if ( len > 0 ){
		jlobj = new JLinkObject( buf+sizeof(t_id)+sizeof(t_extra), len );
		if ( !jlobj->_Validate( ) ){
			jlobj = NULL;
			error = JLE_NOTENOUGH;
			return FALSE;
		}
	}
	return TRUE;
}
void JLUFO::Set(JLinkObject *_jlobj)
{
	if (jlobj==_jlobj)
		return;
	if (jlobj){
		jlobj->Release();
	}
	jlobj = _jlobj->Clone();
}

#if 1	// 実装に難あり
// buf参照型のSet()
// bufは所有しない
// bufは他の関数と異なり、正味のデータの先頭であることに注意！！(IDは含まない)
// IDはSetID()で、別途Setする必要がある。
bool JLUFO::SetRef( const uint8_t *buf, int len )
{
	if ( jlobj ){
		jlobj->Release( );
		jlobj = NULL;
	}
	jlobj = new JLinkObject( buf, len, true );
	if ( !jlobj->_Validate( ) ){
		jlobj = NULL;
		error = JLE_NOTENOUGH;
		return false;
	}
	return true;
}
#endif

// JLinks //////////////////////////////////////////////////////////////

#ifdef GUI
BOOL JLinks::CanClose( )
{
	for ( int i=0;i<get_num();i++ ){
		if ( !(*this)[i].CanClose( ) )
			return FALSE;
	}
	return TRUE;
}
#endif

int JLinks::SearchID( t_id id )
{
	for ( int i=0;i<get_num();i++ ){
		if ( (*this)[i].GetID() == id )
			return i;
	}
	return -1;
}

int JLinks::SearchObj( Pdic *dic, t_id id )
{
	for ( int i=0;i<get_num();i++ ){
		JLink &jl = (*this)[i];
		if ( jl.GetDic() == dic && jl.GetID() == id ){
			return i;
		}
	}
	return -1;
}

int JLinks::SearchType( t_jlink type, int first )
{
	for ( ;first<get_num();first++ ){
		if ( (*this)[first].GetType() == type )
			return first;
	}
	return -1;
}

void JLinks::ChangeDic( Pdic &dic )
{
	for ( int i=0;i<get_num();i++ ){
		JLink &jl = (*this)[i];
		if ( jl.GetDic() != &dic ){
			jl.SetDic( &dic );
			jl.SetID( dic.GetObjectNumber() );
		}
	}
}

void JLinks::Clone( JLinks &jlinks )
{
	for ( int i=0;i<jlinks.get_num();i++ ){
		add( jlinks[i].Clone( NULL ) );
	}
}
int JLinks::GetAudioIndex(int firstIndex)
{
	for ( int i=firstIndex;i<(*this).get_num();i++ ){
		if ((*this)[i].IsAudio()){
			return i;
		}
	}
	return -1;	// not found
}

JLink *JLinks::FindPlayObj(bool ole)
{
#if 1
	int index = GetAudioIndex();
	if (index>=0){
		return &(*this)[index];
	}
#ifdef USE_OLE
	if (ole){
		index = SearchType( JL_OLE );
		if ( index != -1 )
			return &(*this)[index];	// OLEデータ
	}
#endif
	return NULL;
#else	// 旧方式
	int i;
#ifdef JL_VOICE
	i = SearchType( JL_VOICE );
	if ( i != -1 )
		return &(*this)[i];	// 音声データ
#endif
#ifdef USE_OLE
	i = SearchType( JL_OLE );
	if ( i != -1 )
		return &(*this)[i];	// OLEデータ
#endif
	return NULL;
#endif
}

