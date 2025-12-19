#ifndef __DIB_H
#define	__DIB_H

#ifndef __TNDEFS_H
#include "tndefs.h"
#endif

#define HDIB HANDLE

/* DIB constants */
#define PALVERSION   0x300

/* DIB macros */
inline BOOL IS_WIN30_DIB( BITMAPINFOHEADER *lpbi )
{
	return ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER));
}

inline int RECTWIDTH( RECT *lpRect )
{
	return lpRect->right - lpRect->left;
}

inline int RECTHEIGHT( RECT *lpRect )
{
	return lpRect->bottom - lpRect->top;
}

inline int RECTWIDTH( RECT &lpRect )
{
	return lpRect.right - lpRect.left;
}

inline int RECTHEIGHT( RECT &lpRect )
{
	return lpRect.bottom - lpRect.top;
}


/* function prototypes */
HANDLE          AllocRoomForDIB(BITMAPINFOHEADER bi, HBITMAP hBitmap);

/* Error constants */
enum {
	  DIBERR_MIN = 0,                     // All error #s >= this value
	  DIBERR_NOT_DIB = 0,                 // Tried to load a file, NOT a DIB!
	  DIBERR_MEMORY,                      // Not enough memory!
	  DIBERR_READ,                        // Error reading file!
	  DIBERR_LOCK,                        // Error on a GlobalLock()!
	  DIBERR_OPEN,                        // Error opening a file!
	  DIBERR_CREATEPAL,                   // Error creating palette.
	  DIBERR_GETDC,                       // Couldn't get a DC.
	  DIBERR_CREATEDDB,                   // Error create a DDB.
	  DIBERR_STRETCHBLT,                  // StretchBlt() returned failure.
	  DIBERR_STRETCHDIBITS,               // StretchDIBits() returned failure.
	  DIBERR_SETDIBITSTODEVICE,           // SetDIBitsToDevice() failed.
	  DIBERR_STARTDOC,                    // Error calling StartDoc().
	  DIBERR_NOGDIMODULE,                 // Couldn't find GDI module in memory.
	  DIBERR_SETABORTPROC,                // Error calling SetAbortProc().
	  DIBERR_STARTPAGE,                   // Error calling StartPage().
	  DIBERR_NEWFRAME,                    // Error calling NEWFRAME escape.
	  DIBERR_ENDPAGE,                     // Error calling EndPage().
	  DIBERR_ENDDOC,                      // Error calling EndDoc().
	  DIBERR_SETDIBITS,                   // Error calling SetDIBits().
	  DIBERR_FILENOTFOUND,                // Error opening file in GetDib()
	  DIBERR_INVALIDHANDLE,               // Invalid Handle
	  DIBERR_DIBFUNCTION,                 // Error on call to DIB function
	  DIBERR_MAX                          // All error #s < this value
	 };

	 /* Function prototypes */

#define WIDTHBYTES(bits)    ((((bits) + 31) & ~31) >> 3)

HDIB      FAR  BitmapToDIB (HBITMAP hBitmap, HPALETTE hPal);
HDIB      FAR  ChangeBitmapFormat (HBITMAP  hBitmap,
								   WORD     wBitCount,
								   DWORD    dwCompression,
								   HPALETTE hPal);
HDIB      FAR  ChangeDIBFormat (HDIB hDIB, WORD wBitCount,
								DWORD dwCompression);
HBITMAP   FAR  CopyScreenToBitmap (LPRECT);
HDIB      FAR  CopyScreenToDIB (LPRECT);
HBITMAP   FAR  CopyWindowToBitmap (HWND, WORD);
HDIB      FAR  CopyWindowToDIB (HWND, WORD);
HPALETTE  FAR  CreateDIBPalette (HDIB hDIB);
HDIB      FAR  CreateDIB(DWORD, DWORD, WORD);
WORD      FAR  DestroyDIB (HDIB);
void      FAR  DIBError (int ErrNo);
DWORD     FAR  DIBHeight (LPSTR lpDIB);
WORD	  FAR  DIBNumColors ( BITMAPINFOHEADER *lpDIB );
HBITMAP   FAR  DIBToBitmap (HDIB hDIB, HPALETTE hPal);
DWORD     FAR  DIBWidth (LPSTR lpDIB);
UINT 	  FAR PaletteSize( BITMAPINFOHEADER *lpbi );
inline LPSTR FAR FindDIBBits( BITMAPINFOHEADER *lpbi )
{
   return ((LPSTR)lpbi) + (int)lpbi->biSize + (int)PaletteSize(lpbi);
}

HPALETTE  FAR  GetSystemPalette (void);
HDIB      FAR  LoadDIB ( const char *filename );
HDIB	  FAR  LoadDIB( const char *lpbi );
BOOL	  FAR  GetDIBInfo( const char *lpFileName, BITMAPINFOHEADER *lpbih );
BOOL      FAR  PaintBitmap (HDC, LPRECT, HBITMAP, LPRECT, HPALETTE);
BOOL      FAR  PaintDIB (HDC, LPRECT, HDIB, LPRECT, HPALETTE);
int       FAR  PalEntriesOnDevice (HDC hDC);
WORD      FAR  PrintDIB (HDIB, WORD, WORD, WORD, LPSTR);
WORD      FAR  PrintScreen (LPRECT, WORD, WORD, WORD, LPSTR);
WORD      FAR  PrintWindow (HWND, WORD, WORD, WORD, WORD, LPSTR);
WORD      FAR  SaveDIB (HDIB, LPSTR);
BOOL      FAR  SaveDIB( HDIB );

void BitBltDIB( HDC hdc, RECT *dcRect, RECT *dibRect, HDIB hDib );
void StretchBltDIB( HDC hdc, RECT *dcRect, RECT *dibRect, HDIB hDib );

#endif	// __DIB_H
