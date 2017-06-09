#include "pdclass.h"
#pragma	hdrstop
#include "pdconfig.h"
#include "LangProc.h"
#include "multidic.h"
#include "id.h"
#include "winsqu.h"
#include "winmsg.h"
#include "wordcount.h"
#include "LangProc.h"
#include "WinSquUI.h"
#include "PopupConfig.h"
#include "BookmarkMan.h"

#ifdef _Windows
#define	USE_GS	1		// uses google suggest
#else
#define	USE_GS	0		// uses google suggest
#endif

#if USE_GS
#include "GoogleSuggest.h"
#include "UserMsg.h"
#endif

#ifdef __ANDROID__
#define	USE_INVALID		0	// 再描画せずにdispStar()で代用する	//TODO: Windows版もこちらのほうがいいかも？ちらつきが減るはず？
#else
#define	USE_INVALID		1	// 再描画で選択状態を変更する
#endif

//#ifdef _DEBUG
#if 0
#define	D	DBW
#else
#define	D	(void)
#endif

#define	USE_STOP	0	// SrchParamが検索threadとぶつかる場合は1

//	FoundCount : 見つかったときの検索文字列の長さ
//

static diclist_t gDicList = ~0;

bool KeepStarTop = true;	// 常にcursor位置は固定（ユーザー目線の移動量を最小化するため）
// 大文字・小文字が同一視されたため、必ずしも１行目にできず、結局ちらつく
// 条件が絞り込まれてくれば現状の方がちらつかず、見やすい。

void Squre::MoveStar( const tchar *str )
{
	if ( !get_num() )
		return;
	int y = GetStarLocAbs( str, 0, Dic.GetOrder() );
	if ( y >= 0 )
		MoveStarLocAbs( y );
}

// inxへ * を移動する
// offsetなどは自動計算
// !INCSRCH_NARROWの場合は常にcury=0となるように移動させる
void Squre::MoveStarLocAbs( int inx )
{
	IndexMiOffset = 0;
	if ( inx < IndexOffset || ( inx - IndexOffset ) > LastIndex ){
		// 表示されていない部分への移動
		cury = 0;
		IndexOffset = inx;
		AdjustOffset( );
#if USE_INVALID
		Invalidate();
#else
		dispStar(0);
#endif
	} else {
		// 表示されている部分への移動
		dispStar( inx - IndexOffset );
	}
}
void Squre::SetIndexOffset( int inx )
{
	if (inx>=0 && inx<pool.get_num()){
		IndexOffset = inx;
	}
}

