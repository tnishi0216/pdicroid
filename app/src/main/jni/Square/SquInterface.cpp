//---------------------------------------------------------------------------
#include "pdclass.h"
#pragma hdrstop

#include "SquInterface.h"
#include "SquView.h"
#include "SquFrm.h"

#include "winsqu.h"

#include "id.h"	// for CM1_COPY

//---------------------------------------------------------------------------

#pragma package(smart_init)

TSquareIF::TSquareIF(TSquareFrame *frame, TSquareView *view)
{
	Frame = frame;
	View = view;
	squ = new TSquare(Frame, view, frame->ItemView);
}
TSquareIF::~TSquareIF()
{
	if (squ)
		delete squ;
}

void TSquareIF::SetUIMain(class TSquUIMain *uimain)
{
	squ->SetUIMain(uimain);
}

void TSquareIF::Setup()
{
#ifdef _Windows
	squ->Setup(View->PaintBox->Canvas->Handle);
	squ->SetOrg(View->Width, View->Height);
#endif
}
void TSquareIF::SetupView()
{
#ifdef _Windows
	squ->SetupView(View->PaintBox->Canvas->Handle);
#endif
}
void TSquareIF::SetHWindow(HWND hwnd)
{
	squ->SetHWindow(hwnd);
}
void TSquareIF::LoadProfile()
{
	squ->LoadProfile();
}

void TSquareIF::SetOrg()
{
#ifdef _Windows
	squ->SetOrg(View->Width, View->Height);
#endif
}
class DicGroup &TSquareIF::GetDicGroup()
{
	return squ->GetDicGroup();
}
void TSquareIF::SetDicGroup(class DicGroup &dg)
{
	squ->SetDicGroup(dg);
}
class MPdic *TSquareIF::GetDic()
{
	return squ->GetDic();
}
void TSquareIF::Open()
{
	squ->Open();
}
void TSquareIF::Close()
{
	squ->Close();
}
void TSquareIF::TempClose()
{
	squ->TempClose();
}
void TSquareIF::Reopen()
{
	squ->Reopen();
}
int TSquareIF::ODACloseReq()
{
	return squ->ODACloseReq();
}
bool TSquareIF::CanClose(bool withmsg)
{
	if (withmsg)
		return squ->CanCloseMessage();
	else
		return squ->CanClose();
}
bool TSquareIF::IsTempClosed()
{
	return squ->IsTempClosed();
}
bool TSquareIF::IsDicOpened()
{
	return squ->IsDicOpenedStable();
}

int TSquareIF::OpenDictionary(const class DicGroup &dg, DicNames *dicnames, int *dicno)
{
	return squ->OpenDictionary(dg, dicnames, dicno);
}
void TSquareIF::SaveDicProfiles( class TRegKey *section, TRegKey *common)
{
	squ->SaveDicProfiles(section, common);
}
void TSquareIF::LoadDicProfiles( TRegKey *section, TRegKey *common )
{
	squ->LoadDicProfiles(section, common );
}
void TSquareIF::Clear()
{
	squ->Clear();
}
void TSquareIF::Reset()
{
	squ->Reset();
}
void TSquareIF::Escape()
{
	squ->Escape();
}
void TSquareIF::ResetSearch()
{
	squ->ResetSearch();
}
void TSquareIF::StopSearch()
{
	squ->StopSearch();
}
void TSquareIF::EndSearch()
{
	squ->EndSearch();
}
int TSquareIF::IncSearch(const tchar*str, diclist_t diclist )
{
	return squ->incsearch(str, diclist);
}
bool TSquareIF::RequestScroll(int offset, bool backward)
{
	return squ->Request(offset, backward);
}
void TSquareIF::StartExtSearch()
{
	squ->StartExtSearch();
}
void TSquareIF::CancelExtSearch()
{
	squ->CancelExtSearch();
}
void TSquareIF::SetUpDown(bool f)
{
	squ->SetUpDown(f);
}
void TSquareIF::clsStar()
{
	squ->clsStar();
}
int TSquareIF::GetCur()
{
	return squ->GetCur();
}
bool TSquareIF::IsCurOn()
{
	return squ->IsCurOn();
}
bool TSquareIF::IsCurRight()
{
	return squ->IsCurRight();
}
bool TSquareIF::CanExecuteObject()
{
	return squ->CanExecuteObject();
}
bool TSquareIF::EditObject()
{
	return squ->EditObject();
}
bool TSquareIF::JumpHyperLink(bool test)
{
	return squ->cmdJumpHyperLink(test);
}
bool TSquareIF::IsHideSearching()
{
	return squ->IsHideSearching();
}
bool TSquareIF::IsSearching()
{
	return squ->IsSearching();
}
bool TSquareIF::IsIncSearch()
{
	return squ->IsIncSearch();
}

