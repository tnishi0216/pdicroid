#include "tnlib.h"
#pragma hdrstop
#include "BookmarkFile.h"
#include "BookmarkMan.h"
#include "pdprof.h"
#include "pdstrlib.h"
#include "pdtime.h"

// file format:
//word<tab>flag<tab>date<tab>comment
// if word is empty, then save as '\x01'.

bool TBookmarkFile::Load(const tchar *filename)
{
	if (!fexist(filename)){
		return true;	// doest not exist;
	}
	TIFile file;
	if (file.open(filename)){
		return false;
	}
	TempItem = NULL;
	LoadOne(file, Root, 0);
	return true;
}
bool TBookmarkFile::LoadOne(TIFile &file, TBookmarkItem &parent, int level)
{
	parent.CreateChild();
	TBookmarkItems &items = *parent.Child;
	CurTab = 0;
	bool warning = false;
	for (;;){
		tnstr line;
		if (file.getline(line)<0)
			break;
		if ( line.empty() )
			continue;
		// count tabs //
		const tchar *p = line;
		while (*p=='\t') p++;

		int tab = STR_DIFF(p,line.c_str());

		tnstr_vec sitems;
		sitems.split(p, _t("\t"));
		if (sitems.size()==0)
			continue;	// no data
		const tchar *word;
		if (sitems[0][0]=='\x01' && !sitems[0][1]){
			word = NULL;
		} else {
			word = sitems[0];
		}
		timex_t t = sitems.size()>=2?str2date(sitems[1]):0;
		if (t==-1)
			xtime(&t);
		int flags = sitems.size()>=3?_ttoi(sitems[2].c_str()):0;
		const tchar *comment = sitems.size()>=4?sitems[3].c_str():NULL;
		TBookmarkItem *item;

		if (tab>level){
			// indent increased.
			TBookmarkItem *new_parent = items.get_num()?&items[items.get_num()-1]:NULL;
			if (!new_parent){
				continue;	// format error, but continue
			}
			item = new TBookmarkItem(new_parent, word, t, flags, comment);
			new_parent->CreateChild()->add(item);
			if (LoadOne(file, *new_parent, tab)){
				warning = true;
			}
			if (CurTab<level)
				// indent decreased
				break;
			if (TempItem){
#if USE_PARENT
				TempItem->Parent = &parent;
#endif
				items.add(TempItem);	// add as sibling from lower layer.
				TempItem = NULL;
			}
			continue;
		}
		// sibling
		item = new TBookmarkItem(&parent, word, t, flags, comment);
		if (tab<level){
			// indent decreased.
			TempItem = item;
			CurTab = tab;
			break;
		}
		items.add(item);
	}
	if (items.get_num()==0){
		parent.DeleteChild();
	}
	return warning;
}
bool TBookmarkFile::Save(const tchar *filename)
{
	TOFile file;
	if (file.create(filename)){
		return false;
	}
	file.settextmode(prof.GetTextFileCode());
	if (prof.IsTextFileBOM())
		file.bom();
	if (!Root.Child)
		return true;	// no data
	return SaveOne(file, *Root.Child, 0);
}

bool TBookmarkFile::SaveOne(TOFile &file, TBookmarkItems &items, int level)
{
	for (int ii=0;ii<items.get_num();ii++){
		TBookmarkItem &item = items[ii];
		tnstrbuf buf;
		for (int i=0;i<level;i++) buf += '\t';
		const tchar *delim = _t("\t");
		if (item.Word.empty()){
			buf += _t("\x01");	// =<blank>
		} else {
			buf += item.Word;
		}
		buf += delim;
		buf += date2str1(item.Date);
		buf += delim;
		buf += itos(item.Flags);
		buf += delim;
		buf += item.Comment;
		file.putline(buf.c_str());
		if (item.Child){
			if (!SaveOne(file, *item.Child, level+1)){
				return false;
			}
		}
	}
	return true;
}

