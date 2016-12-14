#ifndef __MouseCapBase_h
#define	__MouseCapBase_h

//	ヒットテスト項目・結果
#define	HT_NONE			0
#define	HT_WORDITEM		0x010000		// 英単語の選択
									// 下位８ビットが項目インデックス
#define	HT_MEMORY		0x800000u		// "暗"
#define	HT_MODIFY		0x400000		// "修"
#define	HT_OUT			0x008000		// 暗、修のとき、マウスの位置がはずれた場合
// 注：ヒットテスト項目にHT_WORDITEMを指定するとHT_MEMORYかHT_MODIFYが
// 返ってくることがある。そのときはHT_WORDITEMは付かない。
// また、下位８ビットの意味は変わらない

//#define	HT_TITLEBAR		0x020000		// タイトルバー
//#define	HT_TITLEBORDER	0x040000		// タイトルバーの境界

#define	HT_UPPER		0x080000		// 表示領域より上の方
#define	HT_LOWER		0x100000		// 表示領域より下の方
//#define	HT_MSGBAR		0x200000		// メッセージバー

#define	HT_MASK			0xff0000
#define	HT_MASK_TYPE	HT_MASK
#define	HT_MASK_INDEX	0x00ffff
#define	HT_MASK_ALL		0xffffff

class TMouseViewIFBase {
public:
	virtual void SetHintText(const tchar *msg){}
};

class TMouseCaptureBase {
protected:
	TMouseViewIFBase &View;
	int fCapturing;
public:
	TMouseCaptureBase(TMouseViewIFBase &view);
	TMouseViewIFBase &GetView() const
		{ return View; }
	bool IsCapturing() const
		{ return fCapturing?true:false; }
	void SetCapturing(int capturing)	// wsobj.cpp専用 -> 削除したい
		{ fCapturing = capturing; }
};

#endif	// __MouseCapBase_h

