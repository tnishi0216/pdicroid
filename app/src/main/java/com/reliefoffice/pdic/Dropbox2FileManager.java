package com.reliefoffice.pdic;

import android.content.Context;
import android.os.AsyncTask;
import android.util.Log;
import android.widget.Toast;

import com.dropbox.core.DbxRequestConfig;
import com.dropbox.core.android.Auth;
import com.dropbox.core.v2.DbxClientV2;

import java.io.File;
import java.util.concurrent.Semaphore;

/**
 * Created by tnishi on 2016/09/08.
 */
public class Dropbox2FileManager extends DropboxBaseFileManager {
    private static Dropbox2FileManager This;
    private DbxClientV2 mClient;
    private DbxPollingFiles pollingFiles;
    private Dropbox2PollingFolders dbxPollingFolders;
    private Dropbox2Utils dbxUtils;
    private Semaphore mutexApi = new Semaphore(1);
    boolean authCompleted;

    private Dropbox2FileManager(Context context){
        This = this;
        this.context = context;
        //SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);

        pollingFiles = new DbxPollingFiles();
        dbxUtils = Dropbox2Utils.getInstance(context);

        startAuth(true);
    }

    public static Dropbox2FileManager createInstance(Context context){
        if (This==null)
            new Dropbox2FileManager(context);
        return This;
    }
    public static Dropbox2FileManager getInstance(Context context){
        if (context!=null)
            This.context = context;
        return This;
    }

    //Note: when call getApi(), should call lockApi/unlockApi.
    public DbxClientV2 getClient(){ return mClient; }
    public void lockApi(){ try { mutexApi.acquire(); } catch(InterruptedException e){ e.printStackTrace(); } }
    public void unlockApi(){ mutexApi.release(); }

    //Note: should call authComplete() in onResume()
    @Override
    public boolean startAuth(boolean easyMode){
        //dbxUtils.clearToken();    //DBG: for debug
        if (!dbxUtils.hasLoadAndroidAuthSession()) {
            if (!easyMode) {
                INetDriveUtils.AppKeys appKeys = dbxUtils.getAppKeys();
                if (Utility.isNotEmpty(appKeys.key)) {
                    try {
                        Auth.startOAuth2Authentication(context, appKeys.key);
                    } catch(Exception e){
                        Log.d("PDD", "Auth.startOAuth2Authentication exception: "+e.getMessage());
                        return false;
                    }
                }
            }
            return false;
        } else {
            DbxRequestConfig config = DbxRequestConfig.newBuilder("pdic/1.0")
                    //.withHttpRequestor(OkHttp3Requestor.INSTANCE)
                    .build();
            String access_token = dbxUtils.getAccessToken();
            try {
                mClient = new DbxClientV2(config, access_token);
            } catch (VerifyError error){
                return false;
            }
            return true;
        }
    }

    //Note:
    // 認証画面に切り替わる前にonResume()→authComplete()が呼ばれる。
    // そのときはAuth.getOAuth2Token()はnullが返ってくる
    @Override
    public boolean authComplete(Context context){
        if (authCompleted) return false;

        // Required to complete auth, sets the access token on the session
        String token = Auth.getOAuth2Token();
        if (Utility.isEmpty(token)) {
            // Log.i("PDP", "Access Token is null!?");
            return false;
        }
        dbxUtils.storeOauth2AccessToken(token);
        if (context != null)
            Toast.makeText(context, context.getString(R.string.msg_authenticated), Toast.LENGTH_SHORT).show();
        authCompleted = true;
        return true;
    }

    public boolean isAuthCompleted(){
        return authCompleted;
    }

    @Override
    public void disconnect(){}  // 該当methodなし？

    // add a dictionary file for download monitor
    public void add(DicInfo dicInfo, INetDriveFileInfo.UpdateNotifier notifier){
        Log.d("PDD", "add local: "+dicInfo.filename);
        pollingFiles.add(dicInfo.filename, dicInfo.ndvPath, dicInfo.ndvRevision, dicInfo, notifier);
        if (dbxPollingFolders==null && mClient!=null){
            dbxPollingFolders = new Dropbox2PollingFolders(this);
            dbxPollingFolders.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }
    }

    // add a file for download monitor
    public void add(String localName, String remoteName, String remoteRevision, INetDriveFileInfo.UpdateNotifier notifier){
        pollingFiles.add(localName, remoteName, remoteRevision, null, notifier);
        if (dbxPollingFolders==null && mClient!=null){
            dbxPollingFolders = new Dropbox2PollingFolders(this);
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

    // Donload Task //
    @Override
    public void executeDownload(String from, File to, final OnExecuteListener listener){
        DownloadTask task = new DownloadTask(this, from, to, listener);
        task.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }
    class DownloadTask extends Dropbox2DownloadTask {
        OnExecuteListener listener;
        public DownloadTask(Dropbox2FileManager dbxFM, String from, File to, OnExecuteListener listener) {
            super(dbxFM, from, to);
            this.listener = listener;
        }

        @Override
        protected void onPostExecute(Object result) {
            listener.onPostExecute(downloaded, from, to, revision);
        }
    }
}
