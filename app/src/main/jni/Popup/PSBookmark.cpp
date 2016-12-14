//---------------------------------------------------------------------------
#include "tnlib.h"
#pragma hdrstop

#include "PSBookmark.h"
#include "filestr.h"
#include "pdtime.h"
#include "pdprof.h"

//---------------------------------------------------------------------------

#define	MAX_MARKEDWORD_LEN	40	// 最大のmarked word length

#pragma package(smart_init)

#if 0
void LoadPSBookmarkOne(TBookmarkItem &item, TBMLParam &p);
void LoadPSBookmark(class TWordHistory &out, bool alphasort, bool reverse)
{
	Bookmark.Load();
	TBMLParam param(out, alphasort);
	LoadBookmarkOne(Bookmark.Root, param);

#if 0
	if (sort & HF_SORT_FREQ){
		//TODO: SortByNumReversed()を作って高速化
		WordHist.SortByNum();
	} else
#endif
	{
		param.fast.Sort();
	}

	if (reverse)
		out.SortReverse();
}

void LoadBookmarkOne(TBookmarkItem &item, TBMLParam &p)
{
	if (!item.GetChild())
		return;
	TBookmarkItems &items = *item.GetChild();
	for (int i=0;i<items.get_num();i++){
		if (p.alphasort){
			p.out.AddAlphaSorted(items[i].Word);
		} else {
			bool pre_reverse = false;	//TODO: what is used for?
			p.fast.Add(items[i].Word, pre_reverse);
		}
		//p.out.add(new HISTFREQ(items[i].Word));
		if (items[i].GetChild()){
			LoadBookmarkOne(items[i], p);
		}
	}
	
}
#endif

class TPSBookmark {
protected:
	tnstr FileName;
	//vector<int> FileLocs;	// filenameのある行のファイル上の位置
	//map<tnstr, int> FileLoc;	// filenameのある行のファイル上の位置
public:
	TPSBookmark(const tchar *filename)
		:FileName(filename)
	{
	}
	~TPSBookmark()
	{
	}
	const tchar *GetFileName() const { return FileName; }
	bool LoadFileNames(tnstr_vec &files, bool sort=false);
	inline bool LoadFileInfo(const tchar *filename, int &position, tnstr *revision)
		{ return Load(filename, position, revision, NULL); }
	bool Load(const tchar *filename, int &last_position, tnstr *revision, vector<TPSBMItem> *items, const int position=-1);
	bool Parse(const tchar *line, TPSBMItem &item);
	bool ParseFile(const tchar *line, TPSBMFile &item);
	void Build(const TPSBMItem &item, tnstr &line);
	void BuildFile(const tchar *filename, int position, const tchar *revision, tnstr &line);
	bool Save(const tchar *filename, int position, const tchar *revision, const TPSBMItem *item, bool _delete=false);
	bool Delete(const tchar *filename, const TPSBMItem &item)
		{ return Save(filename, -1, NULL, &item, true); }
};

int sortByDate( TPSBMFile **s1, TPSBMFile **s2 )
{
	return (*s2)->date - (*s1)->date;	// discendant order
}
bool TPSBookmark::LoadFileNames(tnstr_vec &files, bool sort)
{
	FlexObjectArray<TPSBMFile> psbmFiles;

	TIFile tif;
	if (tif.open(FileName)) return false;
	while (1){
		tnstr line;
		if (tif.getline(line)<0)
			break;
		if (line.empty()) continue;
		if (line[0]!='\t'){
			// filename
			//FileLoc[line] = tif.tell();
			TPSBMFile file;
			ParseFile(line, file);
			if (file.filename[0]){
				if (sort){
					psbmFiles.add(new TPSBMFile(file));
				} else {
					files.push_back(file.filename);
				}
			}
		}
	}
	if (sort){
		psbmFiles.sort(sortByDate);
		for (int i=0;i<psbmFiles.get_num();i++){
			files.push_back( psbmFiles[i].filename );
		}
	}
	return true;
}

