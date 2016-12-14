#include "pdclass.h"
#pragma hdrstop
#include "autolink.h"
#include "winsqu.h"
#include "windic.h"
#include "LangProc.h"
#include "psconfig.h"
#include "assexec.h"
#include "depw.h"
#include "tid.h"

#include "WinSquUI.h"

#include "PopupSrch.h"
#include "PopupConfig.h"

#include "MouseCapture.h"

#define	DCAST_TWINDOW(p)	((TWinControl*)p)

bool AutoLinkCtrlKey = false;	// Ctrl+MouseMoveでautolink

// EPWingのAutoLinkSearchは、マウス位置情報をDEPWに渡していないため、
// 非常に時間がかかる
// そのため現在のところ未対応

#if 0	//*++ EPWingは未対応(AutoLink)
#ifndef SMALL
extern "C" {
int WINAPI _export SearchAutoLinkText( const char *str, const char *prevstr, THyperLinks *hls, bool SingleByte )
{
#if !NEWAUTOLINK
	if ( !(AutoLinkConfig.Item & HLI_EPWING) ) return 0;
#endif
	return SearchAutoLink( str, *hls, HypLinkCurrentItem, SingleByte );
}
}
#endif

#endif	// 0
#if 0	//*++ EPWingは未対応(AutoLink)
#ifdef _UNICODE
#define	_IsDBCSLeadByte(x)	false
#else
#define	_IsDBCSLeadByte(x)	IsDBCSLeadByte(x)
#endif
//---------------------------------------------------------------------------
// １文字列分の自動リンク検索を行う //
// hlsへは追加で処理
int SearchAutoLink( const tchar *str, THyperLinks &hls, int item, bool SingleByte, int maxnum )
{
	int option = PopupConfig.GetOption() | SLW_ELIMHYPHEN;

	bool fLinkAttr = dic.SetLinkAttr( OLA_NOTREADOBJ & ~OLA_NOTREADEPWING, true );	// 高速化のため

	const tchar *_str = str;
	int n = 0;
	MatchArray found;
	const tchar *foundstr;
	// 単語きり出し＆検索 //
	int r;
	bool multi;
	while ( *str ){
		if ( !PopupConfig.fMultiByte || SingleByte ){
			// 日本語フォントで日本語文字は除外
			multi = _IsDBCSLeadByte( *str );
			if ( !SingleByte && multi ){
				str = CharNext( str );
				continue;
			}
			if ( !IsWordChar( *str ) ){
				goto next1;
			}
		} else {
			while ( *str && !_IsDBCSLeadByte( *str ) && !IsWordChar( *str ) ) str = CharNext( str );
			if ( !*str ) break;
			multi = _IsDBCSLeadByte( *str );
			if ( multi ){
				// 日本語検索 //
				// 必ず日本語フォントを使用している
				if ( (r=mbSearchLongestWordOptional( dic, str, 0, &found )) != 0 ){
					foundstr = found[found.get_num()-1].word;
					THyperLink *hl = new THyperLink;
					hl->type = HLT_WORD;
					hl->item = item;
					hl->key = foundstr;
					hl->loc = STR_DIFF(str,_str);
					hl->length = r;
					hls.add( hl );
					str += r;
					n++;
					if ( hls.get_num() >= maxnum )
						goto jend;
				} else {
					str = CharNext( str );
				}
				continue;
			} else goto srch_ank;
		}
	srch_ank:;
		if ( (r=SearchLongestWordOptional( dic, str, NULL, option, &found )) != 0 ){
			// 日本語１バイトチェック
			foundstr = found[found.get_num()-1].word;
			if ( !SingleByte && r <= 2 && _IsDBCSLeadByte( *str ) && (_tcslen(foundstr) <= 1) ){
				goto next;
			}
			THyperLink *hl = new THyperLink;
			hl->type = HLT_WORD;
			hl->item = item;
			hl->key = foundstr;
			hl->loc = STR_DIFF(str,_str);
			hl->length = r;
			hls.add( hl );
			str += r;
			n++;
			if ( hls.get_num() >= maxnum )
				goto jend;
			if ( PopupConfig.fMultiByte && !SingleByte ) goto jjj1;
		} else {
	next:
			if ( PopupConfig.fMultiByte && !SingleByte ){
	jjj1:;
				while ( !_IsDBCSLeadByte( *str ) && IsWordChar( *str ) ) str = CharNext(str);
				continue;
			} else {
				while ( IsWordChar( *str ) ) SingleByte ? str++ : str = CharNext(str);
			}
		}
	next1:
		while ( *str && !IsWordChar( *str ) ) SingleByte ? str++ : str = CharNext(str);
	}
jend:;
	dic.SetLinkAttr( OLA_NOTREADOBJ, fLinkAttr );

	return n;
}
#endif	// 0

