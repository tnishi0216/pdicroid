#include <jni.h>
#include <wchar.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include "pdclass.h"
#include "android.h"
#include "PSBookmark.h"

// View Flags for WordLsit //
//#define	VF_NUMBER	1
//#define	VF_ATTR	2
//#define	VF_WORD	4
#define	VF_PRON	0x08
//#define	SNF_LEVEL	0x10
//#define	SNF_JAPA	0x20
#define	VF_EXP		0x20		// 注意
//#define	VF_EXP1	0x40
//#define	SNF_EPWING	0x80		// ﾌｫﾝﾄ処理でのみ使用
//#define	SNF_POPUP	0x100		// popup search window(フォント処理でのみ使用）
#define	VF_ALL		0xFFFF

#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, "PDJ", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "PDJ", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "PDJ", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "PDJ", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "PDJ", __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "PDJ", __VA_ARGS__)

tnstr AppInternalPath;
tnstr TempPath;

void DeleteOldInternalFiles();
bool unlinkDocPath(const char *filename);
bool CopyInternalFiles(AAssetManager *mgr);
bool LoadAssetFile(AAssetManager *mgr, const char *src, const char *dstpath, const char *dstname);
bool InitLangProc();
void LoadAllProfile( );

// JNI Utilities //
class JString {
protected:
	jstring str;
	int str_length;
	std::wstring wstr;
	JNIEnv *env;
	const jchar *jstr;
	__tnstrA utf8;
public:
	JString(JNIEnv *_env, jstring _str)
		:env(_env)
		,str(_str)
	{
		str_length = str ? env->GetStringLength(str) : 0;
		if (str_length>0){
			jstr = env->GetStringChars(str, NULL);
			wstr.assign(jstr, jstr+str_length);
		} else {
			jstr = NULL;
		}
	}
	~JString(){
		if (jstr){
			env->ReleaseStringChars(str, jstr);
		}
	}
	bool empty() { return !jstr || !jstr[0]; }
	inline const wchar_t *get() const { return wstr.c_str(); }
	inline const wchar_t *c_str() const { return wstr.c_str(); }
	wchar_t operator [](int i){
		return wstr[i];
	}
	operator const wchar_t *(){ return wstr.c_str(); }
	const char *c_utf8() {
		if (utf8.empty()){
			utf8.setUTF8(wstr.c_str());
		}
		return utf8;
	}
};

class JReturn {
public:
	JReturn(JNIEnv *env, jobject obj, const tchar *str)
	{
		jclass clsj = env->GetObjectClass(obj);
		jfieldID fid = env->GetFieldID(clsj, "retString", "Ljava/lang/String;");
		jstring jstr = env->NewStringUTF(__cstr(str).utf8());
		env->SetObjectField(obj, fid, jstr);
		env->DeleteLocalRef(jstr);
	}
	JReturn(JNIEnv *env, jobject obj, int val)
	{
		jclass clsj = env->GetObjectClass(obj);
		jfieldID fid = env->GetFieldID(clsj, "retInt", "I");
		env->SetIntField(obj, fid, val);
	}
};


extern "C" {
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_inittest(JNIEnv* env, jint param1)
{
	return 0;
}

// tempPath: temporary path (メモリ不足時解放）
// appInternalPath: アプリ固有のstorage(application uninstallで削除)
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_initPdic(JNIEnv* env, jint param1, jclass thiz, jobject assetManager, jstring tempPath, jstring appInternalPath)
{
	if (!assetManager){
		return -1;
	}
	AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
	if (!mgr){
		return -2;
	}

	JString wtempPath(env, tempPath);
	TempPath = wtempPath;
	JString wappInternalPath(env, appInternalPath);
	AppInternalPath = wappInternalPath;

	InitAndroid(wtempPath);
	
	DeleteOldInternalFiles();

	if (!CopyInternalFiles(mgr)){
		return -3;
	}

	if (!InitLangProc()){
		return -4;
	}
	LoadAllProfile( );
	return 0;
}
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_cleanupPdic(JNIEnv* env)
{
	return 0;
}

} /* "C" */

#include "SquFrm.h"
#include "SquInterface.h"
#include "UIMain.h"
#include "dicgrp.h"
#include "windic.h"
#include "LangProc.h"
//#include "dicname.h"

