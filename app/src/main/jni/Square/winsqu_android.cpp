// winsqu.hのAndroid用
#include "tnlib.h"
#pragma hdrstop
#include "defs.h"
#include "pddefs.h"
#include "srchout.h"
#include "winsqu.h"
#include "windic.h"
#include "pdprof.h"
#include "WinSquUI.h"

// temporary //
#include "DicBackup.h"

namespace squfont {
int HtmlEnabled = SQM_ALL & ~(SQM_PRON);
bool CanDispDicName = false;
};

// temporary //
COLORREF LinkColors[ N_SECTION+1+1 ];
TDicBackup *DicBackup = NULL;
const tchar *StrNull = _t("");
int nFontsOpened = 0;
#include "PopupConfig.h"
POPUPCONFIG PopupConfig;

int POPUPCONFIG::GetOption()
{
    return 0;
}

HGDIOBJ SelectObject(HDC, TNFONT *)
{
    return NULL;
}
bool DicConvertWizard(TWinControl *parent, const tchar *filename, tnstr &newname, bool typeconvert=false){ return false; }
int MessageDialog( HWND hwnd, const tchar *Title, const tchar *Message,
	const tchar *Button1, const tchar *Button2, int helpid )
{
	DBW("MessageDialog: %s %s %s %s", __cstr(Title).utf8(), __cstr(Message).utf8(), __cstr(Button1).utf8(), __cstr(Button2).utf8());
	return 0;
}
bool BrowseProc( HWND hwnd, int type, tnstr &tfile, int msgno, int help, bool save )
{
	return false;
}
int CreateNewDictionary( TWinControl *parent, const tchar *fname, bool inquiry, tnstr *out_filename, bool confirmMode )
{
	return -1;
}
void ErrorMessage( HWND hwnd, int msgno, const tchar *str, const tchar *title )
{
	DBW("ErrorMessage: msgno=%d %s %s", msgno, __cstr(str).utf8(), __cstr(title).utf8());
}
int MsgBox(int text, int caption, UINT type)	// defined in wpdcom3.cpp
{
	DBW("MsgBox1: text=%d caption=%d type=0x%x", text, caption, type);
	return 0;
}
int MsgBox(HWND hwnd, int text, int caption, UINT type)	// defined in wpdcom3.cpp
{
	DBW("MsgBox2: text=%d caption=%d type=0x%x", text, caption, type);
	return 0;
}
void _SetCurrentHDC( HDC hdc )
{

}
void _CreateTextFonts( HDC hdc )
{
}
void _DeleteTextFonts( )
{

}
TNFONT *hnFont = NULL;

bool AssociateExecute( HWND hwnd, const tchar *filename, int type, bool activate )
{
	return false;
}
void GetTextSize( HDC hdc, TFontAttr &fa, int &cx, int &cy, int &charset, int *attr2=NULL )
{

}

void SrchOutBase::Close( )
{

}
BOOL SrchOutBase::Output( const tchar *word, Japa &japa, int freq )
{
	return FALSE;
}
int SrchOut::GetErrorCode( )
{
	return 0;
}
#include "MouseCapture.h"
TMouseCaptureBase::TMouseCaptureBase(TMouseViewIFBase &view)
	:View(view)
{
	fCapturing = HT_NONE;
}
THyperLink::THyperLink()
{
}
THyperLink::~THyperLink()
{
}
CharHT::CharHT( int _item, POINT *_pt )
{
}
#if 0
TAutoLinkCommon::TAutoLinkCommon(TMouseViewIFCommon &view)
	:View(view)
{
	CursorState = 0;
}
#endif
// //

int Squre::DispOneLine( int no, const tchar *word, int rev, Japa &japa, int dispf, CharHT *cht )
{
	return 0;
}

void Squre::InitView()
{
	LastIndex = 0;
	MaxLines = MAX_NDISP*8;
	IndexOffset = 0;
	IndexMiOffset = 0;

	charcolor = -1;
	backcolor = -1;

	//InitFont();
}
void Squre::SetupView( HDC hdc )
{
}
void Squre::SetOrg()
{
}
void Squre::ClearLastIndex()
{

}

