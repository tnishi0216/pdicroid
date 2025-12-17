#include "pdclass.h"
#pragma hdrstop
#include "SquFrm.h"
#include "WinSquUI.h"

#include	"multidic.h"
#include	"winsqu.h"
#include	"wpdcom.h"
#include	"winmsg.h"
#include "msgbox.h"
#include "pdprof.h"
#include "wsview.h"
#include "textext.h"
#include "MouseCapture.h"
#include "SrchMed.h"
#ifdef OLE2
#include	"ole2s.h"
#include	"pdtdd.h"
#include "o2if.h"
#endif
#ifdef _Windows
#include "PopupSrch.h"
#endif

#ifdef _Windows
// for InetDicUIMan
#include "UIMain.h"
#include "InetDicUIMan.h"
#endif

// for dictionary //
#include "dicgrp.h"

#ifdef _Windows
#include "DicCvtWizard.h"
#endif

#define	DEF_INCSRCHPLUS		false	// default value of the incremental search plus.

#define	USE_STOP		0

#ifdef __ANDROID__
class PoolViewer : public IPoolViewer {
protected:
	TSquareFrame *Frame;	// ref
public:
	PoolViewer(TSquareFrame *frame)
		:Frame(frame)
	{}
	virtual void Clear()
	{
		Frame->ListClear();
	}
	virtual void Add(tnstr *word, Japa *japa, int dicno, int level)
	{
		Frame->ListAdd(word, japa, dicno, level);
	}
	virtual void Insert(int index, tnstr *word, Japa *japa, int dicno, int level)
	{
		Frame->ListInsert(index, word, japa, dicno, level);
	}
	virtual void Del(int index)
	{
		Frame->ListDel(index);
	}
	virtual void Del(int index1, int index2)
	{
		Frame->ListDel(index1, index2);
	}
};
#endif

Squre::Squre(class TSquareFrame*frame, class TSquareView *view, TSquItemView &itemView)
	:Frame(frame)
	,View(view)
	,ItemView(itemView)
#ifdef _Windows
	,eolist(frame->GetEditObjList())
#endif
	,Dic(*new MPdic)
#ifdef __ANDROID__
	,pool(new PoolViewer(frame))
#endif
{
#ifdef USE_SINGLEBYTE
	for ( int i=0;i<N_SECTION;i++ ){
//		fontsASC[ i ] = NULL;
		fSingleByte[ i ] = false;
	}
	fSingleByte[ SN_PRON ] = true;	// 固定
	fSingleByte[ SN_EXP1 ] = false;
#endif

	cury = -1;
//	dicmode = MD_MERGE;
	srchout = NULL;
	BaseNum = 0L;
//	mousetimer = 0;

#if !NEWINCSRCH
	NotFoundCount = FoundCount = 0;
#endif
	SrchMed = NULL;
	SrchParam = NULL;
	SrchParams = NULL;
	bAllowDisp = false;
	AllowDispMode = false;
	ForceUpdate = false;
	SetUpDown(false);

	ExtSrchThreadKey = -1;
	
	fUpdateDisp = false;

#ifndef SMALL
	fLongBar = false;
	JMerge = true;
	PronCR = false;
	_HtmlEnabled = SQM_ALL & ~(SQM_PRON);
#endif
#ifdef LIGHT
	_DispDicName = true;
#else
	_DispDicName = false;
#endif

	PastTime = -1;
	//_FoundCount = -1;

	MouseCap = new TMouseCapture(this);

	hdc = NULL;

#if USE_DISPLEVEL
	MaxDispLevel = 0;
#endif

#ifndef SMALL
	FLIconic = FL_CONTENT;
#endif

	fRecalcLine = false;

#if USE_ASSOCMENU
	AssocMenu = NULL;
#endif

#ifdef OLE2
//	ddStart = 0;
	fDragging = false;
	ddTarget = NULL;
#endif
#ifdef USE_JLINK
	LastMoveIndex = -1;
	LastMoveObjIndex = -1;
#endif
#if USE_DT2
	CurHist = 0;

	fAutoLinkCapture = false;
//	alItem = -1;
//	alPopup = NULL;
//	alJLinks = NULL;
//	alHold = false;
#endif
#if USE_DT2 || USE_VIEWAUTOPOP
	ps = NULL;
#endif
#if SQUONELINE
	ViewOneLine = 
#ifdef SMALL
		true;
#else
		false;
#endif
#endif	// SQUONELINE
#if USE_SLASH
	ConcatExp = false;
#endif
#if INCSRCHPLUS
	ISPWaiting = false;
	IncSrchPlus = DEF_INCSRCHPLUS;
#endif

	// sub search //
	sub_words = NULL;
	sub_srchparams = NULL;
	sub_srchparam = NULL;

	// Dictionary //
	dicGroup = new DicGroup;
	bTempClose = false;
	InetDicMan = NULL;
	Opening = false;
	Closing = true;	// closing or closed

	InitView();
	LastVRange = 0;
	
	// UI setup //
	UIWord = Frame->GetUIWord();
	UIMain = Frame->GetUIMain();
}