bool TPSBookmark::Load(const tchar *filename, int &last_position, tnstr *revision, vector<TPSBMItem> *items, const int find_position)
{
	last_position = 0;

	TIFile tif;
	if (tif.open(FileName)) return false;
	bool found = false;
	while (1){
		tnstr line;
		if (tif.getline(line)<0)
			break;
		if (line.empty()) continue;
		if (line[0]!='\t'){
			// filename
			if (found)
				break;	// end of load
			TPSBMFile file;
			ParseFile(line, file);
			if (file.filename.empty()) continue;
			if (IsSameFile(file.filename, filename)){
				found = true;
				last_position = file.position;
				if (revision) revision->set( file.revision );
				if (!items) return true;
			}
		} else {
			// bookmark item
			if (found){
				TPSBMItem item;
				if (Parse(line, item)){
					if (find_position!=-1){
						if (item.position == find_position){
							items->push_back(item);
							return true;
						}
					} else {
						items->push_back(item);
					}
				}
			}
		}
	}
	return true;
}
// <改行>→"\r" or "\n" <タブ>→"\t" \→"\\"
bool escape_ctrl(const tchar *str, tnstrbuf &out)
{
	bool escaped = false;
	const tchar *top = str;
	while (1){
		tchar c = *str++;
		if (!c) break;
		if (c<' '){
			int len = STR_DIFF(str, top);
			if (len>1) out.cat(top, len-1);
			top = str;
			escaped = true;
#if 0
			if (c=='\r')
				out << _t("\\r");
			else
#endif
			if (c=='\n')
				out << _t("\\n");
			else
			if (c=='\t')
				out << _t("\\t");
			// else // other control codes dropped
		}
	}
	int len = STR_DIFF(str, top);
	if (len>1) out.cat(top, len-1);
	return escaped;
}
bool unescape_ctrl(const tchar *str, tnstrbuf &out)
{
	bool escaped = false;
	const tchar *top = str;
	while (1){
		tchar c = *str++;
		if (!c) break;
		if (c=='\\'){
			escaped = true;
			int len = STR_DIFF(str, top);
			if (len>1) out.cat(top, len-1);
			c = *str++;
			top = str;
			if (!c) break;
			if (c=='t')
				out << _t("\t");
#if 0
			else
			if (c=='r')
				out << _t("\r");
#endif
			else
			if (c=='n')
				out << _t("\n");
			// else // other control codes dropped
		}
	}
	int len = STR_DIFF(str, top);
	if (len>1) out.cat(top, len-1);
	return escaped;
}
bool TPSBookmark::Parse(const tchar *line, TPSBMItem &item)
{
	tnstr_vec s;
	s.split(line, _t("\t"));
	if (s.size()>0){
		tnstrbuf buf;
		if (s.size()>1){ // item.date = _ttoi(s[1]);
		if (s.size()>2){ item.position = _ttoi(s[2]);
		if (s.size()>3){ item.length = _ttoi(s[3]);
		if (s.size()>4){ item.style = _ttoi(s[4]);
		if (s.size()>5){ item.color = (unsigned)_ttoi(s[5]);
		if (s.size()>6){ if (unescape_ctrl(s[6], buf)) item.marked_word = buf; else item.marked_word = s[6];
		if (s.size()>7){ if (unescape_ctrl(s[7], buf)) item.comment = buf; else item.comment = s[7];
		}}}}}}
		}
	}
	return true;
}
bool TPSBookmark::ParseFile(const tchar *line, TPSBMFile &item)
{
	tnstr_vec s;
	s.split(line, _t("\t"));
	item.position = 0;
	item.date = 0;
	if (s.size()>0){
		tnstrbuf buf;
		if (s.size()>0){ item.filename = s[0];
		if (s.size()>1){ item.date = _ttoi(s[1]);
		if (s.size()>2){ item.position = _ttoi(s[2]);
		if (s.size()>3){ item.revision = s[3];
		}}}}
	}
	return true;
}
void TPSBookmark::Build(const TPSBMItem &item, tnstr &line)
{
	tnstr alt_marked_word;
	const tchar *marked_word = item.marked_word;
	if (item.length>MAX_MARKEDWORD_LEN){
		alt_marked_word.set(item.marked_word, MAX_MARKEDWORD_LEN);
		marked_word = alt_marked_word;
	}

	tnstrbuf alt_marked_word2;
	if (escape_ctrl(marked_word, alt_marked_word2)){
		marked_word = alt_marked_word2;
	}

	tnstrbuf alt_comment;
	const tchar *comment = item.comment;
	if (escape_ctrl(item.comment, alt_comment)){
		comment = alt_comment;
	}
	
	tnstrbuf buf;
	buf << _t("\t") << date2str2(xtime(NULL)) << _t("\t") << itos(item.position) << _t("\t") << itos(item.length) << _t("\t") << itos(item.style) << _t("\t") << itos(item.color) << _t("\t") << marked_word << _t("\t") << comment;
	line = buf;
}
void TPSBookmark::BuildFile(const tchar *filename, int position, const tchar *revision, tnstr &line)
{
	tnstrbuf buf;
	if (position==-1) position = 0;
	buf << filename << _t("\t") << date2str2(xtime(NULL)) << _t("\t") << itos(position);
	if (revision){
		buf << _t("\t") << revision;
	}
	line = buf;
}
// position : file cursor position - -1にすると原則書き換えない
// item : can be null if write a position only.
// _delete: 削除。positionが一致したものが削除される
bool TPSBookmark::Save(const tchar *filename, int position, const tchar *revision, const TPSBMItem *item, bool _delete)
{
	// 対象ファイル以外はそのまま行単位コピー
	// 対象ファイルの時は、行を比較し、挿入行を探す
	// 同じ行が存在する場合は、桁を比較する
	// 行と桁が一致する場合は上書きする(長さは関係ない)
	// markした範囲が重複した場合はどうする？

	tnstr tmpfile( FileName );
	tmpfile += _t(".tmp");
	TIFile inf;
	if (inf.open(FileName)){
		if (_delete) return true;

		// no file -> create a new file
		TOFile tof;
		if (tof.create(FileName)){
			return false;
		}

		tof.settextmode(prof.GetTextFileCode());
		if (prof.IsTextFileBOM())
			tof.bom();

		tnstr line;
		BuildFile(filename, position, revision, line);
		tof.putline(line);
		if (item){
			Build(*item, line);
			tof.putline(line);
		}
		return true;
	}
	TOFile tmpf;
	if (tmpf.create(tmpfile)){ return false; }

	tmpf.settextmode(prof.GetTextFileCode());
	if (prof.IsTextFileBOM())
		tmpf.bom();

	bool found = false;
	int last_position = 0;
	bool done = false;
	while (1){
		tnstr line;
		if (inf.getline(line)<0)
			break;
		if (line.empty()) continue;
		if (!done){
			if (line[0]!='\t'){
				// filename
				if (found && !done && item){
					// add a new line
					tnstr newline;
					Build(*item, newline);
					tmpf.putline(newline);
					done = true;
				}
				TPSBMFile file;
				ParseFile(line, file);
				if (file.filename.empty()) continue;
				if (!found){
					if (IsSameFile(file.filename, filename)){
						found = true;
						last_position = file.position;
						if (position!=-1 /*&& last_position != position*/){
							// rewrite last file position
							BuildFile(filename, position, revision, line);
						}
					}
				}
			} else {
				if (item){
					if (found){
						TPSBMItem pitem;
						if (Parse(line, pitem)){
							bool ins = false;		// insert this item
							bool replace = false;	// replace with this item
							if (pitem.position==item->position){
								ins = true;
								replace = true;
							}
							if (pitem.position > item->position){
								ins = true;
							}
							if (ins){
								if (!_delete){
									tnstr newline;
									Build(*item, newline);
									tmpf.putline(newline);
								}
								done = true;
								if (replace) continue;
							}
						}
					}
				}
			}
		}
		tmpf.putline(line);
	}
	if (!done && !_delete){
		// add a new file
		tnstr line;
		if (!found){
			BuildFile(filename, position, revision, line);
			tmpf.putline(line);
		}
		if (item){
			Build(*item, line);
			tmpf.putline(line);
		}
	}

	inf.close();
	tmpf.close();
	if (!DeleteFile(FileName)){
		DBW("DeleteFile failed: %ws (%d)", FileName.c_str(), GetLastError());
	}
	if (!MoveFile(tmpfile, FileName)){
		DBW("MoveFile failed: %ws (%d)", FileName.c_str(), GetLastError());
	}
	return true;
}

