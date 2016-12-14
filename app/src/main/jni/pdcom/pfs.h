#ifndef __pfs_h
#define	__pfs_h

#include "pdconfig.h"

#ifdef	DEFINE_PFS
#define	DEF_PFS(name, str)	const tchar PFS_##name[] = _t(str)
#else
#define	DEF_PFS(name, str)	extern const tchar PFS_##name[];
#endif

DEF_PFS(DEBUG, "Debug");
DEF_PFS(ALOG, "alog");

DEF_PFS(CFLAGS2, "CFlags2");
DEF_PFS(FASTDB, "FastDB");

// Main //
DEF_PFS(DOKOSEARCH, "DokoSearch");
DEF_PFS(DOKOSEARCH_ACCEPTED, "DokoSearchAccepted");
DEF_PFS(WORDCLEARTIMER, "WordClearTimer");
DEF_PFS(WORDCLEARENABLED, "WordClearEnabled");
DEF_PFS(ACTIVEREVERSE, "ActiveReverse");

// Version //
DEF_PFS(VERSION, "Version");
DEF_PFS(OSVERSION, "OSVersion");

// Common //
DEF_PFS(COMMON, "Common");

DEF_PFS(GROUP, "Group");
DEF_PFS(GROUPNAME, "GroupName");
DEF_PFS(MESSAGEBAR, "MsgBar");
DEF_PFS(TITLEWIDTH, "TitleWidth");
DEF_PFS(WINDOW, "Window");
DEF_PFS(ITEMBAR, "ItemBar");
DEF_PFS(FONTNAME, "FontName");
DEF_PFS(FONTCHARSET, "FontCharSet");
DEF_PFS(FONTHEIGHT, "FontHeight");
DEF_PFS(FONTBOLD, "FontBold");
DEF_PFS(LINTER, "LineInter");
DEF_PFS(LEVELVALUE, "LevelValue");
DEF_PFS(WNDSHOW, "WndShow");
DEF_PFS(WNDSIZE, "WndSize");
DEF_PFS(DICTIONARY, "Dictionary");
DEF_PFS(GROUPSEL, "grpsel");
DEF_PFS(TITLEVIEW, "TitleView");
DEF_PFS(VIEWTOP, "ViewTop");
DEF_PFS(CBMODE, "CBMode");
DEF_PFS(LINKCOLOR, "LinkColor");
DEF_PFS(DEFFILE, "DefFile");
DEF_PFS(JEDITDLG, "JEditDlg3");
DEF_PFS(BACKCOLOR, "BackColor");
DEF_PFS(CHARCOLOR, "CharColor");
DEF_PFS(USERDLL, "UserDll");
DEF_PFS(LONGBAR, "LongBar");
DEF_PFS(ZEN2HAN, "Zen2Han");
DEF_PFS(IMECTRL, "IMECtrl");
DEF_PFS(UNIQ, "Uniq");
DEF_PFS(ITEMNAME, "ItemName");
DEF_PFS(EXTSRCHTIMER, "ExtSrchTimer");
DEF_PFS(ONDEMANDACCESS, "OnDemandAccess");
DEF_PFS(ODACLOSEDELAY, "ODACloseDelay");

DEF_PFS(SEARCHMT, "SearchMT");

DEF_PFS(OLDMEM, "OldMem");
DEF_PFS(OLDJEDIT, "OldJEdit");
DEF_PFS(WHEELSCROLL, "WheelScroll");
DEF_PFS(CBDELAYEDTIME, "CBDelayedTime");

DEF_PFS(DICLOG, "DicLog");

// Square //
DEF_PFS(KEEPTOP, "KeepTop");

DEF_PFS(WTEST, "WTest");
DEF_PFS(WTEST_RES, "WTestRes");

