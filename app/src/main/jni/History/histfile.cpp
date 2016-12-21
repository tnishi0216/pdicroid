#include "tnlib.h"
#pragma hdrstop
#include <algorithm>
#include "pdconfig.h"

#include "wpdcom.h"
#include "filestr.h"
#include "histfile.h"
#include "windic.h"
#include "winmsg.h"
#include "pdprof.h"
#include "pdtime.h"
#include "UIMain.h"

#ifdef WINCE
static const tchar *HistoryFileName = _T(PATH_MYDOCUMENTS) _T("\\HistoryPdic.TXT")
#else
static const tchar *HistoryFileName = _T("History.txt");
#endif

#define	GetHistoryFileNameR	GetHistoryFileName

#define	EXTENDED		1

//static MPdic *GetActiveDic();
static tnstr getkey(const tchar *line);
static tnstr construct(const tchar *word, const tchar *url);
static void split(const tchar *line, tnstr *word, tnstr *url, timex_t *t);

//Note:
// registry : History\DirectSave を1にすると保存が速くなる

//---------------------------------------------------------------------------
// THistoryFile class
//---------------------------------------------------------------------------
THistoryFile::THistoryFile(const tchar *filename)
	:FileName(filename)
{
	Thread = NULL;
	LastCheckedTextMode = -1;
}

THistoryFile::~THistoryFile()
{
	StopThread();
	WaitThread();
}

tnstr THistoryFile::GetFileName()
{
	TAutoLock m(Mutex);
#ifndef WINCE
	const tchar *p = ::GetFileName( FileName );
	if ( p == (const tchar *)FileName){
		// pathの指定が無い場合
		return tnstr(prof.GetPersonalFile(FileName));
	}
#endif	// !WINCE
	return tnstr(FileName);
}
void THistoryFile::SetFileName(const tchar *filename)
{
	lock();
	FileName = filename;
	unlock();
}

bool THistoryFile::Load( TWordHistory &WordHist, int sort, int offset, TWordUrlList *urls )
{
	TAutoLock m(Mutex);
	return _Load(WordHist, sort, offset, urls);
}

bool THistoryFile::Load(tnstr_vec &lines, map<tnstr, int> &strindex, vector<int> *dupindex)
{
	TAutoLock m(Mutex);
	return _Load(lines, strindex, dupindex);
}

// 現在の履歴ファイルにsrcfileをmerge
// convert_format==true
//	srcfileが現在のtextmodeでなければ変換して、original history fileと置き換える
bool THistoryFile::Merge(const tchar *srcfile, bool convert_format)
{
	TAutoLock m(Mutex);
	return _Merge(srcfile, convert_format);
}

// you must delete the return value if not null.
TOFile *THistoryFile::CreateOpen()
{
	lock();
	TOFile *tof = _Create(GetFileName());
	if (tof){
		LastCheckedTextMode = prof.GetTextFileCode();
		return tof;	// return without unlock
	}
	unlock();
	return NULL;
}

void THistoryFile::CreateClose(TOFile *tof)
{
	if (!tof)
		return;
	delete tof;
	unlock();
}

void THistoryFile::Delete()
{
	lock();
	DeleteFile(GetFileName());
	unlock();
}

int THistoryFile::GetCount()
{
	TAutoLock m(Mutex);

	tnstr filename = GetFileName();
	TIFile tif;
	if ( tif.open( filename ) == -1 ){
		return 0;
	}
	int count = 0;
	tnstr line;
	for(;;){
		if (ReadLine(tif, line)<0)
			break;
		if (!line[0])
			continue;	// 空行はカウントしない
		count++;
	}
	return count;
}

int THistoryFile::DeleteWords(CBDeleteWordsHistory cb, DELETEWORDPARAM *dwp)
{
	TAutoLock m(Mutex);
	return _DeleteWords(cb, dwp);
}

TOFile *THistoryFile::OpenForAppend()
{
	tnstr filename = GetFileName();
	TOFile &tof = *new TOFile();
	if ( tof.open( filename )){
		return NULL;
	}
	tof.end();

	return &tof;
}

