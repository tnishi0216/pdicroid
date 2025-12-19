#include	"tnlib.h"
#pragma hdrstop
#include "pddefs.h"
#include "pdicansi.h"
//---------------------------------------------------------------------------
#include "pdc600.h"
#include "japa.h"
#include "depwif.h"

static const tchar *DllNamePdicAnsi = _T("PDC630.DLL");

PdicAnsi::PdicAnsi( )
{
	OleLoaded = false;
	EPWingSupported = true;
}
PdicAnsi::~PdicAnsi()
{
	if (OleLoaded){
		depwif.UnloadLibrary();
	}
}
bool PdicAnsi::Load( )
{
	return inherited::Load( DllNamePdicAnsi );
}
void PdicAnsi::InitReadPare( MergeMode mergemode, int TransFlags )
{
	FNPDInitSearch fnPDInitSearch;
	GETPROC( PDInitSearch, "PDInitSearch" );
	fnPDInitSearch( hPdc, NULL, SRCH_ALL );

	memset( &prp, 0, sizeof(prp) );
	prp.MergeMode = mergemode;
	prp.CFlags = TransFlags;

	memset( &pdcdata, 0, sizeof(pdcdata) );
	pdcdata.cp_word = CP_ACP;
	pdcdata.cp_japa = CP_ACP;
	pdcdata.cp_exp = CP_ACP;
	pdcdata.cp_pron = CP_ACP;
}
int PdicAnsi::readPare( CChar &word, CJapa &japa )
{
	int r;
	for (;;){
		r = fnPDGetNextSearch( hPdc, &pdcdata );
		if ( r == AS_CONTINUE )
			continue;
		break;
	}
	switch ( r ){
		case AS_ERROR:
		case AS_END:
			return -1;
		default:
			word = pdcdata.word;
			japa.japa = pdcdata.japa;
			japa.exp = pdcdata.exp;
			japa.pron = pdcdata.pron;
			japa.attr = pdcdata.attr;
			japa.clearObject();
			if (UseObjectData){
				if (ReadObject(japa)!=0)
					return -1;
			}
			return 0;
	}
}
int PdicAnsi::record(const char *word, CJapa &japa )
{
	pdcdata.word = (char*)word;
	pdcdata.japa = japa.japa.c_str();
	pdcdata.exp = japa.exp.c_str();
	pdcdata.pron = japa.pron.c_str();
	pdcdata.attr = japa.attr;
	return fnPDQueryRecord( hPdc, &pdcdata, &prp );
}
bool PdicAnsi::_Open( HPDC hpdc, const tchar *filename, bool readonly )
{
	FNPDOpen fnPDOpen;
	GETPROC( PDOpen, "PDOpen" );
	char _filename[MAXPATH];
	UnicodeToAnsi( filename, _filename, sizeof(_filename) );
	return fnPDOpen( hpdc, _filename, readonly );
}
bool PdicAnsi::_Create( HPDC hpdc, const tchar *filename, PDCREATEINFO *info )
{
	FNPDCreate fnPDCreate;
	GETPROC( PDCreate, "PDCreate" );
	char _filename[MAXPATH];
	UnicodeToAnsi( filename, _filename, sizeof(_filename) );
	return fnPDCreate( hpdc, _filename, info );
}

