package com.reliefoffice.pdic;

import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.Toast;

import com.reliefoffice.pdic.text.config;

import java.io.File;

/**
 * Created by nishikawat on 2016/05/25.
 */
public class DropboxDownloadActivity extends NetDriveDownloadActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (ndvFM.startAuth(false)) {
            selectFile();
        } else {
            if (!ndvUtils.appKeysConfirmed) {
                MyDropboxAppKeysDialog dlg = new MyDropboxAppKeysDialog();
                dlg.show(getFragmentManager(), "dbx app keys");
            }
        }
    }
    /*@Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Button btnDropboxSearch = (Button)findViewById(R.id.dropbox_search);
        btnDropboxSearch.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dropboxSearch();
            }
        });
        Button btnDropboxMetadata  = (Button)findViewById(R.id.dropbox_metadata);
        btnDropboxMetadata.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dropboxMetadata();
            }
        });
    }*/
    @Override
    void createNetDriveComponents() {
        ndvUtils = DropboxUtils.getInstance(this);
        ndvFM = DropboxFileManager.createInstance(this);
    }

    @Override
    void selectFile() {
        DropboxFileSelectionDialog dlg = DropboxFileSelectionDialog.createInstance(this, this, ndvFM, false);
        dlg.setOnCancelListener(new DropboxFileSelectionDialog.OnCancelListener() {
            @Override
            public void onCancel() {
                finish();
            }
        });
        dlg.show(new File(ndvUtils.getInitialDir()), config.DicTextExtensions);
        fromNetDrive = true;
    }

    public static class MyDropboxAppKeysDialog extends DropboxAppKeysDialog {
        @Override
        protected void onDropboxAppKeys(DropboxUtils.AppKeys appKeys){
            ((DropboxDownloadActivity)This).onDropboxAppKeys(appKeys);
        }
        @Override
        public void onCancel(DialogInterface dialogInterface){
            This.finish();
        }
    }
    void onDropboxAppKeys(DropboxUtils.AppKeys appKeys) {
        ndvUtils.setAppKeys(appKeys);
        downloadFile();
    }
    /*
    //Memo: 2016.5.25 使用していなかったのでcomment out。
    // 本来なら、DropboxSearchTaskのようなクラスにすべき(DropboxDownloadTask参照)
    SearchTask searchTask;
    private void dropboxSearch(){
        searchTask = new SearchTask();
        searchTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    class SearchTask extends AsyncTask {
        public SearchTask(){
            super();
        }
        @Override
        protected Object doInBackground(Object[] param){
            ndvFM.lockApi();
            try {
                int filelimit = -1; // Default is 10,000 if you pass in 0 or less
                List<DropboxAPI.Entry> entries = ndvFM.getApi().search("/", "txt", filelimit, false);
                for (DropboxAPI.Entry entry : entries) {
                    // display found items.
                    Log.i("PDD",
                            String.format("path=%s,rev=%s",
                                    entry.path,
                                    entry.rev));
                }
            } catch (DropboxUnlinkedException e) {
                Log.e("PDD", "DropboxUnlinkedException");
            } catch (DropboxServerException e) {
                Log.e("PDD", "DropboxServerException");
            } catch (DropboxIOException e) {
                Log.e("PDD", "DropboxIOException");
            } catch (DropboxException e) {
                Log.e("PDD", "DropboxException");
            } catch (Exception e) {
                e.printStackTrace();
                Log.e("PDD","Search Error", e);
            }
            ndvFM.unlockApi();
            return null;
        }
        @Override
        protected void onPostExecute(Object result) {

        }
    }*/
}
