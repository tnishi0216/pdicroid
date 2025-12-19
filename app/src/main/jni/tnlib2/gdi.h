#ifndef	__GDI_H
#define	__GDI_H

#include "tndefs.h"

class LogFont {
	public:
    	LOGFONT lf;
    public:
    	LogFont( );
        void clear( void );
        void SetPoint( HDC hdc, int pt );
		void SetName( const tchar *str );
		operator LOGFONT &() { return lf; }
		operator LOGFONT *() { return &lf; }
};

class GDI {
	private:
		HPEN hpen;
		HBRUSH hbrush;
		HFONT hfont;
		HBITMAP hbmp;
		HBITMAP hbmpOld;
		int savehdc;	//save,restore用
	public:
		HDC hdc;
	public:
		GDI();
		~GDI();
#ifndef WINCE
		void SetMapping( int mode, int wx, int wy, int vx, int vy, int vox, int voy );
		void SetMapping( int mode )
			{ SetMapMode( hdc, mode ); }
#endif
		virtual BOOL open( HDC _hdc );
		HDC GetDC( HWND );		// GetDC()によってオープン
		BOOL ReleaseDC( HWND );
		virtual HDC close( );
		void save( void );			//HDCのセーブ
		void restore();
		void ResetObjects( void );	//BLACK_PEN,WHITE_BRUSH,SYSTEM_FONTに戻す
		void ResetDraw( void );		//BLACK_PEN,WHITE_BRUSHに戻す
		void ResetFont( void );
		void Select( LogFont &lf );
		void DeleteObjects( );	//hpen, hbrushの削除
		//以下の関数に渡したハンドルは後で削除されます！！
		//それがいやなときはこれらの関数を呼んだ直後に保存したハンドルを0にする
		void Select( HPEN _hpen );
		void Select( HBRUSH _hbrush );
		void Select( HFONT _hfont );
		void Select( HBITMAP hbmp );
	public:
		// GDI置き換え関数
#ifdef WIN32
		DWORD GetTextExtent( LPCTSTR str )
		{
			SIZE size;
			GetTextExtentPoint32( hdc, str, lstrlen( str ), &size );
			return size.cx;
		}
#else
		DWORD GetTextExtent( const tchar *str )
			{ return ::GetTextExtent( hdc, str, lstrlen( str ) ); }
#endif
#ifndef WINCE
		void TextOut( int x, int y, const tchar *str )
			{ ::TextOut( hdc, x, y, str, lstrlen( str ) ); }
#endif
		BOOL Ellipse( int left, int top, int right, int bottom )
			{ return ::Ellipse( hdc, left, top, right, bottom ); }
		BOOL PatBlt( int left, int top, int width, int height, DWORD rop )
			{ return ::PatBlt( hdc, left, top, width, height, rop ); }

		COLORREF SetTextColor( COLORREF ref )
			{ return ::SetTextColor( hdc, ref ); }
		COLORREF SetBkColor( COLORREF ref )
			{ return ::SetBkColor( hdc, ref ); }
};

// このクラスは怪しい？
class TextGDI : public GDI {
	protected:
	public:
		BOOL open( HDC _hdc );
		HDC close( )
			{ return GDI::close(); }	// 前は、{}になっていた？？？
#ifndef WINCE
		void put( int x, int y, LPTSTR str )	{TextOut( x, y, (LPTSTR)str);}
#endif
		void align( );
};

//行単位出力
class LineTextGDI : public TextGDI {
	protected:
		int x0, y0;
		int wx, hy;
	public:
		BOOL open( HDC _hdc );
		void put( int x, int y, LPCSTR str );
};

class GraphGDI : public GDI {
};

class BitmapGDI {
};


#endif	// __GDI_H