// Convert OLE data into japa.
bool PdicAnsi::ConvertObject(const CJapa &j, Japa &japa)
{
#ifdef USE_JLINK
#ifdef _UNICODE
	japa.jlinks.clear( );
	for ( int i=0;i<j.objects.size();i++ ){
		TAnsiObjData &obj = *(TAnsiObjData*)j.objects[i];
		JLink *o;
		switch (obj.objtype){
			case PDCOBJ_FILE:
				o = new JLFile(NULL, _mustr(obj.GetFileName()), obj.GetId());
				if (!o)
					return false;	// no memory
				o->SetID(obj.GetId());
				o->SetTitle((const tchar*)_mustr(obj.GetTitle()));
				break;
			case PDCOBJ_OLE:
				o = GetObjectOle(obj, obj.GetId());
				if (!o)
					return false;	// no memory or no valid data.
				break;
			case PDCOBJ_EPWING:
				o = new JLEPWing(NULL, obj.GetId(), obj.bookno, obj.pos);
				if (!o)
					return false;
				break;
			case PDCOBJ_BINARY:
				{
					JLinkObject *mem = (JLinkObject*)obj.GetData();
					if (!mem)
						continue;	// ignore
					o = new JLUFO(NULL, obj.GetId());
					if (!o)
						return false;
					o->SetTitle((const tchar*)_mustr(obj.GetTitle()));
					((JLUFO*)o)->Set(mem);
					((JLUFO*)o)->SetExtraData(obj.pos);	// extra data
				}
				break;
			default:
				// unknown object
				continue;
		}
		japa.jlinks.add(o);
	}
#else
#error	// To be implemented.
#endif
#endif
	return true;
}

class JLOle *PdicAnsi::GetObjectOle(TAnsiObjData &obj, int id)
{
	// Create the ole data.
	autoptr<JLOle> org(new JLOle(NULL, NULL));
	if (!org)
		return NULL;

	if (!OleLoaded){
		if (!depwif.LoadLibrary()){
			return NULL;
		}
		OleLoaded = true;
	}

	for (;;){
		TMemoryObject *mem = obj.GetData();
		if (!mem){
			break;	// no valid data.
		}
		PDCOLEDATAPARAMW paramW;
		ConvertOleDataParam(*obj.GetOleParamA(), paramW);
		if (!org->SetData(&paramW, mem->get(), mem->size(), *obj.GetAspectPtr(), id)){
			break;	// invalid OLE data?
		}
		org->SetTitle((const tchar*)_mustr(obj.title));

		// Copy the ole data to the raw image.
		long len = org->GetLength();
		uint8_t *data = new uint8_t[len];
		if (!data){
			break;	// no memory
		}
		if (!org->Get(data)){
			break;	// no valid data?
		}
		org.reset();

		JLOle *o = new JLOle(NULL, NULL);
		if (o){
			o->SetNL(data, len);
		}
		delete[] data;
		return o;
	}
	return NULL;	// error
}

inline bool mb2uc(const char *strA, wchar_t *strW, int sizeW)
{
	if (MultiByteToWideChar(CP_ACP, 0, strA, strlen(strA), strW, sizeW)==0)
		return false;	// something error
	strW[sizeW-1] = '\0';
	return true;
}

// PDCOLEDATAPARAMA -> PDCOLEDATAPARAMW
bool ConvertOleDataParam(const PDCOLEDATAPARAMA &paramA, PDCOLEDATAPARAMW &paramW)
{
	if (!mb2uc(paramA.ServerName, paramW.ServerName, sizeof(paramW.ServerName)/sizeof(wchar_t)))
		return false;
	paramW.Rect = paramA.Rect;
	paramW.ObjectType = paramA.ObjectType;
	paramW.nMag = paramA.nMag;
	paramW.nAspect = paramA.nAspect;
	paramW.DataLength = paramA.DataLength;
	return true;
}

bool IsPdicAnsi( const tchar *filename )
{
	PdicAnsi *dic = new PdicAnsi( );
	PDHEADERINFO header;
	if ( dic->GetHeaderInfo( filename, &header ) ){
		//return header.Version >= 0x400;
		delete dic;
		return true;
	}
	delete dic;
	return false;
}

Japa &Japa::operator = (const class CJapa &j)
{
	japa = _mt(j.japa);
	exp = _mt(j.exp);
	pron = _mt(j.pron);
	for (int i=0;i<j.objects.size();i++){
		//TODO: To be implemented.
		__assert__;
	}
	return *this;
}