static TPSBookmark *PSBookmark = NULL;

void OpenPSBookmark(const tchar *filename)
{
	if (PSBookmark) return;
	PSBookmark = new TPSBookmark(filename);
}

void ClosePSBookmark()
{
	if (!PSBookmark) return;
	delete PSBookmark;
	PSBookmark = NULL;
}

// 	  - filename
//		<\t>date<\t>line<\t>column<\t>length<\t>style<\t>color<\t>marked_word<\t>comment ←１ファイルに複数行
//void SavePSBookmark(const tchar *filename, int line, int col, int len, int style, COLORREF color, const tchar *marked_word, const tchar *comment)
void SavePSBookmark(const tchar *filename, const TPSBMItem &item)
{
	if (!PSBookmark){
		DBW("!!!Need OpenPSBookmark");
		return;
	}

	PSBookmark->Save(filename, -1, NULL, &item);
}
bool SavePSFileInfo(const tchar *filename, int position, const tchar *revision)
{
	if (!PSBookmark){
		DBW("!!!Need OpenPSBookmark");
		return false;
	}
	return PSBookmark->Save(filename, position, revision, NULL);
}
bool DeletePSBookmark(const tchar *filename, const TPSBMItem &item)
{
	if (!PSBookmark){
		DBW("!!!Need OpenPSBookmark");
		return false;
	}

	return PSBookmark->Delete(filename, item);
}