Squre::~Squre()
{
	if (SrchParams)
		delete SrchParams;
	if (sub_srchparams)
		delete sub_srchparams;
	if (sub_words)
		delete sub_words;
	while (DelFiles.size()){
		DeleteFile(DelFiles[0]);
		DelFiles.del(0);
	}
	if (dicGroup)
		delete dicGroup;
	if (&Dic)
		delete &Dic;
#if INETDIC
	if (InetDicMan)
		delete InetDicMan;
#endif
	if (MouseCap)
		delete MouseCap;
	if (SrchMed)
		delete SrchMed;
}

void Squre::SetUIMain(TSquUIMain *uimain)
{
	UIWord = NULL;
	UIMain = uimain;
}

#if INCSRCHPLUS
void Squre::SetIncSrchPlus( bool enabled )
{
	IncSrchPlus = enabled;
	prof.WriteInteger( PFS_COMMON, PFS_INCSRCHPLUS, IncSrchPlus );
}
#endif

void Squre::Setup( HDC _hdc )
{
	hdc = _hdc;
	SetupView(_hdc);
	hdc = NULL;

	EnableVScroll( false );
	SetHRange( 0, false );
#if !defined(SMALL)
	tnstr str = prof.ReadString( PFS_COMMON, PFS_TEXTEXT, TextExtension::DefTextExts );
	textext.Set(str);
#endif

	KeepStarTop = prof.ReadInteger(PFS_COMMON, PFS_KEEPTOP, KeepStarTop);
}

// ウィンドウのサイズが変わったときに呼ぶ
// 必ずSetup()が終わってから呼ぶこと
void Squre::SetOrg( int lx, int ly )
{
	SetOrg();
}

// InitTarget/UninitTarget - single instanceにしか対応していない
void Squre::Activate()
{
#ifdef USE_OLE
	if (prof.IsOleFull()){
		InitTarget( );	// It requires depu.dll. 何とかならんものか。。。
	}
	OleItem::SetCallback( GetWHandle() );
#endif
	squfont::HtmlEnabled = IsHtmlEnabled();
	squfont::CanDispDicName = CanDispDicName();
#if INETDIC
	if (InetDicMan)
		InetDicMan->Activate();
#endif
}
void Squre::_Deactivate()
{
#if INETDIC
	if (InetDicMan)
		InetDicMan->Deactivate();
#endif
	StopISPTimer();
	MouseCap->Deactivate();
#if USE_DT2
	CloseAllPopup();
#endif
#ifdef USE_OLE
	FinishTarget();
#endif
}

void Squre::SetHtmlEnabled(int enabled)
{
	squfont::HtmlEnabled = _HtmlEnabled = enabled;
}
void Squre::SetDispDicName(bool on)
{
	squfont::CanDispDicName = _DispDicName = on;
}

void Squre::_SetupTextFonts( HDC hdc, TFontAttr &fa, int &cxMax, int &cyMax, int type )
{
	int cx, cy;
	int charset;
	GetTextSize( hdc, fa, cx, cy, charset );
	cxMax = min( cx, cxMax );	// 一番小さいサイズを得る
	cyMax = min( cy, cyMax );
#ifdef USE_SINGLEBYTE
	fSingleByte[ type ] = charset & DEFAULT_CHARSET ? false : true;
#endif
#ifndef NOSQUCOLOR
//	LinkColors[type] = GetLinkColor( type );
#endif
}

