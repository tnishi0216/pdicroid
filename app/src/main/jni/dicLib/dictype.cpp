#include "tnlib.h"
#pragma	hdrstop
#include "dic.h"
#include "japa.h"
#include "utydic.h"
#include "dictype.h"
#include "faststr.h"
#include "dictext.h"
#include "filestr.h"
#include "pdstrlib.h"

bool IsCSV(const tchar *buf1, const tchar *buf2);

// 辞書の自動判定
int GetDicFileType( const tchar *filename )
{
	int dictype_ex;
	return GetDicFileTypeEx( filename, dictype_ex);
}

// ファイルの種類の取得
// -1: 不明
//	   ただし、dictype_ex!=-1のときは、
//		dictype_ex = DT_PDIC: DICERR_NOT_SUPPORT
// -2: ファイルを開けない
int GetDicFileTypeEx( const tchar *filename, int &dictype_ex )
{
	dictype_ex = -1;

	if (!filename)
		return -1;

	const tchar *ext = GetFileExtension(filename);

	if (ext){
		// 拡張子による判断
		if (!_tcsicmp(ext, _t("csv"))){
			// CSV
			return DT_CSV;
		}
		if (!_tcsicmp(ext, _t("tsv"))){
			// TSV
			return DT_TSV;
		}
		if (!_tcsicmp(ext, _t("xml"))){
			// XML
			return DT_XML;
		}
		if (!_tcsicmp(ext, _t("pdi"))){
			return DT_PDI;
		}
	}

	int errcode;
	{
		auto_Pdic pdic(filename);
//		if (pdic->OpenReadOnly( filename )==0)
		if (pdic->ReadHeader(filename))
		{
			pdic->Close( );
			return DT_PDIC;
		}
		errcode = pdic->GetErrorCode();
	}

	if (errcode==DICERR_DICVERSION || errcode==DICERR_NOTPDIC){
		autoptr<Pdic2> pdic2(new Pdic2);
		if (pdic2->Open(filename, FOM_READONLY)==0){	//TODO: ReadHeaderに対応
			return pdic2->GetDicType();
		}
	}
	if (errcode==DICERR_NOT_SUPPORT){
		dictype_ex = DT_PDIC;
		return -1;
	}

	tnstr buf1;
	tnstr buf2;

	{
		TIFile tif;
		if ( tif.open( filename ) ){
			return -2;
		}
#if defined(_UNICODE) && !defined(UNIX)
		if (!tif.isunicode()){
			// ANSI -> Unicode
			tif.getlineA( buf1 );
			tif.getlineA( buf2 );
		} else
#endif
		{
			tif.getline( buf1 );
			tif.getline( buf2 );
		}
	}

	const tchar *p;

	// １行テキスト？
	if ( ::_tcsstr( buf1, StrOneLineDelim )
		&& (!buf2[0] || ::_tcsstr( buf2, StrOneLineDelim )) ){
		return DT_EXTPDICTEXT;
	}
	// TABで区切られている？
	if ( ::_tcschr( buf1, '\t' ) && ::_tcschr( buf2, '\t' ) ){
		// WX2,Level-1
		p = jfstrrchr( buf1, ':' );
		if ( p ){
			// WX2?
			p++;
			if ( _tcsncmp( p, _T("名詞"), 4 ) == 0 || ( _tcslen(p) >= 2 && _tcsncmp( p+2, _T("詞"), 2 ) == 0 ) ){
				return DT_WX2TEXT;
			}
		}
		p = ::_tcschr(buf1, '\t');
		p = ::_tcschr(p+1, '\t');
		if (p){
			// TABが２つ以上ある
			return DT_TSV;
		}
		if ( buf1[0] >= '0' && buf1[0] <= '9' && buf2[0] >= '0' && buf2[0] <= '9' ){
			// level-1
			return DT_LEVEL;
		}
		if ( ::_tmbschr( buf1, ',' ) && ::_tmbschr( buf2, ',' ) ){
			// level-2
			return DT_LEVEL;
		}
	}

#if 1
	{
		CSVFile csv;
		if (!csv.Open(filename, FOM_READONLY)){
			tnstr word;
			Japa japa;
			int i;
			for (i=0;i<4;i++){
				int r = csv.readPare(word, japa);
				if (r != 0){
					if (r==-1 && i>0)	// EOF
						return DT_CSV;
					break;
				}
				if (word.empty()) break;
				if (japa.japa.empty()) break;
				if (word[0] == _T('■') && word[1] == '"') break;	// 辞郎形式の可能性が高い
			}
			if (i>=4){
				return DT_CSV;
			}
		}
	}
#else	// old way
	if (IsCSV(buf1, buf2)){
		return DT_CSV;
	}
#endif
	
	// FENG V5形式
	if ( *((ushort*)&buf1[0]) == _TW("■") ){
//	if ( (uchar)buf1[0] == (uchar)0x81 && (uchar)buf1[1] == (uchar)0xa1 ){	// ■
		return DT_FENG5;
	}

	// EPWING //
	p = GetFileName( filename );
	// ファイル名で判断
	if ( !_tcsnicmp( p, _T("Catalog"), 7 ) ){
		if ( !ext ){
			// 拡張子が無い
			return DT_EPWING;
		}
	}

	// PDIC TEXT?
	p = &buf1[0];
	while ( *p ){
		if ( (tuchar)*p < ' ' && *p != '\t' && *p != '\r' && *p != '\n' ){
			break;
		}
		NEXT_P( p );
	}
	if ( !*p ){
		return DT_PDICTEXT;
	}
	return -1;
}

bool IsCSV(const tchar *buf1, const tchar *buf2)
{
	const tchar *p;

	if (buf1[0] == '"' && buf2[0] == '"'){
		p = &buf1[1];
		p = skipto( p, '"' );
		if ( *p == '"' && _tcslen( p ) > 5 ){
			p++;
			if ( *p == ',' && *(p+1) == '"' ){
				p = skipto( p+2, '"' );
				if ( *p == '"' ){
					return true;
				}
			}
		}
	}
	p = buf1;
	for (int i=0;i<2;i++){
		int comma = 0;
		while (*p){
			if (*p==','){
				comma++;
			}
			NEXT_P(p);
		}
		if (comma<4)
			return false;	// ４項目以上をCSV
		p = buf2;
	}
	return true;	// CSV
}