#if NEWINCSRCH
//返り値：
//-1:前回と同じ、検索しない
//0:見つからない(clearされない)Not find.
//1:表示単語有り
int Squre::incsearch( const tchar *str, diclist_t diclist )
{
	if (!IsDicOpenedStable())
		return -1;

	StopISPTimer();

	bool diff_str = false;
	if (str){
		int l = _tcslen(str);
		if ( !l ){
			return -1;
		}
		diff_str = _tcscmp(SearchString, str);
		if (diff_str){
			ClearSubSearch();
		}
		SearchString.set( str );
	} else {
		// clear sub search and retry inc. search.
		//__assert(sub_srchword.exist());	// 2014.10.20 全文検索ではsub_srchword==""なので外した
		if (sub_srchword.empty())
			return -1;	// nothing to do
		str = sub_srchword;
		ClearSubSearch();
	}

	GetDC();
	CreateTextFonts( );

	tnstr srch_str(Dic.create_composit_word(str));
	__kstr _srch_kstr(srch_str, Dic.GetKCodeTrans());
	_kchar *srch_kstr = _srch_kstr;

	// すでに他の検索中である場合は、それを中止する
	if ( !ss.IsIncSearch() ){
		if ( IsSearching() ){
			StopSearch( );
		}
	} else {
		if (diff_str){
			// 検索文字列が異なる場合、sub searchを止める
			_StopSubSearch();
		}
		if ( pool.get_num() == 0 ){
			// poolには無い
		} else {
			bool keep = false;
			bool searching = false;
			tnstr fword;
			if ( Dic.cmpword( srch_kstr, _kwstr(pool.fw[0],Dic.GetKCodeTrans()) ) < 0 ){
				// poolの先頭が辞書の先頭か？
				searching = ss.IsSearching();
				if ( searching ) StopSearch();
				if ( !Dic.CheckPrevWord( pool.fw[0], &fword ) ){
					keep = true;	// 辞書の先頭
					if ( searching ){
						// 検索続行(backward)
						Request( 0, true );
					}
				} else {
					// 辞書の途中
					if ( Dic.cmpword( srch_kstr, _kwstr(fword,Dic.GetKCodeTrans()) ) > 0 ){
						// だが、pool.fw[0]の一つ前の単語は
						// strより前
						keep = true;
						if ( searching ){
							// 検索続行(forward)
							Request(pool.get_num(), false);
						}
					}
				}
			}
			else
			if ( Dic.cmpword( srch_kstr, _kwstr(pool.fw[pool.get_num()-1],Dic.GetKCodeTrans()) ) > 0 ){
				// poolの最後が辞書の最後？
				searching = ss.IsSearching();
				if ( searching ) StopSearch();
				if ( !Dic.CheckNextWord( pool.fw[pool.get_num()-1], &fword ) ){
					keep = true;	// 辞書の最後
					if ( searching ){
						// 検索続行(forward)
						Request( pool.get_num(), false );
					}
				} else {
					// 辞書の途中
					if ( Dic.cmpword( srch_kstr, _kwstr(fword,Dic.GetKCodeTrans()) ) < 0 ){
						// だが、pool.fw[pool.get_num()-1]の一つ後の単語は
						// strより後ろ
						keep = true;
						if ( searching ){
							// 検索続行(backward)
							Request(0, true);
						}
					}
				}
			} else {
				keep = true;
			}
			if ( keep ){
				// 留保
//				MoveStarLocAbs( i );
				if (KeepStarTop){
					int y = GetStarLocAbs( str, 0, Dic.GetOrder() );
					if (y>=0){
						if (!Request(y, false)){
							// 2016.7.9 restartできなかった場合out of rangeのため追加
							if (y == pool.get_num()){
								y = pool.get_num()-1;
							}
						}
						
						if (IndexOffset!=y || cury!=0){
							IndexOffset = y;
							cury = 0;
							IndexMiOffset = 0;
#if USE_INVALID
							Invalidate();
#else
							dispStar(0);
							Invalidate(true);
#endif
						} else {
							// starのみの更新
							dispStar(y-IndexOffset);
						}
					}
				} else {
					MoveStar( str );
				}
				DeleteTextFonts();
				ReleaseDC();
				if (GetReverseStatus()!=SQU_REV_FULL){
					StartISPTimer();
				}
#if USE_VIEWAUTOPOP
				if (ViewAutoPop&&cury>=0){
					// incremental searchのみ
					tnstr word;
					GetMainWindow()->pew->GetTextEx(word);
					if (!_tcscmp(word,GetWord(cury))){
						OpenAutoPop( cury+IndexOffset );
					}
				}
#endif	// USE_VIEWAUTOPOP
				if (!IsSearching()){
					if (CanStartSubSearch()){
						_StartSubSearch();
					} else {
						ClearMessage();
					}
				}
				return 1;
			}
#if 0
			int i = pool.BSearch( str );
			if ( i == 0 ){
				if ( Dic.cmpword( str, pool.fw[0] ) < 0 ){
					// pool外
					i = -1;
				}
			}
			if ( i >= 0 && i < pool.get_num() ){
				// pool内
				MoveStarLocAbs( i );
				DeleteTextFonts();
				ReleaseDC();
				if (GetReverseStatus()!=SQU_REV_FULL){
					StartISPTimer();
				}
				return 1;
			}
#endif
		}
	}
	// retry search //
	ss.uDicList = diclist;
	int result;
	bAllowDisp = false;
	Clear( );
	StartIncSearch( );
	ss.word = Dic.GetLangProc()->Normalize(str);	// keywordで検索
	tchar buf[LWORD+1];
	buf[LWORD] = '\0';
	_tcsncpy(buf, ss.word, LWORD);

	// reset multithread
#if !USE_STOP
	SrchParam = GetSrchParam(true);
#endif
	SrchParam->multiThread = false;
	ForceUpdate = false;

	for (;buf[0];){
		Dic.initNsearch2(buf, SrchParam);
		Dic.jumpAS( buf, false, true, SrchParam );	// 前方
		// 最初の１つ目だけ検索
		SearchProc(true);
		if ( pool.get_num() ){
			if ( bAllowDisp )
				dispStar( str, 0 );
			result = 1;
			ss.AddSearchState(SS_BWD);	// 後方にもデータがある(かもしれない)
			if (CanStartSubSearch()){
				_StartSubSearch();
			}
			break;
		} else {
			// Not found
			// 見つかるまで検索文字数を減らす
			buf[_tcslen(buf)-1] = '\0';
			if (buf[0])
				ss.RestartIncSearch();
		}
	}
	if ( !buf[0] ){
		ss.ClearSearchState();
		result = 0;
	}

	DeleteTextFonts();
	ReleaseDC();

	return result;
}
#endif	// NEWINCSRCH