void THistoryFile::CheckFileFormat()
{
	tnstr filename = GetFileName();
	if (!_tcscmp(filename, LastCheckedFile)
		&& LastCheckedTextMode == prof.GetTextFileCode() ){
		return;
	}

	LastCheckedFile = filename;
	LastCheckedTextMode = prof.GetTextFileCode();

	Merge(filename, true);
}


bool THistoryFile::_Load(tnstr_vec &lines, map<tnstr, int> &strindex, vector<int> *dupindex)
{
	tnstr filename = GetFileName();
	TIFile tif;
	if ( tif.open( filename ) == -1 ){
		return false;
	}
	tnstr line;
	//map<tnstr, int> strindex;
	//vector<int> dupindex;
	for(;;){
		if (ReadLine(tif, line)<0)
			break;
		if (line.empty())
			continue;	// 空行はカウントしない
		if (dupindex){
			if (strindex.count(line)){
				dupindex->push_back(strindex[line]);
			}
			strindex[line] = lines.get_num();
		}
		lines.add(line);
	}
	return true;
}

bool THistoryFile::_Load( TWordHistory &WordHist, int sort, int offset, TWordUrlList *urls )
{
	// Load history from file //
	tnstr filename(GetHistoryFileNameR());
	TIFile tif;
	if ( tif.open( filename ) )
		return false;
	TFastReadHistory fast(WordHist);

	tnstr line;

	if (offset!=0){
		// adjust offset
		long curloc = tif.tell();
		long size = tif.get_length();
		if (size-curloc>offset){
			tif.seek(size-offset);
			ReadLine(tif, line);	// dummy read to synchronize
		}
	}

	bool alphasort = sort & HF_SORT_ALPHA ? true : false;
	bool reverse = sort & HF_SORT_REVERSE ? true : false;
	bool pre_reverse = (!reverse && (sort&HF_SORT_DATE)) || (reverse && (sort&HF_SORT_DATE)) ;
	
	for (;;){
		if (ReadLine(tif, line)<0)
			break;
		if (!line[0] || line[0]=='\t')
			continue;
		if (alphasort){
			WordHist.AddAlphaSorted( line );
		} else {
			fast.Add(line, pre_reverse);
		}
		if (urls){
			tnstr word;
			tnstr url;
			split(line, &word, &url, NULL);
			if (url.exist()){
				urls->AddDupCheck(word, url);
			}
		}
	}
	tif.close();

	if (sort & HF_SORT_CHAR){
		WordHist.SortByChar();
	} else
	if (sort & HF_SORT_FREQ){
		//TODO: SortByNumReversed()を作って高速化
		WordHist.SortByNum();
	} else {
		fast.Sort();
	}

	if (reverse)
		WordHist.SortReverse();

	return true;
}

// 現在の履歴ファイルにsrcfileをmerge
// convert_format==true
//	srcfileがUTF-16でなければ変換して、original history fileと置き換える
bool THistoryFile::_Merge(const tchar *srcfile, bool convert_format)
{
	if (!srcfile)
		return false;

	TIFile src;
	if (src.open(srcfile)){
		// no file?
		return false;
	}

	if (convert_format){
		if (src.gettextmode()==prof.GetTextFileCode()){
			// OK, same code
			return false;
		}
	}
	
	tnstr histname = GetFileName();

	tnstr tempname = histname;
	tempname += _t(".___");
	autoptr<TOFile> tof(_Create(tempname));
	if (!tof){
		return false;	// cannot create error??
	}

	tnstr line;
	for (;;){
		if (ReadLine(src, line)<0)
			break;
		WriteLine(*tof, line);
	}
	src.close();

	if (!convert_format){
		TIFile org;
		if (org.open(histname)==0){
			// merge original file
			for (;;){
				if (ReadLine(org, line)<0)
					break;
				WriteLine(*tof, line);
			}
			org.close();
		}
	}
	tof->close();

	if (!DeleteFile(histname)){
		// error
		// ReadOnlyにしていると考えられる。
		// 削除できないエラーが発生すると言うことは、
		// 履歴機能が働かないように（恐らく故意に）されているので
		// 変換に対応しないでおく。
		DeleteFile(tempname);
		return false;
	}
	return MoveFile(tempname, histname);
}

