#include "tnlib.h"
#pragma hdrstop
#include "SrchPatParser.h"
#include "pdconfig.h"
#include "defs.h"
#include "jtype.h"

#define	USE_ANDSRCH	1

//TODO: char funcs
// 欧米文字（単語が半角で区切られる文字）か？
inline bool __iswordchar(ushort c)
{
#ifdef _UNICODE
	return c<0x2000;
#else
#error	// not defined
#endif
}

inline bool __isquote(ushort c)
{
	return c=='"' || c==CODE_QUOTATION;
}
inline bool __isworddelim(ushort c)
{
	return c<=' ' || c==CODE_SPACE;
}

static const tchar *__skipspc(const tchar *str);

// insert the ins to the dp that is terminated at endp.
// [dp]...[endp]
// ^[ins]
// return:
//	new endp.
static tchar *insert(tchar *dp, const tchar *endp, const tchar *ins)
{
	int len = _tcslen(ins);
	memmove(dp+len, dp, (endp-dp)*sizeof(tchar));
	memcpy(dp, ins, len*sizeof(tchar));
	return (tchar*)endp+len;
}

//TODO: move to jtype.h
#define	CODE_ASTERISK	0xFF0A	// ＊
#define	CODE_PLUS		0xFF0B	// ＋
#define	CODE_HYPHEN		0xFF0D	// －
#define	CODE_VERT_LINE	0xFF5C	// ｜

#define	CODE_L_O		0xFF2F
#define	CODE_L_R		0xFF32
#define	CODE_S_O		0xFF4F
#define	CODE_S_R		0xFF52

static bool isOrStr(const tchar *str)
{
	ushort c;
	LD_CHAR(c, str);
	if (tolower(c)=='o' || c==CODE_L_O || c==CODE_S_O){
		LD_CHAR(c, str);
		if (tolower(c)=='r' || c==CODE_L_R || c==CODE_S_R){
			LD_CHAR(c, str);
			return (tuchar)c<=' ' || c==CODE_SPACE;
		}
	}
	return false;
}

// Operation Status class.
class TOpState {
public:
	tchar *wordtop;	// word top pointer in destination buffer. word means including the '|'.
	int op_or;		// 0 : no or operation is used.
					// 1 : operator 'or' or '|' is used.
					// 2 : no operator used, delimiter ' ' is used.
	bool op_brace;	// already braced '(' for or-operation if true.
	bool brace_used;	// real or virtual brace used.

public:
	TOpState(tchar *_wordtop)
		:wordtop(_wordtop)
	{
		op_or = 0;
		op_brace = false;
		brace_used = false;
	}
	inline void openBrace(bool br_used)
	{
		op_brace = true;
		brace_used = br_used;
	}
	inline void closeBrace()
	{
		op_brace = false;
		brace_used = false;
		op_or = 0;
	}
	inline void decBrace()
	{
		op_brace = false;
		brace_used = false;
	}
	// return : next dp
	tchar *setupWordTop(tchar *dp, class TWork &work);
};

#include <stack>
using namespace std;

typedef stack<TOpState> TOpStack;

struct TWork {
	int cnt_brace;	// brace counter
	TOpStack opStack;
	TOpState *curState;

	TWork()
	{
		opStack.push(TOpState(NULL));
		curState = &opStack.top();

		cnt_brace = 0;
	}
	// getter //
	inline bool is_op_or() const
	{
		return curState->op_or;
	}
	inline bool is_op_brace() const
	{
		return curState->op_brace;
	}
	inline bool is_brace_used() const
	{
		return curState->brace_used;
	}
	inline tchar *getWordTop() const
	{
		return curState->wordtop;
	}

