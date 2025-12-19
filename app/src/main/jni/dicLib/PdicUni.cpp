#include	"tnlib.h"
#pragma hdrstop
#include "pdc600.h"
#include "pdicuni.h"
#include "japa.h"
//---------------------------------------------------------------------------

PdicUni::PdicUni( int _EndianMode, int utfmode)
{
	EndianMode = _EndianMode;
	UTFMode = utfmode;
}
PdicUni::~PdicUni()
{
}
bool PdicUni::Load( )
{
	switch ( EndianMode ){
		case LITTLEENDIAN:
			switch ( UTFMode ){
				case UTF16:
					return inherited::Load( _T("PDCUL.DLL") );
				case UTF8:
					return inherited::Load( _T("PDCU8L.DLL") );
				case BOCU:
					if (inherited::Load(_T("PDCUBLN.DLL")))
						return true;
					return inherited::Load(_T("PDCUBL.DLL"));
				case HYPER3:
					return inherited::Load(_T("PDCUBLNR.DLL"));
				default:
					return false;
			}
		case BIGENDIAN:
			switch ( UTFMode ){
				case UTF16:
					return inherited::Load( _T("PDCUB.DLL") );
				default:
					return false;
			}
		default:
			return false;
	}
}

bool PdicUni::ReadExtHeader(const char *key, tnstr &str)
{
	if (!hPdc) return false;

	PDString pds = fnPDCreateString(NULL, 0);
	int len = fnPDGetExtHeaderStr(hPdc, key, pds);
	str.set(fnPDGetString(pds), len);
	fnPDDeleteString(pds);

	return true;
}

bool PdicUni::WriteExtHeader(const char *key, const tchar *str)
{
	if (!hPdc) return false;

	return fnPDSetExtHeaderStr(hPdc, key, str);
}

bool PdicUni::DeleteExtHeader(const char *key)
{
	if (!hPdc) return false;

	return fnPDDeleteExtHeader(hPdc, key);
}

bool PdicUni::_Open( HPDC hpdc, const tchar *filename, bool readonly )
{
	FNPDOpen fnPDOpen;
	GETPROC( PDOpen, "PDOpen" );
//	wchar_t _filename[MAXPATH];
//	AnsiToUnicode( filename, _filename, sizeof(_filename) );
	return fnPDOpen( hpdc, filename, readonly );
}
bool PdicUni::_Create( HPDC hpdc, const tchar *filename, PDCREATEINFO *info )
{
	FNPDCreate fnPDCreate;
	GETPROC( PDCreate, "PDCreate" );
//	wchar_t _filename[MAXPATH];
//	AnsiToUnicode( filename, _filename, sizeof(_filename) );
	return fnPDCreate( hpdc, filename, info );
}
void PdicUni::InitReadPare( MergeMode mergemode, int TransFlags )
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
#if 0
int PdicUni::readPare( WChar &word, WJapa &japa )
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
#endif
// for fast process.
int PdicUni::readPare(tnstr &word, Japa &japa)
{
#ifdef _UNICODE
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
			japa.SetAttr(pdcdata.attr);
#ifdef USE_JLINK
			japa.jlinks.clear();
#endif
			if (UseObjectData){
				WJapa wjapa;
				if (ReadObject(wjapa)!=0)
					return -1;
				if (!SetObject(japa, wjapa)){
					return -1;
				}
			}
			return 0;
	}
#else
	// To be implemented.
	__assert(0);
	return -1;
#endif
}

