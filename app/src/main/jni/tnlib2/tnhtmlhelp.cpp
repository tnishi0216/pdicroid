#include <windows.h>
#pragma hdrstop
#include <htmlhelp.h>
#include "tnhtmlhelp.h"
#include "tnhelp.h"

static HINSTANCE hinst;
static DWORD dwCookie;

class THtmlHelpFinalizer {
public:
	~THtmlHelpFinalizer()
	{
		TNHtmlHelpFree();
	}
};

static THtmlHelpFinalizer finalizer;

void TNHtmlHelpInit()
{
//	TNHtmlHelp(0, NULL, -1, 0);	// load html help
}
void TNHtmlHelpUninit()
{
	TNHtmlHelpFree();
}

HWND TNHtmlHelp(HWND hwnd, const tchar *path, int cmd, int data)
{
	typedef HWND (WINAPI *FPHH) (HWND, LPCTSTR, UINT,
								  DWORD);
	FPHH htmlHelp;
	bool firstcall = false;
	if (hinst==NULL){
		hinst = LoadLibrary(_T("HHCTRL.OCX"));
		if(hinst==NULL){
//		   MessageBox(hwnd, MSG_CANNOTFIND_HHCTRL, StrAppName, MB_OK);
		   return NULL;
		}
		firstcall = true;
	}
#ifdef UNICODE
	const char *api_htmlhelp = "HtmlHelpW";
#else
	const char *api_htmlhelp = "HtmlHelpA";
#endif
	htmlHelp = (FPHH)GetProcAddress(hinst,api_htmlhelp);
	if (htmlHelp){
		if (firstcall){
			htmlHelp(NULL, NULL, HH_INITIALIZE, (DWORD)&dwCookie);
		}
		if (cmd != -1)
			return htmlHelp(hwnd, path, cmd, data);
	}
	return NULL;
}
BOOL WINAPI _TNHtmlHelp(HWND hwnd, LPCTSTR path, UINT cmd, DWORD data)
{
	HWND ret;
	switch (cmd){
		case HELP_FINDER:
			ret = TNHtmlHelp(hwnd, path, HH_DISPLAY_TOPIC, 0);
			break;
		case HELP_CONTEXT:
			ret = TNHtmlHelp(hwnd, path, HH_HELP_CONTEXT, data);
			break;
		case HELP_KEY:
			{
			HH_AKLINK link;
			link.cbStruct =     sizeof(HH_AKLINK);
			link.fReserved =    FALSE ;
			link.pszKeywords =  (LPCTSTR)data;
			link.pszUrl =       NULL ;
			link.pszMsgText =   NULL ;
			link.pszMsgTitle =  NULL ;
			link.pszWindow =    NULL ;
			link.fIndexOnFail = TRUE ;
			ret = TNHtmlHelp(hwnd, path,
						 HH_KEYWORD_LOOKUP, (DWORD)&link);
			}
			break;
	}
	return ret!=NULL;
}
void TNHtmlHelpFree()
{
	if (!hinst)
		return;
	TNHtmlHelp(NULL, NULL, HH_CLOSE_ALL, 0);
	TNHtmlHelp(NULL, NULL, HH_UNINITIALIZE, dwCookie);
	FreeLibrary(hinst);
	hinst = NULL;
}

