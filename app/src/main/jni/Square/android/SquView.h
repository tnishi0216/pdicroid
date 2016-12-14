// for Android
#ifndef SquViewH
#define SquViewH

#include "wsqudef.h"
#include "wsqubase.h"

#include "SquItemView.h"

class TSquareView
{
#if 0
	TTntScrollBar *ScrBar;
	TTntPaintBox *PaintBox;
	void PaintBoxPaint(TObject *Sender);
	void PaintBoxMouseDown(TObject *Sender, TMouseButton Button,
		  TShiftState Shift, int X, int Y);
	void PaintBoxMouseMove(TObject *Sender, TShiftState Shift,
		  int X, int Y);
	void PaintBoxMouseUp(TObject *Sender, TMouseButton Button,
		  TShiftState Shift, int X, int Y);
	void FrameResize(TObject *Sender);
	void ScrBarScroll(TObject *Sender, TScrollCode ScrollCode,
		  int &ScrollPos);
	void PaintBoxDblClick(TObject *Sender);
#endif
private:	// ユーザー宣言
//	enum {
//		UM_NOTIFY_DBLCLK = (WM_USER+0x0400)
//	};
	class TSquareIF *squi;
	class TSquare *squ;
//	class TNPopupMenu *PopupMenu;

public:		// ユーザー宣言
	TSquareView(/*TComponent* Owner,*/ TSquareIF *squ);
	virtual ~TSquareView();
	void SetInterface(TSquareIF *squ);

	// View //
	HDC hdc;
	int hdcRef;
	
public:
	// Basic //
	void Setup( HDC _hdc );
	void SetOrg();
	int GetViewWidth();
	int GetViewHeight();
	HDC GetDC();
	bool ReleaseDC(HDC);
	bool IsValidDC()
		{ return hdc != NULL; }
	void Invalidate();

	// Cursor //
	void SetCursor(const tchar *name);

	// Fonts and Colors //
	void ResetBackColor(COLORREF backcolor);

	// Item View //
	//void SetItemWidth(int type, int width);

	// Hint(Tool Tip) //
	void SetHintText(const tchar *text);
	
	// Page control //
	//int GetLastPageOffset( );

	// Scroller //
	int GetScrollPos(int bar);
	void SetScrollPos(int bar, int pos, bool redraw);
	void SetScrollRange(int bar, int minpos, int maxpos, bool redraw);
	void ShowScrollBar(int bar, bool show);
	void Scroll(int dx, int dy, bool repaint=false);

	// Popup Menu //
	void Popup(HMENU hMenu, POINT pt);
	void JLinkPopupMenu(class JLink &jl, POINT pt);
	void StdPopupMenu(int index, POINT pt);
	void SetupMenu( HMENU hMenu, int inx, BOOL levelmenu );
	HMENU BuildPopupMenu( HMENU hSubMenu1, int inx, POINT pt );
	void SetupPlayVoiceMenu( HMENU hm );

#if 0
	// Popup Window //
	void EvMenuKeyDown(TMessage &msg);
	void EvMenuKeyDown2(TMessage &msg);

	// Mouse/Key //
	void EvNotifyDblClk(TMessage &msg);
protected:
	HINSTANCE GetInstance()
		{ return MainInstance; }
	//virtual void WndProc(TMessage &msg);
	void EvTimer(TMessage &msg);
#endif
};
//---------------------------------------------------------------------------
extern PACKAGE TSquareView *SquareView;
//---------------------------------------------------------------------------
#endif