int Squre::ResearchAutoLinkUpdate( int relno )
{
	return 0;
}
void Squre::DrawUnderline( int item, int start0, int end0, int start, int end, int hitno )
{
	if (hitno+IndexOffset>=get_num())
		return;
#if NEWHYPLINK
	// 自動リンク検索以外はunderlineを表示しない
	if ( MouseCap->al.GetHypIndex() != -1 ) return;
#endif
	//** この表示にすごく時間がかかっている・・・
	CharHT cht( item );
	cht.start0 = start0;
	cht.end0 = end0;
	cht.start = start;
	cht.end = end;
	int saved = GetDefReverseSize();
	SetDefReverseSize(2);
	DispOneLine( hitno, DF_DISP | DF_PINPOINT /* | DF_UNSELECTED */ | DF_UNREDRAW | DF_ONLYUNDERLINE, &cht );
	SetDefReverseSize(saved);
}
THyperLink *Squre::GetHyperLink(int wordIndex, int itemIndex)
{
	if (wordIndex+IndexOffset>=pool.get_num())
		return NULL;
	THyperLinks &hls = pool.GetHL(wordIndex+IndexOffset);
	if (itemIndex>=hls.get_num())
		return NULL;
	return &hls[itemIndex];
}

THyperLink &Squre::_GetHyperLink(int wordIndex, int itemIndex)
{
	return pool.GetHL(wordIndex+IndexOffset)[itemIndex];
}
// index:	relative index
// pt:		screen system
int Squre::HitTestHyperLink(int index, POINT pt)
{
	if (!IsValidHitno(index))
		return -1;
	POINT _pt = pt;
	_pt.y -= GetOffsY(index) - IndexMiOffset;
	THyperLinks &hls = pool.GetHL(index+IndexOffset);
#if 0	// 正順
	for ( int i=0;i<hls.get_num();i++ )
#else	// 逆順 <bx>タグの多重展開対応のため逆順(ParseHtmlHitPosition()注釈参照)
		// TJapaFrame::HitTestHL()も変更必要
	for ( int i=hls.get_num()-1;i>=0;i-- )
#endif
	{
		THyperLink &hl = hls[i];
		if ( hl.HitTest( _pt ) ){
			//dbw("HitTestHL: %d", i);
			return i;
		}
	}
	return -1;
}

tnstr Squre::GetHyperLinkText(int index, POINT pt)
{
	if (index<0){
		int ht = HitTest( pt );
		if ((ht & HT_MASK) == HT_WORDITEM){
			index = ht & HT_MASK_INDEX;
		}
	}
	tnstr key;
	if ( index >= 0 ){
		GetDC();
		CreateTextFonts();
		CharHT cht( -1, &pt );
		if ( CharHitTest( index, cht, DF_WHOLECHAR ) ){
			int hl_index = HitTestHyperLink(index, pt);
			if (hl_index>=0){
				// HyperLinkヒット！！
				THyperLink &hl = pool.GetHL(index+IndexOffset)[hl_index];
				hl.GetKeyWord( key, pool.GetText( IndexOffset + index, hl.item ) );
			}
		}
		DeleteTextFonts();
		ReleaseDC();
	}
	return key;
}