#if !NEWINCSRCH
//返り値：
//-1:前回と同じ
//0:見つからない(clearされない)Not find.
//1:多すぎる(clearされない)Searching...
//2:表示行数が多すぎる(clearされる)Searching...
//3:表示した（正常終了）
//5:表示しきれない(clearされる) Too much.
int Squre::incsearch( const tchar *str, diclist_t diclist )
{
	int l = _tcslen(str);

	if ( !l ){
		return -1;
	}

	// すでに他の検索中である場合は、それを中止する
	if ( !ss.IsIncSearch() && IsSearching() ){
		StopSearch( );
	}

	if ( !ss.IsIncSearch() ){
		FoundCount = 0;	// Clear()でした方が良いのかなぁ・・・
		Clear();
	}

	int equlen = nstr( str, SearchString, Dic.GetOrder() );
#if 1
	if ( !equlen ){	// 前回との一致文字数が０の場合は新規検索とみなして良い？ 1995.5.28
		clear();
	}
#endif
	int flen = _tcslen( SearchString );
	SearchString.set( str );			//

	// 同じ単語の検索かどうかのチェック
	if ( ( !IsSearching() && FoundCount > 0 && FoundCount <= l && equlen >= FoundCount )
		&& ( !IsOver() || l == FoundCount ) ){
			// l == nfindは、一度検索後、ESCで消した後、同じ見出語（実際は一文字のわけだが）
			// を入力した場合、再検索をしないようにするため

#if 0
		if ( IndexOffset ){
			// 次ページ以降へ行っていた場合は再表示？
			// ではまずい。offset=0でもbasenum=0のときは再検索にする必要あり
			IndexOffset = 0;
			cury = 0;
			IndexMiOffset = 0;
			Invalidate( );
			return -1;
		}
#endif
		MoveStar( str );
		return -1;
	}

	if ( NotFoundCount > 0 && NotFoundCount <= l && equlen >= NotFoundCount ){
		return -1;
	}

	int r = 0;

	CreateTextFonts( );

	ss.uDicList = diclist;

//	bool epwing = Dic.HasDicType() == 1;

//	SetMessage( GetStateMessage( MSG_SEARCHING ) );

	if (
		!IsSearching() && (
		// 前回のFoundでの文字数より、今回の一致文字数の方が短い場合
		// lがnfindより短く、equlenとlが一致する場合は、
		// 通常の検索で良い。
		( l > 1 && FoundCount > 0 && FoundCount > equlen && !( FoundCount > l && equlen == l ) )

		// 前回のNotFindでの文字数より、今回の一致文字数の方が短い場合
		|| ( NotFoundCount > 0 && NotFoundCount > equlen )

		// いきなり２文字以上を検索した場合
		|| ( l - flen > 1 && !bUpDown )
		)
		){
		tchar *str2 = new tchar[ l + 1 ];
		Clear();

		bAllowDisp = AllowDispMode;
		// １文字ずつ増やしていく方法
		//** binary-searchにしたほうがよい。
		// 一番長いfindになる長さを求める
//        bool AutoLink = AutoLinkConfig.fEnable;
//        AutoLinkConfig.fEnable = false;
		for ( int i=0;i<l;i++ ){
			str2[i] = str[i];
			str2[i+1] = '\0';
//			if ( epwing && _ismbblead(str2[i]) ) continue;
#if 0
			if ( i > 0 ){
				StopSearch( );
				// 次の検索語がNotFindの場合は中止
				tnstr fwd;
				Japa fjp;
				if ( Dic.Search( str2, fwd, fjp, -1 ) != -1 ){
					if ( strncmp( str2, fwd, i+1 ) )
						break;
				}
			}
#endif
			r = IncSearchProc( str2, i+1, diclist );
//			try {
			if ( r <= 0 ){
				if ( i > 0 ){
					// Not Findの場合は１つ戻す
					str2[ i ] = '\0';
					IncSearchProc( str2, i, diclist );
					SearchProc(true);
//					if ( get_num() )
//                    	RecalcLines( 0, get_num()-1 );
				}
				break;
			}
//			} catch(...){MessageBox(_T(""),_T(""),MB_OK);}
			// 表示画面を超えるまで検索させる
			while ( 1 ){
				r = SearchProc(true);
//				if ( get_num() )
//					RecalcLines( 0, get_num()-1 );
				if ( GetAbsOffsY( get_num() - 1 ) > MaxLines ){
					break;
				}
				if ( !r ){
					// 表示しきれる
					FoundCount = i+1;		// 1995.5.25
					goto exitloop;
				}
			}
		}
exitloop:
#if 0
		AutoLinkConfig.fEnable = AutoLink;
		if ( AutoLink ){
			for ( int i=0;i<get_num();i++ ){
				SearchAutoLink( i );
			}
			Invalidate();
		}
#endif

		bAllowDisp = true;
		if ( get_num() ){
			// 入力文字が検索済みの単語より後ろのほうにあるかどうかチェック
			int i1, i2;
			GetKeepLoc( str, l, i1, i2 );
			if ( i1 == get_num() ){
				// 反転表示する位置がない＝まだ見つかっていない
				while ( 1 ){
					if ( !SearchProc(true) ) break;
					if ( Dic.cmpword( str, GetWordAbs( get_num()-1 ) ) <= 0 ){
						break;
					}
				}
				IndexOffset = 0;
				cury = GetStarLocAbs( str, 0, Dic.GetOrder() );	// 反転表示
			} else {
				// 反転表示位置を探す
				cury = GetStarLoc( str, 0, Dic.GetOrder() );	// 反転表示
			}
			MakeToVisible( cury );
#if 0
			IndexOffset = cury;	// ウィンドウの最初に持ってくるように変更(1998.3.7)
			cury = 0;
			if ( IndexOffset > 5 && (IndexOffset > get_num() - 5) ){
				// 1998.3.8 極端な表示を避けるため
				//** 本当は表示行数を求めてcuryがが面内に出るようにしたほうが良い！！
				IndexOffset -= 5;
				cury += 5;
			}
#endif
#if USE_INVALID
			Invalidate( );
#else
			dispStar(cury);
#endif
			r = 3;
		} else {
			r = 0;
		}

		delete str2;
	} else {
		r = IncSearchProc( str, l, diclist );
		if ( r >= 1 ){
			r = 3;
		}
	}

	DeleteTextFonts( );
	return r;
}