const tchar *GetStateMessage( int msgno )
{
	return _t("");
}
void Squre::LoadProfileView(TRegKey *key){}
void Squre::SaveProfileView(TRegKey *key){}
void Squre::ClsRegion()
{

}
void Squre::CloseAutoLink()
{

}
void Squre::_dispStar( int no, int rev )
{
	//DBW("_dispStar-1:%d rev=%d",no, rev);
	if ( no < 0 || no > LastIndex+1 )
		return;

	TSquUIMain *main = GetMainWindow();
	main->dispStar(IndexOffset+no, rev);	// rev:0=unselect 1=select 2=gray

#if 0
	RECT rc;

#if !defined(SMALL) || SQUONELINE
	if (
#if SQUONELINE
		(!rev && ViewOneLine)
#endif
		){
		DispOneLine( no, GetWord( no ), rev, GetJapa( no ), DF_DISP );
	} else
#endif
	{
		SetRect( &rc, GetOffs( SN_WORD ), GetLocateY( no ), 0, GetLocateY(no)+GetLines(no) );
		COLORREF fore = GET_FORECOLOR( useuniqfont & SNF_WORD ? GetColor( SN_WORD ) : charcolor );
		COLORREF back = GET_BACKCOLOR( backcolor );
		if (rev==1&&cury>=0){
			rev = GetReverseStatus();
		}
		if (rev==2){
			// gray reverse
			fore = graycolor;
		}
		COLORREF oldtext = SetTextColor( hdc, fore );
		COLORREF oldback = SetBkColor( hdc, back );
		DispWord( rc, GetWord( no ), DF_DISP | ( rev ? DF_REVERSE : 0 )
			);
			// 1999.8.25 (cht ? DF_UNREDRAW : 0)追加
			// cht != NULLのとき、Draw2()でfatal errorが発生するため、
			// 対症療法により、chtのときは必ずDF_UNREDRAWにする。
			// これは、WM_PAINT時に発生する。WM_PAINTではcht=NULLになるように小細工
		SetTextColor( hdc, oldtext );
		SetBkColor( hdc, oldback );
	}
#endif
}
int Squre::GetLastPageOffset( )
{
    return -1;
}
int Squre::GetLY( )		// 項目バーを除いた縦のドット数
{
    return 0;
}
void Squre::LoadProfile( )
{

}
void Squre::LoadDicProfiles( TRegKey *section, TRegKey *common )
{
}
void Squre::SaveDicProfiles( class TRegKey *section, TRegKey *common)
{

}
bool Squre::CanExecuteObject()
{
    return false;
}
bool Squre::EditObject()
{
    return false;
}
bool Squre::cmdJumpHyperLink(bool test)
{
	return false;
}
void Squre::ResetFont()
{

}
void Squre::ResetColor()
{

}
bool Squre::IsSelected( )
{
    return false;	//TODO:
}
void Squre::ChangeMaxDispLevel(int offset)
{

}
// hdcが必要
// i : 相対番号
void Squre::ChangeAttr( int i, uchar bit, bool f )
{
	const tchar *word = GetWord( i );
	wa_t orgattr = pool.GetAttr(IndexOffset+i);
	ChangeAttr( word, orgattr, bit, f );
#if 0	//TODO:
	wa_t attr = Dic.get_attr( word );
	BM_AttrSetMark(find_cword_pos(word), attr);
	pool.SetAttr(IndexOffset+i, attr);

	if ( orgattr != attr ){
		if ( bit == WA_MEMORY || bit == WA_JEDIT ){
			if ( IsVisible( SN_ATTR ) ){
				RECT rc;
				SetRect( &rc, GetOffs( SN_ATTR ), GetLocateY(i), GetWidth(SN_ATTR), cyAttr );
				if ( rc.top <= GetLY() && i >= 0 ){
					GetDC();
					CreateTextFonts( );
					SetTextColor( hdc, GET_FORECOLOR( charcolor ) );
					SetBkColor( hdc, GET_BACKCOLOR( backcolor ) );
					DispAttr( rc, attr, true );
					DeleteTextFonts( );
					ReleaseDC();
				}
			}
		}
	}
#endif
}
//Note: Very similar with ToggleAttr(word, bit);
void Squre::ChangeAttr(const tchar *word, uchar bit, bool f)
{
	if (!word){
		ChangeAttr(bit, f);
		return;
	}

#if 0	//TODO:
	tnstr tmp_word = Dic.create_composit_word(word);
	word = tmp_word;

	int r = pool.BSearchExact( word );
	if ( (r < pool.get_num()) && !_tcscmp(pool.fw[r],word) ){
		ChangeAttr( r-IndexOffset, bit, f );
	} else {
		wa_t attr = Dic.get_attr( word );
		if (attr!=0 || Dic.Find(word)>0){
			// 辞書に登録されている場合のみ
			BM_AttrSetMark(find_cword_pos(word), attr);
			ChangeAttr( word, attr, bit, f );
		}
	}
#endif
}
void Squre::ChangeAttr(uchar bit, bool f)
{
	if (cury==-1)
		return;
	ChangeAttr(cury, bit, f);
}
// 見出語による変更
// 表示には影響しない
void Squre::ChangeAttr( const tchar *word, wa_t orgattr, uchar bit, bool f )
{
#if 0	//TODO:
	uchar recbit = bit;
	recbit &= ~WA_MEMORY;
	tnstr tmp_word = Dic.create_composit_word(word);
	if (recbit)
		Dic.ChangeAttrEx(tmp_word, recbit, f, orgattr);
	NotifyChangeAttr(tmp_word, orgattr, f ? (orgattr|bit) : (orgattr&~bit));
#endif
}
void Squre::ToggleAttr( int i, uchar bit )
{
	ChangeAttr( i, bit, ( GetAttr( i ) & bit ) ? false : true );
}
void Squre::ToggleAttr( const tchar *word, uchar bit )
{
#if 0	//TODO:
	if (!word){
		ToggleAttr(bit);
		return;
	}

	tnstr tmp_word = Dic.create_composit_word(word);
	word = tmp_word;

	int r = pool.BSearchExact( word );
	if ( (r < pool.get_num()) && !_tcscmp(pool.fw[r],word) ){
		ToggleAttr( r-IndexOffset, bit );
	} else {
		wa_t attr = Dic.get_attr( word );
		BM_AttrSetMark(find_cword_pos(word), attr);
		ChangeAttr( word, attr, bit, attr & bit ? false : true );
	}
#endif
}
void Squre::ToggleAttr(uchar bit)
{
	if (cury==-1)
		return;
	ToggleAttr(cury, bit);
}
#include "BookmarkMan.h"
void Squre::NotifyChangeAttr(const tchar *word, int oldattr, int newattr)
{
#if USE_BM
	if ((oldattr^newattr) & WA_MEMORY){
		Bookmark.SetMarks(find_cword_pos(word), newattr & WA_MEMORY ? true : false);
	}
#endif
}
bool Squre::cmdDeleteWord(int _cury, bool inquery)
{
	return false;
}
void Squre::cmdCopy(int id)
{

}
void Squre::cmdPlayTTS()
{

}
int Squre::SearchHistory( TSearchHistoryParam &shp )
{
	return 0;
}

void Squre::SelectLinkObject( int i )	// -2: 次へ -3:前へ -1:非選択 0-オブジェクト直接指定
{
}
void Squre::cmdBatchDeleteWord()
{

}
void Squre::cmdEdit(int id)
{

}

void Squre::EvTimer( UINT id )
{
}
void Squre::MoveHistory( bool forward )
{

}
bool Squre::CanMoveHistory( bool forward )
{
	return false;
}

