package com.reliefoffice.pdic;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * Created by nishikawat on 2015/12/16.
 */
public class DictionaryManager {
    static DictionaryManager This = null;
    static int refCounter = 0;
    Context context;
    int openCount = 0;
    PdicJni pdicJni;
    INetDriveFileManager ndvFM;

    private DictionaryManager(){

    }
    public static final DictionaryManager createInstance(Context context){
        if (This == null) {
            This = new DictionaryManager();
            This.pdicJni = PdicJni.createInstance(null, null);  // ‚·‚Å‚É‚Ù‚©‚Ì‚Æ‚±‚ë‚Åinstance‰»‚³‚ê‚Ä‚¢‚é‘O’ñ
        }
        This.context = context;
        This.ndvFM = DropboxFileManager.createInstance(context);
        refCounter++;
        return This;
    }
    private static final DictionaryManager getInstance(Context context){
        if (This!=null) {
            This.context = context;
            This.ndvFM = DropboxFileManager.createInstance(context);
        }
        return This;
    }
    public static final void deleteInstance(){
        if (refCounter>0){
            refCounter--;
            if (refCounter==0) {
                if (This.pdicJni!=null)
                    This.pdicJni.deleteInstance();
                This = null;
            }
        }
    }

    public boolean isDicOpened(){ return openCount>0; }

    public int getDictionaryNum(){
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
        DicPref dicPref = new DicPref(pref);
        String[] dicNames = dicPref.getNamesWithParams();
        return dicNames.length;
    }

    public void openDictionary(){
        if (openCount>0){
            openCount++;
            return;
        }
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
        DicPref dicPref = new DicPref(pref);
        String[] dicNames = dicPref.getNamesWithParams();
        if (pdicJni.openDictionary(dicNames.length, dicNames, 0)==0){
            openCount = 1;
            // add dropbox files
            for (int i = 0; i < dicNames.length; i++) {
                DicInfo dicInfo = dicPref.loadDicInfo(i);
                if (dicInfo == null) break;
                if (Utility.isNotEmpty(dicInfo.ndvPath)) {
                    ndvFM.add(dicInfo, new DicFileManager(dicPref));
                }
            }
        }
    }

    void closeDictionary(){
        if (openCount==0) return;

        openCount--;
        if (openCount==0) {
            pdicJni.closeDictionary(0);

            // Save Revision to dictionary pref //
            //TODO: I think it is too long to save just a revision...
            SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
            DicPref dicPref = new DicPref(pref);
            String[] dicNames = dicPref.getNamesWithParams();
            for (int i=0;i<dicNames.length;i++){
                DicInfo dicInfo = dicPref.loadDicInfo(i);
                if (dicInfo==null) break;
                INetDriveFileInfo info = ndvFM.findByLocalName(dicInfo.filename);
                if (info!=null){
                    Log.d("PDD", "rev info=" + info.remoteRevision + " dicInfo=" + dicInfo.ndvRevision);
                    if (Utility.isNotEmpty(dicInfo.ndvPath)
                            && info.remoteRevision!=dicInfo.ndvRevision){
                        // save revision
                        dicInfo.ndvRevision = info.remoteRevision;
                        dicPref.saveDicInfo(i, dicInfo);
                    }
                }
            }
            // //

            ndvFM.removeDicInfo();
        }
    }

}