	// basic operations //
	inline void openBrace(bool br_used)
	{
		curState->openBrace(br_used);
		cnt_brace++;
	}
	inline void closeBrace()
	{
		curState->closeBrace();
		cnt_brace--;
	}
	inline void decBrace()
	{
		curState->decBrace();
		cnt_brace--;
	}
	inline void _setWordTop(tchar *dp)
	{
		if (!curState->wordtop)
			curState->wordtop = dp;
	}
	//TODO: _setWordTop()にできないか？
	inline void setWordTop(tchar *dp)
	{
		//__assert(!curState->wordtop);	//TODO: ここがassertされなければ_setWordTop()と統合できる
		curState->wordtop = dp;
	}
	inline void resetWordTop()
	{
		curState->wordtop = NULL;
	}
	// mode:
	//	1 : operator '|' or 'or' is used.
	//	2 : delimiter ' ' is used.
	inline void set_op_or(int mode)
	{
		curState->op_or = mode;
	}
	inline void reset_op_or()
	{
		curState->op_or = 0;
	}
	// return : next dp
	tchar *setupWordTop(tchar *dp)
	{
		__assert(!curState->op_or || cnt_brace);
		return curState->wordtop = curState->setupWordTop(dp, *this);
	}
	// Stack operation //
	inline void pop()
	{
		opStack.pop();
		curState = &opStack.top();
	}
	inline void push()
	{
		opStack.push(TOpState(curState->wordtop));
		curState = &opStack.top();
	}
	// Helper //
	// Insert the text to the wordtop.
	inline tchar *insertToWT(tchar *dp, const tchar *ins_text)
	{
		return insert(curState->wordtop, dp, ins_text);
	}
	void close(tchar *dp)
	{
		if (is_op_or() && getWordTop()){
			if (dp[-1]=='|'){
				dp--;	// cancel last or
			}
		}
		while (cnt_brace>0){
			if (dp[-1]=='(' && dp[-2]!='\\'){
				// no content = cancel
				dp--;
			} else {
				*dp++ = ')';
			}
			closeBrace();
			cnt_brace--;
		}
		*dp = '\0';
		resetWordTop();
	}
};

tchar *TOpState::setupWordTop(tchar *dp, TWork &work)
{
	if (op_or){
		// 'a|b c' or 'a b c'
		__assert(op_brace);
//			__assert(curState->cnt_brace);
		if (op_or==1){
			// 'a|b c'
			op_or = 0;
			ST_CHAR(')', dp);
			work.decBrace();
		} else
		if (op_or==2){
			// 'a b c'
			// Continue to process.
		}
	}
	if (!op_brace){
		dp = work.insertToWT(dp, _t("("));
		work.openBrace(false);
	}
	ST_CHAR('|', dp);
	op_or = 2;
	return dp;
}

