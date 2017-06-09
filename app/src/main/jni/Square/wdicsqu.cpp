#include "tnlib.h"
#pragma	hdrstop
#include	"multidic.h"
#include	"tid.h"
#include	"winsqu.h"
#include	"japa.h"
#include	"wpdcom.h"
#include	"srchout.h"
#include "autolink.h"
#include "draw4com.h"

#include "WinSquUI.h"
#include "MouseCapture.h"

#include "SrchMed.h"

#include "BookmarkMan.h"

#define	USE_STOP	0	// SrchParamが検索threadとぶつかる場合は1

// 検索用のバッファをリセット
void Squre::Reset( )
{
	if ( IsIncSearch( ) ){
#if !NEWINCSRCH
		NotFoundCount = FoundCount = 0;
#endif
		SearchString.clear();
		EndSearch( );
	}
}

// 返り値：
//	TRUE	まだ追加可能
//	FALSE	これ以上の追加はoffsetが負になるためできない
bool Squre::add( tnstr *word, Japa *j, int dicno )
{
#ifndef SMALL
	if ( srchout ){
		if ( !srchout->Output( *word, *j ) ){
			// 中止
			delete word;	// 1997.2.15
			delete j;
			return false;
		}
		if ( !bAllowDisp ){
			delete word;	// 1997.2.15
			delete j;
			return true;
		}
	}
#endif
	bool r = true;
	if ( get_num() == MAX_NDISP ){
#if 0	// 2008.2.1 外した
//#ifdef USE_OLE
		// 未使用オブジェクトの解放
		Dic.FreeObjects( );
#endif
		pool.Del( 0 );
		memmove( &LinesList[0], &LinesList[1], sizeof( int ) * ( MAX_NDISP - 1 ) );
		ss.SetRemain( F_BACKWARD );	// 後方検索必要
		BaseNum++;
		if ( --IndexOffset <= 0 ){
			IndexOffset = 0;
			BaseNum--;
			r = false;
		}
	}
	pool.AddDirect( word, j, dicno );
#if 0
	if ( AutoLinkConfig.fEnable ){
		if ( SearchAutoLink( pool.get_num()-1 ) ){
			// Invalidate();
		}
	}
#endif
	UpdateProc( get_num() - 1 );
	return r;
}

// visible :
//	境界(i==IndexOffset)に追加時、表示できるように調整する
// return:
//	1  = poolの先頭を破棄した
//	-1 = poolの最後を破棄した
int Squre::insert2( int i, tnstr *word, Japa *j, int dicno, bool visible, int level )
{
	__assert(i>=0&&i<=MAX_NDISP);
	if ( i >= MAX_NDISP ){	// エラーチェック
		return 0;
	}
	int ret = 0;
	int n = get_num();
	if (n>0){
		if (i<=IndexOffset){
			// 表示範囲より上に追加
			if (i==0){
				// 全体をshift
				if (!ForceUpdate){
					if (!visible || i!=IndexOffset)
						IndexOffset++;
				}
				if (n==MAX_NDISP){
					pool.Del(MAX_NDISP-1);
					ss.SetRemain(F_FORWARD);	// 前方検索必要
					ret = -1;
				}
			} else {
				if (n==MAX_NDISP){
					// 前半を前へshiftし、poolから追い出す
					BaseNum++;
					pool.Del(0);
					i--;
					ss.SetRemain(F_BACKWARD);	// 前方検索必要
					ret = 1;
				} else {
					if (!ForceUpdate){
						if (!visible || i!=IndexOffset)
							IndexOffset++;
					}
				}
			}
		} else {
			// 表示範囲以降
			if (n==MAX_NDISP){
				pool.Del(n-1);
				ss.SetRemain(F_FORWARD);	// 前方検索必要
				ret = -1;
			}
		}
	}
	memmove(&LinesList[i+1], &LinesList[i], sizeof(TSquLineInfo) * (MAX_NDISP - i - 1));
	LinesList[i].NumLines = 0;	// invalidate
	pool.InsertDirect( i, word, j, dicno, level );
	return ret;
}