#if 0
bool Squre::ReverseAutoLink(POINT *pt==NULL)
{
	CloseAutoLink();
	CloseAutoLinkPopup();
}
#endif

#define	HLPL	1	//TODO: poup&linkとhyperlinkを両立する(debugが完了したら削除)

// autolink : popup&linkをするか？
bool Squre::ReverseAutoLink( POINT *pt, bool popup, bool autolink )
{
	__assert(pt);
	int r;
	int ht = HitTest( *pt );
	int hitno = ht & HT_MASK_INDEX;
	if ( (ht & HT_MASK) == HT_WORDITEM ){
		GetDC();
		CreateTextFonts();
		CharHT cht( -1, pt );
		if ( CharHitTest( hitno, cht, DF_WHOLECHAR ) ){
			int hl_index = HitTestHyperLink(hitno, *pt);
			if (hl_index>=0){
				// HyperLinkヒット！！
				TAutoLink &al = MouseCap->al;
				THyperLink &hl = pool.GetHL(hitno+IndexOffset)[hl_index];
				//dbw("hitno=%d, cht.item=%d, hl_index=%d, hl.loc=%d, hl.loc+hl.length=%d %d %d", hitno, cht.item, hl_index, hl.loc, hl.loc+hl.length, al.GetItemIndex(), al.CompIndex(hitno, cht.item, -2, hl.loc));
				if ( al.GetItemIndex() != -1){
					if ( al.CompIndex(hitno, cht.item, -2, hl.loc) &&
						((hl.item < HLI_EPWING) || (hl.wordcount == al.GetWordCount())) ){
						if (!hl.bxtag){
							DeleteTextFonts();
							ReleaseDC();
						}
						if (
#if !VIEWCLICKONLY
							HyperClickPopup &&
#endif
							popup && !IsAutoLinkPopupOpened() ){
							StartHypPopup( hl, *pt, false );
						}
						if (hl.bxtag){
							goto jpopuplink;
						} else {
							return popup;
						}
					}
					CloseAutoLink();
				}
				al.SetIndex(hitno, cht.item, hl_index, hl.loc, hl.loc+hl.length);
				al.SetWordCount(hl.wordcount);

				CloseAutoLinkPopup();

#if !NEWHYPLINK
				if ( al.GetItemIndex() < HLI_EPWING ){
					al.DrawUnderline( );
				}
#endif
#if !HLPL
				DeleteTextFonts();
				ReleaseDC();
#endif

				if ( popup ){
#if HLPL
					DeleteTextFonts();
					ReleaseDC();
#endif
					StartHypPopup( hl, *pt,
#if !VIEWCLICKONLY
						!HyperClickPopup
#else
						false
#endif
						);
					return true;
				} else {
					if (!hl.bxtag){
#if HLPL
						DeleteTextFonts();
						ReleaseDC();
#endif
						return false;
					}
				}
			}
jpopuplink:
#ifndef SMALL
			// マウスの下にある単語を検索する //
			// 単語の取得
#ifndef _UNICODE
			int _item = cht.item >= SN_OBJECT ? SN_OBJECT : cht.item;
#endif
			if ( autolink ){
				const tchar *buf;
				switch ( cht.item ){
					case SN_JAPA:
#if !NEWAUTOLINK
						if ( !AutoLinkConfig.fJapa )
							goto jend;
#endif
						buf = pool.GetJapa(hitno+IndexOffset);
						break;
					case SN_EXP1:
#if !NEWAUTOLINK
						if ( !AutoLinkConfig.fExp )
							goto jend;
#endif
						buf = pool.GetExp(hitno+IndexOffset);
						break;
					case SN_PRON:
#if !NEWAUTOLINK
						if ( !AutoLinkConfig.fPron )
							goto jend;
#endif
						buf = pool.GetPron(hitno+IndexOffset);
						break;
					default:
#if !NEWAUTOLINK
						if ( !AutoLinkConfig.fEPwing )
							goto jend;
#endif
#ifdef USE_JLINK
						if ( cht.item >= SN_OBJECT ){
							JLink &jl = pool.GetJLinks(hitno+IndexOffset)[cht.item-SN_OBJECT];
							if ( jl.GetType() == JL_EPWING ){
#if 0	//*++ EPWingは未対応(AutoLink)
								depwif.fAutoLinkSearch = true;
								THyperLinks *hls = new THyperLinks;
								pool.fj[hitno+IndexOffset].jlinks.SearchHyperLink( *hls );
								depwif.fAutoLinkSearch = false;
								delete hls;
#else
								//**
								goto jend;
//									break;
#endif
							}
						}
#endif
						goto jend;
				}
				int start, end, prevstart;
#ifndef _UNICODE
				if ( fSingleByte[ _item ] ){
					r = Dic.GetLangProc()->GetWord( buf, cht.pos, start, end, prevstart, true );	//** fLongest or not
				} else
#endif
				{
					r = Dic.GetLangProc()->mbGetWord( buf, cht.pos, start, end, true );	//** fLongest or not
					prevstart = start;
				}
				if ( r )
				{
					TAutoLink &al = MouseCap->al;
					THyperLink &alHyperLink = al.GetHyperLink();
					if ( !al.CompIndex(hitno, cht.item, -1) ||
						 alHyperLink.type != HLT_WORD ||
						 alHyperLink.wordcount != cht.pos ){

						//TODO: CloseAllPopup()を呼び出した方がいいのでは？
						CloseAutoLinkPopup();
						if ( ps ){
							ps->Free(0);
							ps = NULL;
						}

						ps = new TPopupSearch;
						ps->SetNoFixSizeMove();
						tnstr key;
						key.set( buf + prevstart, end-prevstart );
						int len = ps->PrePopupSearch((const tchar *)key+(start-prevstart), key, cht.pos-prevstart, false, SLW_ELIMHYPHEN | PopupConfig.GetOption() );
						if ( len > 0 ){
							const MATCHINFO &mi = *ps->GetHitWord(ps->GetNumHitWords()-1);
							start += mi.start;

							if (!al.CompIndex(hitno, cht.item, -1, start)){

								if ( al.IsIndexValid() ){
									// erase previous underline.
									al.DrawUnderline();
									al.InvalidateIndex();
								}

								alHyperLink.key.set( mi.word );
								al.SetIndex(hitno, cht.item, -1, start, start+len);
									// alIndex=-1 自動ポップアップ検索であることを示す
								alHyperLink.type = HLT_WORD;
								alHyperLink.wordcount = cht.pos;

								// アンダーラインの表示
							
								al.DrawUnderline();
								if ( popup ){
									StartHypPopup( alHyperLink, *pt,
#if !VIEWCLICKONLY
										!HyperClickPopup
#else
										false
#endif
										);
								}
							}
						} else {
							// 該当なし
#if HLPL
							if (hl_index<0)
#endif
							{
								// hyper linkがないときのみ
								if ( al.IsIndexValid() ){
									// erase previous underline.
									al.DrawUnderline();
									al.InvalidateIndex();
								}
							}
						}
					}
					DeleteTextFonts();
					ReleaseDC();
					return true;
				}
			}
#endif
		}
	jend:;
		DeleteTextFonts();
		ReleaseDC();
	}
	if ( MouseCap->al.GetItemIndex() != -1 ){
		CloseAutoLink();
	}
	return false;
}