// インクリメンタルサーチ本体
// word : 検索単語
// nsearch : 最大表示行数
// l == lstrlen( str )であること！！l < lstrlen(str)であると期待した動作をしない！！
// 返り値：
//			0		見つからない
//			-1		エラー（未対応）
//			1		見つかった＆検索中
//			2		見つかった＆検索終了（MAX_NDISP以上あることもあるが）
int Squre::IncSearchProc( const tchar *str, int l, diclist_t diclist )
{
	// 通常のインクリメンタルサーチ（iflagを考慮に入れていない！！）

	int fnum = get_num();

	if ( fnum
		&& FoundCount
		&& FoundCount <= l	// 見つかったときの長さが、現在の長さより短いとき
		/* && pool.Included( str, l, iflag ) */ ){
		int i1, i2;
		GetKeepLoc( str, l, i1, i2 );
//		if ( i1 != i2 )
//			Keep( i1, i2 );
		if ( IsSearching() ){
			if ( i1 == i2 ){
				// 未検索の部分に該当単語があるかもしれない場合
				// 今後どうなるのか全く予想できない
				// あまり速くキーボードを打つと、一番長い文字列で検索してしまうので問題。うーむ？？？？
				// Search()で探して、該当単語がない場合は検索を行なわないようにすると良い？？
				// やっぱり、みつかるまでここでループした方が一番速い（あとはバックグランド）
				// ・・・もう一度考え直して・・・・
				// 取敢えず、Search()をして見つかったならgoto jmp1をして、
				// 見つからない場合は、一つ前に戻して検索し直して表示する。
				// 問題点：結局、次の文字を追加すると再検索を行なってしまい、
				//         再び、NotFindとなり、無駄な検索を行なってしまう。
				//　　　　 つまり、今までの検索結果を考慮にいれる必要がある、ということだ！
				//         ＞＞＞根本的な解決必要か？？
				// 解決方法１
				// 　学習させる。
				// 　というほど大したことではなく、今まで入力した文字列を記憶しておき、
				//   それぞれの文字での検索結果を記しておく。
				//　　問題点：単語の修正・削除・追加した場合・・・
				//　　　　　　その単語と記憶している単語を比較して、その記憶を抹消する
				//    問題点その２：大文字・小文字の同一視の場合の処理が更に複雑に？
				//　　問題点その３：検索後、画面サイズやフォントなどを変更した場合
				//　　　　　　　　　記憶した情報が無駄になるものがある(;_;)
				// ◎解決方法２
				//　　新たに文字列を入力したら、Search()を取敢えず行い、
				//　　NotFindになるかどうか調べる。
				//　　もしある場合は大手を振って検索する。
				//　　何と！一番単純なこの方法が一番速い！
				//　　問題点：SearchSmallest()で一度見つかり、再度ここにくると、
				//　　　　　　もう一度SearchSmallest()を行なってしまい、表示語数が減ってしまう。
				//　　　　　　例：retail
				StopSearch( );
				bAllowDisp = false;
				tnstr fwd;
				int r = Dic.SearchSmallest( str, fwd, diclist, SearchParam );
				if ( r == 1
					&& Dic.cmpnword( str, fwd, l ) == 0 ){
						// 見つかった場合
						// bAllowDisp = true;	// ここを有効にすれば、ばたばた表示になるがレスポンスが速い！
											// 問題点を発見！！
											// l-1の文字列で表示しきれるときがある！！
											// 例：tenso
											// 解決方法：取敢えず、l-1の文字で検索をして
											// lの時の文字が出てくるかどうか調べる
						goto jmp1;
				}
				// 見つからない場合
#if 0		// 1995.5.25変更 こっちで良いかなぁ？ 前者の方が単語が多くても正しくヒットするが・・・。
				fwd.set( pool.fw[ get_num() - 1 ] );
				Clear();
				ReSearch( fwd, FALSE, 0 );
#else
				ReSearch( pool.fw[ get_num() - 1 ], FALSE, 0 );
				bAllowDisp = true;
				cury = GetStarLoc( SearchString, 0, Dic.GetOrder() );
#if USE_INVALID
				Invalidate( );
#else
				dispStar(cury);
#endif
#endif
								// "fy"を入力して確かめよ！
			} else {
				// 検索済みの中にstrに該当する単語がある場合
				// 画面に表示しきれるかどうかの判断を行なう必要がある。
				// もし、表示しきれない場合は継続で良いが、
				// 表示しきれる場合、最後まで検索して表示する必要がある
				// ここで判断しないと、次のキー入力を受け付けた場合・・訳がわからなくなってしまった(^^;
//				Keep( i1, i2 );
				FoundCount = l;
				if ( ss.state & SS_FWD ){
//					if ( bAllowDisp )
					{
						MoveStarLocAbs( i1 );
					}
					// 表示画面を超えるまで検索させる
					int r = SearchOnWindow( );
					if ( r ) return 2;
				} else {
					cury = GetStarLoc( str, 0, Dic.GetOrder() );
					FoundCount = l;
					StopSearch( );
#if USE_INVALID
					Invalidate();
#else
					dispStar(cury);
#endif
					return 2;
				}
			}
			// 何もしない（＝そのまま継続検索）
		} else if ( ss.state & SS_FWD ){
			// 検索は終了したが、まだ前方に残りがある場合
			if ( i1 == i2 && i1 >= MAX_NDISP ){	// poolに該当単語がなく、なおかつ前方に存在する可能性がある場合
				goto jmp1;
			}
			if ( i2 - i1 <= MAX_NDISP ){
				// すでに該当単語が検索しきっている場合は何もしない
				MoveStar( str );	// 1995.5.25fix
				return 2;
			}
			Keep( i1, i2 );
			MoveStarLocAbs( 0 );
			ReSearch( pool.fw[ i2 - i1 - 1 ], FALSE, 0 );
		} else {
//			if ( i1 == i2 ){
				// 修正要請：
				// ここにくるのは文字列の追加のみではない！
				// ESCで取り消して再入力した場合でも来てしまう
				// ESCを押したときにフラグをなおした方が良いのだが・・・。
//				if ( !IsSearching() ){	// 常に真のはずであるが、念のため
					// i1 と curyが異なったときはカーソルを移動する
					if ( i1 != cury ){
						bAllowDisp = true;
						MoveStar( str );
					}
					if ( i1 != i2 )
						goto jmp3;
					ss.state = 0;
					return 1;
//				}
//			}
//			Keep( i1, i2 );
			// Keepによって個数が変わったなら
			// --> Keepにしないで、反転カーソルを移動する
jmp3:
			bAllowDisp = true;
//			if ( fnum != get_num() )
//				Invalidate( );
//			cury = 0;
//			FoundCount = l;
//			if ( IndexOffset + cury >= get_num() )
//				IndexOffset = 0;
//			IndexOffset = 0;	// トラッキングは？
//			SearchString.set( str );
		}
		return 1;
	} else {
jmp1:
		bAllowDisp = false;
		Clear( );
		StartIncSearch( );
#if USE_STOP
		SearchParam->Stop(true);
#else
		SearchParam = GetSrchParam();
#endif
#if NEWINCSRCH
		Dic.initNsearch2( str, SearchParam );
#else
		Dic.initNsearch( str, SearchParam );
#endif
		Dic.jumpAS( str, false, true, SrchParam );	// 前方
		ss.word.set( str );
		// 最初の１つ目だけ検索
		FoundCount = l;
//		SearchString.set( str );
		SearchProc(true);
		if ( get_num() ){
//        	RecalcLine( 0 );
			FoundCount = l;
			NotFoundCount = 0;
			if ( bAllowDisp )
				dispStar( str, 0 );
			return 1;
		} else {
			FoundCount = 0;
			NotFoundCount = l;
			ss.state = 0;
			return 0;
		}
	}
}
#endif	// !NEWINCSRCH
// 0 : maxlineを越えた
// 0以外 : １画面分を表示した
int Squre::SearchOnWindow( )
{
	while ( 1 ){
		if ( GetAbsOffsY( get_num() - 1 ) > MaxLines ){
			break;
		}
		int r = SearchProc(true);
		if ( !r ){
			// 表示しきれる
			return 2;
		}
	}
	return 0;	// 最大表示行数(MaxLines)を越えた
}

