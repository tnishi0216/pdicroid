#ifndef	__POOL_H
#define	__POOL_H

#include "japa.h"
#include "wsqudef.h"
#if USE_DT2
#include "hyplink.h"
#endif

#define	MIN_NDISP	5		// 最小表示行数

#ifdef SMALL
#define	MAX_NDISP	64
#else
#if defined(__WIN32__)
#define MAX_NDISP	256
#elif defined(__ANDROID__)
#define MAX_NDISP	64
#else
#define MAX_NDISP	512
#endif
#endif

#ifdef USE_TREE
#define	ST_CLOSE	0
#define	ST_EXPAND	1
#define	ST_OPEN		2
#endif

#define	USE_DISPLEVEL	0

#include "PrimaryPool.h"

struct TPoolItem {
	Japa *japa;
	int fdn;			// マージ表示のときは重複単語数
#if USE_DISPLEVEL
	int level;			// display level
#endif
#if USE_DT2
	THyperLinks hl;
#endif
	TPoolItem(Japa *_japa, int _fdn, int _level);
	~TPoolItem();
#if USE_DT2
	int ExtractStaticWords( THyperLinks *hls, const Japa *japa );
#endif
	void UpdateHyperLink( );
};

class Pool : public PrimaryPool {
typedef PrimaryPool super;
protected:
	FlexObjectArray<TPoolItem> Items;
public:
	Pool( );
	~Pool();
	virtual int BSearch( const tchar *str );
	virtual void Clear();
	virtual void Add( const tchar *word, const Japa *japa, int dicno, int level=0 );
	virtual void AddDirect( tnstr *word, Japa *japa, int dicno, int level=0 );
	virtual void Insert( int i, const tchar *word, const Japa *japa, int dicno, int level=0 );
	virtual void InsertDirect( int i, tnstr *word, Japa *japa, int dicno, int level=0 );
	void UpdateHyperLink( int i );
	virtual void Del( int i );
	virtual void Del(int si, int ei);
	void Keep( int bi, int ei );	// biからeiのみを残して後は削る
	//bool Keep( const tchar *str, int len, int &i1 );	// 変化無しの場合はfalseを返す
	void GetKeepLocK( const tchar *str, int len, int &i1, int &i2 );
	void GetKeepLocC( const tchar *str, int len, int &i1, int &i2 );
	//bool Included( const tchar *str, int len, bool ignorecase );
#if USE_DT2
	int ExtractStaticWords( THyperLinks *hl, const Japa *japa );
#endif
#if defined(USE_JLINK) && USE_DT2==1
	int SearchHyperLinks( int i )	// EPWINGの自動リンク検索を行う
		{ return Items[i].japa->jlinks.SearchHyperLink( Items[i].hl ); }
#endif
	const tchar *GetText( int no, int sqmno );

	// setter/getter //
	wa_t GetAttr(int index)
		{ return Items[index].japa->GetAttr(); }
	void SetAttr(int index, wa_t attr)
		{ Items[index].japa->SetAttr(attr); }

	Japa &GetJapaObj(int index)
		{ return *Items[index].japa; }
	void SetJapa(int index, const Japa &japa)
		{ *Items[index].japa = japa; }
	const tchar *GetJapa(int index)
		{ return Items[index].japa->japa; }
	const tchar *GetExp(int index)
		{ return Items[index].japa->exp; }
	const tchar *GetPron(int index)
		{ return Items[index].japa->pron; }
#ifdef USE_JLINK
	JLinks &GetJLinks(int index)
		{ return Items[index].japa->jlinks; }
#endif

	int GetFDN(int index)
		{ return Items[index].fdn; }
	void SetFDN(int index, int fdn)
		{ Items[index].fdn = fdn; }

	int GetDispLevel(int index)
#if USE_DISPLEVEL
		{ return Items[index].level; }
#else
		{ return 0; }
#endif

#if USE_DT2
	THyperLinks &GetHL(int index)
		{ return Items[index].hl; }
#endif

#if 0
	// 通常の検索の再検索に使用する見出し語の取得
	// Seamより前の見出し語の場合はNULLを返す
	const tchar *GetTopSearchWord()
	{
		return SeamOffset>=0?fw[SeamOffset].c_str():NULL;
	}
	const tchar *GetLastSearchWord()
	{
		return SeamOffset>=get_num()?NULL:fw[get_num()-1].c_str();
	}
#endif
};

// 要素がupdateされるときにviewを呼び出す
class IPoolViewer {
public:
	virtual ~IPoolViewer() {}
	virtual void Clear() = 0;
	virtual void Add(tnstr *word, Japa *japa, int dicno, int level) = 0;
	virtual void Insert(int index, tnstr *word, Japa *japa, int dicno, int level) = 0;
	virtual void Del(int index) = 0;
	virtual void Del(int index1, int index2) = 0;
	//virtual void Update(int index) = 0;
};
class PoolView : public Pool {
typedef Pool super;
protected:
	IPoolViewer *Viewer;	// owner
public:
	PoolView(IPoolViewer *viewer);
	~PoolView();
	IPoolViewer *GetViewer() const { return Viewer; }
	virtual void Clear();
	//virtual void Add( const tchar *word, const Japa *japa, int dicno, int level=0 );
	virtual void AddDirect( tnstr *word, Japa *japa, int dicno, int level=0 );
	//virtual void Insert( int i, const tchar *word, const Japa *japa, int dicno, int level=0 );
	virtual void InsertDirect( int i, tnstr *word, Japa *japa, int dicno, int level=0 );
	virtual void Del( int i );
	virtual void Del(int si, int ei);
	//void Keep( int bi, int ei );	// biからeiのみを残して後は削る
	//bool Keep( const tchar *str, int len, int &i1 );	// 変化無しの場合はFALSEを返す
};

#endif	// __POOL_H