class MPdic *Squre::GetDic()
	{ return Dic.GetDicNum()>0 ? &Dic : NULL; }

// 初期状態からTempClose状態にする
void Squre::SetDicGroup(DicGroup &dg)
{
	__assert(!bAllowDisp && !bTempClose);	// これ以外の条件では対応していない
	Open();
	TempClose();
	dicGroup->Copy(dg);
}

// 辞書open時に呼ばれる
void Squre::Open( )
{
	EnableVScroll( true );
	bAllowDisp = true;
	SetJMerge();
	bTempClose = false;
	pool.SetKCodeTrans(Dic.GetKCodeTrans());
	__assert(!sub_words);
	sub_words = new TSubWordItems(Dic.GetKCodeTrans());
#if USE_STOP
	SrchParam = Dic.CreateSearchParam();
#endif
}

void Squre::OpenSrchMed()
{
	if (!SrchMed)
		SrchMed = new TSearchMediator(this, Dic, ss, sub_ss, pool, *sub_words, sub_words_map);
}

void Squre::Close( )
{
	bTempClose = false;
	Closing = true;
	CancelExtSearch();
	EndSearch( );
	clear( );
	EnableVScroll( false );
	bAllowDisp = false;

	CloseCommon();
}

void Squre::TempClose( )
{
	bTempClose = true;
	Closing = true;
	CancelExtSearch();
	Clear();	// OLEの関係でクリアしないと駄目
	StopSearch();
	//PauseSearch(); // 2003.6.29 StopとPauseどちらがいい？

	CloseCommon();
}

//Note:
// ネット共有辞書がある場合、Windows message processに入ってしまうため、
// 他のmethodの呼び出しが出きてしまう。
// しかし、検索などのmethodの多くは再入に対応していないため、
// 呼び出し側で再入防止策が必要
void Squre::CloseCommon()
{
#if defined(DISPDICNAME) && DDN_DEFAULT
	Japa::DispDicName = 
	JLink::DispDicName = false;
#endif
	if (SrchParams){
		delete SrchParams;
		SrchParams = NULL;
	}
#if USE_STOP
	if (SrchParam){
		delete SrchParam;
		SrchParam = NULL;
	}
#else
	SrchParam = NULL;
#endif
	ClearSubSearch();
	if (sub_srchparams){
		delete sub_srchparams;
		sub_srchparams = NULL;
	}
	sub_srchparam = NULL;
	if (sub_words){
		delete sub_words;
		sub_words = NULL;
	}
	ClosePopup();	// 2008.4.1 added
	CloseDictionary();
}

void Squre::Reopen( )
{
	SetJMerge();
	//ss.ContinueSearch( );	// 2003.6.29 上記PauseSearch()とペア
	bTempClose = false;
}
// return : the number of close request posted.
int Squre::ODACloseReq()
{
	if (!IsDicOpened() || IsSearching())
		return 0;

	return Dic.ODAClose( );
}
bool Squre::CanClose( )
{
	if (Opening)
		return false;
#ifdef USE_OLE
#ifdef MACCS
	if ( !Dic.CanClose() )
		return false;
#endif
	eolist.FreeObjects( );
	return eolist.get_num() ? false : true;
#else
	return true;
#endif
}
bool Squre::CanCloseMessage( )
{
	if (Opening)
		return false;
#ifdef USE_OLE
	while ( 1 ){
		if ( !CanClose( ) ){
			if ( MessageBox( HWindow, _LT(MSG_EXISTEDITOBJECT), MsgError, MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION ) == IDYES ){
				eolist.clear( );
				continue;
			}
			return false;
		}
		break;
	}
#endif
	return true;

}

#include "Dicproc.h"
#include "filestr.h"
#include "msgdlg.h"
#include "id.h"
#include "browse.h"
#ifdef _Windows
#include "DicCreateDlg.h"
#endif