// you must delete the return value if not null.
TOFile *THistoryFile::_Create(const tchar *filename)
{
	TOFile *tof = new TOFile();
	if (!tof)
		return NULL;
	if ( tof->create(filename) == -1 ){
		delete tof;
		return NULL;
	}
#ifdef _UNICODE
	tof->settextmode(prof.GetTextFileCode());
	if (prof.IsTextFileBOM())
		tof->bom();
#endif
	return tof;
}

// 逐次処理法による削除
int THistoryFile::_DeleteWords(CBDeleteWordsHistory cb, DELETEWORDPARAM *dwp)
{
	bool direct = (bool)prof.ReadInteger(PFS_HISTORY, PFS_DIRECTSAVE, true);

	tnstr filename = GetFileName();
	TIFile tif;
	if ( tif.open(filename) == -1 ){
		DBW("open failed - _DeleteWords");
		return -1;
	}
	long filesize = tif.get_length();
	if (dwp)
		dwp->filesize = filesize;

	int count = 0;
	int c;
	if (dwp)
		dwp->ch = &c;
	bool changed = false;
	tnstr_vec lines;
	for (;;){
		tnstr line;
		c = ReadLine(tif, line);
		if (c < 0)
			break;
		if (line.empty()) continue;
		if ( cb ){
			// 任意の処理
			switch ( cb( line, *dwp ) ){
				case 0: // 削除しない
					break;
				case -1:
					cb = NULL;
					// fall thru
				case 1:	// 削除する
					changed = true;
					continue;
			}
		}
		count++;
		lines.push(line);
	}
	if (!tif.eof()){
		// read error
		DBW("read failed");
		__assert__;
		return -1;
	}
	tif.close();

	if (changed){
		tnstr dest(filename);
		dest.cat( _T(".tmp") );
		autoptr<TOFile> tof(_Create(direct?filename:dest));
		if (!tof){
			DBW("tof failed!! Some changes have not been taken");
			return -1;
		}
		foreach_tnstr_vec(lines, line){
			if (WriteLine(*tof, *line)<0){
				DBW("WriteLine failed");
				return -1;	// write error
			}
		}
		tof->close();

		if (!direct){
			if (!DeleteFile(filename)){
				// It may be a readonly file or sharing violation.
				DeleteFile(dest);
				return -1;
			}
			MoveFile(dest, filename);
			DeleteFile(dest);
		}
		if (dwp && lines.size()>0){
			dwp->lastline = lines[lines.size()-1];
		}
		LastCheckedTextMode = prof.GetTextFileCode();
	}
	return count;
}

//---------------------------------------------------------------------------
// TQueHistory class
//---------------------------------------------------------------------------
TQueHistory::TQueHistory(THistoryFile &file, bool use_extra)
	:File(file)
{
	ExtraMode = use_extra;
	Count = 0;
	hwndParent = NULL;
	msgParent = 0;
#ifdef SMALL
	LimitLower = 20480;
	LimitUpper = 16384;	// 16KB
#else
	LimitLower = 52428;
	LimitUpper = 65536;	// 64KB
#endif
	IgnoreDupWord = true;
}
void TQueHistory::SetMaxFileSize(int size)
{
	if (size<1000)
		size = 1000;
	LimitUpper = size;
	LimitLower = size-size*2/10;
}

void TQueHistory::Add(const tchar *word, const tchar *url)
{
	if (IgnoreDupWord){
		// 前回と同じ単語は登録しない
		if ( !_tcscmp( PrevWord, word ) ){
			return;
		}
	}
	PrevWord = word;

	if (ExtraMode){
		tnstr line = construct(word, url);
		HistoryQue.add( line );
	} else {
		HistoryQue.add( word );
	}

	Count++;
#ifdef _Windows
	if ( hwndParent ){
		if ( HistoryQue.get_num() == 1 ){
			PostMessage( hwndParent, msgParent, 0, 0 );
		}
	}
#endif
}

