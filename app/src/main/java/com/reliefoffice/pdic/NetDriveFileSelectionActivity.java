package com.reliefoffice.pdic;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.Toast;

import java.io.File;

/**
 * Created by nishikawat on 2016/10/12.
 */

public abstract class NetDriveFileSelectionActivity extends FileDirSelectionActivity {
    INetDriveFileManager ndvFM;
    INetDriveUtils ndvUtils;
    boolean fromNetDrive = true;    //TODO: これいらない？
    boolean onlySelection = false;  // not download a file

    public NetDriveFileSelectionActivity(){
        super();
        createNetDriveComponents();
        m_noReadDate = true;
    }

    @Override
    protected void prepareParams(){
        super.prepareParams();
        String path = ndvUtils.getInitialDir();
        if (Utility.isNotEmpty(path))
            setFileDirectory(ndvUtils.getInitialDir());

        Intent i = getIntent();
        if (i.getBooleanExtra("onlySelection", false)){
            onlySelection = true;
        }
    }

    abstract void createNetDriveComponents();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (ndvFM.startAuth(false)) {
            startSelectFile();
        } else {
            if (!ndvUtils.appKeysConfirmed) {
                showAppKeyDialog();
            }
        }
    }

    // 認証dialogを表示
    abstract protected  void showAppKeyDialog();

    // 認証が通っていればtrueを返す
    @Override
    protected boolean checkStartSelectFile(){
        if (ndvFM.authComplete(this)) {
            ndvUtils.appKeysConfirmed = true;
            return true;    // startSelectFile開始
        }
        return false;
    }

    @Override
    protected void onFileSelect(FileInfo file) {
        if (fromNetDrive) {
            ndvUtils.setInitialDir(file.getParent());
            if (onlySelection){
                exit(file.getAbsolutePath(), null);
            } else {
                String toFileName = ndvUtils.convertToLocalName(file.getAbsolutePath());
                File toFile = new File(toFileName);
                ndvUtils.makeDir(this, toFile);
                executeDownloadFile(file.getAbsolutePath(), toFile);    // execute download file
            }
        } else {
            File dicDir = DicUtils.getDictionaryPath(this);
            if (dicDir == null) return;
            File toFile = new File(dicDir, file.getName());
            downloadFile(file.getAbsolutePath(), toFile);

            m_strInitialDir = file.getParent();
            SharedPreferences.Editor edit = pref.edit();
            edit.putString(PFS_INITIALDIR, m_strInitialDir);    // execute download file
            edit.commit();
        }
    }

    void executeDownloadFile(String from, File to) {
        downloadFile(from, to);
        createProgressDialog(msgLoading);
    }

    void downloadFile(String from, File to){
        ndvFM.executeDownload(from, to, new INetDriveFileManager.OnExecuteListener() {
            @Override
            public void onPostExecute(boolean downloaded, String from, File to, String revision) {
                deleteProgressDialog();
                if (!downloaded){
                    // failed to downloada
                    finish();
                } else {
                    // downloaded successfully
                    exit(to.getAbsolutePath(), from);
                }
            }
        });
    }

    void exit(String filename, String downloadedRemoteName){
        Intent i = new Intent();
        super.setupReturnValues(filename, i);
        i.putExtra("remotename", downloadedRemoteName);
        setResult(Activity.RESULT_OK, i);
        finish();
    }

    // Progress Dialog //
    ProgressDialog progress;
    final static int msgListing = 0;
    final static int msgLoading = 1;
    final static int msgSaving = 2;
    void createProgressDialog(int option){
        progress = new ProgressDialog(this);
        switch (option){
            case 1:
                progress.setTitle(getString(R.string.title_prog_file_loading));
                progress.setMessage(getString(R.string.msg_prog_file_loading));
                break;
            case 2:
                progress.setTitle(getString(R.string.title_prog_file_saving));
                progress.setMessage(getString(R.string.msg_prog_file_saving));
                break;
            default:
                progress.setTitle(getString(R.string.title_prog_dirlsit));
                progress.setMessage(getString(R.string.msg_prog_dirlist));
                break;
        }
        progress.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        progress.show();
    }
    void deleteProgressDialog(){
        if (progress==null) return;
        progress.dismiss();
        progress = null;
    }
}
