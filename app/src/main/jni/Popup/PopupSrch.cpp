//---------------------------------------------------------------------------

#include "pdclass.h"
#pragma hdrstop

#include "pdvclUtil.h"
#include "TntControls.hpp"
#include "TntForms.hpp"

#include "PopupSrchUser.h"	// To invoke TMenuWindowUser::setfunc
#include "PopupSrch.h"
#include "PopupConfig.h"
#include "JapaFrm.h"
//---------------------------------------------------------------------------

#include "id.h"
#include "tid.h"
#include "popid.h"
#include "wpdcom.H"
#include "tnthread.h"
#include "japa.h"
#include "hyplink.h"
#include "LangProc.h"
#include "windic.h"
#include "pdprof.h"

// For additional functions
#include "retproc.h"
#include "assexec.h"
#include "PopupHk.h"
#include "WebSrch.h"
#include "psconfig.h"
#include "autolink.h"

#pragma package(smart_init)

#include "UserMsg.h"

#include "UIMain.h"
#include "squfont.h"

#include "BookmarkMan.h"

#define	USE_GS	1

#if USE_GS
#include "GoogleSuggest.h"
#endif

//Note:
// TPopupBaseWindow compatibility
// LastAction -> ModalResult

void ReplaceTab(tchar *str);

#if 0
// IPopupSearchImpl class //
TWinControl *IPopupSearchImpl::GetParent() const
	{ return ps->GetParent(); }
IPopupSearch *IPopupSearchImpl::GetRoot()
	{ return ps->GetRoot(); }
bool IPopupSearchImpl::IsYourWindow( HWND hwnd, bool recur ) const
	{ return ps->IsYourWindow(hwnd, recur); }
bool IPopupSearchImpl::IsOpened() const
	{ return ps->IsOpened(); }
HWND IPopupSearchImpl::GetChildHandle() const
	{ return ps->GetChildHandle(); }
#endif

TPopupSearch *TPopupSearch::Root = NULL;
int TPopupSearch::MsgLoop = 0;
bool TPopupSearch::DoingPreSearch = false;


// TPopupSearch class
TPopupSearch::TPopupSearch()
{
	DBP("create:%08X",this);

	ExcStyle = 0;

	POINT zero = {0,0};
	orgpt = zero;
	//Size = zero;
	Flags = 0;
	ViewFlags = PSF_SHOWMASK & ~PSF_SHOWDICNAME;
	MaxWordNum = 1;
	type = 0;
#ifdef EPWING
	bookno = 0;
	pos = 0;
	epdic = NULL;
#endif
	Parent = NULL;
	Child = NULL;
	Receiver = NULL;
	Owner = NULL;
	Popup = NULL;
	MenuWin = NULL;
#if SQUFONT
	hMenuFont = NULL;
#endif
	HitWords = new MatchArray;
	pt = zero;
	HitWordIndex = 0;
	hwndPopup = NULL;
	hwndMenuWin = NULL;
	freeId = 0;
	SLWRunning = false;
	ThreadKey = 0;
}
TPopupSearch::~TPopupSearch()
{
	if (Child){
		Child->Parent = NULL;
	}
	DBP("delete:%08X",this);
	SLWStop();
#if SQUFONT
	if ( hMenuFont ) DeleteObject( hMenuFont );
#endif
//	if ( jlinks ) delete jlinks;
	delete HitWords;
	if (MenuWin) delete MenuWin;
//	if (iPS)
//		delete iPS;
	if (Popup)
		delete Popup;
	if (Root==this)
		Root = NULL;
}

//
// Override methods for IPopupSearch
//

// 上位と同じ属性のPopup Windowを作成するためのcopy method
void TPopupSearch::AssignParameters(IPopupSearch *_ps)
{
	TPopupSearch *ps = (TPopupSearch*)_ps;
	this->MaxWordNum = ps->MaxWordNum;
	this->Receiver = ps->Receiver;
	this->Flags = ps->Flags;
	this->ViewFlags = ps->ViewFlags;
}
// hwndが自分のpopup or menuか？
// recurがtrueである場合は自分の子も調べる
// stopがある場合は、stopより下のchildは調べない
bool TPopupSearch::IsYourWindow( HWND hwnd, bool recur, HWND stop ) const
{
	if ( GetWindowHandle() == hwnd )
		return true;
	if (stop && (GetWindowHandle()==stop))
		return false;
	if ( recur ){
		if (IsPopupOpened()){
			IPopupSearch *childps = Popup->GetChildPS();
			//dbw("Opened:childps=%08X/%08X", (int)childps, (int)Root);
			if (childps){
				return childps->IsYourWindow( hwnd, recur, stop );
			}
		} else
		if (IsMenuOpened()){
			if (MenuWin->IsPopupOpened()){
				TPopupWindowUser *popup = MenuWin->GetPopupWindow();
				if (popup){
					if (popup->GetHandle()==hwnd){
						return true;
					}
				}
			}
		}
	}
	return false;
}
IPopupSearch *TPopupSearch::GetRoot()
{
	return Root;
}

// 最も前面にあるpopup windowを返す
TPopupSearch *TPopupSearch::GetTopMost()
{
	TPopupSearch *ps = Root;
	while (ps){
		if (!ps->IsPopupOpened()){
			break;
		}
		if (!ps->Child)
			break;
		ps = ps->Child;
	}
	return ps;
}

// ptにあるpopup windowを返す
TPopupSearch *TPopupSearch::GetFromPos(POINT pt)
{
	HWND hwndPt = GetTopParent(WindowFromPoint(pt));
	if (!hwndPt)
		return NULL;
	TPopupSearch *ps = Root;
	while (ps){
		if (!ps->IsPopupOpened()){
			return NULL;
		}
		if (ps->GetWindowHandle()==hwndPt)
			return ps;
		ps = ps->Child;
	}
	return NULL;
}