// noは相対番号
void Squre::CloseAutoLink( )
{
	TAutoLink &al = MouseCap->al;
	if ( al.IsIndexValid() ){
		CloseAutoLinkPopup();
		if ( al.GetItemIndex() < HLI_EPWING ){
			al.DrawUnderline();
		}
		al.InvalidateIndex();
	}
}

void Squre::StartHypPopup( THyperLink &hl, POINT &pt, bool delayed )
{
	MouseCap->al.StartHypPopup(hl, pt);
}

bool Squre::JumpWordLink(bool test)
{
	if (cury<0)
		return false;

	// <→xxx> <word:xxx> jump
	THyperLinks &hls = pool.GetHL(cury+IndexOffset);
	for (int i=0;i<hls.size();i++){
		THyperLink &hl = hls[i];
		if (hl.type==HLT_WORD || hl.type==HLT_WORD2){
			if (!test){
				POINT pt = {0,0};
				HypLinkJump(hl, pt, cury);
			}
			return true;
		}
	}
	return false;
}

// mouseによる、LinkJump
// poolindex : relative index
void Squre::HypLinkJump( THyperLink &hl, POINT &pt, int poolindex )
{
	tnstr key;
	switch ( hl.type ){
		case HLT_NONE:
			return;
		case HLT_EPWING:
			// EPWINGのリンクはポップアップのみ！
			StartHypPopup( hl, pt, false );
			break;
		case HLT_WORD:
		case HLT_WORD2:
			// WORDへ転送する
			hl.GetKeyWord( key, pool.GetText( IndexOffset + poolindex, hl.item ) );
			LinkJump( key );
			break;
		case HLT_HTML_BXTAG:
			// nothing to do
			break;
		default:
			hl.GetKeyWord( key, pool.GetText( IndexOffset + poolindex, hl.item ) );
			if (!AssociateExecuteEx( GetMainWindow()->GetHWindow(), key, hl.type, true, NULL )){
				SetMessage(_LT(MSG_OpenError));
				//txr: MSG_OpenError=Open error.
			}
			break;
	}
}