#if 0
static int henkan;
static int CALLBACK UserInterface( int mode )
{
	switch ( mode ){
		case 1:
			switch ( cvtdlg->SelectWord( word, slev, dlev ) ){
				case ID_NEXT:
					return 0;
				case ID_AUTO:
					inqf = 0;
					// fall thru
				case ID_REPLACE:
					return 1;	// normal
				default:
					return -1;
			}
			// return;
		case 2:
			switch ( cvtdlg->SelectJapa( word, srcjp, destj, newj, overflg, pdic ) ){
				case ID_MERGE:
					force = 1;
					return 1;	// normal
				case ID_NEXT:
					return 0;
				case ID_REPLACE:
					recj = &srcjp;
					break;
				case ID_AUTO:
					inqf = 0;
					return 1;	// normal
				default:
					return -1;
			}
		case 3:
			switch ( cvtdlg->SelectJapa( word, srcjp, destj, newj, 0, pdic ) ){
				case ID_MERGE:
					force = 1;
					return 1;
				case ID_NEXT:
					return 0;
				case ID_REPLACE:
					recj = &srcjp;
					return 1;
				case ID_AUTO:
					inqf = 0;
					return 1;
				default:
					return -1;
			}
		case 4:
	}
}
#endif
int PdicUni::record(const wchar_t *word, WJapa &japa )
{
	pdcdata.word = (wchar_t*)word;
	pdcdata.japa = japa.japa.c_str();
	pdcdata.exp = japa.exp.c_str();
	pdcdata.pron = japa.pron.c_str();
	pdcdata.attr = japa.attr;
	super::SetObject(japa);
	return fnPDQueryRecord( hPdc, &pdcdata, &prp );
}
const tchar *PdicUni::GetErrorMessage( int error )
{
#if 0
	const wchar_t *p = (const wchar_t*)inherited::GetErrorMessage( error );
	if (!p)
		return NULL;
	int size = wcslen(p) + 1;
	char *buf = new char[ size ];
	UnicodeToAnsi( p, buf, size );
	ErrorMessage.SetBuf( buf );
	return ErrorMessage;
#else
	// To be implemented.
	__assert__;
	return _T("");
#endif
}

// EndianMode : 0:Little Endian 1:Big Endian
bool IsPdicUni( const tchar *filename, int *EndianMode, int *UTFMode )
{
	PdicUni *dic = new PdicUni( false, 0 );
	PDHEADERINFO header;
	if ( dic->GetHeaderInfo( filename, &header ) ){
		if ( EndianMode )
			*EndianMode = 0;
		delete dic;
		return header.Version >= 0x500;
	}
	delete dic;
	dic = new PdicUni( true, 0 );
	if ( dic->GetHeaderInfo( filename, &header ) ){
		if ( EndianMode )
			*EndianMode = 1;
		delete dic;
		return header.Version >= 0x500;
	}
	delete dic;
	dic = new PdicUni( false, 1 );	// Little-endian, UTF8
	if ( dic->GetHeaderInfo( filename, &header ) ){
		if ( EndianMode )
			*EndianMode = 0;
		delete dic;
		return header.Version >= 0x400;
	}
	delete dic;
	return false;
}

// Object is not transferred. Because the Pdic object is required.
Japa &Japa::operator = (const class WJapa &j)
{
#ifdef _UNICODE
	attr = j.attr;
	japa = j.japa;
	exp = j.exp;
	pron = j.pron;
#else
#error	// To be implemented.
#endif
	return *this;
}

#ifdef USE_JLINK
#include "ole2s.h"
#include "o2if.h"
#endif
bool PdicUni::SetObject(Japa &japa, const WJapa &j)
{
#ifdef USE_JLINK
#ifdef _UNICODE
	japa.jlinks.clear( );
	for ( int i=0;i<j.objects.size();i++ ){
		TUniObjData &obj = *(TUniObjData*)j.objects[i];
		JLink *o;
		switch (obj.objtype){
			case PDCOBJ_FILE:
				o = new JLFile(NULL, obj.GetFileName(), obj.GetId());
				if (!o)
					return false;	// no memory
				o->SetID(obj.GetId());
				o->SetTitle(obj.GetTitle());
				break;
			case PDCOBJ_OLE:
				o = GetObjectOle(obj, obj.GetId());
				if (!o)
					return false;	// no memory or no valid data.
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

#ifdef USE_JLINK
//Note: 2007.12.22
//	OLEデータを一度OLE objectに展開してからJLinkObject(Raw Data)に変換している。
//	そのため効率が悪い。
//	理由：PDCxxx.dllのAPI仕様に合わせたため
//
JLOle *PdicUni::GetObjectOle(TUniObjData &obj, int id)
{
	// Create the ole data.
	autoptr<JLOle> org(new JLOle(NULL, NULL));
	if (!org)
		return NULL;

	for (;;){
		TMemoryObject *mem = obj.GetData();
		if (!mem){
			break;	// no valid data.
		}
		if (!org->SetData(obj.GetOleParamW(), mem->get(), mem->size(), *obj.GetAspectPtr(), id)){
			break;	// invalid OLE data?
		}
		org->SetTitle(obj.title);

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
#endif	// USE_JLINK


