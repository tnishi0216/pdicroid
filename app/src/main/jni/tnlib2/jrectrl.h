#ifndef	__JRECTRL_H
#define	__JRECTRL_H

#include	"rexpgen.h"

#include	"jreusr.h"
#include "jrefuzzy.h"

typedef BOOL (WINAPI *LPFUZZY_OPEN)( JREFUZZYDATA* pData, BOOL fDummy);
typedef BOOL (WINAPI *LPFUZZY_CLOSE)( JREFUZZYDATA* pData );
typedef BOOL (_cdecl *LPFUZZY_OPTIONDIALOG)( HWND hwndParent, UINT flagsDisable );
typedef JREFUZZYDATA* (WINAPI *LPFUZZY_GETFUZZYDATAINJRE)( JRE2* pJre );



//	JREObjectのインスタンスは複数存在しても構わない（？？JRE.DLLで対応しているかどうかも問題であるが）
//	このオブジェクトはなるべく寿命を短くした方が良い（JRE.DLLの仕様による？）

class JREObject : public GenericRexp {
typedef GenericRexp super;
protected:
	static HINSTANCE hinstJRE;		// JREインスタンスハンドル
	static int nInstance;			// このオブジェクトのインスタンスカウンター
	static LPJRE2OPEN lpfnJre2Open;
	static LPJRE2CLOSE lpfnJre2Close;
	static LPJRE2GETMATCHINFO lpfnJre2GetMatchInfo;
	static LPJRE2COMPILE lpfnJre2Compile;
	static LPFUZZY_OPTIONDIALOG lpfnFuzzy_OptionDialog;
	static LPFUZZY_GETFUZZYDATAINJRE lpfnFuzzy_GetFuzzyDataInJre;
	static bOpened;
public:
	JRE2 jre2;						// JRE構造体
	int error;
	static const tchar *DllName;			// JRE dll filename
public:
	JREObject( );
	~JREObject( );

	WORD GetVersion( );

	// 必ずOpen-Closeの対で使用する事
	BOOL Open( );
	void Close( );
	virtual BOOL CanUse( )
		{ return hinstJRE != NULL; }

	BOOL SubOpen( );
	void SubClose( );
	virtual BOOL CompileImpl( const tchar *patern );

	virtual BOOL CompareImpl( const tchar *str );

	virtual int GetErrorCode( );

	// 検索開始位置指定による比較
	// これを呼んだ後、Compare( const tchar *str )を呼ぶ場合は
	// jre2.nStart を0にしなければならない（改良必要）
	BOOL Compare( const tchar *str, int startpos );
	virtual BOOL Compare( const tchar *str, TMatchInfos *matches=NULL, int user=0 )
		{ return super::Compare(str, matches, user); }
	

	BOOL GetMatchImpl( int &len, int &loc );	// len : マッチ長 loc : マッチ位置

	void UseFuzzy( BOOL f )
	{
		SetFlag( GRXP_FUZZY, f );
	}
	void OptionDialog(HWND parent);
protected:
	void SetupIgnoreCase();
};

#endif	__JRECTRL_H