// １つでもpopup windowがopenされているか？
bool TPopupSearch::IsAllOpened()
{
	return Root && Root->IsOpened();
}
//Note:
// 一回呼んだだけではうまくいかないかもしれない。（３つ以上windowを開いた場合）
void TPopupSearch::CloseAll()
{
	TPopupSearch *ps = Root;
	while (ps){
		if (ps->Close())
			break;
		ps = ps->Child;
	}
}

HWND TPopupSearch::GetChildHandle() const
{
	if ( Popup ){
		if ( IsPopupOpened() )
			return Popup->GetHandle();
	}
	if ( MenuWin )
		if ( IsMenuOpened() )
			return MenuWin->GetHandle();
	return NULL;
}
// popup window / menu windowをcloseすると、true
bool TPopupSearch::Close(bool wait)
{
	bool r = false;
	SLWStop();
	if ( Popup ){
		if ( IsPopupOpened() ){
			//NotifyChildClose(Popup->Handle);
			Popup->Close( );
			r = true;
		}
		CloseNotify();	// self notify
	}
	if ( MenuWin ){
		if ( IsMenuOpened() ){
			//NotifyChildClose(MenuWin->Handle);
			MenuWin->Close( );
			r = true;
		}
		CloseNotify();	// selfnotify
	}
#if SQUFONT
	if ( hMenuFont ){
		DeleteObject( hMenuFont );
		hMenuFont = NULL;
	}
#endif
	SLWStop();
	HitWords->clear();	// メモリ節約のため
#if 0
	if (wait){
		// 完全に終了するまで待つ
		// 2010.1.30 comment outしてみた
		//Application->ProcessMessages();	//TODO: popupしている状態で、PSWを閉じると例外が発生することに対する対症療法
	}
#endif
	return r;
}

void TPopupSearch::CloseNotify()
{
	SLWStop();
	if (Root==this)
		Root = NULL;
	if (Popup){
		Popup->SetPS(NULL);
		Popup->Release();
		Popup = NULL;
	}
	if (MenuWin){
		MenuWin->SetPS(NULL);
		MenuWin->Release();
		MenuWin = NULL;
	}
}

void TPopupSearch::GetNums(int &entry, int &numHitWords)
{
	entry = HitWords->get_num() - HitWordIndex;
	numHitWords = HitWords->get_num();
}

int TPopupSearch::GetStyle()
{
	TPopupBaseWindowUser *win = GetActiveWindow();
	if (!win)
		return 0;
	return win->GetStyle();
}
void TPopupSearch::SetStyle(int style)
{
	TPopupBaseWindowUser *win = GetActiveWindow();
	if (!win)
		return;
	win->SetStyle(style);
}
const tchar *TPopupSearch::GetCompleteWord()
{
	TPopupBaseWindowUser *win = GetActiveWindow();
	if (!win)
		return NULL;
	return win->GetCompleteWord();
}


//
// Belows are unique members for this class.
//