jobject jniCallback;

class TWordEdit : public IWordEdit {
public:
	virtual tnstr GetText(){ return _t(""); }
};
//Note: Javaのthreadで動く必要があるため、envは最新である必要がある
class TWordList : public IWordList {
protected:
	JNIEnv *env;
	jmethodID midClear;
	jmethodID midAdd;
	jmethodID midInsert;
	jmethodID midDelete;
	jmethodID midSelect;
	int ViewFlags;
public:
	TWordList()
	{
		env = NULL;
		midClear = NULL;
		midAdd = NULL;
		midInsert = NULL;
		midDelete = NULL;
		midSelect = NULL;
		ViewFlags = VF_ALL;
	}
	void SetEnv(JNIEnv *_env)
	{
		env = _env;
	}
	void SetView(int flags){
		ViewFlags = flags;
	}
	virtual void Clear(){
		if (!midClear){
			jclass jniCallbackClass = env->GetObjectClass(jniCallback);
			midClear = env->GetMethodID(jniCallbackClass, "clearWords", "()V");
		}
		env->CallVoidMethod(jniCallback, midClear);
	}
	virtual void Add(const tchar *word, const tchar *japa, const tchar *pron, const tchar *exp)
	{
		if (!midAdd){
			jclass jniCallbackClass = env->GetObjectClass(jniCallback);
			midAdd = env->GetMethodID(jniCallbackClass, "addWord", "(Ljava/lang/String;Ljava/lang/String;)I");
		}
		tnstrbuf text;
		if ((ViewFlags & VF_PRON) && pron && pron[0]){
			text += pron;
			text += _t(" ");
		}
		text += japa;
		if ((ViewFlags & VF_EXP) && exp && exp[0]){
			text += _t(" / ");
			text += exp;
		}
#if 0
		DBW("Add: %08X %d: %s", env, _tcslen(word), __cstr(word).utf8());
#endif
		jstring str1 = env->NewStringUTF(__cstr(word).utf8());
		jstring str2 = env->NewStringUTF(__cstr(text).utf8());
		env->CallIntMethod(jniCallback, midAdd, str1, str2);
		env->DeleteLocalRef(str1);
		env->DeleteLocalRef(str2);
	}
	virtual void Insert(int index, const tchar *word, const tchar *japa, const tchar *pron, const tchar *exp)
	{
		if (!midInsert){
			jclass jniCallbackClass = env->GetObjectClass(jniCallback);
			midInsert = env->GetMethodID(jniCallbackClass, "insertWord", "(ILjava/lang/String;Ljava/lang/String;)I");
		}
		tnstrbuf text;
		if ((ViewFlags & VF_PRON) && pron && pron[0]){
			text += pron;
			text += _t(" ");
		}
		text += japa;
		if ((ViewFlags & VF_EXP) && exp && exp[0]){
			text += _t(" / ");
			text += exp;
		}
#if 0
		DBW("Ins: %08X %d: %s", env, _tcslen(word), __cstr(word).utf8());
#endif
		jstring str1 = env->NewStringUTF(__cstr(word).utf8());
		jstring str2 = env->NewStringUTF(__cstr(text).utf8());
		env->CallIntMethod(jniCallback, midInsert, index, str1, str2);
		env->DeleteLocalRef(str1);
		env->DeleteLocalRef(str2);
	}
	virtual void Delete(int index)
	{
		if (!midDelete){
			jclass jniCallbackClass = env->GetObjectClass(jniCallback);
			midDelete = env->GetMethodID(jniCallbackClass, "deleteWord", "(I)V");
		}
		env->CallVoidMethod(jniCallback, midDelete, index);
	}
	virtual void Select(int index, int rev)
	{
		if (!midSelect){
			jclass jniCallbackClass = env->GetObjectClass(jniCallback);
			midSelect = env->GetMethodID(jniCallbackClass, "select", "(II)V");
		}
		env->CallVoidMethod(jniCallback, midSelect, index, rev);
	}
};

TWordEdit *WordEdit = NULL;
TWordList *WordList = NULL;
TUIMain *UIMain = NULL;
TPdicMain *Main = NULL;
TSquareFrame *Frame = NULL;
int FrameInstance = 0;

