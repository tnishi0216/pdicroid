#include "tnlib.h"
#pragma hdrstop
#include "dicmix.h"
#include "LangProc.h"
#include "dicUtil.h"

void split_word(const tchar *word, tnstr *cword, tnstr *kword)
{
	const tchar *p = word;
	for(;*p;){
		if (*p=='\t')
			break;
		p++;
	}
	if (!*p){
		if (cword) cword->set(word, (int)(p-word));
		if (kword) kword->clear();
		return;
	}
	if (kword) kword->set(word, (int)(p-word));
	p++;
	if (cword) *cword = p;
}
tnstr extract_cword(const tchar *word)
{
	const tchar *p = word;
	for(;*p;){
		if (*p=='\t'){
//			if (p[1]=='\t' || !p[1])
//				return word;	// illegal string?
			return tnstr(p+1);
		}
		p++;
	}
	return word;
}

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

const _kchar *find_cword_pos(const _kchar *word)
{
	const _kchar *p = word;
	for(;*p;){
		if (*p=='\t'){
			return p+1;
		}
		p++;
	}
	return word;
}

#if 0
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
#elif 1	// kword!=cword‚Ì‚Ý
	if (!kword[0]){
		return cword;
	}
	if (_tcscmp(kword, cword)==0){
		return cword;
	}
	return tnstr(kword, _t("\t"), cword);
#else
	// keyword‚ÌŽ©“®•t‰Á‚È‚µ
	return cword;
#endif
}
#endif
// return value : word - composit_word
// composit_word‚Ìkeyword•”‚Å”äŠr
int comp_word(const tchar *word, const tchar *composit_word)
{
	for(;*word;){
		tchar cc = *composit_word++;
		if (!cc)
			return 1;
		if (cc=='\t')
			return 1;
		int ret = *word++ - cc;
		if (ret!=0)
			return ret;
	}
	return *composit_word=='\t'?0:-1;
}
// return value : word - composit_word
// composit_word‚Ìkeyword•”‚Å”äŠr
int comp_word(const _kchar *word, const _kchar *composit_word)
{
	for(;*word;){
		_kchar cc = *composit_word++;
		if (!cc)
			return 1;
		int ret = (int)*(uint8_t*)word++ - (int)(uint8_t)cc;
		if (ret!=0)
			return ret;
		if (cc=='\t')
			return 0;
	}
	return *composit_word=='\t'?0:-1;
}
#if 0
// word‚©‚çkword+word‚ðì¬‚·‚é
// word‚ª‚·‚Å‚Écomposit word‚Ìê‡‚Í‰½‚à‚µ‚È‚¢
tnstr create_composit_word(TLangProc *proc, const tchar *word)
{
	if (is_composit_word(word)){
		return word;
	} else {
		return join_word(word, proc->Normalize(word));
	}
}

tnstr create_kword(TLangProc *proc, const tchar *word)
{
	return proc->Normalize(word);
}

#endif