void TPopupSearch::Initialize()
{
	SLWStop();
	Child = NULL;
//	if ( jlinks ) delete jlinks, jlinks = NULL;
}
int TPopupSearch::GetBaseStyle() const
{
	int ret = 0;
	if (PopupConfig.fFixPos){
		ret |= MWS_FIXEDMODE;
	}
	if (PopupConfig.fFixWidth){
		ret |= MWS_FIXEDSIZE;
	}
	return ret & ~ExcStyle;
}
// 指定行からn行読み込む
bool ReadTextFileLine( const tchar *filename, int line, int n, _jMixChar &text )
{
	TIFile tif;
	if ( tif.open( filename ) == -1 ) return false;
	while ( line > 0 && (tif.skipline( ) != -1) ) line--;
	if ( line == 0 ){
		// 正常に行番号へジャンプした
		if ( n > 0 ){
			for (;;){
				tnstr s;
				if ( tif.getline( s ) < 0 ) break;
				text.cat( s );
				if ( --n == 0 ) break;
				text.cat( _T("\r\n") );
			}
		}
		return true;
	}
	return false;
}
void TPopupForWordsParam::ForwardWords( int skipchar )
{
	curi++;
	if ( curi < wordlengths.get_num() ){
		// すでに検索した単語の中
		curp = word + wordlengths[curi];
	} else {
		// まだ検索していない単語
		curp += skipchar;
		while ( *curp && !IsWordChar( *curp ) ) curp++;
	}
}
void TPopupForWordsParam::BackwardWords()
{
	curp = word + wordlengths[--curi];
}
void TPopupForWordsParam::SetWord(const tchar *__word, const tchar *__prevword, int __click_pos)
{
	int offset = __prevword ? (int)(__word - __prevword) : 0;
	wholeword = __prevword ? __prevword : __word;
	prevword = __prevword ? (const tchar*)wholeword : (const tchar*)NULL;
	word = &wholeword[offset];
	click_pos = __click_pos<=0 ? 0 : __click_pos;
	curp = word;
	curi = 0;
}
// 検索のみ行い、ポップアップ表示は行わない
void TPopupSearch::SearchHitWords( const tchar *_word, const tchar *_prevword, tnstr_vec &hitwords )
{
	const int curpos = 0;
	int r = PrePopupSearch( _word, _prevword, curpos, false, PopupConfig.GetOption() | SLW_ELIMHYPHEN );
	if ( r ){
		for ( int i=0;i<HitWords->get_num();i++ ){
			hitwords.add( (*HitWords)[i].word );
		}
	}
}
// 複数単語検索機能付きPopup
// orgpt,Flags,Receiver,MaxWordNumは予めセットしておく
void TPopupSearch::PopupForWords( const tchar*__word, int click_pos, const tchar *__prevword, tnstr *foundword, bool tmp_clickpopup, const tchar *url )
{
	pfw.SetWord(__word, __prevword, click_pos);
	if ( Parent ){
#if 0
		int style;
		TPopupBaseWindowUser *win = ((TPopupSearch*)Parent)->GetActiveWindow();
		__assert(win);
		if (win){
			style = win->GetStyle();
			win->SetStyle(style & ~MWS_INACTIVECLOSE);
		}
#endif
		((TPopupSearch*)Parent)->Child = this;
		PopupForWordsLoopStart(tmp_clickpopup);
		while (IsOpened()){
			WaitAppMessageIdle();
		}
		if (Parent)
			((TPopupSearch*)Parent)->Child = NULL;
#if 0
		if (win){
			win->SetStyle(style);
		}
#endif
	} else {
		// 自分自身がroot windowである場合のみmessage loopする
		PopupForWordsLoop( this, foundword, tmp_clickpopup, url );
	}
}
int TPopupSearch::PopupForWordsLoopStart(bool tmp_clickpopup, const tchar *url)
{
	Flags &= ~(PSF_FORCEMENU|PSF_CLICKSEARCH);
	Flags |= PSF_QUICKPOPUP;
#if !ONLYCLICK
	if (tmp_clickpopup){	// 4.11 1999.5.30 追加
		Flags |= PSF_CLICKPOPUP;
	}
#endif
#if SQUFONT
	hMenuFont = NULL;
#endif
	Popup = NULL;
	MenuWin = NULL;
	POINT pt = orgpt;
	int r = OpenPopupSearch( Owner, Parent, pfw.curp, pfw.click_pos, pfw.curp == pfw.word ? pfw.prevword : NULL, false, PopupConfig.GetOption() | SLW_ELIMHYPHEN, url );

	if ( pfw.curi == pfw.wordlengths.get_num() ){
		pfw.wordlengths.add( STR_DIFF(pfw.curp,pfw.word) );
	}

	if ( !r ){
		// ヒットしない場合
		if ( Flags & PSF_NOHITREC ){
			// 登録へ
			tnstr _word( pfw.word );
			::ReturnProc( NULL, _word, -1, NULL, false );
			return 0;	// end
		}
		const tchar *sp = pfw.curp;
		while ( IsWordChar( *sp ) ) sp++;
		r = STR_DIFF(sp,pfw.curp);
	}
	orgpt = pt;
	return r;
}
// この関数を呼べるのは、root parent windowのみ
void TPopupSearch::PopupForWordsLoop( TPopupSearch *ps, tnstr *foundword, bool tmp_clickpopup, const tchar *url )
{
	int PrevAction = MWA_NONE;
	TPopupSearch *target_ps = ps;
	//__assert(!Root);
	Root = ps;
	TPopupSearch *root_ps = ps;
	DBP("root_ps=%08X",root_ps);

	MsgLoop++;
	ZOrderInhibit++;
		// Inhibit the z-order change of the main window.
		// Do not return without decr. the variable.

	while ( 1 ){
		int r = target_ps->PopupForWordsLoopStart(tmp_clickpopup, url);
#if 0	// 2009.4.7 外した（これを有効にするとヒットしない記号のみの文字をquick popupすると落ちる）
		if ( r == 0 ){
			break;
		}
#endif

		int last_action;
		while ( 1 ){
			last_action = MWA_NONE;
			TPopupBaseWindowUser *target_win;
			HWND target_hwnd;
			if ( target_ps->MenuWin || target_ps->Popup ){
				if ( foundword ){
					foundword->set( target_ps->srchWord );
				}
				do {
					HWND root_hwnd = root_ps->GetActiveWindow()->GetHandle();

					target_win = target_ps->GetActiveWindow();
					target_hwnd = target_win->GetHandle();
					if ( target_ps == root_ps ){
						if (PopupConfig.fToFront){
							if (PopupConfig.fSetFocus){
								MoveToTop( target_hwnd );	// rootである場合のみ最前面へ
							} else {
								MoveToTopNA( target_hwnd );	// rootである場合のみ最前面へ
							}
						}
					}
					target_win->SetModalResult(MWA_NONE);
					while ( ::IsWindow( root_hwnd ) ){
						TPopupSearch *ps = NULL;
						last_action = root_ps->GetLastAction(&ps);
						if ( last_action != MWA_NONE ){
							target_ps = ps;
							target_win = target_ps->GetActiveWindow();
							target_hwnd = target_win ? target_win->GetHandle() : NULL;
							break;
						}
						WaitAppMessageIdle( );
					}
					if ( last_action == MWA_CHANGEWIN_FOR
						|| last_action == MWA_CHANGEWIN_BAK
						|| last_action == MWA_REFRESH
						|| last_action == MWA_CHANGEWORD
						)
						break;
				} while ( target_ps->IsOpened() );
			} else {
				// Windowなし
				// ここはどういうときに来る？？
				last_action = PrevAction;
			}

			if ( last_action != MWA_NONE ){
				TPopupForWordsParam &pfw = target_ps->pfw;
				PrevAction = last_action;
				// チェック
				if ( last_action == MWA_CHANGEWIN_FOR ){
					if ( pfw.curi+1 >= pfw.wordlengths.get_num() ){
						// まだ検索していない単語 //
						const tchar *sp = pfw.curp + r;
						while ( *sp && !IsWordChar( *sp ) ) sp++;
						if ( !*sp ) continue;	// 文の最後だからcontinue
					}
				} else
				if ( last_action == MWA_CHANGEWIN_BAK ){
					if ( pfw.curi == 0 )
						continue;
				} else
				if ( last_action == MWA_CHANGEWORD ){
					pfw.SetWord(target_win->GetCompleteWord(), NULL, 0);
				}
				::SendMessage( target_hwnd, WM_CLOSE, 0, 0 );
			}
			break;
		}

		if ( last_action == MWA_NONE )
			break;

		// 前のwindowのclose処理が終了するまで待つ
		// 2010.12.20
		// そうしないと、TPopupSearch無いのclose処理も完了しないため、
		// 新しいwindowができてから終了処理が走ってしまい、
		// 新しいwindow handleを使って終了処理を行ってしまう。
		WaitAppMessageIdle( );

		target_ps->Initialize();

		if ( last_action == MWA_CHANGEWIN_FOR ){
			target_ps->pfw.ForwardWords( r );
		} else
		if ( last_action == MWA_CHANGEWIN_BAK ){
			target_ps->pfw.BackwardWords( );
		}
	}
	ZOrderInhibit--;
	MsgLoop--;
	Root = NULL;
}
//Note:
// child windowとして開く場合は、Parent->SetChild(self); が必要
void TPopupSearch::Open( TPopupSearch *parent, const tchar *word, bool tmp_fixedpopup, const tchar *url )
{
	Parent = parent;

	if (!Parent){
		Parent = GetTopMost();
		if (Parent==this)
			Parent = NULL;
	}

	if (Parent)
		Parent->SetChild(this);
	if (!Root)
		Root = this;

	__assert(Parent!=this);	// んなアホな
		
	pt = orgpt;
#if 0	// 2008.6.29非対応
	if (!(Flags & PSF_FIXEDSIZE)){
		pt.x += 16;
		pt.y += 16;
	}
#endif
	// ポップアップ検索表示
	Japa *japa = new Japa;
	if ( HitWords->get_num() == 0 )
		HitWords->Add( word, 0, 0 );
	HitWordIndex = HitWords->get_num()-1;

	int AddStyle = GetBaseStyle();
	int ExcViewFlags = 0;

	MPdic *dic;
	switch ( type ){
		case 0:
		case HLT_WORD:
		case HLT_WORD2:
			ReadData(HitWordIndex, japa);
			dic = GetActiveDic();
			if (dic){
				if (SLWRunning)
					SLWStop();
				//__assert(!SLWRunning);
				if (dic->SearchLongestWordExt(word, NULL, HitWords, ThreadKey, SLWExtCallback, (int)this)){
					// search started.
					SLWRunning = true;
				}
			}
			break;
#ifdef EPWING
		case HLT_EPWING:
			japa->jlinks.add( new JLEPWing( epdic, 0, bookno, pos ) );
			AddStyle = MWS_LINKOBJECT;
			ExcViewFlags |= PSF_TTSPLAY;
			break;
#endif
#ifdef USE_JLINK
		case HLT_FILE:
			{
				dic = GetActiveDic();
				if (dic){
					tnstr fullpath;
					int i;
					FindFileLinkPath(dic, fullpath, word, i);
					japa->jlinks.add( new JLFile( i!=-1 ? &(*dic)[i] : NULL, i!=-1?fullpath.c_str():word, 0 ) );
					AddStyle = MWS_LINKOBJECT;
				}
				ExcViewFlags |= PSF_TTSPLAY;
			}
			break;
#endif
#if USE_DT2
		case HLT_TEXT:
			{
				tnstr name;
				int line, col;
				GetTextLinkParam( word, name, line, col, GetActiveDic());
//				if ( line > 3 ) line -= 3;	// ３行前から
				if ( !ReadTextFileLine( name, line, 20, japa->japa ) ){	// ２０行読む
//					return ;	// cannot read
				}
				AddStyle = MWS_LINKOBJECT;
				ExcViewFlags |= PSF_TTSPLAY;
			}
			break;
#endif
		case HLT_HTTP:
		case HLT_MAILTO:
		case HLT_HTML_HREF:
			AssociateExecuteEx( NULL, word, type, true, NULL );
			delete japa;
			return;
		default:
			delete japa;
			return;
	}

#if 0
	if (!IsToMainSrchWord()){
//	srchWord = word;
	}
#endif
	if ( !Popup || !IsWindow(Popup->GetHandle()) ){
		if (Popup)
			delete Popup;
		Popup = new TPopupWindowUser( Owner );
#if 0	// 2008.6.29非対応
		if (Flags & PSF_FIXEDSIZE){
			Popup->SetSize(Size);
		}
#endif
	} else {
#ifndef SMALL
		if ( tmp_fixedpopup ){
			//TODO: this will not run correctly
			pt.x = pt.y = -1;
		}
#endif
	}
	Popup->SetTitle(word);
	Popup->SetJapa(japa);
	Popup->SetPosition(pt);

	Popup->SetPS(this);
	if ( Receiver )
		Popup->SetReceiver( Receiver );

//	if ( Owner )
//		Popup->SetOwner(Owner);
	if (Flags & PSF_ADDHIST){
		Popup->SetAddHistory(true);
		Popup->SetUrl(url);
	}

	AddStyle |= (Flags & PSF_COMMASK);

	if ( !(Flags & PSF_QUICKPOPUP) )
		AddStyle |= MWS_DISABLE_POPUPWIN;

	AddStyle |= 
		(
#ifndef SMALL
		tmp_fixedpopup ? (MWS_DRAGMOVE|MWS_FIXEDMODE) :
#endif
		MWS_CLICKCLOSE)|MWS_USEHYPERLINK|MWS_PASSMOUSEMOVE;

//	if ( HitWords->get_num() > 1 )
//		AddStyle |= MWS_MULTIHIT;
	Popup->SetMemory(japa->IsMemory());
	Popup->SetStyle(Popup->GetStyle() | AddStyle);
	Popup->SetViewFlag(ViewFlags & ~ExcViewFlags);
#ifdef USE_POPUPHOOK
	ExecPopup( word );
#endif
	hwndPopup = Popup->GetHandle();
	hwndMenuWin = NULL;
	if (!Popup->IsVisible()){
		//SetVCLHookTarget(Popup->GetHandle());
		if ( Flags & PSF_NOACTIVATE )
			Popup->ShowNoActivate();
		else
			Popup->Show();
		//SetVCLHookTarget(NULL);
	} else {
		Popup->SetLayeredHalf(false);
		Popup->SetupWindow();
	}
	//Popup->Play();
}
void TPopupSearch::OpenAsMenu( const tchar *word )
{
	// ヒットしない >> 一覧表示
	pt = orgpt;
	if ( Flags & PSF_CLICKSEARCH ){
		pt.x += 16;
		pt.y += 16;
	}

	MPdic *dic = GetActiveDic();
	if (dic){
		if (SLWRunning)
			SLWStop();
		//__assert(!SLWRunning);
		if (dic->SearchLongestWordExt(word, NULL, HitWords, ThreadKey, SLWExtCallback, (int)this)){
			// search started.
			SLWRunning = true;
		}
	}

	// ポップメニューの表示
	MenuWin = new TMenuWindowUser( Owner );
	MenuWin->SetPosition(pt);
	if ( Receiver )
		MenuWin->SetReceiver(Receiver);
//	if ( Owner )
//		MenuWin->SetOwner(Owner);
	MenuWin->SetPS(this);

	int AddStyle = GetBaseStyle();
	
	if ( !(Flags & PSF_CLICKSEARCH) || (Flags & PSF_FORCEMENU) ){
		AddStyle |= MWS_CAPTURE;	// マウスがメニュー外に移動してもクローズしない
	}

	AddStyle |= (Flags & (PSF_NOACTIVATE|PSF_CLICKPOPUP|PSF_MULTIWORD|PSF_CTRLCLOSE));
	AddStyle |= MWS_USEHYPERLINK|MWS_INACTIVECLOSE | MWS_CLICKCLOSE;	// 2000.1.6 MWS_CLICKCLOSE追加
	if ( !(Flags & PSF_QUICKPOPUP) )
		AddStyle |= MWS_DISABLE_POPUPWIN;

	MenuWin->SetViewFlag( ViewFlags );
	MenuWin->SetStyle(MenuWin->GetStyle() | AddStyle);
	
#if NOHITLIST
	MPdic *dic = GetActiveDic();
	if (!dic)
		return;	// not opened.

	tnstr oword;
	if ( dic->SearchOptimalWord( word, MaxWordNum, oword ) ){
		dic->ForwardSearch( oword, MaxWordNum, setfunc, MenuWin );
	}
#endif

	if ( TUIMain::GetInstance() && TUIMain::GetInstance()->IsEnabled() ){
#if !NOHITLIST
		if (word[0]){
			MenuWin->SetTitle(word);
			MenuWin->SetInitialSelect(1);
		}
#endif
		MenuWin->AppendMenu( _LT( IDS_SEARCHMAIN ), CMW_SEARCH, 0, NULL, true );	// 本体で検索
#ifdef USE_WEBSEARCH
		for ( int i=0;i<MAX_WEBSRCHMENU;i++ ){
			tnstr site;
			int shortcut;
			if (!WebSrchGetMenu(i, site, shortcut))
				break;
			site = tnstr(_t("\t")) + site;	// <keyword>\t<cword>
			MenuWin->AppendMenu( site, CM_WEBSEARCH+i, shortcut, NULL, true );	// Webで検索
		}
#endif
#ifndef LIGHT
		MenuWin->AppendMenu( _LT( IDS_RECORDWORD ), CMW_RECORD, VK_RETURN+SK_CTRL, NULL, true );	// 単語登録
#endif
		MenuWin->AppendMenu( _LT( IDS_PSPLAYTTS ), CMW_PLAYTTS, 'R'+SK_CTRL, NULL, true );	// 単語登録
		MenuWin->AppendMenu(_LT(IDS_PSCONFIG), CM_PSCONFIG, 'G'+SK_CTRL, NULL, true);
#ifdef SMALL
		MenuWin->AppendMenu( _LT( IDS_CLOSEMENU ), CMW_CLOSEMENU, 0, NULL, true );	// メニューを閉じる
#endif
	}
	hwndPopup = NULL;
	if ( MenuWin->GetMenuItemNum() ){
		hwndMenuWin = MenuWin->GetHandle();
#if USE_GS
		if (gsEnabled()){
			gsHttpGetAsync(word, hwndMenuWin, UM_GOOGLESUGGEST);
		}
#endif
		if ( Flags & PSF_NOACTIVATE )
			MenuWin->ShowNoActivate();
		else
			MenuWin->Show();
	} else {
		delete MenuWin;
		MenuWin = NULL;
#if SQUFONT
		DeleteObject( hMenuFont );
		hMenuFont = NULL;
#endif
	}
}
// 次の候補の単語へ
void TPopupSearch::NextHitWord(bool next)
{
	if ( !IsPopupOpened() ) return;

	MPdic *dic = GetActiveDic();
	if (!dic)
		return;

	if ( HitWords->get_num() <= 1 ) return;

	if (next){
		if ( --HitWordIndex < 0 ) HitWordIndex = HitWords->get_num()-1;
	} else {
		if ( ++HitWordIndex >= HitWords->get_num() ) HitWordIndex = 0;
	}
	Japa *japa = new Japa;
	const tchar *word = (*HitWords)[HitWordIndex].word;
	ReadData(HitWordIndex, japa);
	if (!IsToMainSrchWord()){
		srchWord.set( word );	// ヒットした単語
	}
	Popup->SetMemory(japa->IsMemory());
	Popup->SetTitle(word);
	Popup->SetJapa(japa);
//	Popup->SetPosition(pt);	// はずした 2008.6.16
	Popup->ClearPlayed();
	Popup->SetupWindow();
	//Popup->Play();
}

