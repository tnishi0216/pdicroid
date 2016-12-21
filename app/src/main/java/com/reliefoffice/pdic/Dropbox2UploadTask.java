package com.reliefoffice.pdic;

import android.os.AsyncTask;
import android.util.Log;

import com.dropbox.client2.DropboxAPI;
import com.dropbox.client2.exception.DropboxException;
import com.dropbox.core.DbxException;
import com.dropbox.core.v2.files.FileMetadata;
import com.dropbox.core.v2.files.WriteMode;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

/**
 * Created by tnishi on 2016/09/08.
 */
public class Dropbox2UploadTask extends AsyncTask {
    protected Dropbox2FileManager dbxFM;
    protected File from;
    protected String to;
    protected String revision;
    protected boolean uploaded;
    public Dropbox2UploadTask(Dropbox2FileManager dbxFM, File from, String to){
        super();
        this.dbxFM = dbxFM;
        this.from = from;
        this.to = to;
        uploaded = false;
    }
    @Override
    protected Object doInBackground(Object[] params) {
        dbxFM.lockApi();
        try {
            FileInputStream inputStream = new FileInputStream(from);
            //final DropboxAPI.Entry entry = dbxFM.getApi().putFileOverwrite(to, inputStream, from.length(), null);
            final FileMetadata metadata = dbxFM.getClient().files().uploadBuilder(to)
                    .withMode(WriteMode.OVERWRITE)
                    .uploadAndFinish(inputStream);
            revision = metadata.getRev();
            uploaded = true;
            Log.i("PDP", "The file's rev is: " + metadata.getRev());
        } catch(IOException e){
            e.printStackTrace();
        } catch(DbxException e){
            e.printStackTrace();
        }
        dbxFM.unlockApi();
        return null;
    }
}