// i（絶対番号)の単語を削除
void Squre::del( int i )
{
#ifdef USE_OLE
	eolist.Delete( GetWordAbs( i ) );
#endif
	pool.Del( i );
	DeleteLine( i );
	if ( !get_num() ){
		Invalidate();
	}
	MouseCap->SetSelItemIndex(-1);	// 削除のときは範囲選択を無効にする

#ifdef USE_OLE
		// 未使用オブジェクトの解放
		if ( !IsSearching( ) )
			Dic.FreeObjects( );
#endif

	if ( i >= IndexOffset && i <= LastIndex+IndexOffset+1 ){
		Invalidate( );
	}
}

int Squre::GetInsertLoc(const tchar *word)
{
	if (get_num()==0)
		return 0;

	int index;
	if (ss.IsSubSearch()){
		// 新規登録はないはず？
		// 2009.10.13 Web辞書検索で呼ばれる場合があったので、
		// __assert()が成立しない場合がある。(index==1,pool.fw[index]!=0だった)
		//TODO: 関連語検索した後、単語登録をメインウィンドウ以外から行うとassertionが発生する
		// →2014.10.22 sub searchのmultithread対応により、random insertionが発生するため _tcscmp()!=0 がありえる
		//   そのためassertionを外した
		index = GetStarLocAbs(word, 0, Dic.GetOrder());
		//__assert(index<=0||index==pool.get_num()||_tcscmp(word, pool.fw[index])==0);
	} else {
		index = pool.BSearchExact(word);
	}
	return index;	// absolute index
}

#if 0
// ssにfword,fjapaが合致するかどうか
BOOL IsMatch( const char *fword, Japa &, SrchStat &ss )
{
	switch ( ss.GetSearchType() ){
		case SST_INCSEARCH:
			return dic.cmpnword( fword, ss.word, _tcslen( ss.word ) ) == 0;
		case SST_SEARCH:
			return FALSE;	// 未完成！！！
	}
	return FALSE;
}
#endif

// Squre::Update()のプレースホルダ
LRESULT Squre::UmUpdate( WPARAM , LPARAM lParam )
{
	Update( (const tchar *)lParam );
	return 0;
}