int Squre::OpenDictionary(const DicGroup &dg, DicNames *dicnamesPtr, int *dicno)
{
	Closing = false;
	Opening = true;
jretry:;
	DicNames dicnamesAuto;
	if (!dicnamesPtr){
		if (!dicgrp.Read( dg, dicnamesAuto )){
			__assert__;
		}
	}
	DicNames &dicnames = dicnamesPtr ? *dicnamesPtr : dicnamesAuto;
	dicGroup->Copy(dg);

	int r;
	int errflag = 0;

	Clear( );

	Dic.AllClose();
	Hourglass( true );
	bool pathchanged = false;
	for ( int i=0;i<dicnames.get_num();i++ ){
		r = OpenDic( Dic, dicnames[i] );
		DBW("OpenDic: r=%d error=%d", r, Dic.GetErrorCode());
		if (r==0){
#if !defined(__ANDROID__)
			if (Dic[i].GetLangProcId()==0){
				// 旧方式 lang proc.
				Clear( );
				Dic.AllClose( );
				tnstr filename = dicnames[i].name;
				// Convert the old pdic dictionary with wizard.
				//txr: MSG_OldTypeDicErrorQ=%s\n古いタイプのPDIC用辞書です.新しい辞書に変換しますか？
				tnstr msg = tnsprintf(_LT(MSG_OldTypeDicErrorQ), filename.c_str());
				int ret = MsgBox(msg, _LT(MSG_DICFORMATERROR), MB_YESNO | MB_ICONQUESTION);
				if (ret==IDYES){
					tnstr newname;
					if (DicConvertWizard(GetControl(), filename, newname, true)){
						// changed.
						dicnames[i].name = newname;
						dicgrp.Write(dg, dicnames);
						// retry to open proc.
						goto jretry;	// retry again.
					}
				}
				break;	// break the open process loop.
			}
#endif	// !__ANDROID__
		} else
		if ( r ){
			int error = Dic.GetErrorCode();
			errflag = 1;
			if ( error == DICERR_OPEN_CREATE || error == DICERR_OPEN_NOCREATE ){	// open error
				tnstr filename = dicnames[i].name;
				tnstr fullpath;
				if (FindDic(filename, fullpath)){
					// found on another path.
					dicnames[i].SetDicName( fullpath );
					i--;
					pathchanged = true;
					continue;	// もう一度やり直し
				}
				tnstr msg = tnsprintf( _LT(MSG_NOTFINDDIC_WHICH), (const tchar *)filename );
				switch ( ::MessageDialog( GetWHandle(), _LT(CAP_DICNOTEXIST), msg, _LT(IDS_SEARCH_DIC), error==DICERR_OPEN_NOCREATE ? _LT(IDS_NEW_DIC0) : _LT(IDS_NEW_DIC), 0 ) ){
					case ID_BUTTON1:	// search dictionary
						filename = dicnames[i].GetDicName();
						while (1){
							if ( BrowseProc( GetWHandle(), DT_PDIC|DT_NODICLIST, filename, IDS_EDITFILENAME ) ){
								tnstr oldname = GetFileName(dicnames[i].GetDicName());
								if (!IsSameFileName(oldname, GetFileName(filename))){
									// filenameが一致しない
									msg = tnsprintf( _LT(MSG_Q_FILENAME_DIFF), oldname.c_str(), GetFileName(filename) );
									if (MsgBox(msg, _LT(IDS_EDITFILENAME), MB_YESNO|MB_DEFBUTTON2)!=IDYES)
										continue;
								}
								AbsToRel( filename );
								dicnames[i].name = filename;
								dicgrp.Write( dicgrp.GetSel(), dicnames );
								i--;
								goto jcontinue;
							}
							break;
						}
						break;
					jcontinue:;
						continue;
					case ID_BUTTON2:	// create dictionary
						if ( CreateNewDictionary( GetControl(), filename, false, &filename, true ) == 0 ){
							dicnames[i].name = filename;
							dicgrp.Write( dicgrp.GetSel(), dicnames );
							i--;
							continue;
						}
						break;
				}
			} else {
				Clear( );
				Dic.AllClose( );
#if !defined(__ANDROID__)
				if (error==DICERR_NOTPDIC){
					tnstr filename = dicnames[i].name;
					t_dictype dt = GetDicFileType(filename);
					if (dt==DT_PDIC_OLD_ANS||dt==DT_PDIC_OLD_UNI){
						// Convert the old pdic dictionary with wizard.
						tnstr msg = tnsprintf(_LT(TID_OLDPDIC_CONVERT), filename.c_str());
						int ret = MsgBox(msg, _LT(MSG_DICFORMATERROR), MB_YESNO | MB_ICONQUESTION);
						if (ret==IDYES){
							tnstr newname;
							if (DicConvertWizard(GetControl(), filename, newname)){
								// changed.
								dicnames[i].name = newname;
								dicgrp.Write(dg, dicnames);
								// retry to open proc.
								goto jretry;	// retry again.
							}
						}
						break;	// break the open process loop.
					}
					
				}
#endif	// !__ANDROID__
				if (dicno) *dicno = i;
				ErrorMessage( GetWHandle(), error, dicnames[i].name, MsgDicError );
			}
			break;	// オープン出来ない
		}
#ifdef USE_COMP
		Dic[i].SetCompFlag( dicnames[i].comp );
#endif
	}
	if ( pathchanged ){
		dicgrp.Write( dicgrp.GetSel(), dicnames );
	}
	if ( Dic.GetDicNum() ){
		if (Dic.GetDicNum()!=dicnames.get_num()){
			MsgBox(MSG_DicOpenSomeError, 0, MB_OK);
		}
#ifdef USE_NETWORK
		if ( dg.fNetwork )
			Dic.Flags |= MPF_NETWORK;
#endif
		Dic.SetLangProc( dicnames.LangProc );
#if 0
#ifndef SMALL
		if ( !dicgrphistfixed )
		{
			dicgrphist.Add( new HistoryArrayItem(dicgrp.GetSelGroupName()) );
			while ( dicgrphist.GetCount() > 2 ){
				dicgrphist.DeleteBottom();
			}
			if ( dicgrphist.GetCount() >= 2 ){
				prof.WriteString( PFS_COMMON, PFS_PREVDIC, dicgrphist[0].string );
			}
		}
#endif
#endif
#if INETDIC
		if (InetDicMan)
			delete InetDicMan;
		InetDicMan = TInetDicUIMan::CreateInstance(*TUIMain::GetInstance(), Dic);
#endif
		Open( );
#if 0	//TODO: 辞書グループごとの値にする？
		// 固定辞書番号の補正
		if ( nFixDicNo >= Dic.GetDicNum() ){
			nFixDicNo = 0;
		}
#endif
	} else {
		// オープンできないため終了する
		Clear();
		Close();
		errflag = -1;
		MsgBox(MSG_DicOpenNotOpened, 0, MB_OK);
	}
	Hourglass( false );
	Opening = false;
	return errflag;
}

