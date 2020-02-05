#include "tnlib.h"
#pragma hdrstop

#include "LangProc.h"
#include "LangProcMan.h"
#include "BiPool.h"
#include "filestr.h"
#include "jtype.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

#define	USE_PREV2	1	// ２つ前からの単語を切り出す

#ifndef foreach
#define	foreach(obj, it, type) \
	for (type::iterator it=(obj).begin();it!=(obj).end();it++)
#endif

bool TIrregDics::IsModified() const
{
	for (int i=0;i<get_num();i++){
		if ((*this)[i].IsModified()){
			return true;
		}
	}
	return false;
}
bool TIrregDics::Read(int index, const tchar *filename, tnstr &s)
{
	while (index>=get_num()){
		add(new TIrregDic);
	}
	return (*this)[index].Read(filename, s);
}

////////////////////////////////////////////////////
// TLangProc calss
////////////////////////////////////////////////////
TLangProc::TLangProc()
{
	Table = NULL;
	LPMLoader = NULL;
	IrregPool = NULL;
	IrregCheckCase = false;
	Buffer = NULL;
	BufferSize = 0;
	KCodeTranslate.encodeKT = NULL;
	KCodeTranslate.decodeKT = NULL;

	SPTable = NULL;
	FCodeTranslate.encodeKT = NULL;
	FCodeTranslate.decodeKT = NULL;
}
TLangProc::~TLangProc()
{
	Close();
}
void TLangProc::Close()
{
	CloseIrreg();
	CloseTable();
#if USE_LPM
	CloseLPM();
#endif
	CloseSPTable();
	FreeBuffer();
}
void TLangProc::IncRef()
{
	LangProcMan.IncRef(this);
}
void TLangProc::Release()
{
	LangProcMan.Release(this);
}
tnstr TLangProc::CompositeWord(const tchar *word)
{
	if (is_composit_word(word)){
		return word;
	} else {
		return join_word(word, Normalize(word));
	}
}

tnstr TLangProc::Normalize(const tchar *str)
{
	if (!str)
		return NULL;
	__assert(Table!=NULL /*&& LPMLoader!=NULL*/);
#if 1
	int len = _tcslen(str);
	tchar *dst = GetBuffer(len+1);
	if (!dst){
		return tnstr(str);
	}
	tchar *dp = dst;
	for (;*str;){
		tchar c = *str++;
		tchar tc = Table->Table[c];
		if (tc){
			*dp++ = tc;
		}
	}
	*dp = '\0';
	return tnstr(dst);
#else
	int len = _tcslen(str);
	tchar *dst = new tchar[len+1];
	if (!dst){
		return tnstr(str);
	}
	LCMapString(LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH|LCMAP_HIRAGANA|LCMAP_UPPERCASE,
		str, -1, dst, len+1);
	tnstr ret(dst);
	delete[] dst;
	return ret;
#endif
}

bool TLangProc::OpenSPTable()
{
	return OpenSPTable(SrchPatFile);
}

//Note:
//	You should call OpenSPTable() before use this function.
// Note:
//	同一strに書き込むため、normalizeより長くなってはいけない
tchar *TLangProc::NormalizeSearchPattern(tchar *str)
{
	if (!str)
		return NULL;
	//__assert(SPTable!=NULL /*&& LPMLoader!=NULL*/);
	if (!SPTable)
		return NULL;

	tchar *dp = str;
	for (;*str;){
		tchar c = *str++;
		tchar tc = SPTable->Table[c];
		if (tc){
			*dp++ = tc;
		}
	}
	*dp = '\0';
	return dp;
}

const tchar *TLangProc::GetAddWords() const
{
	return Table->PopupAddWords;
}

bool TLangProc::Open(const tchar *filename)
{
	__assert(filename);
	if (!OpenTable(filename)){
		return false;
	}
#if 0
	if (!OpenLPM(filename)){
	}
#endif
	FileName = filename;
	return true;
}
bool TLangProc::OpenTable(const tchar *table_filename)
{
	if (!Table){
		__assert(table_filename);
		Table = new TLPTable(table_filename);
	}
	if (!Table->Read()){
		DBW("TLangProc::Read failed: %s", __cstr(table_filename).utf8());
		delete Table;
		Table = NULL;
		return false;
	}
	return true;
}
void TLangProc::CloseTable()
{
	if (!Table)
		return;
	delete Table;
	Table = NULL;
}
#if USE_LPM
//Note: not in used for now.
bool TLangProc::OpenLPM(const tchar *filename)
{
	if (LPMLoader){
		delete LPMLoader;
	}
	LPMLoader = new TLPMLoader(filename);
	if (!LPMLoader){
		return false;
	}
	if (!LPMLoader->Load()){
		delete LPMLoader;
		LPMLoader = NULL;
		return false;
	}
	return true;
}
void TLangProc::CloseLPM()
{
	if (!LPMLoader)
		return;
	delete LPMLoader;
	LPMLoader = NULL;
}
#endif
bool TLangProc::OpenSPTable(const tchar *filename)
{
	if (!SPTable){
		SPTable = new TLPTable(filename);
	}
	if (!SPTable->Read()){
		delete SPTable;
		SPTable = NULL;
		return false;
	}
	return true;
}
void TLangProc::CloseSPTable()
{
	if (!SPTable)
		return;
	delete SPTable;
	SPTable = NULL;
}
tchar *TLangProc::GetBuffer(int size)
{
	if (BufferSize>=size)
		return Buffer;
	if (Buffer)
		delete[] Buffer;
	Buffer = new tchar[size];
	if (Buffer){
		BufferSize = size;
		return Buffer;
	} else {
		BufferSize = 0;
		return NULL;
	}
}
void TLangProc::FreeBuffer()
{
	if (Buffer){
		delete[] Buffer;
		Buffer = NULL;
		BufferSize = 0;
	}
}

