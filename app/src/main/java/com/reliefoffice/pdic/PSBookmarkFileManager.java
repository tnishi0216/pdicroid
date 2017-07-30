package com.reliefoffice.pdic;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * Created by nishikawat on 2015/08/20.
 */
public class PSBookmarkFileManager implements INetDriveFileInfo.UpdateNotifier {
    static final String PFS_PSBOOKMARK_FILENAME = "PSBookmarkFilename";
    static final String PFS_PSBOOKMARK_REMOTE_FILENAME = "PSBookmarkRemoteFilename";
    static final String PFS_PSBOOKMARK_REMOTE_REVISION = "PSBookmarkRemoteRevision";

    static PSBookmarkFileManager This = null;
    static int refCounter = 0;

    int openCount = 0;

    Context context;
    INetDriveFileManager ndvFM;
    private SharedPreferences pref;

    PdicJni pdicjni;

    private PSBookmarkFileManager(Context context, INetDriveFileManager ndvFM){
        this.context =context;
        this.ndvFM = ndvFM;
        pref = PreferenceManager.getDefaultSharedPreferences(context);

        pdicjni = PdicJni.createInstance(null, null);   // ‚Ù‚©‚Åinstance‰»‚³‚ê‚Ä‚¢‚é‘O’ñ

        String localName = getFilename();
        String remote = getRemoteFilename();
        String remoteRevision = getRemoteRevision();
        if (Utility.isNotEmpty(localName) && Utility.isNotEmpty(remote)){
            ndvFM.add(localName, remote, remoteRevision, this);
        }
    }

    public static PSBookmarkFileManager createInstance(Context context, INetDriveFileManager ndFM){
        if (This == null){
            This = new PSBookmarkFileManager(context, ndFM);
        }
        refCounter++;
        return This;
    }

    public void deleteInstance(){
        if (refCounter > 0){
            refCounter--;
            if (refCounter == 0) {
                if (This.pdicjni!=null)
                    This.pdicjni.deleteInstance();
                This = null;
            }
        }
    }

    public boolean open(){
        if (openCount++>0) return true;
        String filename = getFilename();
        if (pdicjni.openPSBookmark(filename, false)){
            openCount++;
            return true;
        }
        Log.w("PDD", "Open failed PSBookmarkFileManager: " + filename);
        return false;
    }

    public void clearAll(){
        if (openCount>0){
            pdicjni.closePSBookmark();
        }
        String filename = getFilename();
        pdicjni.openPSBookmark(filename, true);
        if (openCount>0){
            pdicjni.openPSBookmark(filename, false);
        }
    }

    public void close(){
        if (openCount>0){
            openCount--;
            if (openCount==0) {
                pdicjni.closePSBookmark();
            }
        }
    }

    public String getFilename(){
        return pref.getString(PFS_PSBOOKMARK_FILENAME, null);
    }
    public String getRemoteFilename(){
        return pref.getString(PFS_PSBOOKMARK_REMOTE_FILENAME, null);
    }
    public String getRemoteRevision(){
        return pref.getString(PFS_PSBOOKMARK_REMOTE_REVISION, null);
    }
    public void changeFilename(String filename, String remote, String revision){
        // remove previous filee from ndv polling file list //
        String localName = getFilename();
        if (Utility.isNotEmpty(localName)) ndvFM.remove(localName);

        // change filenames //
        putFilename(filename, remote);
        if (Utility.isNotEmpty(remote)){
            ndvFM.add(filename, remote, revision, this);  // add file for polling
        }

        // apply to jni
        if (openCount>0){
            pdicjni.closePSBookmark();
        }
        if (openCount>0){
            pdicjni.openPSBookmark(getFilename(), false);
        }
    }
    private void putFilename(String filename, String remote){
        SharedPreferences.Editor edit = pref.edit();
        if (Utility.isNotEmpty(filename)) {
            edit.putString(PFS_PSBOOKMARK_FILENAME, filename);
            edit.putString(PFS_PSBOOKMARK_REMOTE_FILENAME, remote);
        } else {
            edit.putString(PFS_PSBOOKMARK_FILENAME, null);
        }
        edit.commit();
    }

    public boolean isRemoteEnabled(){
        return Utility.isNotEmpty(getFilename()) && Utility.isNotEmpty(getRemoteFilename());
    }

    @Override
    public void uploaded(INetDriveFileInfo ndInfo){
        SharedPreferences.Editor edit = pref.edit();
        edit.putString(PFS_PSBOOKMARK_REMOTE_REVISION, ndInfo.remoteRevision);
        edit.commit();
    }
    @Override
    public void downloaded(INetDriveFileInfo ndvInfo){
        SharedPreferences.Editor edit = pref.edit();
        edit.putString(PFS_PSBOOKMARK_REMOTE_REVISION, ndvInfo.remoteRevision);
        edit.commit();
    }

    // remotePrefix: i.e. "dbx:"
    public static String buildFileName(String localName, String remoteName, final String remotePrefix){
        if (Utility.isNotEmpty(remoteName)){
            return remotePrefix+remoteName;
        }
        return localName;
    }
}
