#ifndef __KSTR_H
#define	__KSTR_H

#include "pdconfig.h"

#ifndef struct_TKCodeTranslateSet
#define	struct_TKCodeTranslateSet
struct TKCodeTranslateSet {
	FNPreCodeTranslate encodeKT;
	FNPreCodeTranslate decodeKT;
};
#endif

extern TKCodeTranslateSet KCodeTranslateSetN;

#ifdef DIC_BOCU
#include "dicmix.h"
// _kchar <=> wchar_t conversion class
class __kstr {
protected:
	const struct TKCodeTranslateSet *Translator;
	void *buffer;
	char *newbuffer;
	int length;
	int type;
public:
	__kstr( const TKCodeTranslateSet *trans );
	__kstr( const _kchar *str, const TKCodeTranslateSet *trans );
	__kstr( const _kchar *str, int len, const TKCodeTranslateSet *trans );
	__kstr( const wchar_t *str, const TKCodeTranslateSet *trans );
	~__kstr();
	const TKCodeTranslateSet *GetTranslator() const
		{ return Translator; }
	void clear();
	bool empty() const
		{ return length==0; }
	bool exist() const
		{ return length>0; }
	void set(const _kchar *str);
	void set(const wchar_t *str);
	operator _kchar *();
	operator wchar_t *();
};

class __kstrn : public __kstr {
typedef __kstr super;
public:
	__kstrn( )
		:super(&KCodeTranslateSetN)
	{}
	__kstrn( const _kchar *str )
		:super(str, &KCodeTranslateSetN)
	{}
	__kstrn( const _kchar *str, int len )
		:super(str, len, &KCodeTranslateSetN)
	{}
	__kstrn( const wchar_t *str )
		:super(str, &KCodeTranslateSetN)
	{}
};

// BOCU1 -> UTF8ïœä∑êÍópclass
#if INETDIC
class __kstr_utf8 {
protected:
	void *buffer;
	char *newbuffer;
public:
	__kstr_utf8( const _kchar *s );
	~__kstr_utf8();
	operator const char *();
};
#endif	// INETDIC
#endif	// DIC_BOCU


#endif	// __KSTR_H