//
// External Search
//
int Squre::cbExtSearch(TWebSearchThread *th, int type, int param, int user)
{
	return ((Squre*)user)->ExtSrchCallback(type, param);
}

int Squre::ExtSrchCallback(int type, int param)
{
	switch (type){
		case LPSLW_UPDATED:
			// param : updated index of the HitWords. = -1
			if (ExtSrchThreadKey!=-1 && SearchString.exist()){
				tnstr s(Dic.create_composit_word(SearchString));
				Update(s);
			}
			break;
	}
	return 0;
}

void Squre::StartExtSearch()
{
	if (ExtSrchThreadKey!=-1){
		CancelExtSearch();
	}
	if (IsDicOpenedStable()){
		Dic.SearchLongestWordExt(SearchString, NULL, NULL, ExtSrchThreadKey, cbExtSearch, (int)this);
	}
#if USE_GS
	// Google Suggest
	if (gsEnabled()){
		if (GetReverseStatus()!=SQU_REV_FULL){
			if (SearchString.size()>=2){
				gsHttpGetAsync(SearchString, GetMainWindow()->GetHWindow(), UM_GOOGLESUGGEST);
			}
		}
	}
#endif
}
void Squre::CancelExtSearch()
{
	if (ExtSrchThreadKey==-1)
		return;
	Dic.SearchLongestWordCmd(LPSLW_CANCEL, ExtSrchThreadKey);
	ExtSrchThreadKey = -1;
}