// Message //
const tchar *TSquareIF::GetMessage(int *pasttime)
{
	return squ->GetMessage(pasttime);
}

// View //
void TSquareIF::Paint(HDC hdc, RECT &rc)
{
	squ->Paint(hdc, rc);
}
void TSquareIF::ForceDraw()
{
	squ->ForceDraw();
}
void TSquareIF::DispStar(int cur)
{
	squ->dispStar(cur);
}
void TSquareIF::MoveStarLocAbs(int index)
{
	squ->MoveStarLocAbs(index);
}
void TSquareIF::SetIndexOffset(int index)
{
	squ->SetIndexOffset(index);
}
bool TSquareIF::IsViewOneLine()
{
	return squ->IsViewOneLine();
}
void TSquareIF::SetViewOneLine(bool oneline)
{
	squ->SetViewOneLine(oneline);
}
void TSquareIF::ResetFont()
{
	squ->ResetFont();
}
void TSquareIF::ResetColor()
{
	squ->ResetColor();
}
void TSquareIF::DispStar(const tchar *word, int dicno)
{
	squ->dispStar(word, dicno);
}
void TSquareIF::Invalidate()
{
	squ->Invalidate();
}
void TSquareIF::InvalidateLines()
{
	squ->InvalidateLines();
}
bool TSquareIF::IsSelected()
{
	return squ->IsSelected();
}
void TSquareIF::ChangeMaxDispLevel(int offset)
{
	squ->ChangeMaxDispLevel(offset);
}
int TSquareIF::GetFLIconic() const
{
	return squ->GetFLIconic();
}
void TSquareIF::SetFLIconic(int index)
{
	squ->SetFLIconic(index);
}
void TSquareIF::NotifyItemViewChanged()
{
	squ->NotifyItemViewChanged();
}
bool TSquareIF::CanDispDicName() const
{
	return squ->CanDispDicName();
}

// Commands //
void TSquareIF::ToggleAttr(uchar bit)
{
	squ->ToggleAttr(bit);
}
void TSquareIF::ToggleAttr(const tchar *word, uchar bit)
{
	squ->ToggleAttr(word, bit);
}
void TSquareIF::ChangeAttr(const tchar *word, int attr, bool f)
{
	squ->ChangeAttr(word, attr, f);
}
#if 0
void TSquareIF::ChangeAttr(wa_t bit)
{
	squ->cmdChangeAttr(bit);
}
#endif
void TSquareIF::DeleteWord()
{
	squ->cmdDeleteWord();
}
void TSquareIF::Copy(int id)
{
	squ->cmdCopy(id);
}
void TSquareIF::PlayTTS()
{
	squ->cmdPlayTTS();
}