void Squre::CloseDictionary()
{
	if (!IsDicOpened())
		return;
	Hourglass( true );
	Dic.AllClose( );
#if INETDIC
	if (InetDicMan){
		delete InetDicMan;
		InetDicMan = NULL;
	}
#endif
	Hourglass( false );
	if (SrchMed){
		delete SrchMed;
		SrchMed = NULL;
	}
}

bool Squre::IsDicOpened()
{
	return Dic.GetDicNum()>0;
}

// 完全にopenされて安定している状態
bool Squre::IsDicOpenedStable()
{
	return !Closing && IsDicOpened();
}

void Squre::SetJMerge()
{
#ifndef SMALL
	int mode = JMerge ? MD_MERGE : MD_NOTMERGE;
	Dic.SetMergeMode(mode);
#if USE_STOP
	if (SrchParam) SrchParam->SetMode(mode);
#else
	if (SrchParams) SrchParams->SetMode(mode);
#endif
	if (sub_srchparams) sub_srchparams->SetMode(mode);
#endif
#ifdef DISPDICNAME
#if DDN_DEFAULT
	squfont::CanDispDicName =
	JLink::DispDicName = 
	Japa::DispDicName = CanDispDicName();
#endif
	Japa::DicNameTemplate = prof.ReadString(PFS_COMMON, PFS_DICNAMETEMPLATE, NULL);
#endif
}