class TPdicMainImpl : public TPdicMain {
typedef TPdicMain super;
protected:
	TSquareFrame *squfrm;	// reference
public:
	TPdicMainImpl():squfrm(NULL) {}
	void SetFrame(TSquareFrame *squfrm){ this->squfrm = squfrm; }
	virtual MPdic *GetActiveDic() { return squfrm->GetDic(); }
};

extern "C" {
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_createPdicFrame(JNIEnv* env, jobject thiz, jobject callback, int param1)
{
	if (!WordEdit){
		WordEdit = new TWordEdit();
	}
	if (!WordList){
		WordList = new TWordList();
	}
	WordList->SetEnv(env);
	if (!UIMain){
		jniCallback = env->NewGlobalRef(callback);
		TPdicMainImpl *main = new TPdicMainImpl();
		UIMain = TUIMain::CreateInstance(new TPdicMainImpl, WordEdit, WordList);
		Frame = new TSquareFrame(UIMain);
		main->SetFrame(Frame);
		Main = main;
#if 0	// for test
		DicGroup dg(_t("Sample"));
		Frame->squi->OpenDictionary(dg, NULL);
#endif
	} else {
		Frame->SetUIMain( UIMain );
	}
	FrameInstance++;
	return param1;
}
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_deletePdicFrame(JNIEnv* env, jobject thiz, jint param1)
{
	if (--FrameInstance != 0)
		return 0;
	if (UIMain){
		Frame->SetUIMain( NULL );
		delete UIMain;
		UIMain = NULL;
		env->DeleteGlobalRef(jniCallback);
	}
	if (Main){
		delete Main;
		Main = NULL;
	}
	if (WordEdit){
		delete WordEdit;
		WordEdit = NULL;
	}
	if (WordList){
		delete WordList;
		WordList = NULL;
	}
	return 0;
}
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_config(JNIEnv* env, jobject thiz, int view)
{
	if (WordList){
		WordList->SetView(view);
	}
	return 0;
}

// pathname:
// ファイル名の先頭が|である場合、途中の|までがオプション
//	
// dictionary options
// r0: readonly off(writable)
// -1以下: エラーを起こした辞書index情報 (=-index-1)
// -999: それ以外のエラー
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_openDictionary(JNIEnv* env, jobject thiz, jint num, jobjectArray pathNames, jint param1)
{
	if (!Frame) return -999;

	DicGroup dg(_t("Sample"));
	DicNames names;

	// setup parameters //
	for (int i=0;i<num;i++){
		jstring js = (jstring)env->GetObjectArrayElement(pathNames, i);
		bool err = false;

		DICNAME *dn = new DICNAME;
		dn->readonly = true;

		JString path(env, js);

#if 0	// debug
		__tnstrA sa;
		sa.setUTF8(path.c_str());
		DBW("%d: %s", i, sa.c_str());
#endif
		const tchar *filename = path.c_str();
		if (path[0]=='|'){
			const tchar *p = filename+1;
			tchar label = 0;
			int value = 0;
			while (1){
				tchar c = *p++;
				if (c=='|') break;
				if (!c){
					p--;
					break;
				}
				if (c>='a'&&c<='z'){
					// label
					label = c;
					value = 0;
				} else
				if (c>='0'&&c<='9'){
					value = value * 10 + value - '0';
					switch (label){
						case 'r': dn->readonly = value ? true : false; break;
					}
				} else {
					value = 0;
				}
			}
			filename = p;
		}

		if (!filename[0]){
			DBW("No filename??");
			err = true;
		}
		if (!err){
			dn->name = filename;
			names.add( dn );
		}
		
		if (err)
			return -i-1;
	}

	int dicno = -1;
	int ret = Frame->squi->OpenDictionary(dg, &names, &dicno);
	//DBW("openDictionary: ret=%d", ret);
	return ret == 0 ? 0 : (dicno >= 0 ? -dicno-1 : -999);
}
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_closeDictionary(JNIEnv* env, jobject thiz, int param1)
{
	if (WordList){
		WordList->SetEnv(env);
	}
	if (Frame){
		Frame->squi->Close();
	}
	return 0;
}
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_idleProc(JNIEnv* env, jobject thiz, jint param1)
{
	if (WordList){
		WordList->SetEnv(env);
	}
	int ret = 1;	// end of idle
	if (Frame){
		if (Frame->squi->IdleProc()){
			// done
			Pdic::IdleProcHandler();
		} else {
			// continue to idle proc
			ret = 0;
		}
	}
	return ret;
}

// Incremental Search //
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_incSearch(JNIEnv* env, jobject thiz, jstring word)
{
	if (!Frame){
		return -1;
	}
	if (WordList){
		WordList->SetEnv(env);
	}
	int length = env->GetStringLength(word);
	if (length==0){
		Frame->squi->Clear();
	} else {
		const jchar *jword = env->GetStringChars(word, NULL);
		std::wstring str;
		str.assign(jword, jword+length);
		Frame->squi->IncSearch(str.c_str());
		env->ReleaseStringChars(word, jword);
	}
	return 0;
}

JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_requestScroll(JNIEnv* env, jobject thiz, jint offset, jint backward, int indexOffset)
{
	if (!Frame) return -1;
	if (WordList) WordList->SetEnv(env);

	if (indexOffset>=0){
		Frame->squi->SetIndexOffset( indexOffset );
	}

	return Frame->squi->RequestScroll(offset, (bool)backward);
}

// Extract words for popup search //
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_getLPWords
	  (JNIEnv *env, jobject thiz, jstring str, jint pos, /*int &startPos, int &endPos, int &prevStartPos, */ jint fLongest, jint wordcount, jint about)
{
	if (!Frame){
		return -1;
	}
	
	// prepare parameters //
	int str_length = env->GetStringLength(str);
	if (str_length==0){ return -1; }
	const jchar *jstr = env->GetStringChars(str, NULL);
	std::wstring wstr;
	wstr.assign(jstr, jstr+str_length);

	int ret = 0;
	//Frame->squi->Clear();	// 2015.6.17 なぜあった？
	MPdic *dic = Frame->squi->GetDic();
	if (!dic){
		DBW("No activated dic");
	} else {
		int start, end, prevstart;
		//DBW("pos=%d longest=%d wordcount=%d about=%d", pos, fLongest, wordcount, about);
		if (dic->GetLangProc()->GetWord(wstr.c_str(), pos, start, end, prevstart, (bool)fLongest, wordcount, (bool)about)){
			//DBW("found");
			ret = 1;
			// set return values
			jclass cls = env->GetObjectClass(thiz);
			jfieldID fid = env->GetFieldID(cls, "startPos", "I");
			env->SetIntField(thiz, fid, start);
			fid = env->GetFieldID(cls, "endPos", "I");
			env->SetIntField(thiz, fid, end);
			fid = env->GetFieldID(cls, "prevStartPos", "I");
			env->SetIntField(thiz, fid, prevstart);
		}
	}
	env->ReleaseStringChars(str, jstr);
	return ret;
}

// Popup Search //
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_searchLongestWord
	(JNIEnv *env, jobject thiz, jstring words, jint start, jint curpos, jint flags )
{
	if (!Frame || !WordList){
		return -1;
	}
	
	// prepare parameters //
	int words_length = env->GetStringLength(words);
	if (words_length==0){ return -1; }
	const jchar *jwords = env->GetStringChars(words, NULL);
	std::wstring wwords;
	wwords.assign(jwords, jwords+words_length);

	int ret = 0;
	MPdic *dic = Frame->squi->GetDic();
	if (!dic){
		DBW("No activated dic");
	} else {
		if (flags==0){
			//flags = SLW_ELIMHYPHEN | SLW_REPLACE | SLW_CASEIGNORE | SLW_ENGLISH | SLW_UK | SLW_CONJUGATE | SLW_SYMBOLS;	//TODO: 暫定
			flags = SLW_ELIMHYPHEN | SLW_REPLACE | /* SLW_CASEIGNORE | */ SLW_ENGLISH | SLW_UK | SLW_SYMBOLS;
			//TODO: ? SLW_CONJUGATEは動作が変？
		}
#if 0
		__tnstrA sa;
		sa.setUTF8(&wwords[start]);
		DBW("search:%s", sa.c_str());
#endif
		MatchArray hitWords;
		if (dic->SearchLongestWord(&wwords[start], wwords.c_str(), curpos, flags, &hitWords)){
			WordList->SetEnv(env);
			for (int i=hitWords.get_num()-1;i>=0;i--){
				const tchar *word = hitWords[i].word;
				Japa japa;
				if (dic->Find(word, &japa)==1){
#if 0
					__tnstrA sa;
					sa.setUTF8(find_cword_pos(word));
					DBW("%d: %s", _tcslen(find_cword_pos(word)), sa.c_str());
#endif
					WordList->Add(find_cword_pos(word), japa.japa, japa.pron, japa.exp);
					ret++;
				}
			}
		}
	}

	env->ReleaseStringChars(words, jwords);
	return ret;
}

// Search History //
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_addSearchHistory
	(JNIEnv *env, jobject thiz, jstring word)
{
	return 0;
}

JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_enumSearchHistory
	(JNIEnv *env, jobject thiz, int flags)
{
	return 0;
}

// Bookmark //
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_addBookmark
	(JNIEnv *env, jobject thiz, jstring word)
{
	return 0;
}

// PSBookmark //
class TPSBookmarkCallback {
protected:
	JNIEnv *env;
	jmethodID midEnumPSBItem;	// for loadPSBookmark
	jmethodID midEnumPSBFile;	// for loadPSBookmarkFiles
public:
	TPSBookmarkCallback()
	{
		env = NULL;
		midEnumPSBItem = NULL;
		midEnumPSBFile = NULL;
	}
	~TPSBookmarkCallback()
	{
	}
	void SetEnv(JNIEnv *_env)
	{
		env = _env;
	}
	// int enumPSBookmarkItem(int position, int length, int style, int color, String marked_word, String comment)
	int enumPSBookmarkItem(int position, int length, int style, int color, const tchar *marked_word, const tchar *comment){
		if (!midEnumPSBItem){
			jclass jniCallbackClass = env->GetObjectClass(jniCallback);
			midEnumPSBItem = env->GetMethodID(jniCallbackClass, "enumPSBookmarkItem", "(IIIILjava/lang/String;Ljava/lang/String;)I");
		}
		jstring str1 = env->NewStringUTF(__cstr(marked_word).utf8());
		jstring str2 = env->NewStringUTF(__cstr(comment).utf8());
		int ret = env->CallIntMethod(jniCallback, midEnumPSBItem, position, length, style, color, str1, str2);
		env->DeleteLocalRef(str1);
		env->DeleteLocalRef(str2);
		return ret;
	}
	// int enumPSBookmarkFile(String filename)
	int enumPSBookmarkFile(const tchar *filename){
		if (!midEnumPSBFile){
			jclass jniCallbackClass = env->GetObjectClass(jniCallback);
			midEnumPSBFile = env->GetMethodID(jniCallbackClass, "enumPSBookmarkFile", "(Ljava/lang/String;)I");
		}
		jstring str = env->NewStringUTF(__cstr(filename).utf8());
		int ret = env->CallIntMethod(jniCallback, midEnumPSBFile, str);
		env->DeleteLocalRef(str);
		return ret;
	}
};
TPSBookmarkCallback *PSBookmarkCallback = NULL;
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_xopenPSBookmark
	(JNIEnv *env, jobject thiz, jint openMode, jstring filename)
{
	if (!PSBookmarkCallback) PSBookmarkCallback = new TPSBookmarkCallback();
	tnstr _wfilename;
	const wchar_t *wfilename;
	JString jfilename(env, filename);
	if (jfilename.empty()){
		__tnstrA defname; defname << __cstr(AppInternalPath).utf8() << "/PSBookmark.txt";
		_wfilename.setUTF8( defname );
		wfilename = _wfilename;
	} else {
		wfilename = jfilename;
	}
	if (openMode==1){
		__tnstrA filename8;
		filename8.setUTF8( wfilename );
		unlink( filename8 );
		LOGI("Deleted - %s", filename8.c_str());
	}
	DBW("OpenPSBookmark: %s", __cstr(wfilename).utf8());
	OpenPSBookmark(wfilename);
	return 0;
}
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_xclosePSBookmark
	(JNIEnv *env, jobject thiz)
{
	if (PSBookmarkCallback){ delete PSBookmarkCallback; PSBookmarkCallback = NULL; }
	ClosePSBookmark();
	return 0;
}
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_xaddPSBookmark
	(JNIEnv *env, jobject thiz, jstring filename, jint position, jint length, jint style, jint color, jstring marked_word, jstring comment)
{
	DBW("addPSBookmark: position=%d", position);
	JString wfilename(env, filename);
	JString wmarked_word(env, marked_word);
	JString wcomment(env, comment);

	TPSBMItem item;
	item.position = position;
	item.style = style;
	item.length = length;
	item.color = color;
	item.marked_word = wmarked_word;
	item.comment = wcomment;
	SavePSBookmark(wfilename.c_str(), item);
	return 0;
}
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_xdeletePSBookmark
	(JNIEnv *env, jobject thiz, jstring filename, jint position)
{
	DBW("deletePSBookmark");
	JString wfilename(env, filename);
	TPSBMItem item;
	item.position = position;
	return DeletePSBookmark(wfilename, item) ? 1 : 0;
}
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_loadPSBookmark
	(JNIEnv *env, jobject thiz, jstring filename)
{
	if (!PSBookmarkCallback){ return -1;}
	PSBookmarkCallback->SetEnv(env);

	JString wfilename(env, filename);
	vector<TPSBMItem> items;
	LoadPSBookmark(wfilename, items);
	for (vector<TPSBMItem>::iterator it = items.begin();it!=items.end();it++){
		if (PSBookmarkCallback->enumPSBookmarkItem(it->position, it->length, it->style, it->color, it->marked_word, it->comment)){
			break;
		}
	}
	return 0;
}
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_loadPSBookmarkItem
	(JNIEnv *env, jobject thiz, jstring filename, jint position)
{
	if (!PSBookmarkCallback){ return -1;}
	PSBookmarkCallback->SetEnv(env);

	JString wfilename(env, filename);
	TPSBMItem item;
	if (LoadPSBookmarkItem(wfilename, position, item)){
		PSBookmarkCallback->enumPSBookmarkItem(item.position, item.length, item.style, item.color, item.marked_word, item.comment);
		return 1;	// item exists
	}
	return 0;
}
JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_loadPSBookmarkFiles
	(JNIEnv *env, jobject thiz)
{
	if (!PSBookmarkCallback){ return -1;}
	PSBookmarkCallback->SetEnv(env);

	tnstr_vec files;
	LoadPSBookmarkFiles(files, true);
	foreach_tnstr_vec(files, file){
		if (PSBookmarkCallback->enumPSBookmarkFile(file->c_str())){
			break;
		}
	}
	return 0;
}
#if 0
const tchar *GetPSBookmarkFileName();
#endif

JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_savePSFileInfo
	(JNIEnv *env, jobject thiz, jstring filename, jint position, jstring revision)
{
	DBW("savePSFileInfo");
	JString wfilename(env, filename);
	JString wrevision(env, revision);

	if (SavePSFileInfo(wfilename, position, wrevision)){
		return 1;
	}
	return 0;
}

JNIEXPORT jint JNICALL Java_com_reliefoffice_pdic_PdicJni_xloadPSFileInfo
	(JNIEnv *env, jobject thiz, jstring filename)
{
	JString wfilename(env, filename);
	int position = 0;
	tnstr revision;
	if (!LoadPSFileInfo(wfilename, position, &revision)){
		return -1;	// no file info
	}
	JReturn retString( env, thiz, revision );
	return position;
}


} /* "C" */

