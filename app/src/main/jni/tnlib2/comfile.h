#ifndef	__COMFILE_H
#define	__COMFILE_H

#include "tndefs.h"
#include "tnstr.h"

// GetxxxxFilename DialogBox IDs
#define	IDGF_FILENAME		1152	// Edit box
#define	IDGF_FILENAMEEX		1148	// ComBox
#define	IDGF_FILEBOX		1120
#define	IDGF_DIRBOX			1121
#define	IDGF_DIRECTORY		1088
#define	IDGF_FILETYPETITLE	1089
#define	IDGF_FILETYPE		1136
#define	IDGF_DRIVETITLE		1091
#define	IDGF_DRIVE			1137
#define	IDGF_READONLY		1040

struct FilterListStruct {
	tchar *name;
	tchar *pattern;
};

// 終了時に、ofn.Flagsにセットされるフラグ
//	OFN_NOREADONLYRETURN	: 選択ファイルが読み取り専用のファイルでない

//	コモンダイアログボックスクラス
class CommFileDialog {
public:
	OPENFILENAME ofn;
protected:
	tnstr &pathname;
	tnstr filterbuffer;
	tchar *pathbuffer;
#ifndef WINCE
	UINT (CALLBACK *lpfnHook)(HWND, UINT, WPARAM, LPARAM);
#endif
public:
	CommFileDialog( HWND hwnd, tnstr &path, LPCTSTR title=NULL );
		// EnableDragDrop()を使う場合は、かならずhwndが必要(hInstanceのため)
	virtual ~CommFileDialog();
	virtual void Initialize( );		// デフォルトの設定に戻す
	virtual bool Execute( ) = 0;		// 実行開始
							// 実行後、このクラスは自動的に削除されないので注意！

	//////////////////////////////////////
	//	フラグに関する設定				//
	//////////////////////////////////////

	// フラグの直接のセット
	void SetFlags( DWORD flag )
		{ ofn.Flags |= flag; }
	void ResetFlags( DWORD flag )
		{ ofn.Flags &= ~flag; }

#ifndef WINCE
	// マルチファイルセレクト
	// 終了時取得例：c:\files file1.txt file2.txt ..\bin\file3.txt
	//		・ディレクトリとファイル名は別々に
	//		・相対パスが有り得る
	//		・区切り文字は' '
	void AllowMultiSelect( int size );	// sizeはバッファーのサイズ
#endif

	// ファイルが存在していないといけない（パスはもちろんの事）
	//	・CommFileOpenDialogクラスではデフォルトで設定済み
	void FileMustExist( )
		{ ofn.Flags |= OFN_FILEMUSTEXIST; }
	// パスが存在していないといけない
	void PathMustExist( )
		{ ofn.Flags |= OFN_PATHMUSTEXIST; }

	// 読み取り専用チェックボックスを有効にする（デフォルトでは表示されない）
	void EnableReadOnly( )
		{ ofn.Flags &= ~OFN_HIDEREADONLY; }
#ifndef WINCE	// Microsoftの間違い？定義されていない・・・
	// 読み取り専用チェックボックスをチェックにする
	void CheckReadOnly( )
		{ ofn.Flags |= OFN_READONLY; }
#endif

	// ファイルが存在しないとき、新規である事を確認する表示をする
	void CreatePrompt( )
		{ ofn.Flags |= OFN_CREATEPROMPT; }

	// [名前を付けて保存]で、すでにファイルが存在していた場合確認をする
	//	・CommFileSaveDialogクラスではデフォルトで設定済み
	void OverWritePrompt( )
		{ ofn.Flags |= OFN_OVERWRITEPROMPT; }

	// カレントディレクトリをダイアログ作成時のディレクトリに強制的に変更し、
	// 終了すると、元のディレクトリに戻す。
	// （今一つ使い方が解らない）
	void NoChangeDir( )
		{ ofn.Flags |= OFN_NOCHANGEDIR; }

	//////////////////////////////////////
	//	ファイル名に関する設定			//
	//////////////////////////////////////

	// デフォルトの拡張子の設定('.'は含めない）
	void SetDefExtension( const tchar *str )
		{ ofn.lpstrDefExt = (tchar*)str; }

	// ダイアログボックス正常終了時に、パス名を除いたファイル名を取得したいとき
	void SetFileTitleBuffer( LPTSTR buf, int len )
		{
			ofn.lpstrFileTitle = buf;
			ofn.nMaxFileTitle = len;
		}

	// 初期のディレクトリの指定（コンストラクタで渡すpathパスより優先的）
	void SetInitialDir( LPCTSTR str )
		{ ofn.lpstrInitialDir = (LPTSTR)str; }