//next:
//	if + phrase is included, 'next' pointer points to the '+' char.
bool SearchPatternParser(const tchar *str, tnstr_vec &pats, const tchar **next)
{
	str = __skipspc(str);
	tchar *buf = new tchar[_tcslen(str)*3+10+1];	// +10 追加される正規表現記号
	if (!buf)
		return false;	// no memory
	if (next)
		*next = NULL;
	tchar *dp = buf;

	TWork work;
	bool quoted = false;
	ushort c = 0;
	while (1){
		ushort prevc = c;
		LD_CHAR(c, str);
		if (!c){
			break;
		}
		// Preprocess //
		if ((tuchar)c<=' ' || c==CODE_SPACE){
			c = ' ';
			str = __skipspc(str);
		}
		if (prevc==' '){
			if (c==' ')
				continue;
		}
		if (c==CODE_VERT_LINE){
			c = '|';
		} else
		if (c==' ' && isOrStr(str)){
			c = '|';
			str = __skipspc(str+2);
			while (isOrStr(str)){
				// double or
				str = __skipspc(str+2);
				continue;
			}
		} else
		if (c==CODE_QUOTATION){
			c = '"';
		} else
		if (c==CODE_LBR1){
			c = '(';
		} else
		if (c==CODE_RBR1){
			c = ')';
		} else
		if (c==CODE_ASTERISK){
			c = '*';
		}
		// End of preprocess //

		if (__isquote(c)){
			if (quoted){
				if (work.is_op_or()){
					work.closeBrace();
					*dp++ = ')';
				}
				quoted = false;
				work.pop();
				//work.resetWordTop();	//TODO: どっちだ？
			} else {
				if (!work.getWordTop()){
					if (prevc==' '){
						work.close(dp);
						pats.add(buf);
						dp = buf;
						if (c=='-' || c==CODE_HYPHEN){
							*dp++ = '-';
							work.setWordTop(dp);
							continue;
						}
					}
					work.setWordTop(dp);
				} else {
					if (prevc==' '){
#if 1
						// next pattern
						work.close(dp);
						pats.add(buf);
						dp = buf;
#else
						dp = work.setupWordTop(dp);
#endif
					}
				}

				quoted = true;
				work.push();
			}
		} else {
			if (quoted){
				if (c=='*'){
					// wild card
					_tcscpy(dp, _t(".*?"));
					dp += 3;
					str = __skipspc(str);
				} else
				if (c=='|'){
					if (prevc=='|')
						continue;
					if (!work.is_op_or()){
						if (!work.getWordTop()){
							// format error
							goto jesc1;
						}
						if (!work.is_op_brace()){
							dp = work.insertToWT(dp, _t("("));
							work.openBrace(false);
						}
						work.set_op_or(1);
					} else {
						// Already in or mode.
						if (!work.getWordTop()){
							// may be double or.
							continue;
						}
					}
					ST_CHAR(c, dp);
				} else
				if (c=='('){
					// start of 'or'
					if (work.is_op_brace()){
						// already in brace
						goto jesc1;
					}
					ST_CHAR('(', dp);
					work.openBrace(true);
				} else
				if (c==')'){
					if (!work.is_op_or()){
						work._setWordTop(dp);
						*dp++ = '\\';
					}
					// end of'or'
					ST_CHAR(c, dp);
					work.closeBrace();
				} else {
					if (c==' '){
						if (work.is_op_or()){
							work.closeBrace();
							*dp++ = ')';
						}
						ST_CHAR(c, dp);
						work.resetWordTop();
						continue;
					}
					if (c=='-' || c==CODE_HYPHEN){
						*dp++ = '-';
						work.setWordTop(dp);
						continue;
					}
					if (!work.getWordTop()){
						work.setWordTop(dp);
					}
					// normal char
					if (IsRegularChar(c)){
						// escape char.
						ST_CHAR('\\', dp);
					}
					ST_CHAR(c, dp);
				}
			} else {
				if (c==' '){
					continue;
				} else
				if (c=='|'){
//jor2:;
					if (prevc=='|')
						continue;
					if (!work.is_op_or()){
						if (!work.getWordTop()){
							// format error
							goto jesc1;
						}
						if (!work.is_op_brace()){
							dp = work.insertToWT(dp, _t("("));
							work.openBrace(false);
						}
						work.set_op_or(1);
					} else {
						if (!work.getWordTop()){
							// may be double or.
							continue;
						}
					}
					ST_CHAR(c, dp);
				} else
				if (c=='('){
					// start of 'or'
					if (work.is_op_brace()){
						// already in brace
						goto jesc1;
					}
					if (prevc==' '){
						// top of the word, and the previous char is delimiter.
						work.close(dp);
						pats.add(buf);
						dp = buf;
						if (c=='-' || c==CODE_HYPHEN){
							*dp++ = '-';
							work.setWordTop(dp);
							continue;
						}
					}
					// top of the words.
					ST_CHAR('(', dp);
					work.openBrace(true);
				} else
				if (c==')'){
					// end of'or'
					if (!work.is_op_or())
						goto jesc2;
					ST_CHAR(c, dp);
					work.closeBrace();
				} else {
					if (prevc==' '){
						// top of the word, and the previous char is delimiter.
						// next pattern
						if (work.is_op_brace() && !work.is_op_or()){	// if in the brace, at least one '|' operator is required to close.
							// format error
							ST_CHAR(' ', dp);	// store the previous char.
							goto jesc1;
						} else {
							work.close(dp);
							pats.add(buf);
							dp = buf;
							if (c=='-' || c==CODE_HYPHEN){
								c = '-';
								*dp++ = '-';
								work.setWordTop(dp);
								continue;
							}
#if USE_ANDSRCH
							if (c=='+' || c==CODE_PLUS){
								if (next){
									c = '-';
									*next = str-1;
									break;
								}
							}
#endif
						}
						// top of the word.
					}
					// top of the word (top of the strings)
					if (!work.getWordTop()){
						work.setWordTop(dp);
					}
					if ((prevc=='\0'||prevc==' '||prevc=='|'||prevc=='(')
						|| (prevc=='-' && STR_DIFF(dp,buf)==1)){
						if (__iswordchar(c)){
							ST_CHAR('\\', dp);
#ifdef USE_BREGONIG
							ST_CHAR('b', dp);
#else
							ST_CHAR('<', dp);
#endif
						}
					}
jesc1:;
					if (IsRegularChar(c)){
jesc2:;
						// escape char.
						ST_CHAR('\\', dp);
					}
					ST_CHAR(c, dp);
				}
			}
		}
	}
	work.close(dp);
	if (dp!=buf){
		tnstr *s = new tnstr();
		s->setBuf(buf);
		pats.add(s);
	} else {
		delete[] buf;
	}
	return true;
#if 0
jerror:
	if (buf)
		delete[] buf;
	return false;
#endif
}

static const tchar *__skipspc(const tchar *str)
{
	while (*str){
		if (*(tuchar*)str<=' ' || *str==CODE_SPACE){
			NEXT_P(str);
			continue;
		}
		break;
	}
	return str;
}

void split_words(const tchar *str, tnstr_vec &words)
{
	const tchar *p = str;
	const tchar *wordp = p;
	goto jskip;
	while (1){
		if (__isworddelim(*(tuchar*)p)){
			// word delimiter
			if (wordp!=p){
				words.add(new tnstr(wordp, STR_DIFF(wordp, p)));
			}
			if (!*p){
				// End of string.
				break;
			}
jskip:
			p = __skipspc(p);
			wordp = p;	// new word top pointer.
		}
		NEXT_P(p);
	}
}