// 関連語検索 ////////////////////////////////////////////////////
#if INCSRCHPLUS
#include "WinSquUI.h"
int IncSrchPlusTimer = 1000;
void Squre::StartISPTimer()
{
	if ( ISPWaiting ){
		KillTimer( TM_INCSRCHPLUS );
		ISPWaiting = false;
	}
	if ( !IncSrchPlus || (cury<0) ) return;
	if (IndexOffset+cury>=get_num()) return;	// overrun対策

	const tchar *p = GetWord();
	if ( !Dic.cmpword( _kmstr(SearchString,_wSingleByte), _kmstr(p,_wSingleByte) ) ) return;

	SetTimer( TM_INCSRCHPLUS, IncSrchPlusTimer, NULL );
	ISPWaiting = true;
}
void Squre::StopISPTimer()
{
	if ( ISPWaiting ){
		KillTimer( TM_INCSRCHPLUS );
		ISPWaiting = false;
	}
}
// return value:
// true = 見つかった
bool Squre::ISPProc()
{
	if ( ISPWaiting ){
		KillTimer( TM_INCSRCHPLUS );
		ISPWaiting = false;
	}

	const tchar *word = SearchString;
	if ( _tiskanji(word[0]) )	// 先頭が日本語である場合は行わない
		return false;

	if (GetReverseStatus()==SQU_REV_FULL){
		return false;	// full cursorになった場合は何もしない
	}
#if 0	// removed 2007.11.28
	if ( Dic.Find( word, NULL, -1, SearchParam ) ){
		return false;	// WORD欄の単語が辞書に登録されていれば何もしない
	}
#endif

	const int option = PopupConfig.GetOption();
	MatchArray HitWords;

	int r;
	// 英語
	r = SearchLongestWordOptional( Dic, word, word, option, &HitWords );
	if ( r > 0 ){
		int c1 = WordCount( word );
		for ( int i=HitWords.get_num()-1;i>=0;i-- ){
			if ( WordCount( find_cword_pos(HitWords[i].word) ) == c1 )
				break;
			HitWords.del( i );
		}
		if ( HitWords.get_num() ){
			//GetMainWindow()->SetWordText(HitWords.GetTopWord(), true);
			incsearch( HitWords.GetTopWord() );
			return true;
		}
	}
	return false;
}
#endif	// INCSRCHPLUS

//---------------------------------------------------------------------------
//	Special Search
//---------------------------------------------------------------------------
#include "SrchMed.h"
/*

sub_words0 - popup search
sub_words2 - second phase

popup searchはto main時に検索し、sub_words0へ格納する
first phase searchはincremental searchそのもの(poolをそのままkeep)
second phase searchはbackgroundで検索を行い、sub_words2へ格納する。to main時にpoolへ余裕があればcopyする。to main後はclearされない。

見出し語追加・削除に対する処理
  基本的に対処しない。ただし、矛盾が生じる場合は対応する。

重複単語はどうする？

*/

void Squre::StartSpecialSearch(const tchar *str)
{
	D("StartSpecialSearch");
	if (ss.GetSearchType() & SST_SUBSEARCH){
		// The main search is sub search?
		if (sub_srchword==str){
			return;	// normal return.
		} else {
			ClearSubSearch();
		}
	} else {
		// incremental search/all search
		if (SearchString!=str){
			ClearSubSearch();
		}
	}

	ChangeSubSearchToMain(str);
	SetMessage(GetSrchMsgId());
}