#if 0
//---------------------------------------------------------------------------
//	Event Handlers
//---------------------------------------------------------------------------
LRESULT TSquareIF::EvUpdate(TMessage msg)
{
	return squ->UmUpdate(msg.WParam, msg.LParam);
}
LRESULT TSquareIF::EvMenuSelect(TMessage msg)
{
	return squ->UmMenuSelect(msg.WParam, msg.LParam);
}
LRESULT TSquareIF::EvMenuKeyDown(TMessage msg)
{
	return squ->UmMenuKeyDown(msg.WParam, msg.LParam);
}
LRESULT TSquareIF::EvMenuKeyDown2(TMessage msg)
{
	return squ->UmMenuKeyDown2(msg.WParam, msg.LParam);
}
LRESULT TSquareIF::EvHyperLink(TMessage msg)
{
	return squ->UmHyperLink(msg.WParam, msg.LParam);
}
void TSquareIF::EvActivate(bool active)
{
	squ->EvActivate(active);
}
#endif
//---------------------------------------------------------------------------
//	Key operation
//---------------------------------------------------------------------------
void TSquareIF::KeyScrollUp( )
{
	squ->KeyScrollUp();
}
void TSquareIF::KeyScrollDown( )
{
	squ->KeyScrollDown();
}
void TSquareIF::PrevPage()
{
	squ->PrevPage();
}
void TSquareIF::NextPage()
{
	squ->NextPage();
}
void TSquareIF::HomePage()
{
	squ->HomePage();
}
void TSquareIF::EndPage()
{
	squ->EndPage();
}
int TSquareIF::GetNumWords()
{
	return squ->GetNumWords();
}
const tchar *TSquareIF::GetWord()
{
	return squ->GetWord();
}
int TSquareIF::GetReverseStatus()
{
	return squ->GetReverseStatus();
}
// Scroll //
int TSquareIF::ScrollDown(bool fClsRegion, bool down)
{
	return squ->ScrollDown(fClsRegion, down);
}
int TSquareIF::ScrollDown1(bool stopatoboom)
{
	return squ->ScrollDown1(stopatoboom);
}
int TSquareIF::ScrollUp(bool fClsRegion)
{
	return squ->ScrollUp(fClsRegion);
}
int TSquareIF::LineScrollUp()
{
	return squ->LineScrollUp();
}
int TSquareIF::LineScrollDown()
{
	return squ->LineScrollDown();
}
int TSquareIF::MicroScrollUp(int offs)
{
	return squ->MicroScrollUp(offs);
}
// Search //
int TSquareIF::Search( const tchar *str, SrchMode mode, diclist_t diclist, GenericRexp *grexp, SrchOut *srchout, int level1, int level2, int MemOnOff )
{
	return squ->search(str, mode, diclist, grexp, srchout, level1, level2, MemOnOff);
}
int TSquareIF::SearchHistory( TSearchHistoryParam &shp )
{
	return squ->SearchHistory(shp);
}
void TSquareIF::SearchFast(const tchar *str, SrchMode mode, diclist_t diclist)
{
	squ->SearchFast(str, mode, diclist );
}
void TSquareIF::StartSpecialSearch(const tchar *str)
{
	squ->StartSpecialSearch(str);
}
// Start special search in background.
void TSquareIF::StartSubSearch(const tchar *str)
{
	squ->StartSubSearch(str);
}
void TSquareIF::StopSubSearch()
{
	squ->StopSubSearch();
}
void TSquareIF::SelectLinkObject( int i )
{
	squ->SelectLinkObject(i);
}
void TSquareIF::cmdBatchDeleteWord()
{
	squ->cmdBatchDeleteWord();
}
void TSquareIF::CopySelection()
{
	squ->cmdEdit(CM1_COPY);
}
void TSquareIF::LButtonDown(WPARAM wParam, LPARAM lParam)
{
#if 0
	POINT pt;
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	squ->EvLButtonDown(wParam, pt);
#endif
}
void TSquareIF::MouseMove(WPARAM wParam, LPARAM lParam)
{
#if 0
	POINT pt;
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	squ->EvMouseMove(wParam, pt);
#endif
}
void TSquareIF::LButtonUp(WPARAM wParam, LPARAM lParam)
{
#if 0
	POINT pt;
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	squ->EvLButtonUp(wParam, pt);
#endif
}
void TSquareIF::Timer(UINT id)
{
	squ->EvTimer(id);
}
void TSquareIF::CloseAllPopup()
{
#ifdef USE_PS
	squ->CloseAllPopup();
#endif
}
TPopupSearch *TSquareIF::GetPS()
{
#ifdef USE_PS
	return squ->GetPS();
#else
	return NULL;
#endif
}
void TSquareIF::SetPS(TPopupSearch *ps)
{
	return squ->SetPS(ps);
}
bool TSquareIF::IsPopupOpened() const
{
	return squ->_IsPopupOpened();
}
bool TSquareIF::IdleProc()
{
	return squ->IdleProc();
}

bool TSquareIF::StartAutoLink()
{
	//return squ->StartAutoLink();
	return false;
}

void TSquareIF::SetDeleteFile( const tchar *filename)
{
	squ->SetDeleteFile(filename);
}

//---------------------------------------------------------------------------
// History
//---------------------------------------------------------------------------
void TSquareIF::MoveHistory( bool forward )
{
	squ->MoveHistory(forward);
}
bool TSquareIF::CanMoveHistory( bool forward )
{
	return squ->CanMoveHistory(forward);
}

//---------------------------------------------------------------------------
// Debug
//---------------------------------------------------------------------------
void TSquareIF::dump_pool()
{
	squ->dump_pool();
}