	//////////////////////////////////////////
	//	フィルターに関する設定				//
	//////////////////////////////////////////

	// フィルターのセット
	// 構造体による指定(FilterListStructの１次元配列で指定）
	// 配列の最後はnameをNULLにする事
	void SetFilter( struct FilterListStruct *filterlist );
	// 標準方法による指定
	//	chDelimは、filterlistのリストの区切り文字を指定
	// 例：SetFilter( "実行ﾌｧｲﾙ(*.exe)|*.exe|全て(*.*)|*.*|", '|' );
	void SetFilter( LPCTSTR filterlist, tchar chDelim=NULL );

	// 初期のフィルターのインデックス番号の指定
	void SetFilterIndex( int i )
		{ ofn.nFilterIndex = i+1; }	// 0から始まる事に注意

	// 初期のフィルター、及び終了時にはfilterに最後に選択されていたフィルターをセット
	// lenは40以上である事
	// この関数でセットした後、SetFilterIndexを行うと、初期のフィルターとして利用されないので注意！！
	// 詳しくはSDKヘルプを参照
	void SetCustomFilter( LPTSTR filter, int len );

	//////////////////////////////////////////
	//	ユーザインターフェースに関する設定	//
	//////////////////////////////////////////

	tnstr helpname;
	UINT helpcommand;
	DWORD helpdata;
	void UseHelp( const tchar *helpname, UINT command, DWORD data );		// Helpを使用する場合

#ifndef WINCE
	// テンプレートを使用する場合
	void SetTemplate( LPCTSTR lpTemplateName );
	void SetTemplate( HINSTANCE hInstance );
#endif

public:
	WORD HookFlags;		// EnableDragDrop()用フラグ
#define	CHK_DRAGDROP	0x002
public:
#ifndef WINCE
	// フック関数
	// 新たにフックした関数から、親クラスのCommFileOpenHook()を呼ぶこと
	// lCustDataはデフォルトではthisがセットされるので、デフォルト引数のままにしておくこと
	// もし、どうしても別な用途で利用したい場合は、EnableDragDrop()を使わないようにする
	// 更にEnableDragDrop()を用いてさらに、lCustDataをthis以外にしたい場合は
	// 親クラスのCommFileOpenHook()を呼ばずに独自にフック関数を用意する必要がある
	void SetHook( UINT (CALLBACK *lpfnHook)(HWND, UINT, WPARAM, LPARAM), LPARAM lCustData=NULL );
	// UINT (CALLBACK *lpfnHook)(HWND, UINT, WPARAM, LPARAM)

	// フック関数をセット
	void EnableDragDrop( );		// ドラッグドロップによるファイルを受け付ける
#endif
protected:
	static UINT DllExport CALLBACK CommFileDialog::CommFileDialogHook(HWND hwnd, UINT wm, WPARAM wParam , LPARAM lParam );

private:
	void SetHookInternal( );
public:
	//////////////////////////////////////////
	//	終了時に取得可能な関数				//
	//////////////////////////////////////////

	// ファイル名のオフセットを得る
	int GetFileOffset( )
		{ return ofn.nFileOffset; }

	// 拡張子までのオフセットを得る（'.'の次）
	//	・拡張子がない場合は終端の'\0'のオフセットを返す
	//	・ファイル名の最後が'.'で拡張子がない場合は、0を返すので注意！！
	int GetExtensionOffset( )
		{ return ofn.nFileExtension; }

	// SetDefExtensionで設定していた場合、
	// 入力された拡張子が異なっていたときTRUEを返す
	bool IsExtensionDifferent( )
		{ return ( ofn.Flags & OFN_EXTENSIONDIFFERENT ) == OFN_EXTENSIONDIFFERENT; }

	DWORD GetErrorCode( )
		{ return CommDlgExtendedError( ); }

};

// ファイルオープン用コモンダイアログボックスクラス
class CommFileOpenDialog : public CommFileDialog {
public:
	CommFileOpenDialog( HWND hwnd, tnstr &path, LPCTSTR title=NULL );
	virtual void Initialize( );		// デフォルトの設定に戻す
	virtual bool Execute( );		// 実行開始
};

// ファイルセーブ用コモンダイアログボックスクラス
class CommFileSaveDialog : public CommFileDialog {
protected:
//	bool AppendExt;	// 拡張子がない場合FilterIndexに示される拡張子を追加する
public:
	CommFileSaveDialog( HWND hwnd, tnstr &path, LPCTSTR title=NULL );
//	void SetAppendExt(bool enabled)
//		{ AppendExt = enabled; }
	virtual void Initialize( );		// デフォルトの設定に戻す
	virtual bool Execute( );		// 実行開始
//	void AppendExtension();
};

#endif	// __COMFILE_H