void TPopupSearch::PrevHitWord()
{
	NextHitWord(false);
}

void TPopupSearch::ReadData(int index, Japa *japa)
{
	MPdic *dic = GetActiveDic();
	bool found = false;
	MATCHINFO &mi = (*HitWords)[index];
	if (dic){
		tnstr cword;
		cword = dic->create_composit_word(mi.word);
#ifdef DISPDICNAME
		if (squfont::CanDispDicName){
			ViewFlags |= PSF_SHOWDICNAME;
		}
		__EnableDDNBegin(squfont::CanDispDicName);
#endif
		if (dic->Read(cword, japa)){
			found = true;
		}
		__EnableDDNEnd();
		if (found){
			BM_JSetMark(find_cword_pos(cword), *japa);
		}
	}
	if (!found){
		// not found
		japa->clear();
		if (mi.ext.empty()){
			japa->japa = _LT(MSG_NotLinkedLabel);
		}
	}
	if (mi.ext.exist()){
		japa->japa += _t("\n");
		japa->japa += mi.ext;
	}
}


// 現在表示中の単語を返す
// menu形式の場合はNULLを返す
const tchar *TPopupSearch::GetHitWord()
{
	if (MenuWin)
		return NULL;
	return (*HitWords)[HitWordIndex].word;
}
const struct MATCHINFO *TPopupSearch::GetHitWord(int index)
{
	return &(*HitWords)[index];
}
int TPopupSearch::GetNumHitWords() const
{
	return HitWords->get_num();
}
BOOL TPopupSearch::setfunc( int num, const tchar *word, class Japa &japa, void *menuwin )
{
	return TMenuWindowUser::setfunc(num, word, japa, menuwin);
}
void TPopupSearch::PostClose()
{
	if ( Popup ){
		if ( IsPopupOpened() ){
			Popup->DelayedClose();
		}
	}
	if ( MenuWin ){
		if ( IsMenuOpened() ){
			MenuWin->DelayedClose();
		}
	}
#if SQUFONT
	if ( hMenuFont ){
		DeleteObject( hMenuFont );
		hMenuFont = NULL;
	}
#endif
	SLWStop();
	HitWords->clear();	// メモリ節約のため
}