#if USE_DT2
// index : relative index
// pt : screen system
bool Squre::HypLinkJumpTest( int index, POINT pt )
{
	int hl_index = HitTestHyperLink(index, pt);
	if (hl_index>=0){
		// HyperLinkヒット！！
		KillTimer( TM_ALTIMER );
		THyperLinks &hls = pool.GetHL(index+IndexOffset);
		HypLinkJump( hls[hl_index], pt, index );
		return true;
	}
	return false;
}
#endif	// USE_DT2

bool Squre::StartAutoLink()
{
	if ( IsAutoLinkPopupOpened() )
		return false;	// already opened.
	SetTimer( TM_ALTIMERMOVE, 10, NULL );
	return true;
}
void Squre::OpenAutoLinkPopup( TWinControl *Parent, POINT _pt, int type, const tchar *word, THyperLink &hl )
{
	if ( type == -1 )
		return;

	_pt.x += 4;
	_pt.y += 4;
		
	ps->SetMaxWordNum(TPSConfig::MaxNumWords);
//	ps->MainWindow = GetMainWindow();

	ps->SetSrchWord( word );
	ps->SetType(type);
#ifdef EPWING
	ps->SetEPWingParams(hl.dic, hl.bookno, hl.datapos);
#endif
	//ps->SetParent(Parent);
	ps->SetOwner(Parent);
	ps->SetReceiver(DCAST_TWINDOW(GetView()));
	ps->SetFlags(/*PSF_NOMAPPING |*/
#if !VIEWCLICKONLY
		(HyperClickPopup ? PSF_CLICKPOPUP : 0)
#else
		PSF_CLICKPOPUP
#endif
		);
	ps->SetViewFlags(PopupConfig.GetViewFlags());
	ps->SetOrgPt(_pt);

	// ヒット単語一覧で適切な一覧が出るのか？
	ps->Open( NULL, word, TPSConfig::FixedPopup );
}
#if 0
// wordを検索してから表示する
void Squre::OpenAutoLinkPopup2( TWinControl *Parent, POINT _pt, int type, const tchar *word, THyperLink &hl )
{
	if ( type == -1 )
		return;

	ps->MaxWordNum = MAXWORDNUM;
	ps->MainWindow = GetMainWindow();

	ps->type = type;
	ps->epdic = hl.dic;
	ps->bookno = hl.bookno;
	ps->pos = hl.datapos;
	ps->Parent = Parent;
	ps->Receiver = this;
	ps->Flags = PopupConfig.GetFlags() | PSF_NOMAPPING |
#if !VIEWCLICKONLY
		(HyperClickPopup ? PSF_CLICKPOPUP : 0
#else
		PSF_CLICKPOPUP
#endif
		);
	ps->orgpt = _pt;

	ps->Open( word );
}
#endif