// from 2018.11.10 (１年くらい経ったら削除？）
void DeleteOldInternalFiles()
{
	unlinkDocPath("Sample.dic");
	unlinkDocPath("DefLangProc.ctt");
	unlinkDocPath("SrchPat.ctt");
	unlinkDocPath("IrregDic.txt");
	unlinkDocPath("example.txt");
}

bool unlinkDocPath(const char *filename)
{
	__tnstrA path;
	path << DEF_DOCPATH << "/" << filename;
	return unlink(path) == 0;
}

bool CopyInternalFiles(AAssetManager *mgr)
{
	__tnstrA path = __cstr(AppInternalPath).utf8();

	if (!LoadAssetFile(mgr, "Sample.dic", path, "Sample.dic")){
		return false;
	}
	if (!LoadAssetFile(mgr, "DefLangProc.ctt", path, "DefLangProc.ctt")){
		return false;
	}
	if (!LoadAssetFile(mgr, "SrchPat.ctt", path, "SrchPat.ctt")){
		return false;
	}
	if (!LoadAssetFile(mgr, "IrregDic.txt", path, "IrregDic.txt")){
		return false;
	}
	if (!LoadAssetFile(mgr, "example.txt", path, "example.txt")){
		return false;
	}
	return true;
}

bool LoadAssetFile(AAssetManager *mgr, const char *src, const char *dstpath, const char *dstname)
{
	AAsset* asset = AAssetManager_open(mgr, src, AASSET_MODE_STREAMING);
	if (!asset){
		LOGE("asset open error: %s", src);
		return false;
	}
	__tnstrA dstfile;
	dstfile << dstpath << "/" << dstname;
	bool ok = false;
	size_t size = AAsset_getLength(asset);
	char *buf = (char*)malloc(size);
	if (buf){
		AAsset_read(asset, buf, size);
		AAsset_close(asset);
		FILE *fp = fopen(dstfile, "w");
		if (fp){
			int wb = fwrite(buf, size, 1, fp);
			if (wb==1){
				ok = true;
			}
			fclose(fp);
		} else {
			LOGE("open error: %s : %d", dstfile.c_str(), errno);
		}
		free(buf);
	}
	return ok;
}