void TPopupSearch::EnableActiveCheck()
{
	if ( Popup ){
		if ( IsPopupOpened() ){
			Popup->EnableActiveCheck();
		}
	}
	if ( MenuWin ){
		if ( IsMenuOpened() ){
			MenuWin->EnableActiveCheck();
		}
	}
}

// ヒット時 to main で使用する単語は検索に使った単語
bool TPopupSearch::IsToMainSrchWord()
{
	return PopupConfig.fToMainSrchWord;
}

// Getter/Setter //
TPopupBaseWindowUser *TPopupSearch::GetChild()
{
	if ( Popup ){
		return Popup;
	}
	if ( MenuWin )
		return MenuWin;
	return NULL;
}
// GetTopMostPopup()とGetPointPopup()の共通関数
TPopupWindowUser *TPopupSearch::GetPopupCommon( POINT *pt )
{
	TPopupWindowUser *win = NULL;
	TPopupWindowUser *pw = Popup;
	TMenuWindowUser *mw = MenuWin;
	for(;;){
		if (pw){
			if (!pw->IsWindow())
				break;
			if ( pt ){
				// pt上にあるwindowを探す
				RECT rc;
				GetWindowRect( pw->GetHandle(), &rc );
				if ( PtInRect( &rc, *pt ) ){
					// ptがwin::rect内にある
					win = pw;
				}
			} else {
				// 最上位windowを探す
				win = pw;
			}
			TPopupSearch *ps = dynamic_cast<TPopupSearch*>(pw->GetChildPS());
			if (!ps)
				break;	// no child window
			if (!ps->IsOpened())
				break;	// not opened child window
			if (ps->IsPopupOpened()){
				pw = ps->Popup;
			} else {
				mw = ps->MenuWin;
				pw = NULL;
			}
		} else
		if (mw){
			if (!mw->IsPopupOpened())
				break;	// no Popup
			pw = mw->GetPopupWindow();
			mw = NULL;
		} else {
			break;
		}
	}
	return win;
}
// 最も手前のpopup windowを返す
TPopupWindowUser *TPopupSearch::GetTopMostPopup()
{
	return GetPopupCommon( NULL );
}
// ptを含む、最上のpopup windowを返す
TPopupWindowUser *TPopupSearch::GetPointPopup( POINT pt )
{
	return GetPopupCommon( &pt );
}