DEF_PFS(LEVELCONFIG, "LConfig");
DEF_PFS(FROMMEMORY, "FMemory");
DEF_PFS(FROMLEVEL, "FLevel");
DEF_PFS(NUMBER, "Number");
DEF_PFS(LEVEL1, "Level1");
DEF_PFS(LEVEL2, "Level2");
DEF_PFS(ORDER, "Order");
DEF_PFS(READONLY, "ReadOnly");
DEF_PFS(NOSEARCH, "NoSearch");
DEF_PFS(DELHIST, "DelHist");
DEF_PFS(RANDOM, "Random");
DEF_PFS(WTESTHIST, "WTestHist");
DEF_PFS(HISTORY, "History");
DEF_PFS(DIRECTSAVE, "DirectSave");
DEF_PFS(LIMITUPPER, "LimitUpper");
DEF_PFS(INTERVAL, "Interval");
DEF_PFS(VIEWSORT, "ViewSort");
DEF_PFS(BOOKMARK, "Bookmark");
DEF_PFS(BOOKMARKNAME, "BookmarkName");
DEF_PFS(MAXHISTORY, "MaxHistory");
DEF_PFS(HISTNAME, "HistName");
DEF_PFS(CONCATEXP, "ConcatExp");
DEF_PFS(LABELING, "Labeling");
DEF_PFS(DATEFMT, "DateFmt");
DEF_PFS(TIMEFMT, "TimeFmt");
DEF_PFS(LABEL, "Label");
DEF_PFS(COMPPOLICY, "CompPolicy");
DEF_PFS(COMPEXP, "CompExp");
DEF_PFS(FONTNAMEASC, "FontNameASC");
DEF_PFS(FONTCHARSETASC, "FontCharSetASC");
DEF_PFS(TESTFORM, "TestForm");
DEF_PFS(ICONIC, "Iconic");
DEF_PFS(TABCOL, "TabCol");
DEF_PFS(TEXTEXT, "TextExt");
DEF_PFS(TL_CHGMEMORY, "ChangeMemory");
DEF_PFS(TL_UNMEMORY, "UnsetMemory");
DEF_PFS(TL_LEVEL, "ChangeLevel");
DEF_PFS(LEARN, "Learn");
DEF_PFS(TL_MINLEVEL, "MinLevel");
DEF_PFS(PLAYDICS, "PlayDics");
DEF_PFS(TEMPCOPY, "TempCopy");
DEF_PFS(ASRESTRICTION, "ASRest");
DEF_PFS(USECTL3D, "UseCtl3d");
DEF_PFS(DICHELP, "DicHelp");
DEF_PFS(FONTAPPLY, "FontApply");
DEF_PFS(PRONMENU, "PronMenu");
DEF_PFS(NORMALSIZE, "NormalSize");
DEF_PFS(ENABLERB, "EnableRB");
DEF_PFS(TABNAMES, "TabNames");
DEF_PFS(ACTIVETAB, "ActiveTab");

DEF_PFS(WTESTBM, "WTestBM");
DEF_PFS(ONLYMARKED, "OnlyMarked");

// Network //
DEF_PFS(NETWORK, "Network");
DEF_PFS(USEPROXY, "UseProxy");
DEF_PFS(PROXY, "Proxy");
DEF_PFS(LOCALMODE, "LocalMode");
DEF_PFS(INET, "Inet");
DEF_PFS(AUTODOWN, "AutoDown");
DEF_PFS(GOOGLESUGGEST_URL, "GoogleSuggestURL");

// Convert //
DEF_PFS(CONVERT, "Convert");
DEF_PFS(CHKKANJI, "ChkKanji");
DEF_PFS(LEVELFMT, "LevelFmt");
DEF_PFS(EXTFMTSEPA, "ExtFmtSepa");
DEF_PFS(AUTOOPTIMIZE, "AutoOptimize");
DEF_PFS(WORDVISIBLE, "WordVisible");
DEF_PFS(LBR, "Lbr");
DEF_PFS(RBR, "Rbr");
DEF_PFS(CFLAGS, "CFlags");
DEF_PFS(REMOVESYMBOL, "RemoveSymbol");
DEF_PFS(REGULARTYPE, "RegType");
DEF_PFS(SPLANGNAME, "SPLangName");
DEF_PFS(MULTIBYTE, "MultiByte");
DEF_PFS(PREVDIC, "PrevDic");
DEF_PFS(QPNCHAR, "QPNChar");
DEF_PFS(DELIMITER, "Delimiter");
DEF_PFS(TRSITEMS, "TrsItems");
DEF_PFS(ONELINE, "OneLine");
DEF_PFS(ALARMAUTOON, "AlmAutoOn");
DEF_PFS(ALARMAUTOOFF, "AlmAutoOff");
DEF_PFS(FILELINKPATH, "FileLinkPath");
DEF_PFS(FLINKCPATH, "FLinkCPath");

DEF_PFS(TASKTRAY, "TaskTray");
DEF_PFS(PROXYPORT, "ProxyPort");
DEF_PFS(COLOR, "Color");
DEF_PFS(RATIO, "Ratio");
DEF_PFS(DELAY, "Delay");
DEF_PFS(PRON, "Pron");

// Search //
DEF_PFS(SEARCH, "Search");
DEF_PFS(ITEMSEL, "ItemSel");
DEF_PFS(GOOGLESUGGEST, "GoogleSuggest");