void Squre::clear( )
{
	clsStar();
	pool.Clear( );
	BaseNum = 0L;
	ClearLastIndex();
	if ( Dic.GetDicNum() ){
		SetVRange( 0 );
	}
	SetHRange( 0 );
	pagelines = 0;
	SetUpDown(false);
#if USE_DT2
	MouseCap->al.InvalidateIndex();
#endif
#ifdef USE_OLE
//	eolist.clear( );
	Dic.FreeObjects( );	// オブジェクトの解放
#endif
	ss.ClearSearchType();	// Android版で追加(2015.5.21) clearしてもそのあとIdleProc()の呼び出しで検索が継続してしまうため。何かの呼び出しが足りないのかもしれない。それが解決できれば削除OK
}

#if 0	// 誰も使っていない？
#if USE_VIEWAUTOPOP
void Squre::OpenAutoPop( int absindex, bool delay )
{
	if ( !ps ){
		ps = new POPUPSEARCH;
		ps->Flags |= PSF_FIXEDSIZE|PSF_NOACTIVATE;
		RECT rc;
		::GetWindowRect( HWindow, &rc );
		ps->orgpt.x = rc.left;
		ps->orgpt.y = rc.top;
		ps->Size.x = rc.right - rc.left - GetSystemMetrics(SM_CXVSCROLL);
		ps->Size.y = rc.bottom - rc.top;
	}
	ps->Open( GetWordAbs(absindex) );
}
#endif
#endif

void Squre::ClosePopup()
{
#ifdef USE_PS
	if (!ps)
		return;
	ps->Close();
	ps->Free(0);
	ps = NULL;
#endif
}

#ifdef USE_JLINK
// 現在のカーソル位置のfirst objectを調べる
// return value:
//	0 : none
//	1 : voice(JL_VOICE, JT_WAV,JT_MP3)
//	2 : other executable object
//	3 : other object
int Squre::GetFirstObjectType()
{
	if (cury<0 || GetJapa(cury).jlinks.get_num()==0)
		return 0;	// none
	JLink &jl = GetJapa(cury).jlinks[0];
	switch ( jl.GetType() ){
#ifdef JL_VOICE
		case JL_VOICE:
			return 1;
#endif
		case JL_FILE:
		case JL_FILEIMAGE:
			if (((JLFileCommon&)jl).IsAudio())
				return 1;
			// fall through
		default:
			if (jl.CanEdit())
				return 2;
			else
				return 3;
	}
}
#endif	// USE_JLINK
HWND Squre::GetPopupEdit()
{
	if (UIMain)
		return UIMain->GetPopupWindowEditHandle();
	else
		return NULL;
}

void Squre::SetPS(TPopupSearch *_ps)
{
#ifdef USE_PS
	if (ps){
		ps->Free(0);
	}
	ps = _ps;
#endif
}
bool Squre::_IsPopupOpened() const
{
#ifdef USE_PS
	return ps && ps->IsOpened();
#else
	return false;
#endif
}

#ifdef USE_DELAYED_DELFILE
// delaytime[mSec]後にfilenameを削除する
void Squre::SetDeleteFile( const tchar *filename )
{
	while (DelFiles.size()>=MaxDelFiles){
		DeleteFile(DelFiles[0]);
		DelFiles.del(0);
	}
	DelFiles.add(filename);
}
#endif

const tchar *Squre::GetCWord()
{
	const tchar *word = GetWord();
	return find_cword_pos(word);
}

void Squre::SetFLIconic(int index)
{
	if (FLIconic==index)
		return;
	FLIconic = index;
	Invalidate();
	InvalidateLines();
	SaveProfileView(NULL);
}

void Squre::NotifyItemViewChanged()
{
	Invalidate();
	InvalidateLines();
	SaveProfileView(NULL);
}

