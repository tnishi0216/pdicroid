package com.reliefoffice.pdic;

import android.content.DialogInterface;
import android.os.AsyncTask;
import android.util.Log;
import android.widget.Toast;

import com.dropbox.core.DbxException;
import com.dropbox.core.v2.files.DeletedMetadata;
import com.dropbox.core.v2.files.FileMetadata;
import com.dropbox.core.v2.files.FolderMetadata;
import com.dropbox.core.v2.files.ListFolderContinueErrorException;
import com.dropbox.core.v2.files.ListFolderErrorException;
import com.dropbox.core.v2.files.ListFolderResult;
import com.dropbox.core.v2.files.Metadata;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public class Dropbox2FileSelectionActivity extends NetDriveFileSelectionActivity {
    public Dropbox2FileSelectionActivity(){
        super();
        ((DropboxBaseFileManager)ndvFM).setContext(this);
    }

    @Override
    void createNetDriveComponents() {
        ndvUtils = DropboxUtils.getInstance(this);
        ndvFM = DropboxFileManager.createInstance(this);
    }

    // 認証関連 //
    @Override
    protected void showAppKeyDialog() {
        MyDropboxAppKeysDialog dlg = new MyDropboxAppKeysDialog();
        dlg.setOwner(this);
        dlg.show(getFragmentManager(), "dbx app keys");
    }
    public static class MyDropboxAppKeysDialog extends DropboxAppKeysDialog {
        Dropbox2FileSelectionActivity owner;
        public void setOwner(Dropbox2FileSelectionActivity owner){
            this.owner = owner;
        }
        @Override
        protected void onDropboxAppKeys(DropboxUtils.AppKeys appKeys){
            owner.onDropboxAppKeys(appKeys);
        }
        @Override
        public void onCancel(DialogInterface dialogInterface){
            owner.finish();
        }
    }
    void onDropboxAppKeys(DropboxUtils.AppKeys appKeys) {
        ndvUtils.setAppKeys(appKeys);
        if (checkStartSelectFile()){
            startSelectFile();
        }
    }

    // ファイル一覧取得関連 //
    MetadataTask metadataTask;

    @Override
    protected List<FileInfo> getListFileInfo(String path) {
        metadataTask = new MetadataTask((Dropbox2FileManager)ndvFM, path);
        metadataTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        createProgressDialog(msgListing);
        return null;    // pending
    }

    void onMetadataTaskEnd(){
        deleteProgressDialog();
        if( null == metadataTask.children || metadataTask.exception) {
            // illegal directory?
            if (!retryFromRoot()) {
                showErrorMessage();
                cancel();
            }
            return;
        }

        // assert( metadataTask.metadata instanceof FolderMetadata )

        List<FileInfo> listFileInfo = new ArrayList<FileInfo>();
        for( Map.Entry<String,Metadata> fileTemp : metadataTask.children.entrySet() ){
            Metadata md = fileTemp.getValue();
            if (m_exts!=null && m_exts.length!=0 && !(md instanceof FolderMetadata)){
                String lcName = md.getName().toLowerCase();
                boolean found = false;
                for ( String ext : m_exts ){
                    if (lcName.endsWith(ext)){
                        found = true;
                        break;
                    }
                }
                if (!found) continue;
            }
            File file = new File(md.getPathDisplay());
            String path = file.getParent();
            if (path != "/") path += "/";
            FileInfo fileInfo = new FileInfo( md.getName(), md instanceof FolderMetadata, path );
            if (md instanceof FileMetadata){
                FileMetadata fm = (FileMetadata)md;
                fileInfo.setFileSize(fm.getSize());
                fileInfo.setModDate(fm.getClientModified().getTime());
            }
            listFileInfo.add( fileInfo );
        }
        Collections.sort(listFileInfo);

        showPost(m_fileDirectory, listFileInfo);
    }
    void showErrorMessage()
    {
        Toast.makeText(this, metadataTask.exceptionMessage, Toast.LENGTH_LONG).show();
    }

    class MetadataTask extends AsyncTask {
        Dropbox2FileManager dbxFM;
        String path;
        public TreeMap<String,Metadata> children;
        boolean exception;
        String exceptionMessage;
        public MetadataTask(Dropbox2FileManager dbxFM, String path){
            super();
            this.dbxFM = dbxFM;
            this.path = path;
        }
        @Override
        protected Object doInBackground(Object[] params){
            //dbxFM.lockApi();

            // Get the folder listing from Dropbox.

            ListFolderResult result;
            try {
                try {
                    result = dbxFM.getClient().files().listFolder(path.equals("/") ? "" : path);
                }
                catch (ListFolderErrorException e) {
                    //if (e.errorValue.isPath()) {
                    //    if (checkPathError(response, path, e.errorValue.getPathValue())) return;
                    //}
                    //throw e;
                    exception = true;
                    e.printStackTrace();
                    exceptionMessage = e.getMessage();
                    Log.e("PDD", "DropboxException: " + e.getMessage());
                    return null;
                }

                children = new TreeMap<String,Metadata>();
                while (true) {
                    for (Metadata md : result.getEntries()) {
                        if (md instanceof DeletedMetadata) {
                            children.remove(md.getPathLower());
                        } else {
                            children.put(md.getPathLower(), md);
                        }
                    }

                    if (!result.getHasMore()) break;

                    try {
                        result = dbxFM.getClient().files().listFolderContinue(result.getCursor());
                    }
                    catch (ListFolderContinueErrorException e) {
                        //if (e.errorValue.isPath()) {
                        //    if (checkPathError(response, path, e.errorValue.getPathValue())) return;
                        //}
                        exception = true;
                        e.printStackTrace();
                        exceptionMessage = e.getMessage();
                        Log.e("PDD", "DropboxException: " + e.getMessage());
                        return null;
                    }
                }
            }
            catch (DbxException e) {
                //common.handleDbxException(response, user, e, "listFolder(" + jq(path) + ")");
                exception = true;
                e.printStackTrace();
                exceptionMessage = e.getMessage();
                Log.e("PDD", "DropboxException: " + e.getMessage());
                return null;
            }

            //dbxFM.unlockApi();
            return null;
        }
        @Override
        protected void onPostExecute(Object result) {
            onMetadataTaskEnd();
        }
    }
}
