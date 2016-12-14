#include "pdclass.h"
#pragma	hdrstop
#ifndef SMALL
#include	"utydic.h"
#endif
#include	"multidef.h"
#include	"id.h"
#include	"japa.h"
#include	"winsqu.h"
#include "LangProc.h"
#include "windic.h"
#include "WinSquUI.h"
#include "SquItemView.h"

#include "SquView.h"	// for SetCursor

void Squre::SetCursor(const tchar *name)
{
	View->SetCursor(name);
}

// 一致する文字数を返す(コード順)
int __nstr( const tchar *str1, const tchar *str2 )
{
	int n=0;
	while ( *str1 ){
		if (*str1 != *str2) return n;
		n++;
		str1++;
		str2++;
	}
	return n;
}

//iが-1のときは消去
int Squre::dispStar(int i, bool linkui)
{
	DBW("dispStar:%d cury=%d/%d/%d", i, cury, get_num(), IndexOffset);
	if ( i+IndexOffset >= get_num() )
		i = get_num() - IndexOffset - 1;

#if 0	// gray reverseのために外した・・・副作用が起きないか心配 2004.6.9
	if ( i != -1 && i == cury ){
		return 0;
	}
#endif
	if (cury == -1 && i == -1) return 0;
	if (i >= get_num()) return -1;

#if 0
	if (i>=0 && GetDispLevel(i)>MaxDispLevel){
		// hidden index
		int ni = FindNearestVisibleLevel(i);
		__assert(ni>=-IndexOffset);
		if (ni>=-IndexOffset){
			if (cury==ni)
				return 0;
			if (i>=0){
				i = ni;
			} else {
				//TODO: scroll
			}
		}
	}
#endif
	
	GetDC();
	if ( cury != -1 ){
		/*----- カーソルの消去 -----*/
		if (cury+IndexOffset<get_num()){
			_dispStar( cury, 0 );
		}
	} else {
		// カーソルが初めて表示されるとき
	}
	cury = i;
	if (i != -1){
		/*----- カーソルの表示 -----*/
		if (linkui)
			if (UIMain)
				UIMain->SetWordText(GetWord(cury));
		_dispStar( cury, 1 );
	} else {
		// メニューの変更(カーソルが消去されるとき)
		ClsRegion( );
	}
	ReleaseDC();
	return 0;
}

#if USE_DISPLEVEL
// find the neareast index that can be displayed.
int Squre::FindNearestVisibleLevel(int index)
{
	for (int i=index;i>=-IndexOffset;i--){
		if (GetDispLevel(i)<=MaxDispLevel){
			return i;
		}
	}
	return -1;	// not found.
}
#endif

void Squre::AdjustOffset( )
{
	int o = GetLastPageOffset( );
	if ( IndexOffset > o ){
		if ( cury != -1 )
			cury += IndexOffset - o;
		IndexOffset = o;
	}
}

int Squre::GetReverseStatus()
{
	if (cury<0 || cury+IndexOffset>=pool.fw.get_num())	// 22015.1.1 後半の条件追加
		return SQU_REV_NONE;
// 自動判別
	tnstr word = UIMain->GetCompleteWordText();
	if (
		::_tcscmp(UIMain?extract_cword(word).c_str():_T(""), extract_cword(GetWord(cury)))==0
#if 0	// old style
		!dic.cmpword(
		_kmstr(GetWord(cury), _wSingleByte),
		_kmstr((UIMain?UIMain->GetWordText():_T("")), _wSingleByte))
#endif
		){
		// 一致！
		return SQU_REV_FULL;	// full reverse
	} else {
		// 不一致
		return SQU_REV_GRAY;	// gray reverse
	}
}

void Squre::clsStar( )
{
	dispStar( -1 );
}

// pool全体から探す
// 返り値は絶対インデックス、最大get_num()を返すので注意！
// 並列モードは未サポート
int Squre::GetStarLocAbs( const tchar *word, int , int order, int start )
{
	int n,l=0;
	int curi = -1;
	const tchar *wp;

	// マージモード
	tnstr _word(Dic.create_composit_word(word));

	for (;start<get_num();start++){
		wp = GetWordAbs( start );
		n = nstr( wp, _word, order );
		if ( n > l ){
			curi = start;
			l = n;
		} else if (n < l){
			if (!ss.IsSubSearch()){	// 関連語検索のみ非文字コード順
				break;
			}
		} else if ( n == l ){
			int r;
			if ( (r=cmpword( _kwstr(wp,Dic.GetKCodeTrans()), _kwstr(_word,Dic.GetKCodeTrans()) )) < 0 ){
				curi = start;
				l = n;
			} else if ( r == 0 && _tcscmp( wp, _word ) == 0 ){
				curi = start;
			}
		}
	}
	if (curi<0){
		curi = 0;	// 2014.10.16 -1は不都合？なので追加
	}
#if 1	// 2016.7.9 curi = get_num() になるとこのメソッドを呼んでいる側で不具合が出るのでは？
		// 2016.11.8 multithreadで検索するとrandom insertionが発生するが、追加する単語がget_num()-1より後ろのwordだと正しいcuriを返さなくなるため復活
		//           ex."it"で終わる見出し語を検索してみる
	else
	// 一番最後なのか、それより１つ前なのかの判断がloopにはないため、一番最後かどうかの判断をする 2014.10.16
	if (curi>0 && curi==get_num()-1){
		if (cmpword( _kwstr(GetWordAbs(get_num()-1),Dic.GetKCodeTrans()), _kwstr(_word,Dic.GetKCodeTrans()) ) < 0 ){
			// 2016.7.9 _wordのほうがword[get_num()-1]より前のorderの場合、curi=get_num()-1が正しいはずなので < 0 の条件を追加した
			curi = get_num();
		}
	}
#endif
	return curi;
}

//	*マーク表示（単語で位置の指定）
void Squre::dispStar( const tchar *word, int dicno )
//返り値：*マークの位置(0～(ndisp-1) ないときは -1)
{
	int curi = GetStarLoc( word, dicno, Dic.GetOrder() );
	if ( curi > LastIndex )	// 1996.3.14
		curi = LastIndex;
	if (cury == curi)	//同じところの場合は再表示しない
		return;
	dispStar(curi);

}

// 引数:cur を見える位置にくるように、offsetを変える
void Squre::MakeToVisible( int cur )
{
	int l = 0;
	int i;
	for ( i=cur+IndexOffset;i>=0;i-- ){
		RecalcLine( i );
		l += LinesList[i].NumLines + ItemView.LineInterval;
		if ( l >= MaxLines ){
        	if ( i < cur+IndexOffset )
        		i++;
			break;
		}
	}
	if ( i > IndexOffset ){
		cury -= i - IndexOffset;
		IndexOffset = i;
	}
}

#if USE_DISPLEVEL
void Squre::ChangeMaxDispLevel(int offset)
{
	__assert(offset);
	MaxDispLevel += offset;
	if (MaxDispLevel<0){
		MaxDispLevel = 0;
	}
	Invalidate();
}

#endif