bool TQueHistory::SaveQueFile()
{
	if (HistoryQue.get_num()==0){
		return true;
	}

	File.CheckFileFormat();
	autoptr<TOFile> tof(File.OpenForAppend());
	if (!tof){
		HistoryQue.clear();
		DBW("open failed - SaveQueFile");
		return false;
	}

#ifdef _UNICODE
	tof->settextmode(prof.GetTextFileCode());
	write_bom(*tof);
#endif

	long l = tof->tell();
	if ( IsLimitOver(l) ){
		// 大きすぎ
		tof->close();
		DeleteOldestWords();
		tnstr filename = File.GetFileName();
		if ( tof->open(filename) == -1 ){
			// ここに来ることなんてあり得ないが
			return NULL;
		}
		tof->end();
	}

	SaveQueOutProc(*tof);	// 実際の出力処理
	
	return true;
}

void TQueHistory::Clear()
{
	HistoryQue.clear();
	File.Delete();
	PrevWord.clear();
}

static int cbDeleteWord(const tchar *line, DELETEWORDPARAM &dwp)
{
	tnstr word;
	split(line, &word, NULL, NULL);
	
	if ( !dwp.words.count(word) ){
		return 0;	// 削除しない
	}
	if ( dwp.all ){
		return 1;	// 削除する
	}
	return -1;	// 削除＆二度と来ない
}

bool TQueHistory::Delete(tnstr_vec &words, bool all)
{
	DELETEWORDPARAM dwp;
	foreach_tnstr_vec(words, it){
		dwp.words[*it] = true;
	}
	dwp.all = all;
	return DeleteWords(cbDeleteWord, &dwp);
}

int TQueHistory::cbDeleteOldestWord(const tchar *fword, DELETEWORDPARAM &dwp)
{
	// サイズ制限
	if ( !dwp.obj->IsLimitLower(dwp.filesize) ){
		return 0;	// 削除しない
	} else {
		dwp.filesize -= LENTOBYTE(*dwp.ch+2);	// +2 means "\r\n"
		return 1;	// 削除する
	}
}

bool TQueHistory::DeleteOldestWords()
{
	DELETEWORDPARAM dwp;
	dwp.obj = this;
	return DeleteWords(cbDeleteOldestWord, &dwp);
}

int TQueHistory::GetCountFile()
{
	return Count = File.GetCount();
}

// lines:
//	smaller index = older history
//	larger index = newer history
// exword:
//	a word to be deleted. dupcheck should be true.
// return:
//	0 : OK
//	-1: error
bool TQueHistory::LoadFile(tnstr_vec &lines, int maxLines, bool dupcheck, const tchar *exword)
{
	map<tnstr, int> strindex;
	vector<int> dupindex;

	if (!File.Load(lines, strindex, dupcheck ? &dupindex : NULL))
		return false;
	
	Count = lines.get_num();

	if (exword){
		__assert(dupcheck);
		if (strindex.count(exword)){
			dupindex.push_back(strindex[exword]);
		}
	}
	if (dupcheck){
		std::sort(dupindex.begin(), dupindex.end());
		for (vector<int>::reverse_iterator index=dupindex.rbegin();index!=dupindex.rend();index++){
			lines.del(*index);
		}
	}
	if (maxLines>0){
		if (lines.get_num()>maxLines){
			lines.del(0,lines.get_num()-maxLines-1);
		}
	}
	return true;
}

// lines:
//	smaller index = older history, or reverse
bool TQueHistory::SaveFile(tnstr_vec &lines, bool reverse)
{
	TOFile *tof = File.CreateOpen();
	if (!tof)
		return false;
	
	if (reverse){
		for(int i=lines.get_num()-1;i>=0;i--){
			if (File.WriteLine(*tof, lines[i])<0){
				goto jerr;
			}
		}
	} else {
		for(int i=0;i<lines.get_num();i++){
			if (File.WriteLine(*tof, lines[i])<0){
				goto jerr;
			}
		}
	}
	File.CreateClose(tof);
	return true;

jerr:
	File.CreateClose(tof);
	return false;
}

// 逐次処理法による削除
bool TQueHistory::DeleteWords(CBDeleteWordsHistory cb, DELETEWORDPARAM *dwp)
{
	int ret = File.DeleteWords(cb, dwp);
	if (ret<0)
		return false;	// error
	Count = ret;
	if (dwp){
		if (Count==0)
			PrevWord.clear();
		else
			PrevWord = dwp->lastline;
	}
	return true;
}

