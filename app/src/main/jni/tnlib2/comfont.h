#ifndef	__COMFONT_H
#define	__COMFONT_H

//	未対応の処理
//	CF_NOFACESEL,CF_NOSIZESEL,CF_NOSTYLESEL,CF_USESTYLE,rgbColors,lpszStyle,nFontType

class ChooseFontDialog {
	public:
		CHOOSEFONT cf;
	public:
		// 注意：lfの初期化時の扱いについて
		//			デフォルトでは、lfで初期化するようにCF_INITTOLOGFONTSTRUCTをセットします
		//			これをしたくない場合は、SetNoInit()を呼ぶこと

		// スクリーンフォントのみ
		ChooseFontDialog( HWND hwnd, LOGFONT *lf );

		// WYSIWYGフォントのみ
		ChooseFontDialog( HWND hwnd, LOGFONT *lf, HDC hdc );

		// 初期化
		void Initialize( );

		virtual BOOL Execute( );		// 実行開始
								// 実行後、このクラスは自動的に削除されないので注意！

		//////////////////////////////////////
		//	フラグに関する設定				//
		//////////////////////////////////////

		// フラグの直接のセット
		void SetFlags( DWORD flag )
			{ cf.Flags |= flag; }
		void ResetFlags( DWORD flag )
			{ cf.Flags &= ~flag; }

		// GDIのフォントシミュレーションを行わないようにする(?)
		void NoSimulations( )
			{ cf.Flags |= CF_NOSIMULATIONS; }

		void ForceFontExist( )
			{ cf.Flags |= CF_FORCEFONTEXIST; }

		void SetNoInit( )
			{ cf.Flags &= ~CF_INITTOLOGFONTSTRUCT; }

		//////////////////////////////////////////
		//	選択可能フォントリストに関する設定	//
		//////////////////////////////////////////
		//	排他関係に注意する事				//

		// 文字セット	//////////////////////////

		// Windows文字セットのみにする
		void UseAnsiOnly( )
			{ cf.Flags |= CF_ANSIONLY; }

		// TrueTypeのみを選択可能にする
		void UseTTOnly( )
			{ cf.Flags |= CF_TTONLY; }

		// 固定ピッチフォントのみを使用する
		void UseFixedPitchOnly( )
			{ cf.Flags |= CF_FIXEDPITCHONLY; }

		// 可変ピッチフォントのみを使用する
		void UseScalableOnly( )
			{ cf.Flags |= CF_SCALABLEONLY; }

		// ベクターフォントを使用しない
		void NoVectorFonts( )
			{ cf.Flags |= CF_NOVECTORFONTS; }
		void NoOEMFonts( )
			{ cf.Flags |= CF_NOOEMFONTS; }

		// デバイス	///////////////////////////////

		// スクリーンフォントのみを選択可能にする
		void UseScreenFonts( )
			{ cf.Flags |= CF_SCREENFONTS; }

		// プリンターフォントのみ利用する
		void UsePrinterFontsOnly( HDC hdc );

		// プリンターとスクリーンフォントの両方を利用する
		void UseBoth( HDC hdc );

		// プリンターとスクリーンの両方で利用可能なフォントのみを列挙
		//	（この場合、固定ピッチフォントは列挙されない）
		void UseWYSIWYG( HDC hdc );

		// 文字サイズ	////////////////////////////

		// 選択可能なフォントのポイントサイズの範囲を指定
		void SetLimitSize( int min, int max );

		// 装飾	////////////////////////////////////

		// 打ち消しせん、下線、色の効果を選択可能にする
		void UseEffects( )
			{ cf.Flags |= CF_EFFECTS; }


		//////////////////////////////////////////
		//	ユーザインターフェースに関する設定	//
		//////////////////////////////////////////

		void UseHelp( )		// Helpを使用する場合
			{ cf.Flags |= CF_SHOWHELP; }
							// ここではフラグをセットするだけ
							// RegisterWindowMessage( HELPMSGSTRING );
							// の返り値（＝ウィンドウメッセージ）を処理するルーチンを
							// このクラスの親ウィンドウに追加する必要がある

		// テンプレートを使用する場合
		void SetTemplate( LPCTSTR lpTemplateName );
		void SetTemplate( HINSTANCE hInstance );

		// フック関数
		void SetHook( UINT (CALLBACK *lpfnHook)(HWND, UINT, WPARAM, LPARAM), LPARAM lCustData=NULL );
		// UINT (CALLBACK *lpfnHook)(HWND, UINT, WPARAM, LPARAM)

		// 文字飾りボタンを有効にする
		//	・フック関数を用意し、フック関数で WM_COMMAND,wParam=psh3を監視する必要がある
		void SetApply();

		//////////////////////////////////////////
		//	終了時に取得可能な関数				//
		//////////////////////////////////////////

		// 選択されたフォントのポイントサイズの取得（1/10ポイント単位）
		int GetPointSize( );
};



#endif	// __COMFONT_H
