#ifndef __Primaryool_h
#define	__Primaryool_h

class PrimaryPool {
public:
	tnstr_vec fw;
protected:
	const class TKCodeTranslateSet *KCodeTrans;
public:
	PrimaryPool( int preallocSize );
	virtual ~PrimaryPool(){}
	int get_num()
		{ return fw.get_num(); }
	virtual int BSearch( const tchar *str )
		{ return BSearchExact( str ); }
	int BSearchExact( const tchar *str );
	int Find( const tchar *str );
	void  SetKCodeTrans(const TKCodeTranslateSet *trans)
		{ KCodeTrans = trans; }
	const class TKCodeTranslateSet *GetKCodeTrans() const
		{ return KCodeTrans; }
};

// MultiPdic—p
class WJPool : public PrimaryPool {
public:
	tnstr_vec fj;
public:
	WJPool( int preallocSize );
	void Add( const tchar *word, const tchar *japa );
};

#endif	/* __Primaryool_h */