// thisを最上位親（一番下）のwindowとして、すべてのchildpsから
// LastActionを得る
// last_winはそのときの TPopupWindow
int TPopupSearch::GetLastAction( TPopupSearch **pps )
{
	TPopupBaseWindowUser *win = GetActiveWindow();
	bool is_popup = Popup ? true : false;
	for(;win;){
		HWND hwnd = win->GetHandle();
		if ( !::IsWindow( hwnd ) ){
			break;
		}
		if ( win->GetModalResult() != MWA_NONE ){
			if ( pps )
				*pps = win->GetPS();
			return win->GetModalResult();
		}
		if ( !is_popup )
			break;
		TPopupSearch *ps = dynamic_cast<TPopupSearch*>(((TPopupWindowUser*)win)->GetChildPS());
		if ( !ps )
			break;	// may be menu or normal close
		if ( ps->Popup ){
			win = ps->Popup;
		} else {
			win = ps->MenuWin;
			is_popup = false;
		}
	}
	return MWA_NONE;
}
void TPopupSearch::Free(int id)
{
	if (id==freeId){
		delete this;
	} else {
#if defined(_DEBUG)
		//DBW("TPopupSearch::Free this->freeId=%d id=%d", this->freeId, id);
#endif
	}
}
void TPopupSearch::EvKeyDown(int key, int shift)
{
	TPopupBaseWindowUser *child = GetChild();
	if (!child)
		return;
	child->EvKeyDown(key, shift);
}
bool TPopupSearch::EvChar(int key)
{
	TPopupBaseWindowUser *child = GetChild();
	if (!child)
		return false;
	return child->EvChar(key);
}

