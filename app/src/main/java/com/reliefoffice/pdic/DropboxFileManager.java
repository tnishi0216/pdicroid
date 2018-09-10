package com.reliefoffice.pdic;

import android.content.Context;
import android.os.AsyncTask;
import android.util.Log;
import android.widget.Toast;

import com.dropbox.client2.android.AndroidAuthSession;

import java.io.File;
import java.util.concurrent.Semaphore;

/**
 * Created by nishikawat on 2015/08/20.
 */
public class DropboxFileManager extends DropboxBaseFileManager {
    private static DropboxFileManager This;
    //private SharedPreferences pref;
    private DropboxAPIEx<AndroidAuthSession> mDBApi;
    private DbxPollingFiles pollingFiles;
    private DropboxPollingFolders dbxPollingFolders;
    private DropboxUtils dbxUtils;
    private Semaphore mutexApi = new Semaphore(1);
    boolean authCompleted;

    private DropboxFileManager(Context context){
        This = this;
        this.context = context;
        //SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);

        pollingFiles = new DbxPollingFiles();
        dbxUtils = DropboxUtils.getInstance(context);

        startAuth(true);
    }

    public static INetDriveFileManager createInstance(Context context){
        if (DropboxUtils.useV2){
            return Dropbox2FileManager.createInstance(context);
        }
        if (This==null)
            new DropboxFileManager(context);
        else
            This.context = context;
        return This;
    }

    //Note: when call getApi(), should call lockApi/unlockApi.
    public DropboxAPIEx<AndroidAuthSession> getApi(){ return mDBApi; }
    public void lockApi(){ try { mutexApi.acquire(); } catch(InterruptedException e){ e.printStackTrace(); } }
    public void unlockApi(){ mutexApi.release(); }

    //Note: should call authComplete() in onResume()
    @Override
    public boolean startAuth(boolean easyMode){
        if (!dbxUtils.hasLoadAndroidAuthSession()) {
            if (!easyMode) {
                mDBApi = new DropboxAPIEx<>(dbxUtils.createAndroidAuthSession());

                try {
                    mDBApi.getSession().startOAuth2Authentication(context);
                } catch (IllegalStateException e) {
                    Log.e("PDD", "IllegalStateException");
                } catch (Exception e) {
                    Log.e("PDD", "startOAuth2Authentication error");
                }
            }
            return false;
        } else {
            AndroidAuthSession session = dbxUtils.loadAndroidAuthSession();
            mDBApi = new DropboxAPIEx<>(session);
            return true;
        }
    }

    @Override
    public boolean authComplete(Context context){
        if (authCompleted || mDBApi==null) return false;
        if (mDBApi.getSession().authenticationSuccessful()) {
            try {
                // Required to complete auth, sets the access token on the session
                mDBApi.getSession().finishAuthentication();
                dbxUtils.storeOauth2AccessToken(mDBApi.getSession().getOAuth2AccessToken());
                if (context != null)
                    Toast.makeText(context, context.getString(R.string.msg_authenticated), Toast.LENGTH_SHORT).show();
                authCompleted = true;
                return true;
            } catch (IllegalStateException e) {
                Log.i("PDP", "Error authenticating", e);
            }
        }
        return false;
    }

    @Override
    public void disconnect(){}  // 該当methodなし？

    // add a dictionary file for download monitor
    public void add(DicInfo dicInfo, INetDriveFileInfo.UpdateNotifier notifier){
        Log.d("PDD", "add local: "+dicInfo.filename);
        pollingFiles.add(dicInfo.filename, dicInfo.ndvPath, dicInfo.ndvRevision, dicInfo, notifier);
        if (dbxPollingFolders==null && mDBApi!=null){
            dbxPollingFolders = new DropboxPollingFolders(this);
            dbxPollingFolders.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }
    }

    // add a file for download monitor
    public void add(String localName, String remoteName, String remoteRevision, INetDriveFileInfo.UpdateNotifier notifier){
        pollingFiles.add(localName, remoteName, remoteRevision, null, notifier);
        if (dbxPollingFolders==null && mDBApi!=null){
            dbxPollingFolders = new DropboxPollingFolders(this);
            dbxPollingFolders.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }
    }
    // remove file from download monitor
    public void remove(String localName){
        DbxFileInfo info = pollingFiles.findLocal(localName);
        if (info!=null) {
            if (pollingFiles.size()==1) {
                dbxPollingFolders.cancel(true);
                dbxPollingFolders = null;
            }
            pollingFiles.remove(info);
        }
    }
    public void removeAll(){
        if (dbxPollingFolders!=null) {
            dbxPollingFolders.cancel(true);
            dbxPollingFolders = null;
        }
        pollingFiles.removeAll();
    }
    // remove all files that have DicInfo
    @Override
    public void removeDicInfo(){
        pollingFiles.removeDicInfo();
        if (pollingFiles.empty()) {
            if (dbxPollingFolders!=null) {
                dbxPollingFolders.cancel(true);
                dbxPollingFolders = null;
            }
        }
    }

    @Override
    public INetDriveFileInfo findByLocalName(String filename){
        return pollingFiles.findLocal(filename);
    }

    // Check the file updated by timestamp for upload.
    public void checkUpdate(String filename){
        DbxFileInfo info = pollingFiles.findLocal(filename);
        if (info==null) return;
        if (!info.uploadRequest) {
            if (!info.isUpdated()) return;
            info.uploadRequest = true;
        }
    }

    public void checkUpdate(){
        DbxFileInfo info = pollingFiles.findUpdatedFile();
        if (info!=null){
            Log.d("PDD", "Found updated file: "+info.getLocalName()+" old="+info.lastModified+" new="+info.getLastModified());
            info.uploadRequest = true;
        }
    }

    // polling operations //
    //NOTE: must be thread safe!!
    public DbxFileInfo findRevisionCheck(){
        return pollingFiles.findNeedRevisionCheck();
    }
    public boolean needRemoteCheck(){
        return pollingFiles.needRemoteCheck();
    }
    public void requestDownload(String lcRemoteName, String rev){
        DbxFileInfo info = pollingFiles.findRemote(lcRemoteName);
        if (info != null) {
            if (Utility.isEmpty(rev) || !info.remoteRevision.equals(rev)) {
                info.downloadRequest = true;
            }
        }
    }
    public DbxFileInfo getDownloadRequestedFile(DbxFileInfo cur){
        return pollingFiles.getDownloadRequestedFile(cur);
    }
    public DbxFileInfo getUploadRequestedFile(DbxFileInfo cur){
        return pollingFiles.getUploadRequestedFile(cur);
    }

    // End of polling operations //

    // Download Task //
    @Override
    public void executeDownload(String from, File to, final OnExecuteListener listener){
        DownloadTask task = new DownloadTask(this, from, to, listener);
        task.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }
    class DownloadTask extends DropboxDownloadTask {
        OnExecuteListener listener;
        public DownloadTask(DropboxFileManager dbxFM, String from, File to, OnExecuteListener listener) {
            super(dbxFM, from, to);
            this.listener = listener;
        }

        @Override
        protected void onPostExecute(Object result) {
            listener.onPostExecute(downloaded, from, to, revision);
        }
    }
}