#ifndef SMALL
void Squre::SetMessage( const tchar *str, int pasttime, int count )
{
	if (!UIMain)
		return;
	Message = str;
	PastTime = pasttime;
	//_FoundCount = count;
	Frame->SetMessage((TSquare*)this, str, pasttime, count);
}
void Squre::SetMessage(int id, int pasttime, int count)
{
	return SetMessage(GetStateMessage(id), pasttime, count);
}
#endif
const tchar *Squre::GetMessage(int *pasttime)
{
	if (pasttime)
		*pasttime = PastTime;
	return Message;
}
int Squre::GetSrchMsgId() const
{
	return ss.IsSearching()?MSG_SEARCHING:sub_ss.IsSearching()?MSG_SUBSEARCHING:0;
}

int Squre::GetFoundCount()
{
	if (ss.IsSubSearch() && !(sub_ss.GetSearchState()&SS_FWD)){
		return sub_words->get_num();
	} else {
		if (ss.IsIncSearch())
			return -1;	// invalid
		if (get_num()<MAX_NDISP){
			return get_num();
		} else {
			if (IndexOffset>0 || (ss.GetSearchState()&SS_FWD)){
				return -MAX_NDISP;	// >MAX_NDISP
			} else {
				return MAX_NDISP;
			}
		}
	}
}

bool Squre::IsAutoSpecialSearchEnabled()
{
	return Frame->IsAutoSpecialSearchEnabled();
}

int Squre::MessageBox(const tchar *text, const tchar *caption, UINT type)
{
	return ::MsgBox(HWindow, text, caption, type);
}
int Squre::MessageBox(int text, int caption, UINT type)
{
	return ::MsgBox(HWindow, text, caption, type);
}
int Squre::MessageBox(HWND hwnd, const tchar *text, const tchar *caption, UINT type)
{
	return ::MsgBox(hwnd, text, caption, type);
}
int Squre::MsgBox(const tchar *text, const tchar *caption, UINT type)
{
	return ::MsgBox(HWindow, text, caption, type);
}
int Squre::MsgBox(int text, int caption, UINT type)
{
	return ::MsgBox(HWindow, text, caption, type);
}

void Squre::dump_pool()
{
#if 1	// file save version
	const tchar *filename = prof.ReadInteger(PFS_COMMON, PFS_SEARCHMT, true)
		? _t("\\temp\\t1.txt") : _t("\\temp\\t2.txt");
	TOFile tof;
	tof.settextmode(TFM_UTF8);
	tof.create(filename);
	tof.bom();
	tnstr s = tnsprintf(_t("--- pool.word : %d"), pool.get_num());
	tof.putline(s);
	for (int i=0;i<pool.get_num();i++){
		tof.putline(find_cword_pos(pool.fw[i].c_str()));
	}
	if (sub_words){
		s = tnsprintf(_t("--- sub_words : %d"), sub_words->get_num());
		tof.putline(s);
		for (int i=0;i<sub_words->get_num();i++){
			tof.putline(find_cword_pos((*sub_words)[i].word->c_str()));
		}
	}
#else
	DBW("--- pool.word : %d", pool.get_num());
	for (int i=0;i<pool.get_num();i++){
		DBW("%ws", pool.fw[i].c_str());
	}
	if (sub_words){
		DBW("--- sub_words : %d", sub_words->get_num());
		for (int i=0;i<sub_words->get_num();i++){
			DBW("%ws", (*sub_words)[i].word->c_str());
		}
	}
#endif
}

TMultiAllSearchParams::TMultiAllSearchParams(int dicnum)
	:super(MaxNum = /* dicnum/ */2)	// sub searchがinc.srchで重いときは大きくする
{
	LastIndex = 0;
}
void TMultiAllSearchParams::SetMode(int mode)
{
	for (int i=0;i<get_num();i++){
		(*this)[i].SetMode(mode);
	}
}
TMultiAllSearchParam *TMultiAllSearchParams::GetNext(MPdic &dic)
{
	if (get_num()<MaxNum){
		add(dic.CreateSearchParam());
	}
	TMultiAllSearchParam *p = &(*this)[LastIndex++];
	if (LastIndex>=get_num()) LastIndex = 0;
	return p;
}