// 不規則変化辞書 //
void TLangProc::AddIrregFile(const tchar *filename)
{
	for (int i=0;i<IrregNames.size();i++){
		if (IsSameFileName(IrregNames[i], filename)){
			return;	// already added.
		}
	}
	IrregNames.add(filename);
}
bool TLangProc::OpenIrreg()
{
	if (IrregNames.size()==0){
		__assert__;
		return false;
	}

	if ( IrregPool ){
		if ( Irregs.IsModified() ){
			// 修正されているため再ロード
			//TODO: Not debugged.
			delete IrregPool;
			IrregPool = NULL;
		}
	}
	if ( !IrregPool ){
		// Load //
		// format:word\tjapa
		tnstr line;
		const int MIN_WORDS = 16;
		IrregPool = new TBiPoolJ(MIN_WORDS);
		if (IrregPool){
			for (int i=0;i<IrregNames.size();i++){
				if (Irregs.Read(i, IrregNames[i], line)){
					do {
						if (line[0] && line[0]!='#'){
							tchar *p = ::_tcschr(line, '\t');	// Separated by '\t'
							if (p && p[1]){
								*p = '\0';
								tnstr nw = Normalize(line);	// to ignore case sense.
								if (IrregCheckCase){
									if (nw != line){
										IrregPool->AddSort(line, p+1);
										if (IrregPool->Find( nw )==-1){
											// ない場合のみ登録
											IrregPool->AddSort(nw, p+1);
										}
									} else {
										IrregPool->AddSort(nw, p+1);
									}
								} else {
									IrregPool->AddSort(nw, p+1);
								}
							}
						}
					} while (Irregs.Read(i, NULL, line));
				}
			}
			if (IrregPool->get_num()==0){
				delete IrregPool;
				IrregPool = NULL;
				return false;
			}
		} else {
			return false;
		}
	}
	return true;
}
void TLangProc::CloseIrreg()
{
	if (!IrregPool)
		return;
	delete IrregPool;
	IrregPool = NULL;
}
bool TLangProc::SearchIrreg( const tchar *word, tnstr &trsword )
{
	if (!IrregPool){
		return false;
	}
	int r = -1;
	if (IrregCheckCase){
		r = IrregPool->Find( word );
	}
	if ( r == -1 ){
		tnstr nw = Normalize(word);
		r = IrregPool->Find( nw );
		if ( r == -1)
			return false;
	}
	trsword = IrregPool->japa(r);
	return true;
}

#if 0	// defined in dicUtil.cpp
const tchar *find_cword_pos(const tchar *word)
{
	const tchar *p = word;
	for(;*p;){
		if (*p=='\t'){
			return p+1;
		}
		p++;
	}
	return word;
}
#endif

tnstr join_word(const tchar *cword, const tchar *kword)
{
#if 0
	if (!_tcscmp(cword, kword)){
		return tnstr(cword);
	}
#endif
#if 1
	if (!kword[0]){
		return tnstr(cword, _t("\t"), cword);
	}
	return tnstr(kword, _t("\t"), cword);
#elif 1	// kword!=cwordのみ
	if (!kword[0]){
		return cword;
	}
	if (_tcscmp(kword, cword)==0){
		return cword;
	}
	return tnstr(kword, _t("\t"), cword);
#else
	// keywordの自動付加なし
	return cword;
#endif
}

const tchar *FindWordTop(const tchar *word, int offset, const tchar **_top2)
{
	const tchar *p;

	if (offset==-1){
		offset = 0;
		p = word;
		while (*p>='0' && *p<='9'){
			offset = offset*10 + *p-'0';
			p++;
		}
		if (*p==',') p++;
		word = p;
	}

	int len = _tcslen(word);
	if (offset>=len){
		return NULL;
	}

	//TODO: これはlangprocがやるべきこと
	const tchar *top = NULL;
	const tchar *top1 = NULL;
	const tchar *top2 = NULL;
	const tchar *offword = &word[offset];
	p = word;
	for (;p<=offword;){
#if 1	// 日本語対応版
		const tchar *nd = NextDelim(p);
		// nd may point to a delimitor or a charactor
		if (nd==p){
			p++;
			continue;
		}
		// p : word top
		top2 = top1;
		top1 = top;
		top = p;
		p = nd;
#else	// 英語のみ
		if (IsWordChar(*p)){
			if (!top)
				top = p;
		} else {
			if (top){
				top2 = top1;
				top1 = top;
				top = NULL;
			}
		}
		p++;
#endif
	}

#if USE_PREV2
	*_top2 = top2 ? top2 : top1;
#else
	*_top2 = top1;
#endif

	if (top){
		return top;
	} else
	if (top1){
		return top1;
	}
#if USE_PREV2
	else
	if (top2){
		return top2;
	}
#endif
	return word;
}


