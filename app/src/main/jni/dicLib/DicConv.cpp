//---------------------------------------------------------------------------

#include "tnlib.h"
#pragma hdrstop

#include "DicConv.h"
#include "dictype.h"
#include "utydic.h"
#include "japa.h"

//---------------------------------------------------------------------------

TDicConverter::TDicConverter()
{
	txDic = NULL;
	pDic = NULL;
}
TDicConverter::~TDicConverter()
{
}
bool TDicConverter::ConvertToPdic(const tchar *textname, const tchar *dicname, int dictype)
{
	txDic = CreateText(textname, dictype);
	if (!txDic)
		return false;
	if (txDic->Open(textname, FOM_READONLY))
		return false;
	bool ret = false;
	pDic = new Pdic2;
	if (pDic){
		if (pDic->Open(dicname, FOM_WRITE)==0){
			tnstr wd;
			Japa jp;
			ret = true;
			while (txDic->readPare( wd, jp ) == 0){
				if ( !wd[0] )	// 見出語が無いものは無視
					continue;
				if ( pDic->record( wd, jp ) == -1 ){
					ret = false;
					break;
				}
				jp.clear();
			}
		}
		delete pDic;
		pDic = NULL;
	}
	delete txDic;
	txDic = NULL;
	return ret;
}
bool TDicConverter::ConvertToPdic(const tchar *textname, Pdic *dic, int dictype)
{
	txDic = CreateText(textname, dictype);
	if (!txDic)
		return false;
	if (txDic->Open(textname, FOM_READONLY))
		return false;
	bool ret = false;
	pDic = new Pdic2;
	if (pDic){
		dic->SetFastMode(true);
		dic->SetRecordLock(false);	// 2016.4.15 ReadLockした状態で入ってくる場合があるため、record()時のlockはしないようにする必要がある
		if (pDic->Open(dic, FOM_WRITE)==0){
			tnstr wd;
			Japa jp;
			ret = true;
			while (txDic->readPare( wd, jp ) == 0){
				if ( !wd[0] )	// 見出語が無いものは無視
					continue;
				if ( pDic->record( wd, jp ) == -1 ){
					ret = false;
					break;
				}
				jp.clear();
			}
		}
		delete pDic;
		pDic = NULL;
		dic->SetFastMode(false);
		dic->SetRecordLock(true);
	}
	delete txDic;
	txDic = NULL;
	return ret;
}
bool TDicConverter::ConvertFromPdic(Pdic *dic, const tchar *textname, int dictype)
{
	txDic = CreateText(NULL, dictype);
	if (!txDic)
		return false;
	if (txDic->Open(textname, FOM_CREATE))
		return false;
	bool ret = false;
	pDic = new Pdic2;
	if (pDic){
		txDic->SetCFlags(TRS_ALL&~TRS_KEYWORD);
		if (pDic->Open(dic, FOM_READONLY)==0){
			tnstr wd;
			Japa jp;
			ret = true;
			while (pDic->readPare( wd, jp ) == 0){
				if ( !wd[0] )	// 見出語が無いものは無視
					continue;
				if ( txDic->record( wd, jp ) == -1 ){
					ret = false;
					break;
				}
				jp.clear();
			}
		}
		delete pDic;
		pDic = NULL;
	}
	delete txDic;
	txDic = NULL;
	return ret;
}

bool TDicConverter::SupportedType(const tchar *filename)
{
	switch (GetDicFileType(filename)){
		case DT_CSV:
		case DT_TSV:
//		case DT_XML:
		case DT_EXTPDICTEXT:
		//case DT_EXTPDICTEXT2:	// 判定対象外
			return true;
	}
	return false;
}

Perd12 *TDicConverter::CreateText(const tchar *filename, int dictype)
{
	Perd12 *dic = NULL;
	if (filename){
		int type = GetDicFileType(filename);
		if (type!=DT_PDICTEXT || dictype==0)
			dictype = type;		// dictypeが定義された場合はDT_PDICTEXTより優先的に
	}
	switch (dictype){
		case DT_CSV: dic = new CSVFile; break;
		case DT_TSV: dic = new TSVFile; break;
//		case DT_XML:
		case DT_EXTPDICTEXT: dic = new ExtPdicText; break;
		case DT_EXTPDICTEXT2: dic = new ExtPdicText2; break;
	}
	return dic;
}