// if PSIMOPEN==true
//	このmethodが別threadで呼ばれるため、これ以下の関数はすべてthread safeでなければならない！！
int TPopupSearch::PrePopupSearch(const tchar *word, const tchar *prevword, int curpos, bool complete, int option)
{
	MPdic *dic = GetActiveDic();
	if (!dic)
		return 0;

	if (!dic->ThreadUp())
		return 0;

	int r = 0;
		
	try {		
#if defined(USE_JLINK)
		bool fLinkAttr;
		if ( Flags & PSF_PLAY )
			fLinkAttr = dic->SetLinkAttr( OLA_NOTREADOBJ & ~(OLA_NOTREADVOICE|OLA_NOTREADEPWING), true );	// 高速化のため
		else
			fLinkAttr = dic->SetLinkAttr( OLA_NOTREADOBJ & ~OLA_NOTREADEPWING, true );	// 高速化のため
#endif

		HitWords->clear();
		r = dic->SearchLongestWord( word, prevword, curpos, option, HitWords );
#if defined(USE_JLINK)
		dic->SetLinkAttr( OLA_NOTREADOBJ, fLinkAttr );
#endif
	} catch(...){
		DBW("catch the PrePopupSearch exception");
	}

	dic->ThreadDown();

	return r;
}

#if 0
//TODO: いずれ共通ライブラリへ
class TMutexCounter {
	LONG Counter;
public:
	TMutexCounter(int initial = 0)
	{
		Counter = initial;
	}
	inline TMutexCounter &operator ++ (){
		InterlockedIncrement(&Counter);
	}
	inline TMutexCounter &operator -- (){
		InterlockedDecrement(&Counter);
	}
};
#endif

class PrePopupSearchThread : public tnthread {
typedef tnthread super;
public:
	TPopupSearch *obj;
	const tchar *word;
	const tchar *prevword;
	int curpos;
	bool complete;
	int option;
	int result;

	// mutex for another thread //
	static TSem instSem;
	bool locked;
public:
	PrePopupSearchThread()
		:super(true)
	{
		result = 0;
		locked = false;
	}
	virtual void Execute()
	{
		result = obj->PrePopupSearch(word, prevword, curpos, complete, option);
		if (locked){ instSem.Unlock(); }
	}
	void wait_and_resume()
	{
		const int wait_time = 10000;	// 10sec?
		if (instSem.Wait(wait_time)){
			locked = true;
		} else {
			DBW("PrePopupSearchThread::wait_and_resume: timeout");
		}
		Resume();
	}
};

TSem PrePopupSearchThread::instSem(1, 1);

// sub threading
PrePopupSearchThread *TPopupSearch::PrePopupSearchTH(const tchar *word, const tchar *prevword, int curpos, bool complete, int option)
{
	PrePopupSearchThread *thread = new PrePopupSearchThread;
	thread->obj = this;
	thread->word = word;
	thread->prevword = prevword;
	thread->curpos = curpos;
	thread->complete = complete;
	thread->option = option;
	thread->wait_and_resume();
	//thread->Resume();
	return thread;
}