DEF_PFS(DIST, "Dist");
DEF_PFS(REGULAR, "Regular");
DEF_PFS(FUZZY2, "Fuzzy2");
DEF_PFS(REGTYPE, "RegType");
DEF_PFS(ALLDIC, "AllDic");
DEF_PFS(DICLIST, "DicList");
DEF_PFS(FWORD, "FWord");
DEF_PFS(FJAPA, "FJapa");
DEF_PFS(FEXP, "FExp");
DEF_PFS(FPRON, "FPron");
DEF_PFS(FTITLE, "FTitle");
DEF_PFS(LEVELFLAG, "LevelFlag");
DEF_PFS(MEMORYFLAG, "MemoryFlag");
DEF_PFS(MEMORYON, "MemoryOn");
DEF_PFS(OUTFLAG, "OutFlag");
DEF_PFS(OUTFILE, "OutFile");
DEF_PFS(FORMATFLAG, "FormatFlag");
DEF_PFS(FORMATSTR, "FormatStr");
DEF_PFS(TEMPLATE, "Template");
DEF_PFS(WORD, "Word");
DEF_PFS(MENU, "Menu");
DEF_PFS(LEVEL, "Level");
DEF_PFS(EXP, "Exp");
DEF_PFS(OBJECT, "Object");
DEF_PFS(WIDTH, "Width");
DEF_PFS(HEIGHT, "Height");
DEF_PFS(POPUP, "Popup");
DEF_PFS(HISTORYADD_MINTIME, "HistoryAddMinTime");
DEF_PFS(EPWING, "EPWING");
DEF_PFS(OPTION, "Option");
DEF_PFS(TABLE, "Table");
DEF_PFS(MERGEMODE, "MergeMode");
DEF_PFS(GAIJIEDIT, "GaijiEdit");
DEF_PFS(EXTFORMAT, "ExtFormat");
DEF_PFS(PATH, "Path");

// Search //
DEF_PFS(AUTOSEARCHALLTEXT, "AutoSearchAllText");
DEF_PFS(AUTOLINK, "AutoLink");

// Popup Search //
DEF_PFS(PSWINDOW, "PSWindow");
DEF_PFS(WORDNUM, "WordNum");
DEF_PFS(ASPECT, "Aspect");
DEF_PFS(CASESEARCH, "CaseSearch");
DEF_PFS(SUFFIX, "Suffix");
DEF_PFS(UKSPELL, "UKSpell");
DEF_PFS(CLICKSEARCH, "ClickSearch");
DEF_PFS(PSFILTER, "Filter");
DEF_PFS(PSFILTERDIR, "FilterDir");
DEF_PFS(LONGEST, "Longest");
DEF_PFS(REPLACEONES, "Replace");
DEF_PFS(UMLAUT, "Umlaut");
DEF_PFS(DEUTCH, "Deutch");
DEF_PFS(FIXWIDTH, "FixWidth");
DEF_PFS(FIXPOS, "FixPos");
DEF_PFS(TOFRONT, "ToFront");
DEF_PFS(SETFOCUS, "SetFocus");
DEF_PFS(ALPHA, "Alpha");
DEF_PFS(FIXEDX, "FixedX");
DEF_PFS(FIXEDY, "FixedY");
DEF_PFS(PLAY, "Play");
DEF_PFS(TTSPLAY, "TTSPlay");
DEF_PFS(UPDATEIMMEDIATELY, "UpdateImmediately");

// PSW Bookmark //
DEF_PFS(PSBOOKMARK, "PSBookmark");

// Web Window //
DEF_PFS(WEBWINDOW, "WebWindow");
DEF_PFS(WEBSITENAME, "WebSiteName");

DEF_PFS(WIN32TOUNI, "Win32ToUni");
DEF_PFS(INITPDIC3, "InitPdic3");
DEF_PFS(LANGPROC, "LangProc");
DEF_PFS(FILEHISTORY, "FileHistory");
DEF_PFS(TARGET_PART, "TargetPart");
DEF_PFS(INCSRCHPLUS, "IncSrchPlus");
DEF_PFS(WAVE, "Wave");
DEF_PFS(ALARMCONVERT, "AlarmConvert");

DEF_PFS(FileHistory1, "FileHistory1");
DEF_PFS(FileHistory2, "FileHistory2");

DEF_PFS(DICTIONARYPATH, "DictionaryPath");
//DEF_PFS(ESTDBPATH, "EstDBPath");
DEF_PFS(WORKINGPATH, "WorkingPath");

DEF_PFS(BROWSER_HEIGHT, "BrowserHeight");
DEF_PFS(BROWSER_WIDTH, "BrowserWidth");

