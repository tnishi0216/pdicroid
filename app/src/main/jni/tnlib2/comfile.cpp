#include	<windows.h>
#include	"bc.h"
#pragma	hdrstop
#include	<commdlg.h>
#include	<shellapi.h>
#include	"char.h"
#include	"comfile.h"
#include	"tnwinx.h"
#ifndef WINCE
#include	<dlgs.h>
#endif
#include "tnhelp.h"
#include "filestr.h"

#define	CFD_MAXPATH		256

CommFileDialog::CommFileDialog( HWND hwnd, tnstr &path, LPCTSTR title )
	:pathname( path )
{
	ofn.hwndOwner = hwnd;
	pathbuffer = new tchar[ CFD_MAXPATH ];
	_tcscpy( pathbuffer, path );
	ofn.lpstrTitle = title;

	Initialize( );			// 初期化処理
#ifndef WINCE
	lpfnHook = NULL;
#endif
}


CommFileDialog::~CommFileDialog()
{
#ifndef WIN32
	if ( lpfnHook ){
		FreeProcInstance( (FARPROC)lpfnHook );
	}
#endif
	delete[] pathbuffer;
}

// このイニシャライズを実行すると、マルチセレクトで指定したバッファーサイズが
// キャンセルされるので注意！
void CommFileDialog::Initialize( )
{
	HWND hwnd = ofn.hwndOwner;
	LPCTSTR title = ofn.lpstrTitle;

	HookFlags = 0;
	memset( &ofn, 0, sizeof( OPENFILENAME ) );
	ofn.lStructSize = sizeof( OPENFILENAME );
	ofn.hwndOwner = hwnd;
	ofn.Flags = OFN_HIDEREADONLY	// 読み取り専用チェックボックスは無効に
#ifdef WIN32
		| OFN_LONGNAMES
#endif
	;
	// パスバッファーの設定
	ofn.lpstrFile = pathbuffer;
	ofn.nMaxFile = CFD_MAXPATH;

	// ダイアログボックスタイトル
	ofn.lpstrTitle = title;

	filterbuffer.clear();
}

#ifndef WINCE
void CommFileDialog::AllowMultiSelect( int size )
{
	ofn.Flags |= OFN_ALLOWMULTISELECT;
#if 1
	delete[] pathbuffer;
	pathbuffer = new tchar[ size+1 ];
	lstrcpy( pathbuffer, pathname );
	ofn.lpstrFile = pathbuffer;
	ofn.nMaxFile = size;
#endif
}
#endif

#if 0
void CommFileDialog::SetFilter( struct FilterListStruct *filterlist )
{
	int i = 0;
	int l = 0;
	while ( filterlist[i].name ){
		l += lstrlen( filterlist[i].name ) + 1;
		l += lstrlen( filterlist[i].pattern ) + 1;
		i++;
	}
	if ( i ){
		l++;
		filterbuffer = new tchar[ l ];
		tchar *p = filterbuffer;
		for ( int j=0;j<i;j++ ){
			p += _tcslen( _tcscpy( p, filterlist[j].name ) ) + 1;
			p += _tcslen( _tcscpy( p, filterlist[j].pattern ) ) + 1;
		}
		*p = '\0';
		ofn.lpstrFilter = filterbuffer;
	} else {
		ofn.lpstrFilter = NULL;
	}
}
#endif

void CommFileDialog::SetFilter( LPCTSTR filterlist, tchar chDelim )
{

	if ( chDelim ){
		// フィルターバッファの準備
		filterbuffer = filterlist;

		// フィルターの区切り文字をセット
		const tchar *p = filterbuffer;
		while ( *p ){
			if ( *p == chDelim ){
				*(tchar*)p = '\0';
				p++;
			}
			p = CharNext( p );
		}
		ofn.lpstrFilter = filterbuffer;
	} else {
		ofn.lpstrFilter = filterlist;
	}
}


void CommFileDialog::SetCustomFilter( LPTSTR filter, int len )
{
	ofn.nFilterIndex = 0;
	ofn.lpstrCustomFilter = filter;
	ofn.nMaxCustFilter = len;
}
#ifndef WINCE
void CommFileDialog::SetTemplate( LPCTSTR lpTemplateName )
{
	ofn.Flags |= OFN_ENABLETEMPLATE;
	ofn.lpTemplateName = lpTemplateName;
//	ofn.hInstance = GetWindow_HINSTANCE( ofn.hwndOwner );
	ofn.hInstance = hTNInstance;
}
void CommFileDialog::SetTemplate( HINSTANCE hInstance )
{
	ofn.Flags |= OFN_ENABLETEMPLATEHANDLE;
	ofn.hInstance = hInstance;
}

void CommFileDialog::SetHook( UINT (CALLBACK *_lpfnHook)(HWND, UINT, WPARAM, LPARAM), LPARAM lCustData )
{
	ofn.Flags |= OFN_ENABLEHOOK;
	ofn.lpfnHook = _lpfnHook;
	if ( lCustData ){
		ofn.lCustData = lCustData;
	} else {
		ofn.lCustData = (LPARAM)this;
	}
}