void TQueHistory::SaveQueOutProc(TOFile &tof)
{
	for (int i=0;i<HistoryQue.get_num();i++){
		File.WriteLine(tof, HistoryQue[i]);
	}
	HistoryQue.clear();
}
//---------------------------------------------------------------------------
// TPdicHistory class
//---------------------------------------------------------------------------
class TPdicHistory : public TQueHistory {
typedef TQueHistory super;
	THistoryFile file;
public:
	TPdicHistory()
		:file(HistoryFileName)
		,super(file, true)
	{}
	__override void SetMaxFileSize(int size);
	__override void LoadLimit();
	bool DeleteNoRec();
protected:
	__override void SaveQueOutProc(TOFile &tof);
};

void TPdicHistory::SetMaxFileSize(int size)
{
#ifndef __ANDROID__
	if (LimitUpper!=size)
		prof.WriteInteger(PFS_HISTORY, PFS_LIMITUPPER, size);
#endif
	super::SetMaxFileSize(size);
}
void TPdicHistory::LoadLimit()
{
	int size = prof.ReadInteger(PFS_HISTORY, PFS_LIMITUPPER, LimitUpper);
	super::SetMaxFileSize(size);
}
static int cbDeleteNoRecProc( const tchar *line, DELETEWORDPARAM &dwp )
{
	tnstr fword = getkey(line);
	if ( dwp.dic->Find(fword) != 1 ){
		// not found in dictionary
		return 1;	// 削除する
	}
	return 0;	// 削除しない
}

bool TPdicHistory::DeleteNoRec()
{
	MPdic *dic = GetActiveDic();
	if (!dic)
		return false;	// no open dictionary.

	DELETEWORDPARAM dwp;
	dwp.dic = dic;
	return DeleteWords(cbDeleteNoRecProc, &dwp);
}

void TPdicHistory::SaveQueOutProc(TOFile &tof)
{
	MPdic *dic = GetActiveDic();
	for ( ;HistoryQue.get_num(); ){
		tnstr word = getkey(HistoryQue[0]);
		if ( !dic || dic->Find( word ) == 1 ){
			// 辞書にない単語は削除する
			// 辞書がopenされていない場合は無条件で出力
			tof.putline( HistoryQue[0] );
		}
		HistoryQue.del( 0 );
	}
}

//---------------------------------------------------------------------------
// Global Values
//---------------------------------------------------------------------------
int HistoryFlag = HF_ALL;

static TPdicHistory PdicHistory;

//---------------------------------------------------------------------------
// PDIC History APIs
//---------------------------------------------------------------------------
void SetHistoryNotify( HWND hwnd, int message )
{
	PdicHistory.SetNotify(hwnd, message);
}

int GetHistoryMaxFileSize()
{
	return PdicHistory.GetLimitUpper();
}
void SetHistoryMaxFileSize(int size)
{
	PdicHistory.SetMaxFileSize(size);
}

#define	MIN_HISTORY_LENGTH	2	// ２文字以下は履歴登録しない

void AddHistory( const tchar *word, const tchar *url)
{
	word = find_cword_pos(word);
	if (!word || !word[0])
		return;	// 空
	int len = _tcslen(word);
	if (len<=MIN_HISTORY_LENGTH)
		return;

	PdicHistory.Add(word, url);
}

void HistoryProc()
{
	PdicHistory.SaveQueFile();
}

// 古い履歴をHistoryLimitLowerになるまで削除
// 空行の削除も行う
void DeleteOldestWords( )
{
	PdicHistory.DeleteOldestWords();
}

void DeleteWordsHistory(tnstr_vec &words)
{
	PdicHistory.Delete(words, true);
}

#if 0
#include "dic.h"
// 任意の削除方法ができるDeleteWord from history //
// History Fileから
// specific単語を削除する
// return value:
// true:削除できた
// false:できない
// Callback Function :
// bool cb( const tchar *fword, int cookie );
// fword : 見つかった履歴ファイルの１つの単語
// return : 1 = 削除する
//          0 = 削除しない
//         -1 = 削除するおよび、以降callbackは呼ばない＝以降すべての単語は削除しない
bool DeleteWordsHistory( CBDeleteWordsHistory cb, int cookie )
{
	return PdicHistory.DeleteOldestWords(cb, cookie);
}
#endif