// 見出語word の変更に対しての処理
// wordがウィンドウ内にあれば、Invalidateする
// ---------大前提------------
// 新規登録はインクリメンタルサーチのみ
// 修正・削除は両検索
void Squre::Update( const tchar *word, int upf )
{
#if 1	// 2008.2.26 新方式（お試し）
	int i1 = GetInsertLoc(word);
#else
	int i1;
	if ( !get_num() ){
		i1 = 0;
	} else {
		i1 = GetStarLocAbs(word, 0, Dic.GetOrder());
		//pool.GetKeepLocC( word, _tcslen( word ), i1, i2 );	// 不正確なのでやめた 2008.2.20
	}
#endif
	tnstr fword;
	Japa fjapa;
	int r;
	switch ( upf ){
	case UPF_NONE:
		{
		NeedReset();
		Dic.initNsearch( word );
		__EnableDDNBegin(CanDispDicName());
		r = Dic.nextAS2( fword, &fjapa, ss.uDicList );
		__EnableDDNEnd();
		}
		break;
	case UPF_DELETE:
		goto jmp1;
	}
	switch ( r ){
		case AS_FOUND:
			// 見つかった
			BM_JSetMark(find_cword_pos(fword), fjapa);
			fUpdateDisp = true;
			if ( !_tcscmp( fword, word ) ){
				// 同じ単語
#ifdef USE_TREE	// Update()関数を呼ぶと必ずOpenするため、Openしたくない場合は独自に行う必要がある
				if ( treemode ){
					HTREESQU(this)->nodestate.SetFlag( fword, NS_OPEN );
				}
#endif
				if ( i1>=0 && i1 < get_num() && _tcscmp( pool.fw[i1], word ) == 0 ){
					// poolの中にあればupdateとみなす
					pool.SetJapa(i1, fjapa);
					pool.SetFDN(i1, Dic.GetLastFindDicNo());
#if USE_DT2
					MouseCap->SetSelItemIndex(-1);	// 変更した場合は範囲選択を無効に
					pool.UpdateHyperLink( i1 );
#endif
#if 0
					if ( AutoLinkConfig.fEnable ){
						SearchAutoLink( i1 );
					}
#endif
					Invalidate( );
					LinesList[i1].NumLines = 0;	// 4.11 1999.5.30 追加
				} else {
					// poolの中にないときは新規追加
					// ここにくるのはインクリメンタルサーチのときだけしか来ない！！
					// という大前提
					// 従って、必ずマッチするはず
#if 0
					if ( i1 == i2 && ( i1 == 0 || i1 == get_num() ) ){
						// 検索条件に一致するかどうか調べる
						if ( !IsMatch( fword, fjapa, ss ) )
							goto jmp2;
					}
#endif
					if (i1>=0){
						insert2( i1, new tnstr( fword ), new Japa( fjapa ), Dic.GetLastFindDicNo(), true );
						UpdateProc( i1 );
						if (UIMain){
							if (!_tcscmp(UIMain->GetWordText(), find_cword_pos(fword))){
								// change cur location.
								cury = i1-IndexOffset;
								Invalidate();
							}
						}
					}
#if 0
jmp2:;
#endif
				}
			} else {
jmp1:
				// 違う単語
				// 同じ単語かどうかの確認
				if ( i1 < get_num() && _tcscmp( pool.fw[i1], word ) == 0 ){
					// 単語を削除したと考える
					del( i1 );
					if ( cury != -1 ){
						if ( cury >= get_num() - IndexOffset )
							cury = get_num() - IndexOffset - 1;
						if ( get_num() ){
							if ( get_num() == IndexOffset ){
								// 現在の窓には無くなった
								IndexOffset--;
								cury = 0;
								Invalidate( );
							} else {
								if (UIMain) UIMain->SetWordText( GetWord( cury ) );
							}
						} else {
							Invalidate( );
						}
					}
				} else {
					// 辞書にも無くて、poolにもない場合（何もしなくて良い？？）
				}
			}
			fUpdateDisp = false;
			break;
		case 0:
		case AS_END:
			// 見つからない
			goto jmp1;
		case AS_ERROR:
			return;
	}
}

// iは絶対インデックス番号
void Squre::UpdateProc( int i )
{
	if ( fUpdateDisp ){
		if ( i >= IndexOffset && i <= LastIndex+IndexOffset+1 ){
			Invalidate();
			LinesList[i].NumLines = 0;	// 4.11 1999.5.30 追加
			SetVRange2( fUpdateDisp );
			return;
		}
	}
	// 1999.5.3 以下の２行を追加
	// ある検索を表示してから、別の検索を始めたときに
	// 前の検索の結果がそのまま利用されてしまうのを防ぐため
	fRecalcLine = true;
	LinesList[i].NumLines = 0;
	if ( bAllowDisp && i >= IndexOffset && i <= LastIndex+IndexOffset+1 ){
		GetDC( );
		CreateTextFonts( );
		DispOneLine( i-IndexOffset, DF_DISP );
		DeleteTextFonts( );
		ReleaseDC( );
	}
#if USE_VIEWAUTOPOP
	if (ViewAutoPop && IsIncSearch()){
		// incremental searchのみ
		tnstr word;
		if (UIMain){
			word = UIMain->GetEdit()->GetText();
		}
		if (!_tcscmp(word,GetWordAbs(i))){
			OpenAutoPop( i );
		}
	}
#endif	// USE_VIEWAUTOPOP
	SetVRange2( fUpdateDisp );
}