void LoadPSBookmark(const tchar *filename, vector<TPSBMItem> &items)
{
	// 対象ファイルのbookmark情報を取得する
	if (!PSBookmark){
		DBW("!!!Need OpenPSBookmark");
		return;
	}
	int last_position;
	PSBookmark->Load(filename, last_position, NULL, &items);
}
bool LoadPSBookmarkItem(const tchar *filename, int position, TPSBMItem &item)
{
	if (!PSBookmark){
		DBW("!!!Need OpenPSBookmark");
		return false;
	}
	vector<TPSBMItem> items;
	int last_position;
	if (PSBookmark->Load(filename, last_position, NULL, &items, position)){
		if (items.size()){
			item = items[0];
			return true;
		}
	}
	return false;
}
// ファイルの一覧を取得
void LoadPSBookmarkFiles(tnstr_vec &files, bool sort)
{
	if (!PSBookmark){
		DBW("!!!Need OpenPSBookmark");
		return;
	}
	PSBookmark->LoadFileNames(files, sort);
}

bool LoadPSFileInfo(const tchar *filename, int &position, tnstr *revision)
{
	if (!PSBookmark){
		DBW("!!!Need OpenPSBookmark");
		return false;
	}
	return PSBookmark->LoadFileInfo(filename, position, revision);
}

const tchar *GetPSBookmarkFileName()
{
	if (!PSBookmark) return NULL;
	return PSBookmark->GetFileName();
}