// History Fileから
// specific単語を削除する
// return value:
// true:削除できた
// false:できない
bool DeleteWordHistory( const tchar *word, bool all )
{
	tnstr_vec words;
	words.push_back(word);
	return PdicHistory.Delete(words, all);
}
int GetCountWordHistory( )
{
	return PdicHistory.GetCountFile();
}

#ifndef SMALL
bool DeleteNoRecHistory( )
{
	return PdicHistory.DeleteNoRec();
}
#endif	// ndef SMALL

/////////////////// History Frequency ////////////////////////////////////////
// 単語 - 頻度 の構造体
TWordHistory::TWordHistory()
//	:super(256)	// change the slot size
{
	ExtraMode = false;
}
int TWordHistory::_SortByNum( HISTFREQ **s1, HISTFREQ **s2 )
{
	return (*s2)->Num - (*s1)->Num;	// 大きい順にソート
}
int TWordHistory::_SortByNumWord( HISTFREQ **s1, HISTFREQ **s2 )
{
	int r = (*s2)->Num - (*s1)->Num;	// 大きい順にソート
	if ( r != 0 ) return r;
	return _tcscmp( (*s1)->Word, (*s2)->Word );	// 単語順に
}
int TWordHistory::_SortByChar( HISTFREQ **s1, HISTFREQ **s2 )
{
	int r = (*s1)->Word.length() - (*s2)->Word.length();	// 文字数順
	if (r!=0) return r;
	return _tcscmp( (*s1)->Word, (*s2)->Word );	// 単語順に
}
int TWordHistory::BSearch( const tchar *word )
{
	int left = 0;
	int right = get_num();
	if ( right == 0 ) return 0;
	do {
		int mid = ( left + right ) /2;
		int k = _tcscmp( word, (*this)[mid].Word );
		if (k == 0){
			return mid;
		}
		if (k < 0){
			right = mid;
		} else {
			left = mid + 1;
		}
	} while (left < right);
	return left;
}
// alphabet順追加
void TWordHistory::AddAlphaSorted( const tchar *line )
{
#if EXTENDED
	tnstr _word;
	const tchar *word;
	if (ExtraMode){
		_word = getkey(line);
		word = _word;
	} else {
		word = line;
	}
#else
	const tchar *word = line;
#endif
	// Search insert point
	int i = BSearch( word );
	if ( i >= get_num() ){
		// 新規
		add( new HISTFREQ( word ) );
		return;
	}
	if ( !_tcscmp( word, (*this)[i].Word ) ){
		// 同一単語
		(*this)[i].Num++;
		return;
	}
	// 追加
	insert( i, new HISTFREQ( word ) );
}
#if 0
// @param	movetop == true 最後に追加されたものほどindexの大きい方に位置するようにする
// @param	movetop == false 同じ単語があった場合、すでにあった単語はindexの小さいほうのままにする
void TWordHistory::Add( const tchar *word, bool movetop )
{
	int i;
	for ( i=0;i<get_num();i++ ){
		int r = _tcscmp( (*this)[i].Word, word );
		if ( r == 0 ){
			(*this)[i].Num++;
			if (movetop){
				HISTFREQ *p = discard( i );
				add( p );	// 最後のほうへ移動する
			}
			return;
		}
	}
	// 追加
	add( new HISTFREQ( word ) );
}
#endif
// 逆順にソートする
// →汎用性があるのでFlexObjectArray<>に移動してもOK
void TWordHistory::SortReverse( )
{
	int i = 0;
	int j = get_num()-1;
	for(;i<j;i++,j--){
		exchange(i,j);
	}
}
// 汎用性があるのでいずれtnlibへ
void TWordHistory::RemoveNull()
{
	int i = 0;
	// scan first null element.
	for (;i<get_num();i++){
		if (!array[i])
			break;
	}
	int dst = i;
	for (;i<get_num();i++){
		if (array[i]){
			// not null object
			array[dst++] = array[i];
		}
	}
	num = dst;

	// optimize memory allocation.
	slot_num = (num+slot_size-1)/slot_size;
	if (slot_num==0)
		slot_num=1;
	realloc();
}