// Start special search in background.
void Squre::StartSubSearch(const tchar *str)
{
	if (!IsAutoSpecialSearchEnabled())
		return;

	OpenSrchMed();
	_StartSubSearch(str, true);
}

void Squre::StopSubSearch()
{
	_StopSubSearch();
	SetMessage(ss.IsSearching()?MSG_SEARCHING:0);
}

void Squre::_StopSubSearch()
{
	sub_ss.StopSearch();
}

void Squre::ClearSubSearch()
{
//	ss.ClearSearchType();
	sub_ss.ClearSearchType();
	__assert(SrchMed || !sub_words || sub_words->get_num()==0);
	if (SrchMed)
		SrchMed->ClearSubWords();
}

#if !USE_STOP
TMultiAllSearchParam *Squre::GetSrchParam(bool wait)
{
	if (SrchParam){
		SrchParam->Stop(false);
	}
	if (!SrchParams){
		SrchParams = new TMultiAllSearchParams(Dic.GetDicNum());
	}
	SrchParam = SrchParams->GetNext(Dic);
	if (wait)
		SrchParam->Stop(true);
	return SrchParam;
}
#endif

TMultiAllSearchParam *Squre::GetSubSrchParam()
{
	if (sub_srchparam){
		sub_srchparam->Stop(false);
	}
	if (!sub_srchparams){
		sub_srchparams = new TMultiAllSearchParams(Dic.GetDicNum());
	}
	sub_srchparam = sub_srchparams->GetNext(Dic);
	//sub_srchparam->Stop(true);
	return sub_srchparam;
}


// sub searchを開始できる条件か？
bool Squre::CanStartSubSearch()
{
	if (!IsAutoSpecialSearchEnabled())
		return false;

	if (ss.GetSearchType() & SST_SUBSEARCH){
		// すでにsub searchをmainに切り替えた検索を行っている
		return false;
	}

	__assert(!ss.IsSubSearch());	//TODO: 下のコードは要らないのでは？
	if (ss.IsSubSearch()){
		// foreground
		return true;	// 常に行う
	}

	// 二語以上か？
	tnstr str = GetMainWindow()->GetWordText();
	const tchar *sp = str;
	if (*sp=='"' || *sp=='(')
		return true;	// yes!!
	for (;;){
		ushort c;
		LD_CHAR(c, sp);
		if (!c)
			break;
		if ((c==' ' || c=='|' || c=='*') && *sp){
			return true;	// yes!!
		}
	}
	return false;
}

//Note:
//	Clear the sub_words when the first_phase is true.
void Squre::_StartSubSearch(const tchar *_str, bool first_phase)
{
	D("StartSubSearch:%d", first_phase);
	//if (IsSearching()){
	//	StopSearch();
	//}


	if (_str)
		sub_srchword = _str;
	else
		sub_srchword = GetMainWindow()->GetWordText();

	if (first_phase){
		// first phase search
		if (SrchMed){
			SrchMed->ClearSubWords();
		}
	} else {
		// second phase search
	}

	const diclist_t diclist = gDicList;
	if (StartSpecialSearch(sub_ss, diclist, sub_srchword, first_phase)){
		if (!ss.IsSearching()){
			SetMessage(MSG_SUBSEARCHING);
		}
	}
}

// main/sub どちらでも使用可能
bool Squre::StartSpecialSearch(SrchStat &ss, diclist_t uDicList, const tchar *str, bool first_phase)
{
	return ::StartSpecialSearch(Dic, ss, uDicList, str, first_phase, GetSubSrchParam());
}

