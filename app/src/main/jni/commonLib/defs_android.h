#ifndef __defs_android_h
#define __defs_android_h

#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, "PDJ", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "PDJ", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "PDJ", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "PDJ", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "PDJ", __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "PDJ", __VA_ARGS__)

// WindowsŒÝŠ·‚Ì’è‹`
// ‚±‚¤‚¢‚¤‰ñ”ð‚Í–{—ˆ‚æ‚­‚È‚¢‚ª‚Æ‚è‚ ‚¦‚¸

typedef unsigned		COLORREF;
typedef unsigned *		HFONT;
typedef unsigned *		HDC;
typedef unsigned *		HGDIOBJ;
typedef unsigned *		HWND;
typedef unsigned		WPARAM;
typedef unsigned		LPARAM;
typedef unsigned		LRESULT;
typedef unsigned		HMENU;
typedef unsigned		HINSTANCE;
typedef unsigned		TIMERPROC;

#define RGB(r, g ,b) ((DWORD)(((BYTE)(r) | ((WORD)(g) << 8)) | (((DWORD)(BYTE)(b)) << 16)))

#define	WINAPI
struct LOGFONT {
	int __dummy;
};
#ifndef SYSMAC_H
struct RECT {
	int left;
	int top;
	int right;
	int bottom;
};
struct POINT {
	int x;
	int y;
};
#endif

struct SIZE {
	LONG cx;
	LONG cy;
};
#define	DEFINED_SIZE

struct POINTL {
	LONG x;
	LONG y;
};

#define	CALLBACK
#define	WM_SETFONT			0
#define	COLOR_WINDOW		0
#define	COLOR_WINDOWTEXT	0
#define	SYSTEM_FONT			0

// Message Box //
#define	MB_OK				1
#define	MB_YESNO			2
#define	MB_ICONQUESTION		0
#define	MB_ICONSTOP			0
#define	IDYES				0x100
#define	MB_DEFBUTTON2		0

// Scroll //
#define	SB_VERT				0
#define	SB_HORZ				1
#define	SB_LINEDOWN			2
#define	SB_LINEUP			3
#define	SB_PAGEDOWN			4
#define	SB_PAGEUP			5
#define	SB_THUMBPOSITION	6

struct TWinControl {
	int __dummy;
};

DWORD GetSysColor(int index);
COLORREF SetTextColor(HDC, COLORREF);
int SendMessage(HWND , UINT , WPARAM, LPARAM);
BOOL PostMessage(HWND , UINT , WPARAM, LPARAM);
BOOL UpdateWindow(HWND);
UINT_PTR SetTimer(HWND , UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc);
BOOL KillTimer(HWND, UINT_PTR);
HWND SetCapture(HWND);
BOOL DeleteFile(LPCTSTR lpFileName);
BOOL MoveFile(const tchar *src, const tchar *dst);
COLORREF SetBkColor( HDC hdc, COLORREF crColor );
HGDIOBJ SelectObject( HDC hdc, HGDIOBJ hgdiobj );
HGDIOBJ SelectObject( HDC hdc, class TNFONT *tnfont );
HGDIOBJ GetStockObject( int fnObject );
int GetLastError();

#define	DECLARE_RESPONSE_TABLE(x)

#endif