UINT CALLBACK DllExport CommFileDialog::CommFileDialogHook(HWND hwnd, UINT wm, WPARAM wParam , LPARAM lParam )
{
	static CommFileDialog *dlg = NULL;
	switch (wm) {
		case WM_INITDIALOG:
			{
			dlg = (CommFileDialog *)((OPENFILENAME *)lParam)->lCustData;
			WORD wFlag = dlg->HookFlags;
			if ( wFlag & CHK_DRAGDROP ){
				DragAcceptFiles( hwnd, TRUE );
			}
			return true;
			}
		case WM_DROPFILES:
			{
			HDROP hDrop = (HDROP)wParam;
			UINT num = DragQueryFile( hDrop, -1, NULL, 0 );	// ドロップされたファイルの個数を得る
			if ( num != 1 )
				return true;
			int size = DragQueryFile( hDrop, 0, NULL, 0 ) + 1;	// ファイルの長さを得る
			tchar *name = new tchar[ size ];
			if (name){
				DragQueryFile( hDrop, 0, name, size );
				HWND hEdit = GetDlgItem( hwnd, 1152 );
				SetWindowText( hEdit, name );
				SendMessage( hEdit, EM_SETSEL, 0, MAKELONG( 0, 32767 ) );
				delete[] name;
			}
			DragFinish( hDrop );
			return true;
			}
		case WM_COMMAND:
			if ( LOWORD(wParam) == pshHelp ){
				// ヘルプ処理
		j1:;
				if ( dlg->ofn.Flags & OFN_SHOWHELP ){
					TNWinHelp( dlg->ofn.hwndOwner , dlg->helpname, dlg->helpcommand, dlg->helpdata );
					return false;
				}
			}
			break;
		case WM_NOTIFY:
			// なんでMicrosoftはいくつもおんなじようなhelp notifyを作ってるんだ？？
			if ( wParam == 0 ){
				OFNOTIFY *on = (OFNOTIFY*)lParam;
				if ( on->hdr.code == CDN_HELP ){
					goto j1;
				}
			}
			break;
	}
	return false;
}

// 内部で使用するフック関数のセット
void CommFileDialog::SetHookInternal( )
{
	if ( !lpfnHook ){
#ifdef WIN32
		*(FARPROC*)&lpfnHook = (FARPROC)CommFileDialogHook;
#else
		*(FARPROC*)&lpfnHook = MakeProcInstance( (FARPROC)CommFileDialogHook,
			GetWindow_HINSTANCE( ofn.hwndOwner ) );
#endif
		SetHook( lpfnHook );
	}
}

void CommFileDialog::EnableDragDrop( )
{
	SetHookInternal( );
	HookFlags |= CHK_DRAGDROP;
}

void CommFileDialog::UseHelp( const tchar *filename, UINT command, DWORD data )
{
	ofn.Flags |= OFN_SHOWHELP;
	helpname.set( filename );
	helpcommand = command;
	helpdata = data;
}
#endif	// !WINCE

CommFileOpenDialog::CommFileOpenDialog( HWND hwnd, tnstr &path, LPCTSTR title )
	:CommFileDialog( hwnd, path, title )
{
	ofn.Flags |= OFN_PATHMUSTEXIST;
}
void CommFileOpenDialog::Initialize( )
{
	CommFileDialog::Initialize( );
	ofn.Flags |= OFN_PATHMUSTEXIST;
}

bool CommFileOpenDialog::Execute( )
{
	bool r = GetOpenFileName( &ofn );
	if ( r ){
#if defined(WIN32) && !defined(WINCE)
		tchar *p;
		if ( ofn.Flags & OFN_ALLOWMULTISELECT ){
			if ( ofn.Flags & OFN_EXPLORER ){
				p = pathbuffer;
				while ( 1 ){
					if ( *p == '\0' ){
						if ( *(p+1) == '\0' )
							break;
						*p = '|';
					}
					p = CharNext(p);
				}
			} else {
				// space区切りであると判断して実行
				p = pathbuffer + (ofn.nFileOffset>=1?ofn.nFileOffset-1:0);
				while ( *p ){
					if ( *p == ' ' ){
						*p = '|';
					}
					p = CharNext(p);
				}
			}
		}
#endif
	}
	pathname.set( pathbuffer );
	return r;
}


CommFileSaveDialog::CommFileSaveDialog( HWND hwnd, tnstr &path, LPCTSTR title )
	:CommFileDialog( hwnd, path, title )
{
//	AppendExt = true;
	ofn.Flags |= OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
}

void CommFileSaveDialog::Initialize( )
{
	CommFileDialog::Initialize();
	ofn.Flags |= OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
}

bool CommFileSaveDialog::Execute( )
{
	if (!GetSaveFileName( &ofn )){
		return false;
	}
	pathname.set( pathbuffer );
#if 0
	if (AppendExt){
		AppendExtension();
	}
#endif
	return true;
}

#if 0	// ボツ
// 拡張子がない場合FilterIndexに示される拡張子を追加する
// Execute()でtrueを返した場合のみ
void CommFileSaveDialog::AppendExtension()
{
	const tchar *ext = GetFileExtPtr(pathname);
	if (ext)
		return;	// extension exists.
	const tchar *filter = ofn.lpstrFilter;
	if (!filter)
		return;	// no filter
	if (ofn.nFilterIndex<=0)
		return;	// invalid filter index
	for (int i=1;filter[0];i++){
		for(;filter;){
			filter++;
		}
		filter++;
		if (!filter[0])
			break;
		if (i==ofn.nFilterIndex){
			// Found the filter index.
			filter = GetFileExtPtr(filter);
			if (!filter)
				return;	// no extension
			const tchar *p = filter;
			for (;*p;){
				if (*p==';' || *p=='*')
					break;
				p = CharNext(p);
			}
			if (filter==p)
				return;	// no extension
			pathname.cat(filter, STR_DIFF(p, filter));
			break;
		}
		for(;filter;){
			filter++;
		}
		filter++;
	}
}
#endif