// 検索リセット＆再開準備
void Squre::ResetSearch( )
{
	switch ( ss.GetSearchType( ) ){
		case SST_INCSEARCH:
#if USE_STOP
			SrchParam->Stop(true);
#endif
#if NEWINCSRCH
			Dic.initNsearch2( ss.word, SrchParam );
#else
			Dic.initNsearch( ss.word, SrchParam );
#endif
			Dic.jumpAS( ss.lastfound, ss.fBackWard, false, SrchParam );
			break;
		case SST_SEARCH:
		case SST_SEARCH|SST_SUBSEARCH:
#if USE_STOP
			SrchParam->Stop(true);
#endif
			Dic.setAS( ss.word, ss.mode, ss.grexp, SrchParam );
			Dic.jumpAS( ss.lastfound, ss.fBackWard, false, SrchParam );
			break;
	}
	ss.ClearNeedReset();
}

// word:
//	NULL : 現在の検索状態からの継続(現在のところ、sub_wordsからの検索のみ対応）
bool Squre::ReSearch( const tchar *word, bool fBack, int fcnum )
{
	if (fcnum<0)
		fcnum = 0;
	if ( ss.GetSearchState() & SS_NEEDRESET ){
		ResetSearch( );
	}
	switch ( ss.GetSearchType() & (SST_INCSEARCH|SST_SEARCH) ){
	case SST_INCSEARCH:
		if ( IsSearching() && ( ss.fBackWard == fBack ) ){
			// 再検索中の再設定
			// 実際の検索はSearchSetProc()のメッセージループ内にいることに注意
//			DBW("1:ReqAdd:%d", fcnum);
			ss.ReqAdd(fcnum);
		} else {

			// 再検索の処理
			if (word){
#if USE_STOP
				SrchParam->Stop(true);
#endif
#if NEWINCSRCH
				Dic.initNsearch2( word, SrchParam );	// 2007.10.29 added
#else
				Dic.initNsearch( word, SrchParam );
#endif
				Dic.jumpAS( word, fBack, false, SrchParam );
			}

//			DBW("2:ReqAdd:%d", fcnum);
			ss.ReqAdd(fcnum);
			ss.StartSearch( fBack );
			UIMain->message(MSG_SEARCHING);
		}

		break;
	case SST_SEARCH:
//	case SST_SEARCH|SST_SUBSEARCH:
		// fcnum : バッファが一杯になっても強制的に検索を続行する語数
		// 再入可能で、再入した場合前回ReSearchしたものとまとめる。
		// もし、まとめることができない場合は無視される(fcnumがMAX_NDISPを超える場合など
		// 再入で検索方向が変わる場合は、前回までの検索はキャンセルされる
		// 当然、検索種類が異なる（前回がインクリメンタルサーチ）場合は、無視される。
		// もし、fcnumが溜まってきたときは強制的に検索
		if ( IsSearching() && ( ss.fBackWard == fBack ) ){
			// 再検索中の再設定
			// 実際の検索はSearchSetProc()のメッセージループ内にいることに注意
			ss.ReqAdd(fcnum);
		} else {

			if (!SrchMed->SetupNextSearch(fBack, false)){
				// 通常の検索
				// 再検索の処理
				if (word){
#if USE_STOP
					SrchParam->Stop(true);
#endif
					Dic.setAS( ss.word, ss.mode, ss.grexp, SrchParam );	// 2007.10.29 added
					Dic.jumpAS( word, fBack, false, SrchParam );
				}
			}

			ss.ReqAdd(fcnum);
			ss.StartSearch( fBack );
			UIMain->message(MSG_SEARCHING);
		}

		break;
	}
	return true;
}
//#if !NEWINCSRCH
// i1 から i2-1 を残して後はカット
void Squre::Keep( int i1, int i2 )
{
	int n = pool.get_num();
	pool.Keep( i1, i2 );
	memmove( &LinesList[0], &LinesList[i1], sizeof( int ) * ( i2 - i1 + 1 ) );
	if ( i1 != 0 ){
		ss.SetNothing( F_BACKWARD );
	}
	if ( i2 != n ){
		ss.SetNothing( F_FORWARD );
	}
}
//#endif

void Squre::StartIncSearch( )
{
	if (UIMain) UIMain->message(MSG_SEARCHING);
	ss.StartIncSearch( );
}




