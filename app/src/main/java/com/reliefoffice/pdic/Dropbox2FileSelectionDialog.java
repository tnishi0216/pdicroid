package com.reliefoffice.pdic;

import android.content.Context;
import android.os.AsyncTask;
import android.util.Log;
import android.widget.Toast;

import com.dropbox.client2.DropboxAPI;
import com.dropbox.client2.exception.DropboxException;
import com.dropbox.core.DbxException;
import com.dropbox.core.v2.files.DeletedMetadata;
import com.dropbox.core.v2.files.FolderMetadata;
import com.dropbox.core.v2.files.GetMetadataErrorException;
import com.dropbox.core.v2.files.ListFolderContinueErrorException;
import com.dropbox.core.v2.files.ListFolderErrorException;
import com.dropbox.core.v2.files.ListFolderResult;
import com.dropbox.core.v2.files.Metadata;
import com.reliefoffice.pdic.text.config;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * Created by nishikawat on 2016/09/29.
 */

public class Dropbox2FileSelectionDialog extends DropboxFileSelectionDialog {
    public Dropbox2FileSelectionDialog( Context context,
                                       OnFileSelectListener listener, INetDriveFileManager dbxFM, boolean modeForSave ){
        super(context, listener, dbxFM, modeForSave);
    }

    MetadataTask metadataTask;

    @Override
    protected List<FileInfo> getListFileInfo(FileInfo fileDirectory) {
        String path = fileDirectory.getAbsolutePath();
        metadataTask = new MetadataTask((Dropbox2FileManager)ndvFM, path);
        metadataTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        createProgressDialog();
        return null;    // pending
    }

    @Override
    protected void showErrorMessage()
    {
        Toast.makeText(context, metadataTask.exceptionMessage, Toast.LENGTH_LONG).show();
    }

    void onMetadataTaskEnd(){
        deleteProgressDialog();
        if( null == metadataTask.children || metadataTask.exception) {
            // illegal directory?
            if (!retryFromRoot()) {
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
            if (!path.equals("/")) path += "/";
            listFileInfo.add( new FileInfo( md.getName(), md instanceof FolderMetadata, path ) );
        }
        Collections.sort(listFileInfo);

        showPost(m_fileDirectory, listFileInfo);
    }

    class MetadataTask extends AsyncTask {
        Dropbox2FileManager dbxFM;
        String path;
        //public Metadata metadata;
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
