#include "tnlib.h"
#pragma	hdrstop
#include	"id.h"
#include	"utydic.h"
#include	"srchout.h"
#include	"winmsg.h"
#include	"japa.h"
#include	"dictext.h"
#include	"winsqu.h"
#include	"dictype.h"
#include "wpdcom.h"

SrchOutBase::SrchOutBase( )
{
	outflag = OF_WORD | /* OF_LEVEL | */ OF_JAPA | OF_EXP | OF_PRON;
	output = OD_NONE;
	format = DT_PDICTEXT;
	buffer = NULL;
	out = NULL;
	squ = NULL;
	DelayedOutput = false;
	NeedExtension = false;
	NeedDeleteFile = false;
}

SrchOutBase::~SrchOutBase( )
{
	Close();
}

int SrchOutBase::Open( TWinControl *parent )
{
	DWord.clear();	// mark for first time check

	const tchar *fname = _T("");
	int mode = FOM_WRITE;
	if ( output & (OD_FILE|OD_BROWSER) ){
		mode |= FOM_FILE;
		fname = filename;
		if (output & OD_FILE){
			NeedExtension = true;
		}
	}
	if ( output & OD_CLIPBD ){
		mode |= FOM_CLIPBOARD;
		if ( squ )
			fname = (const tchar *)squ->GetWHandle();
	}
#ifdef USE_PS
	if ( output & OD_POPUP ){
		mode |= FOM_EDITCONTROL;
		if ( squ ){
			fname = (const tchar*)squ->GetPopupEdit();
		}
	}
#endif
	if ( output & OD_CHAR ){
		mode |= FOM_CHAR;
		fname = (const tchar *)(buffer);
	}
	if ( output & OD_DEBUG ){
		return 0;
	}
#ifndef SML
	out = MakeDic( format );
	if ( !out )
		return -1;
	if ( parent ){
		if ( out->CanOpen( parent, fname, mode ) <= -1 ){
			delete out;
			out = NULL;
			return -1;
		}
	}
	if ( outflag & OF_PRON )
		out->outflag |= OF_PRON;
	out->optionstr.set( fmttemplate );
	if ( out->Open( fname, mode ) == -1 ){
		delete out;
		out = NULL;
		return -1;
	}

	if ( format == DT_USERTEMP ){
		((UserFile*)out)->LastLine = false;
		DelayedOutput = true;
	} else {
		DelayedOutput = false;
	}

	return 0;
#else
	return -1;
#endif
}

void SrchOutBase::Close( )
{
	if ( out ){
		if ( DelayedOutput ){
			((UserFile*)out)->LastLine = true;
			if (DWord.exist()){
				DelayedOutput = false;
				Output( DWord, DJapa, DFreq );
				DelayedOutput = true;
				// memory release
				DWord.clear();
				DJapa.clear();
			}
		}
		out->Close( );
		delete out;
		out = NULL;
		if (NeedDeleteFile){
			DeleteFile(filename);
		}
	}
}

BOOL SrchOutBase::Output( const tchar *word, Japa &japa, int freq )
{
	if ( DelayedOutput ){
		BOOL r = TRUE;
		if ( DWord.exist() ){
			DelayedOutput = false;
			r = Output( DWord, DJapa, DFreq );
			DelayedOutput = true;
		} // else frist time
		DWord = word;
		DJapa = japa;
		DFreq = freq;
		return r;
	}
#if 0	//TODO:
#if USE_DT2
	if ( output & OD_DEBUG ){
		THyperLinks hls;
		hls.ExtractStaticWords( 0, japa.japa );
		for ( int i=0;i<hls.get_num();i++ ){
			tnstr key;
			hls[i].GetKeyWord( key, japa.japa );
			squ->ps = new POPUPSEARCH;
			POINT pt;
			::GetCursorPos( &pt );
			::ScreenToClient( squ->GetWHandle(), &pt );
			squ->OpenAutoLinkPopup( DCAST_WINDOW(squ), pt, hls[i].type, key, hls[i] );
			while ( squ->IsAutoLinkPopupOpened() ){
				WaitAppMessageIdle( );
			}
			delete squ->ps;
			squ->ps = NULL;
		}
		return true;
	}
#endif
#endif
	out->outflag = outflag;
	out->Frequency = freq;
	return out->record( word, japa ) != -1;
}

SrchOut::SrchOut( Squre *_squ )
{
	squ = _squ;
//	over = TRUE;
	maxnum = -1;
}
#if 0
SrchOut::SrchOut( TSquareFrame *_squ )
{
	squ = _squ;
	maxnum = -1;
}
#endif
SrchOut::~SrchOut()
{
}

int SrchOut::GetErrorCode( )
{
#ifndef SML
	if ( out )
		return out->GetErrorCode();
	else return 0;
#else
	return 0;
#endif
}

