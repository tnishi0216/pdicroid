package com.reliefoffice.pdic;

import android.content.res.AssetManager;

/**
 * Created by tnishi on 2015/06/10.
 */
public class PdicJni {

    public static final int VF_PRON = 0x08;
    public static final int VF_EXP  = 0x20;

    static PdicJni This;
    static int refCounter = 0;

    // generic return value //
    public String retString;

    private PdicJni(){

    }

    static public PdicJni createInstance(AssetManager assetManager, String tempPath)
    {
        if (refCounter==0) {
            if (assetManager == null) {
                System.out.println("AssetManager is null");
                return null;
            }

            System.loadLibrary("pdjni");

            This = new PdicJni();
            //TODO: initPdic()‚ðassertManager!=null‚Å‚ ‚ê‚Îí‚ÉŒÄ‚Ño‚·‚©H
            int ret = This.initPdic(0, assetManager, tempPath);
            if (ret!=0){
                This = null;
                return null;
            }
        }
        refCounter++;
        return This;
    }
    static public void deleteInstance()
    {
        if (refCounter>0){
            refCounter--;
            if (refCounter==0){
                This = null;
            }
        }
    }

    public int createFrame(JniCallback callback, int param1)
    {
        return createPdicFrame(callback, param1);
    }
    public int deleteFrame(){
        return deletePdicFrame(0);
    }
    private native int initPdic(int param1, AssetManager assetManager, String tempPath);
    //public native int inittest(int param1);
    private native int createPdicFrame(JniCallback callback, int param1);
    private native int deletePdicFrame(int param1);

    public native int config(int viewFlags);

    public native int openDictionary(int num, String[] pathNames, int param1);
    public native int closeDictionary(int param1);

    public native int incSearch(String word);
    public native int idleProc(int param1);
    public native int requestScroll(int offset, int backward, int indexOffset);

    public native int getLPWords(String str, int pos, int logest, int maxwordcount, int about);
    // return parameters
    public int startPos;
    public int endPos;
    public int prevStartPos;

    public native int searchLongestWord(String word, int startPos, int curpos, int flags);

    // PSBookmark //
    public final static int BOLD = 0x1;
    public final static int ITALIC = 0x2;
    public final static int UNDERLINE = 0x4;
    public final static int STRIKEOUT = 0x8;
    public boolean openPSBookmark(String filename, boolean clearAll) {
        return xopenPSBookmark(clearAll ? 1 : 0, filename)==0;
    }
    public void closePSBookmark(){
        xclosePSBookmark();
    }

    native int xopenPSBookmark(int openMode, String filename);
    native int xclosePSBookmark();
    public PSBookmarkItem getPSBookmarkItem(String filename, int position){
        PSBookmarkItem item = new PSBookmarkItem();
        JniCallback callback = JniCallback.createInstance();
        callback.setPSBookmarkItem(item);
        int ret = loadPSBookmarkItem(filename, position);
        callback.setPSBookmarkItem(null);
        callback.deleteInstance();
        return ret==1?item:null;
    }
    public boolean addPSBookmark(String filename, PSBookmarkItem item){
        return addPSBookmark(filename, item.position, item.length, item.style, item.color, item.markedWord, item.comment);
    }
    public PSFileInfo loadPSFileInfo(String filename){
        int position = xloadPSFileInfo(filename);
        if (position<0) return null;
        return new PSFileInfo(position, retString);
    }
    public class PSFileInfo {
        public int position;
        public String revision;
        public PSFileInfo(int position, String revision){
            this.position = position;
            this.revision = revision;
        }
    }
    public static int psbmGeneration;
    public boolean addPSBookmark(String filename, int position, int length, int style, int color, String marked_word, String comment){
        if (xaddPSBookmark(filename, position, length, style, color, marked_word, comment)==0){
            psbmGeneration++;
            return true;
        }
        return false;
    }
    public boolean deletePSBookmark(String filename, int position){
        if (xdeletePSBookmark(filename, position)!=0){
            psbmGeneration++;
            return true;
        }
        return false;
    }

    private native int xaddPSBookmark(String filename, int position, int length, int style, int color, String marked_word, String comment);
    private native int xdeletePSBookmark(String filename, int position);
    public native int loadPSBookmark(String filename);
    private native int loadPSBookmarkItem(String filename, int position);
    public native int loadPSBookmarkFiles();
    public native int savePSFileInfo(String filename, int postiion, String remotename, String revision);
    private native int xloadPSFileInfo(String filename);
}