#include "LangProcStd.h"
#include "LangProcSimple.h"
#include "LangProcMan.h"
//static TLangProc *CreateLangProc0()
//{ return new TLangProcStd0(); }
static TLangProc *CreateLangProc1()
{ return new TLangProcStd(); }
#if 0
static TLangProc *CreateLangProc2()
{ return new TLangProcSimple(); }
static TLangProc *CreateLangProc3()
{ return new TLangProcGen(); }
static TLangProc *CreateLangProc4()
{ return new TLangProcSimpleOld(); }
#endif

bool InitLangProc()
{
	//TCConvTableMan::SetDefaultPath(prof.GetCCTablePath());

	// standard language processor
	if (!LangProcMan.Register(TLangProcInfo(TLangProcStd::LangProcId, _t("標準"), CreateLangProc1)))
//	if (!LangProcMan.Register(TLangProcInfo(TLangProcStd::LangProcId, _LT(TID_KEYWORDTRANS_NAME1), CreateLangProc1)))
	{
		LOGE("registration error(STD)");
//		return false;	//TODO: 二重起動以外にエラーは起きないはず？なのでとりあえずエラー扱いにしない
	}
#if 0
	if (prof.IsDebug()){
		// simple language processor
		if (!LangProcMan.Register(TLangProcInfo(TLangProcSimple::LangProcId, _LT(TID_KEYWORDTRANS_NAME2), CreateLangProc2))){
			__assert__;
			//return false;
		}
		// simple language processor
		if (!LangProcMan.Register(TLangProcInfo(TLangProcSimpleOld::LangProcId, _t("Simple(old)"), CreateLangProc4))){
			__assert__;
			//return false;
		}
		// Generic language processor
		if (!LangProcMan.Register(TLangProcInfo(TLangProcGen::LangProcId, _LT(TID_KEYWORDTRANS_NAME3), CreateLangProc3))){
			__assert__;
		}
	}
#endif
#if 0
	// deprecated standard lang proc.
	if (!LangProcMan.Register(TLangProcInfo(TLangProcStd0::LangProcId, _LT(TID_KEYWORDTRANS_NAME0), CreateLangProc0))){
		__assert__;
//		return false;
	}
#endif
	return true;
}
void CleanupLangProc()
{
	const int wait_timeout = 5000;	// 3[sec]
	DWORD start = GetTickCount();
	if (LangProcMan.IsUsing()>0){
		DBW("Waiting for LangProcMan free...");
		do {
			if (GetTickCount()-start>wait_timeout)
				break;
			Sleep(1);
		} while (LangProcMan.IsUsing());
	}
	LangProcMan.Close();
}

void LoadAllProfile( )
{
    ReadGroupDicList();
}
