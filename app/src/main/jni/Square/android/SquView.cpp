#include "pdclass.h"
#pragma hdrstop
#include "SquView.h"
#include "SquInterface.h"

TSquareView::TSquareView(/*TComponent* Owner,*/ TSquareIF *squint)
{
	squi = squint;
	squ = squint?squint->GetSquare():NULL;
//	PopupMenu = NULL;
	hdc = NULL;
	hdcRef = 0;
}
TSquareView::~TSquareView()
{
//	if (PopupMenu)
//		delete PopupMenu;
}
void TSquareView::SetInterface(TSquareIF *squi)
{
	this->squi = squi;
	this->squ = squi->GetSquare();
}
void TSquareView::SetScrollPos(int bar, int pos, bool redraw)
{

}
void TSquareView::SetScrollRange(int bar, int minpos, int maxpos, bool redraw)
{

}
void TSquareView::ShowScrollBar(int bar, bool show)
{

}
void TSquareView::SetCursor(const tchar *name)
{
}
HDC TSquareView::GetDC()
{
	return NULL;
}
void TSquareView::Scroll(int dx, int dy, bool repaint)
{

}
bool TSquareView::ReleaseDC(HDC _hdc)
{
	return false;
}
void TSquareView::Invalidate()
{

}
int TSquareView::GetScrollPos(int bar)
{
	return 0;
}
