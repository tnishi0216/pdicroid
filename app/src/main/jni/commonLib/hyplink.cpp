#include "tnlib.h"
#pragma	hdrstop
#include "hyplink.h"
#include "draw4def.h"

THyperLink::THyperLink()
{
	type = HLT_NONE;
	item = 0;
	unnamed = 0;
	SetRect( &area.rect, 0, 0, 0, 0 );
	area.cy = 0;
	length = 0;
}
THyperLink::~THyperLink()
{
}
void THyperLink::Clear()
{
	type = HLT_NONE;
	item = 0;
	state = 0;
	key.clear();
	SetRect( &area.rect, 0, 0, 0, 0 );
	area.cy = 0;
	length = 0;
}
struct NameList {
	const tchar *name;
	int len;
	int skiplen;
	int type;
};
static const NameList namelist[] =
{
	{ _T("word:"), 5, 5, HLT_WORD },
	{ _T("Å®"), BYTETOLEN(2), BYTETOLEN(2), HLT_WORD2 },
	{ _T("http:"), 5, 0, HLT_HTTP },
	{ _T("https:"), 6, 0, HLT_HTTP },
	{ _T("mailto:"), 7, 0, HLT_MAILTO },
	{ _T("file:"), 5, 5, HLT_FILE },
	{ _T("text:"), 5, 5, HLT_TEXT },
//	{ _T("html:"), 5, HLT_HTML },	// HTMLÇÃpop-up
};
void THyperLink::GetKeyWord( tnstr &word, const tchar *text )
{
	if ( type == HLT_EPWING || key[0] ){
		word = key;
	} else
	if ( text ){
		text += loc;
		const tchar *start = text;
		text += namelist[type-1].skiplen+(*start =='<' ? 1 : 0);
		word.set( text, length - (text-start) - (*start =='<' ? 1 : 0) );
	}
}
// this->type is required to parse the no-bracket http text correctly.
tchar *THyperLink::GetLink(const tchar *p, const tchar *_text, const tchar *text)
{
	if ( p > _text && p[-1] == '<' ){
		// enveloped by <>
		p--;
		text = _tcschr( text, '>' );
		if ( text ){
			text++;
		} else {
			text = p + _tcslen(p);
		}
	} else {
		// no <>
		if (this->type==HLT_HTTP){
			for (;;){
				tuchar c = *text;
				if (IsHypLinkTerm(c))	// terminator
					break;
				text++;
			}
		} else {
			for (;;){
				if ( (tuchar)*text <= ' ' || *text == ',' )	// terminator
					break;
				text++;
			}
		}
	}
	this->length = (int)(text - p);
	this->loc = (int)(p-_text);
	return (tchar*)text;
}
//
// THyperLinks class
//
THyperLinks::THyperLinks( )
{
	done = false;
	req_parse = 0;
	nextIndex = ~0;
	tag = NULL;
}
//TODO: Ç¢Ç∏ÇÍdraw4Ç…ìùçá
int THyperLinks::ExtractStaticWords( byte _item, const tchar *text )
{
	req_parse = 0xffff;

	const tchar *_text = text;
	const tchar *p;
	int n = 0;
	while ( *text ){
		text = _tcschr( text, ':' );
		if ( !text )
			break;
		text++;
		for ( int i=0;i<sizeof(namelist)/sizeof(NameList);i++ ){
			if ( (int)(text - _text) >= namelist[i].len
				&& !_tcsncmp( text-namelist[i].len, namelist[i].name, namelist[i].len ) ){
				p = text - namelist[i].len;
				THyperLink *hl = new THyperLink;
				hl->type = namelist[i].type;
				hl->item = _item;
				text = hl->GetLink(p, _text, text);
				add( hl );
				n++;
				break;
			}
		}
	}
	text = _text;
	while ( *text ){
#ifdef _UNICODE
		text = _tcschr( text, _T('Å®') );
		if ( !text )
			break;
#else
		text = _tcschr( text, 0x81 );	// Å®
		if ( !text )
			break;
		if ( (tuchar)text[1] == 0xA8 )
#endif
		{
			p = text;
			if ( p > _text && p[-1] == '<' ){
				p--;
				text = _tcschr( text, '>' );
				if ( text ){
					text++;
				} else {
					text = _T("");
				}
			} else {
#if 0	// Å®word ÇæÇØÇÕÇÕÇ∏ÇµÇΩ
				for (;;){
					if ( (tuchar)*text <= ' ' || *text == ',' )
						break;
					text++;
				}
#else
				goto jmp1;
#endif
			}
			THyperLink *hl = new THyperLink;
			hl->type = HLT_WORD2;
			hl->item = _item;
			hl->length = STR_DIFF(text,p);
			hl->loc = STR_DIFF(p,_text);
			add( hl );
			n++;
		}
#ifdef _UNICODE
		continue;
	jmp1:;
			text++;
#else
		else {
	jmp1:;
			if ( !text[1] ){
				break;
			}
			text += 2;
		}
#endif
	}
	return n;
}

void THyperLinks::StartEnum()
{
	nextIndex = 0;
}
THyperLink *THyperLinks::Next(byte type)
{
	for (;nextIndex<size();nextIndex++){
		THyperLink &hl = (*this)[nextIndex];
		if (hl.type == type){
			nextIndex++;
			return &hl;
		}
	}
	return NULL;
}