// 複数語対応検索
int TPopupSearch::OpenPopupSearch(TComponent *owner, IPopupSearch *parent, const tchar *_word, int curpos, const tchar *_prevword, bool complete, int option, const tchar *url)
{
	MPdic *dic = GetActiveDic();
	if (!dic)
		return 0;

	if (DoingPreSearch) return 0;	// これでstack overflowは避けられるが。。

#if PSIMOPEN	// 検索前にウィンドウを表示
	PrePopupSearchThread *th = PrePopupSearchTH( _word, _prevword, curpos, complete, option );
	if (!th)
		return 0;
	if ( !Popup || !IsWindow(Popup->GetHandle()) ){
		if (Popup)
			delete Popup;
		Popup = new TPopupWindowUser( owner );	// 2014.12.28 Ownerになっていたため、NULLが渡され初回のpopupではうまく動作しない時があった
		Popup->SetPosition(orgpt);
#if 0
		Japa japa;
		int find = dic->Find(_word, &japa);
		if (find>0){
			// ここの処理はTPopupSearch::Open()と同じ処理が必要
			Popup->SetTitle(_word);
			Popup->SetJapa(&japa);
		} else 
#endif
		{
			Popup->SetTitle(_t("Searching..."), true);
			Popup->SetLayeredHalf(true);
		}
		//SetVCLHookTarget(Popup->GetHandle());
		if ( Flags & PSF_NOACTIVATE )
			Popup->ShowNoActivate();
		else
			Popup->Show();
		hwndPopup = Popup->GetHandle();
		hwndMenuWin = NULL;
		//SetVCLHookTarget(NULL);
	}

	Hourglass(true);
	DoingPreSearch = true;
	int r = -1;
	while (1){
		Application->ProcessMessages();	//TODO: ここをwaitしているときに他のthreadが起動して別のPopupが開き、それが幾度も重なりstack overflowとなる
		if (th->IsFinished()){
			r = th->result;
			break;
		}
	}
	delete th;
	DoingPreSearch = false;
	Hourglass(false);
	
#else	// !PSIMOPEN

	//const int curpos = 0;
	int r = PrePopupSearch( _word, _prevword, curpos, complete, option );
#endif
	if ( r == -1 )
		return 0;

#if 0
	if (!MutexInitialized){
		InitializeCriticalSection(&Mutex);
		MutexInitialized = true;
	}
#endif

	Owner = owner;
	Parent = parent;

	SetSrchWord( _word );
	ReplaceTab(srchWord.c_str());

	tnstr word;
	if ( r && !(Flags & PSF_FORCEMENU) ){
#if USE_HISTORY
		if ( Flags & (PSF_POPUPWINDOW|PSF_QUICKPOPUP) ){
			Flags |= PSF_ADDHIST;
		}
#endif
		if (GetNumHitWords()==0)
			return 0;	// 2014.10.20 追加
		word.set( GetHitWord( GetNumHitWords()-1 )->word );
		Open( (TPopupSearch*)parent, word, TPSConfig::FixedPopup, url );
	} else {
		if (Popup){
			Close();
		}
		if ( Flags & PSF_ONLYHITWORD ) return 0;
		OpenAsMenu( _word );
	}
	return r;
}

// External Search Longest Word
void TPopupSearch::SLWStop()
{
	if (SLWRunning){
		SLWRunning = false;
		MPdic *dic = GetActiveDic();
		if (dic){
			dic->SearchLongestWordCmd(LPSLW_CANCEL, ThreadKey);
		}
	}
}

int TPopupSearch::SLWExtCallback(class TWebSearchThread *, int type, int param, int user)
{
	return ((TPopupSearch*)user)->SLWExtCallback(type, param);
}

int TPopupSearch::SLWExtCallback(int type, int param)
{
	switch (type){
		case LPSLW_COMPLETED:	// external search thread completed
			break;
		case LPSLW_UPDATED:
			// param : updated index of the HitWords.
			_UpdateHitWords(param /*, false*/);
			break;
		default:
			__assert_debug;
			break;
	}
	return 0;
}

// sync==true : requires a synchronization with GUI thread
void TPopupSearch::_UpdateHitWords(int hitword_index /*, bool sync*/)
{
	if (IsOpened()){
		if (IsMenuOpened() || hitword_index==HitWordIndex){
			UpdateNotify(/*sync*/);
		}
	}
}

// sync==true : requires a synchronization with GUI thread
// do not use sync!! because the LockThread() is not fully safe.
void TPopupSearch::UpdateNotify(/*bool sync*/)
{
	HWND hwnd = GetChildHandle();
	if (hwnd){
		if (IsMenuOpened()){
			MenuWin->SetLastAction(MWA_REFRESH);
		} else {
			if (Popup && (type==0||type==HLT_WORD||type==HLT_WORD2)){
				// Update popup window
				Japa *japa = Popup->GetJapa();
				if (japa){
					//if (!sync || LockThread())
					{
						// assert(synchronized with VCL thread)
						UpdateContent();
						//if (sync) UnlockThread();
					}
				}
			}
			PostMessage(hwnd, UM_UPDATENOTIFY, 0, 0);	// update window
		}
	}
}

#if 0
int TPopupSearch::cbUpdateContent(int param)
{
	return ((TPopupSearch*)param)->UpdateContent();
}
#endif

int TPopupSearch::UpdateContent()
{
	Japa *japa = Popup->GetJapa();
	if (japa){
		ReadData(HitWordIndex, japa);
	}
	return 0;
}

#if 0
CRITICAL_SECTION TPopupSearch::Mutex;
bool TPopupSearch::MutexInitialized = false;
bool TPopupSearch::Locked = false;

//Note: このままではよくない
// 理由1：この機構を使用しない別threadとの同期が取れないため
// 理由2：lockしたthreadでSendMessage()をするとdeadlockする
// 理由3：lockしてもthread unsafe objectをtouchすると例外を発生する場合がある(OLE object)
bool TPopupSearch::LockThread()
{
	if (!TUIMain::GetInstance())
		return false;	// application end?

	HWND hwnd = TUIMain::GetInstance()->GetHWindow();
	if (!hwnd)
		return false;

	EnterCriticalSection(&Mutex);
	PostMessage(hwnd, UM_SYNCHRONIZE, (int)this, (int)cbLockProc);
	clock_t start = clock();
	for (;!Locked;){
		Sleep(10);
		clock_t now = clock();
		if (now-start>10000){
			LeaveCriticalSection(&Mutex);
			return false;
		}
	}
	return true;
}

void TPopupSearch::cbLockProc(int param)
{
	((TPopupSearch*)param)->LockProc();
}

void TPopupSearch::LockProc()
{
	Locked = true;
	EnterCriticalSection(&Mutex);
	LeaveCriticalSection(&Mutex);
	Locked = false;
}

void TPopupSearch::UnlockThread()
{
	LeaveCriticalSection(&Mutex);
}
#endif

//TODO: 汎用的ならstrlibへ
// tab -> space
void ReplaceTab(tchar *str)
{
	// replace tab to space to avoid find_cword_pos() error
	while (*str){
		if (*str=='\t')
			*str = ' ';
		str++;
	}
}