#if INETDIC
DEF_PFS(INETPW, "InetPW");
DEF_PFS(ROOTSERVER, "RootServer");
DEF_PFS(INETSRCHSRV, "InetSrchSrv");
#endif

#if defined(USE_POPUPHOOK)
DEF_PFS(PH_INI, "PH.INI");
DEF_PFS(KICKPRG, "KickPrg");
DEF_PFS(POPUPHOOK, "PopupHook");
#endif

#ifndef SMALL
DEF_PFS(ESCCNT, "EscCnt");
#endif
#ifndef SMALL
DEF_PFS(JMerge, "JMerge");
DEF_PFS(PRONCR, "PronCR");
DEF_PFS(HTMLENABLED, "HTMLEnabled");
#endif

#if defined(_UNICODE) && (defined(DIC_UTF8)||defined(DIC_UTF16))
DEF_PFS(GROUPSEL_UNI, "grpsel_uni");
#endif
#ifdef DISPDICNAME
DEF_PFS(DISPDICNAME, "DispDicName");
DEF_PFS(DICNAMETEMPLATE, "DicNameTemplate");
#endif
DEF_PFS(SEARCHDLGTYPE, "SearchDlgType");

DEF_PFS(JEDIT, "JEdit");
DEF_PFS(ALTENTER, "AltEnter");
DEF_PFS(EXPTOP, "ExpTop");
DEF_PFS(EXPHEIGHT, "ExpHeight");

DEF_PFS(PLAYAUDIOISSUE, "PlayAudioIssue");
DEF_PFS(CLIPBOARDISSUE, "ClipboardIssue");

DEF_PFS(VUPNOTIFY, "VUPNotify");
DEF_PFS(TEXTFILEBOM, "TextFileBOM");		// TextFileにBOMをつけて出力するか？
DEF_PFS(TEXTFILECODE, "TextFileCode");	// TextFile出力時に使用する文字コード
//DEF_PFS(SAVEASRTF, "SaveAsRTF");
DEF_PFS(OLEFULL, "OleFull");

DEF_PFS(FONTUSEUNIQ, "FontUseUniq");

// JEdit //
DEF_PFS(KEYWORDVISIBLE, "KeywordVisible");
DEF_PFS(PRONVISIBLE, "PronVisible");
DEF_PFS(WANTTABS, "WantTabs");
DEF_PFS(WANTRETURNS, "WantReturns");
DEF_PFS(INITCARETPOS, "InitCaretPos");

DEF_PFS(FASTSEARCH_PRONENABLED, "FastSearchPronEnabled");

// EditObj //
DEF_PFS(EDITOBJ, "EditObj");
DEF_PFS(OBJECTLIST, "ObjectList");

// ClipHistory //
DEF_PFS(CLIPHISTORY, "ClipHistory");
DEF_PFS(COUNT, "Count");
DEF_PFS(ENABLED, "Enabled");
DEF_PFS(MAXCOUNT, "MaxCount");
DEF_PFS(MAXTEXTSIZE, "MaxTextSize");

// Dictionary Wizard //
DEF_PFS(DICWZD, "DicWzd");
DEF_PFS(PDINAME, "PdiName");
DEF_PFS(READMENAME, "ReadMeName");
DEF_PFS(VUPKEY, "VupKey");

// Setup //
DEF_PFS(SETUP, "Setup");
DEF_PFS(SETUPCODE, "SetupCode");
DEF_PFS(SETUPKEY, "SetupKey");
DEF_PFS(DICURL, "DicUrl");
DEF_PFS(COPYRIGHT, "Copyright");
DEF_PFS(UPGRADEKEY, "UpgradeKey");
DEF_PFS(README, "ReadMe");
DEF_PFS(INSTALLEDPATH, "InstalledPath");
DEF_PFS(USERADDFILES, "UserAddFiles");

// ShortCut //
DEF_PFS(SHORTCUT, "ShortCut");

// WebSearch //
DEF_PFS(WEBSEARCH, "WebSearch");
DEF_PFS(WEBCMD, "WebCmd");

// BatchDelete //
DEF_PFS(BATCHDEL, "BatchDel");
DEF_PFS(COPYMODE, "CopyMode");

// FileLink //
DEF_PFS(FILELINK, "FileLink");
DEF_PFS(COPYSEL, "CopySel");
DEF_PFS(COPYPATH, "CopyPath");

// Backup //
DEF_PFS(BACKUP, "Backup");
DEF_PFS(MAXDICSIZE, "MaxDicSize");
DEF_PFS(DICCHECK, "DicCheck");

#endif	/* __pfs_h */