void Squre::CloseAutoLinkPopup()
{
	if ( TPSConfig::FixedPopup )
		return;
	if ( ps ){
		ps->Close();
	}
}

bool Squre::IsAutoLinkPopupOpened()
{
	return ps && ps->IsOpened();
}

void Squre::CloseAllPopup( )
{
	KillTimer( TM_ALTIMERMOVE );
	KillTimer( TM_ALTIMERMOVE2 );
	KillTimer( TM_ALTIMER );
	KillTimer( TM_ALTIMERCLICK );
	CloseAutoLink();
	if ( ps ){
		ps->Free(0);
		ps = NULL;
	}
}

void Squre::AddHistory( const tchar *orgword, const tchar *jmpword )
{
	while ( WordHistory.get_num() > CurHist+1 ){
		WordHistory.del( WordHistory.get_num() - 1 );
	}
	if ( WordHistory.get_num() == 0 || _tcscmp( WordHistory[CurHist], orgword ) ){
		WordHistory.add( orgword );
	}
	WordHistory.add( jmpword );
	CurHist = WordHistory.get_num()-1;
}
void Squre::MoveHistory( bool forward )
{
	if ( forward ){
		if ( CurHist+1 >= WordHistory.get_num() ){
			// no history
			JumpWordLink();
			return;
		}
		GetMainWindow()->SetWordText( WordHistory[++CurHist], true );
	} else {
		if (ss.IsSubSearch()){
			// Sub Search back.
			incsearch(NULL);
		}
		if ( CurHist == 0 ){
			return;
		}
		GetMainWindow()->SetWordText( WordHistory[--CurHist], true );
	}
}
bool Squre::CanMoveHistory( bool forward )
{
	if ( forward ){
		if ( CurHist+1 >= WordHistory.get_num() ){
			return JumpWordLink(true);
		}
	} else {
		if (ss.IsSubSearch()){
			// the main search is sub search mode.
			return true;
		}
		if ( CurHist == 0 ){
			return false;
		}
	}
	return true;
}

bool Squre::BxTagExists(int relIndex)
{
	if (!IsValidHitno(relIndex))
		return false;
	THyperLinks &hls = pool.GetHL(relIndex+IndexOffset);
	for (int i=0;i<hls.size();i++){
		if (hls[i].bxtag) return true;
	}
	return false;
}

#include "filestr.h"

// FindFileLinkPathのfull版
// iに見つかった辞書番号
// CommandPath/CurrentDirの場合はi=-1
bool FindFileLinkPath(MPdic *dic, tnstr &fullpath, const tchar *filename, int &i)
{
	__assert(dic);
	tnstr dicpath;
	tnstr dicflink;
	if (dic){
		i = dic->SearchFileLinkPath(filename, FileLinkPath, fullpath);
		if (i!=-1){
			return true;
		}
	}
	i = -1;
	return FindFileLinkPath(fullpath, filename, NULL, NULL, NULL, true);
}

// fullpathのfilenameを設定されているfilelink pathからの相対パスに変換する
// 相対パスにできない場合は、relpathにfilenameそのものが入る
bool GetRelFileLinkPath(Pdic *dic, tnstr &relpath, const tchar *filename)
{
	if (dic){
		if (dic->GetRelFileLinkPath(filename, FileLinkPath, relpath))
			return true;
	}
	return GetRelFileLinkPath(relpath, filename, NULL, NULL, NULL);
}

