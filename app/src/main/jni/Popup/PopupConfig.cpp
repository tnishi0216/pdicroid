//---------------------------------------------------------------------------

#include "tnlib.h"
#pragma hdrstop
#include "defs.h"
#include "PopupConfig.h"
#include "LangProcDef.h"
#include "PopId.h"
#include "pdprof.h"
#include "id.h"

#ifdef PDICW
//#include "pdicw.h"
#endif

//---------------------------------------------------------------------------

#pragma package(smart_init)

POPUPCONFIG PopupConfig = {
	false,	// fCaseIgnore
	true,	// fSuffix
//	true,	// fReplaceOnes
#if USE_UKSPELL
	true,	// fUKSpell
#endif
#ifndef LIGHT
	false,	// fUmlaut
	false,	// fDeutch
#endif
//	true,	// fWordDelim

	true,	// fLongest

//	false,	// fMultiByte

#ifndef LIGHT
	true,	// fShowLevel
	true,	// fShowPron
	false,	// fShowExp
	true,	// fShowObject
	true,	// fPlay
	false,	// fTTSPlay
#endif
#ifndef SML
	false,	// fFixWidth
	false,	// fFixPos
	true,	// fToFront
	true,	// fSetFocus
	DEF_PS_ALPHA_VALUE,	// Alpha value
#endif
	false,	// fToMainSrchWord
};

int POPUPCONFIG::GetOption()
{
	int option = SLW_REPLACE | SLW_WORDDELIM | SLW_SYMBOLS;
#ifdef WINCE
	if ( fCaseSearch ) option |= (SLW_CASEIGNORE & ~SLW_CASEIGNORE4);
	// WinCEでは、当面大文字への変換同一視はサポートしない(速度の面から)
#else
#if OLDCASE
	if ( fCaseIgnore ) option |= SLW_CASEIGNORE;
#endif
#endif
	if ( fSuffix ) option |= SLW_ENGLISH;
//	if ( fReplaceOnes ) option |= SLW_REPLACE;
#if USE_UKSPELL
	if ( fUKSpell ) option |= SLW_UK;
#endif
#ifndef LIGHT
	if ( fUmlaut ) option |= SLW_UMLAUT;
	if ( fDeutch ) option |= SLW_DEUTCH;
#endif
//	if ( fWordDelim ) option |= SLW_WORDDELIM;
	return option;
}
int POPUPCONFIG::GetViewFlags()
{
#ifndef LIGHT
	return  (fShowLevel ? PSF_SHOWLEVEL : 0)
			| (fShowPron ? PSF_SHOWPRON : 0)
			| (fShowExp ? PSF_SHOWEXP : 0)
			| (fShowObject ? PSF_SHOWOBJECT : 0)
			| (fPlay ? PSF_PLAY : 0 )
			| (fTTSPlay ? PSF_TTSPLAY : 0)
			| (PSF_SHOWMASK&~(PSF_SHOWLEVEL|PSF_SHOWPRON|PSF_SHOWEXP|PSF_SHOWOBJECT|PSF_SHOWDICNAME|PSF_PLAY|PSF_TTSPLAY));
#else
	return PSF_SHOWMASK;
#endif
}
#ifdef PDICW
TRegKey *POPUPCONFIG::GetSectionKey()
{
	if ( CurGroupReg && (uniq & UQ_POPUP) ){
		return CurGroupReg;	//TODO: 確認：この下のPSWindowではない？ 2006.12.26
	} else {
		return prof.SetSection( PFS_PSWINDOW );
	}
}
void POPUPCONFIG::LoadProfile()
{
	TRegKey *key = GetSectionKey();
	fCaseIgnore = key->ReadInteger( PFS_CASESEARCH, fCaseIgnore);
	fSuffix = key->ReadInteger( PFS_SUFFIX, fSuffix );
//	fReplaceOnes = key->ReadInteger( PFS_REPLACEONES, fReplaceOnes );
#if USE_UKSPELL
	fUKSpell = key->ReadInteger( PFS_UKSPELL, fUKSpell );
#endif
#ifndef LIGHT
	fUmlaut = key->ReadInteger( PFS_UMLAUT, fUmlaut );
	fDeutch = key->ReadInteger( PFS_DEUTCH, fDeutch );
#endif
#ifndef SML
	fFixWidth = key->ReadInteger( PFS_FIXWIDTH, fFixWidth );
	fFixPos = key->ReadInteger(PFS_FIXPOS, fFixPos);
	fToFront = key->ReadInteger(PFS_TOFRONT, fToFront);
	fSetFocus = key->ReadInteger(PFS_SETFOCUS, fSetFocus);
	Alpha = key->ReadInteger(PFS_ALPHA, Alpha);
#endif
//	fWordDelim = key->ReadInteger( PFS_WORDDELIM, fWordDelim );
	fLongest = key->ReadInteger( PFS_LONGEST, fLongest );
//	fMultiByte = key->ReadInteger( PFS_MULTIBYTE, fMultiByte );
#ifndef LIGHT
	fShowLevel = key->ReadInteger( PFS_LEVEL, fShowLevel );
	fShowPron = key->ReadInteger( PFS_PRON, fShowPron );
	fShowExp = key->ReadInteger( PFS_EXP, fShowExp );
	fShowObject = key->ReadInteger( PFS_OBJECT, fShowObject );
#endif
	fPlay = key->ReadInteger( PFS_PLAY, fPlay );
	fTTSPlay = key->ReadInteger( PFS_TTSPLAY, fTTSPlay );
}
void POPUPCONFIG::SaveProfile()
{
	TRegKey *key = GetSectionKey();
	key->WriteInteger( PFS_CASESEARCH, fCaseIgnore);
	key->WriteInteger( PFS_SUFFIX, fSuffix );
//	key->Write( PFS_REPLACEONES, (int)fReplaceOnes );
#if USE_UKSPELL
	key->WriteInteger( PFS_UKSPELL, fUKSpell );
#endif
#ifndef LIGHT
	key->WriteInteger( PFS_UMLAUT, fUmlaut );
	key->WriteInteger( PFS_DEUTCH, fDeutch );
#endif
#ifndef SML
	key->WriteInteger(PFS_FIXWIDTH, fFixWidth);
	key->WriteInteger(PFS_FIXPOS, fFixPos);
	key->WriteInteger(PFS_TOFRONT, fToFront);
	key->WriteInteger(PFS_SETFOCUS, fSetFocus);
	key->WriteInteger(PFS_ALPHA, Alpha);
#endif
//	key->WriteInteger( PFS_WORDDELIM, (BOOL&)fWordDelim );
	key->WriteInteger( PFS_LONGEST, fLongest );
//	key->WriteInteger( PFS_MULTIBYTE, fMultiByte );
#ifndef LIGHT
	key->WriteInteger( PFS_LEVEL, fShowLevel );
	key->WriteInteger( PFS_PRON, fShowPron );
	key->WriteInteger( PFS_EXP, fShowExp );
	key->WriteInteger( PFS_OBJECT, fShowObject );
#endif
	key->WriteInteger( PFS_PLAY, fPlay );
	key->WriteInteger( PFS_TTSPLAY, fTTSPlay );
}
#endif	// PDICW