void TFastReadHistory::Add(const tchar *line, bool movetop)
{
	tnstr word = getkey(line);
	if (WordMap.count(word.c_str())){
		// duplicated.
		int index = WordMap[word.c_str()];
		History[index].Num++;
		if (movetop){
			// Move the element to the tail.
			WordMap[word.c_str()] = History.get_num();	// update the index
			History.add(&History[index]);
			((HISTFREQ**)History.__get_array())[index] = NULL;	// void
		}
		return;
	}
	WordMap[word.c_str()] = History.get_num();	// the index to be added.
	History.add(new HISTFREQ(word));
}

// 
void TWordUrlList::AddDupCheck(const tchar *word, const tchar *url)
{
	vector<tnstr> &item = (*this)[word];
	foreach(item, it, vector<tnstr>){
		if (*it == url){
			return;
		}
	}
	item.push_back(url);
}

// alphasort : 単語順にloadする(高速)
// else : 履歴登録順にloadする(indexが小さいほうが古い)
// reverse : 逆順にする
//		alphasort == true  : 単語の逆順
//		alphasort == false : indexが小さいほうが新しい履歴
// offset : 読み込み開始位置（高速化のため)
//		    ファイルの最後からのoffset
// obsolete function.
bool LoadHistory( TWordHistory &WordHist, bool alphasort, bool reverse, int offset )
{
	return LoadHistory(WordHist, (alphasort?HF_SORT_ALPHA:0)|(reverse?HF_SORT_REVERSE:0), offset);
}

bool LoadHistory( TWordHistory &WordHist, int sort, int offset, TWordUrlList *urls )
{
	WordHist.EnableExtraMode();
	return PdicHistory.GetFile().Load(WordHist, sort, offset, urls);
}

tnstr GetHistoryFileName()
{
	return PdicHistory.GetFile().GetFileName();
}
void SetHistoryFileName(const tchar *filename)
{
	PdicHistory.GetFile().SetFileName(filename);
}

#if 0
// you must delete the return value if not null.
TOFile *CreateHistoryFile(const tchar *filename)
{
	return HistoryFile.Create(filename);
}
#endif

// 現在の履歴ファイルにsrcfileをmerge
// convert_format==true
//	srcfileが現在のtextmodeでなければ変換して、original history fileと置き換える
bool MergeHistoryFile(const tchar *srcfile, bool convert_format)
{
	return PdicHistory.GetFile().Merge(srcfile, convert_format);
}

// ftに前回呼び出したときの値を渡す
bool IsHistoryFileUpdated(__int64 &ft)
{
	__int64 cft = FileTime(PdicHistory.GetFile().GetFileName());

	bool r = cft != ft;
	ft = cft;
	return r;
}

#if 0
static MPdic *GetActiveDic()
{
	TUIMain *main = TUIMain::GetInstance();
	if (!main)
		return NULL;
	return main->GetActiveDic();
}
#endif

static tnstr getkey(const tchar *s)
{
#if EXTENDED
	const tchar *p = _tcschr(s, '\t');
	if (!p) return s;
	return tnstr(s, STR_DIFF(p, s));
#else
	return s;
#endif
}

static tnstr construct(const tchar *word, const tchar *url)
{
#if EXTENDED
	tnstrbuf line;
	line << word << _t("\t") << date2str2( xtime(NULL) );
	if (url && url[0]){
		line << _t("\t") << url;
	}
	return tnstr(line);
#else
	return word;
#endif
}

static void split(const tchar *line, tnstr *word, tnstr *url, timex_t *t)
{
#if EXTENDED
	tnstr_vec items;
	items.split(line, _t("\t"));
	if (items.size()>0){
		if (word) *word = items[0];
		if (items.size()>1){
			if (t) *t = str2date(items[1]);
			if (items.size()>2){
				if (url) *url = items[2];
			}
		}
	}
#else
	word = line;
#endif
}