// sub searchをmain searchにmoveする
// word:
//	If word is not null and has the word, add the result of i.Plus to the sub words.
void Squre::ChangeSubSearchToMain(const tchar *word)
{
	OpenSrchMed();
	bAllowDisp = true;	// 2009.5.11 added

	bool need_restart = false;

	Clear();

	if (sub_ss.GetSearchPhase()==0){
		// not started sub search yet.
		_StartSubSearch(word, true);
	}

	// Search and add words using IncPlus.
	//TODO: LangProc対応
	if ( word && word[0] && !_tiskanji(word[0]) )	// 先頭が日本語である場合は行わない
	{
		const int option = PopupConfig.GetOption();
		MatchArray HitWords;
		int r;

		need_restart = true;

		// 英語
		const int curpos = 0;
		r = Dic.SearchLongestWord( word, word, curpos, option, &HitWords );
		if ( r > 0 ){
			int wc = WordCount(word);
			int count = 0;
			for (int i=0;i<HitWords.get_num();i++){
				const tchar *aword = HitWords[i].word;
				if (WordCount(find_cword_pos(aword))>=wc){	// word countが少ないものは対象外
					if (sub_words_map.count(aword)){
						// 別の検索(phase1)で見つかった場合
						int index = sub_words->FindWord(aword);
						__assert(index>=0);
						sub_words->del(index);
					}
					SrchMed->AddSubWord0(new tnstr(aword));
					sub_words_map[aword] = true;
					count++;
				}
			}
		}
	}

	// Add the words that was picked up in the idle proc.
	int phase;
	AddSubWordsToMain(phase);

	ss.Assign(sub_ss);

	if (sub_ss.IsSubSearchEnd()){
		// すでにすべて検索終了
		if (sub_words->get_num()>MAX_NDISP){
			ss.SetRemain(false);
		}
		sub_words_map.clear();	// clear the duplicate check to release the memory.
	} else {
		if (need_restart){
			SrchMed->RestartSpecialSearch(sub_ss, word);
		}
	}

	ss.SetSearchType(SST_SEARCH | SST_SUBSEARCH);	// set to phase 0.

	if (ss.IsSearching()){
		if (get_num()>=MAX_NDISP){
			ss.StopSearch();
			ss.SetRemain( false );
		}
		
	}
	
	IndexOffset = 0;
	cury = 0;
	IndexMiOffset = 0;

	SetVRange2( true );
	InvalidateLines();
}

// SubSearchを検索バッファとして利用する
// ※::StartSpecialSearch()をベースに作成
bool Squre::StartSubSearchForNormalSearch(const tchar *str, SrchMode mode, diclist_t uDicList, GenericRexp *grexp, int level1, int level2, int MemOnOff)
{
	OpenSrchMed();

	// search setup //
	sub_ss.uDicList = uDicList;

	int search_type = SST_SUBSEARCH | SST_SEARCH | SST_PHASE2;
	sub_srchparam = GetSubSrchParam();
	sub_srchparam->multiThread = true;
	sub_ss.SetupSearch( search_type, str, mode, sub_ss.uDicList, level1, level2, MemOnOff );
	sub_ss.SetGRexp(grexp);
	if (sub_ss.IsRexp()){
		if ( sub_ss.CanUseRexp() ){
			if ( !sub_ss.Compile( ) ){
				// コンパイルエラー
				// 通常の検索を使用
				int error = sub_ss.GetRexpErrorCode( );
				sub_ss.SetGRexp( NULL );
				if ( error != GRE_NOREGULAR
					&& (str[0] && error != GRE_NOSTRINGS)){	// "など記号を検索するときに発生するため
					DBW("RegularExp Error:%d", error);
					return false;	// fatal error
				}
				// use normal search
			}
		} else {
			return false;	// Cannot use regular expression.
		}
	}

	sub_ss.ResetCount();
	sub_ss.StartSearch(false);

	ss.SetSearchType(SST_SEARCH | SST_SUBSEARCH);	// set to phase 0.

	SrchMed->SetupPhase(sub_ss.GetSearchPhase(), sub_ss.GetSearchState());

	return true;
}

// Move the found words to the view pool.
void Squre::AddSubWordsToMain(int &phase)
{
	CopyToPool(*sub_words, sub_ss.uDicList, false);
	phase = SrchMed->SetupPhase(sub_ss.GetSearchPhase(), sub_ss.GetSearchState());
}

//
// CopyToPool method group
//
// Adapter classes for CopyToPool //
//
// return:
//	the number of added words.
int Squre::CopyToPool(TSubWordItems &words, diclist_t uDicList, bool insert_top)
{
	int count = 0;
	Japa *japa = NULL;
#if USE_DISPLEVEL
	int offset = 1;
	const int disp_level = 1;
#endif
	for (int i=0;i<words.get_num();i++){
		if (!insert_top && get_num()>=MAX_NDISP)
			break;
		if (!japa)
			japa = new Japa;
		__EnableDDNBegin(CanDispDicName());
		const tchar *word = words.GetWord(i);
		int r = Dic.Read(word, japa, uDicList);
		__EnableDDNEnd();
		if (r<0) break;
		if (r){
			BM_JSetMark(find_cword_pos(word), *japa);
#if USE_DISPLEVEL
			insert(cury+offset+IndexOffset, new tnstr(word), japa, Dic.GetLastFindDicNo(), true, disp_level);
			offset++;
#else
			if (insert_top){
				insert2(count, new tnstr(word), japa, Dic.GetLastFindDicNo(), false);
			} else {
				add(new tnstr(word), japa, Dic.GetLastFindDicNo());
			}
#endif
			//DBW("cury=%d word:%ws", cury, sub_words0[i].c_str());
			//UpdateProc(cury+1);
			japa = NULL;
			count++;
		}
	}
	if (japa)
		delete japa;
	return count;
}

