package com.reliefoffice.pdic;

import android.os.AsyncTask;
import android.util.Log;

import com.dropbox.client2.DropboxAPI;
import com.dropbox.client2.exception.DropboxException;
import com.dropbox.core.DbxException;
import com.dropbox.core.v2.files.FileMetadata;
import com.dropbox.core.v2.files.ListFolderGetLatestCursorResult;
import com.dropbox.core.v2.files.ListFolderResult;
import com.dropbox.core.v2.files.Metadata;

/**
 * Created by tnishi on 2016/09/08.
 */
public class Dropbox2PollingFolders extends AsyncTask<Void, Void, Void> {
    final int PollingInterval = 5000;

    private Dropbox2FileManager dbxFM;
    private Dropbox2Utils dbxUtils;
    private String dbCursor;
    private String path = "/";

    public Dropbox2PollingFolders(Dropbox2FileManager dbxFileMan){
        this.dbxFM = dbxFileMan;
        dbxUtils = Dropbox2Utils.getInstance(null);
        dbCursor = dbxUtils.getCursor();
    }

    @Override
    protected Void doInBackground(Void... voids) {
        while (!isCancelled()){
            if (Utility.isNotEmpty(dbCursor)) break;
            dbxFM.lockApi();
            try {
                //dbCursor = dbxFM.getClient().files().listFolder(path).getCursor();    // これではないよね？
                String listFolderPath = path == "/" ? "" : path;
                ListFolderGetLatestCursorResult cursorResult = dbxFM.getClient().files().listFolderGetLatestCursor(listFolderPath);
                dbCursor = cursorResult.getCursor();
            } catch (DbxException e){
                e.printStackTrace();
            }
            dbxFM.unlockApi();
            if (Utility.isNotEmpty(dbCursor)) break;
            Log.d("PDD", "dbCursor is null, retry to retrieve");
            try {
                Thread.sleep(PollingInterval);
            } catch (InterruptedException e){
                e.printStackTrace(); break;
            }
        }
        Log.d("PDD", "dbCursor is "+dbCursor);
        while (!isCancelled()) {
            checkUpload();
            if (checkRevision()){
                checkDownload();
            }
            if (dbxFM.needRemoteCheck()) {
                dbxFM.lockApi();
                try {
                    //Log.d("PDD", "get delta cursor="+dbCursor);
                    //Log.d("PDD", "/delta path="+path);
                    //DropboxAPI.DeltaPage<DropboxAPI.Entry> dbPage = dbxFM.getApi().delta(dbCursor, path);
                    ListFolderResult dbPage = dbxFM.getClient().files().listFolderContinue(dbCursor);
                    //Log.d("PDD", "delta returned");
                    if (dbPage != null) {
                        dbCursor = dbPage.getCursor();
                        if (false) {    //TODO: to be deleted
                            // cancel all operation for this path.
                            //Log.d("PDD", "reset operations");
                            //break;
                        } else {
                            for ( Metadata _entry : dbPage.getEntries()) {
                                FileMetadata entry = (FileMetadata)_entry;
                                Log.d("PDD", "delta: path=" + entry.getPathLower());
                                dbxFM.requestDownload(entry.getPathLower(), entry.getRev());
                            }
                        }
                    }
                } catch (DbxException e) {
                    e.printStackTrace();
                } catch (Exception e){
                    e.printStackTrace();
                }
                dbxFM.unlockApi();

                checkDownload();
            }

            try {
                Thread.sleep(PollingInterval);
            } catch (InterruptedException e){
                e.printStackTrace(); break;
            }
        }
        return null;
    }

    @Override
    protected void onPostExecute(Void res){
        dbxUtils.setCursor(dbCursor);
    }

    boolean checkRevision(){
        DbxFileInfo info = dbxFM.findRevisionCheck();
        if (info==null) return false;

        boolean needDownload = false;

        dbxFM.lockApi();
        try {
            //DropboxAPI.Entry entry = dbxFM.getApi().metadata(info.remoteName, 0, null, false, null);
            FileMetadata metadata = (FileMetadata)dbxFM.getClient().files().getMetadata(info.remoteName);
            if (metadata != null) {
                if (info.remoteRevision == null || !info.remoteRevision.equals(metadata.getRev())){
                    // revision is updated, we need to download
                    Log.d("PDD", "revision differ!: remoteName="+info.remoteName+" rev="+info.remoteRevision+" srev="+metadata.getRev());
                    info.downloadRequest = true;
                    needDownload = true;
                }
            }
        } catch (DbxException e) {
            e.printStackTrace();
        }
        dbxFM.unlockApi();

        return needDownload;
    }

    void checkDownload(){
        DbxFileInfo file = null;
        do {
            file = dbxFM.getDownloadRequestedFile(file);
            if (file == null) break;
            downloadFile(file);
        } while (true);
    }
    void downloadFile(DbxFileInfo file){
        DownloadTask downloadTask = new DownloadTask(dbxFM, file);
        file.downloading = true;
        downloadTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }
    class DownloadTask extends Dropbox2DownloadTask {
        DbxFileInfo dbxFile;
        public DownloadTask(Dropbox2FileManager dbxFM, DbxFileInfo file){
            super(dbxFM, file.remoteName, file.getLocalFile());
            dbxFile = file;
            Log.d("PDD", "Start download: file="+dbxFile.remoteName);
        }
        @Override
        protected void onPostExecute(Object result) {
            Log.d("PDD", "Download onPost file:"+dbxFile.getLocalName());
            if (downloaded){
                dbxFile.downloadRequest = false;
                dbxFile.needRevisionCheck = false;
                dbxFile.remoteRevision = super.revision;
                Log.d("PDD", "Polling Task downloaded - " + to + " rev: " + super.revision);
                INetDriveFileInfo info2 = dbxFM.findByLocalName( dbxFile.getLocalName() );
                if (info2!=null)
                    Log.d("PDD", "info2.rev="+info2.remoteRevision);
                else
                    Log.w("PDD", "info2.rev is null");
                dbxFile.updateLocalFileTime();
                dbxFile.downloadNotify();
            }
            dbxFile.downloading = false;
        }
    }

    void checkUpload(){
        dbxFM.checkUpdate();
        DbxFileInfo file = null;
        do {
            //Log.d("PDD", "Checking upload");
            file = dbxFM.getUploadRequestedFile(file);
            if (file == null) break;
            uploadFile(file);
        } while (true);
    }
    void uploadFile(DbxFileInfo file){
        Log.d("PDD", "uploadFile: "+file.getLocalName());
        UploadTask uploadTask = new UploadTask(dbxFM, file);
        file.uploading = true;
        uploadTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }
    class UploadTask extends Dropbox2UploadTask {
        DbxFileInfo dbxFile;
        public UploadTask(Dropbox2FileManager dbxFM, DbxFileInfo file){
            super(dbxFM, file.getLocalFile(), file.remoteName);
            dbxFile = file;
        }
        @Override
        protected void onPostExecute(Object result) {
            if (uploaded){
                dbxFile.uploadRequest = false;
                dbxFile.remoteRevision = super.revision;
                dbxFile.updateLocalFileTime();
                dbxFile.uploadNotify();
                Log.d("PDD", "Polling Task uploaded - "+from);
            }
            dbxFile.uploading = false;
        }
    }
}
