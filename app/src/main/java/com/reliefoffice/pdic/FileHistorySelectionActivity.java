package com.reliefoffice.pdic;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.CallSuper;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by nishikawat on 2016/10/14.
 */

public class FileHistorySelectionActivity extends FileSelectionActivity {
    FileHistoryManager fileHistory;
    boolean noHistory;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        fileHistory = new FileHistoryManager(this);
    }

    @Override
    protected List<FileInfo> getListFileInfo(FileInfo fileDirectory) {
        if (fileHistory.size()==0){
            // no history exists
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            //builder.setTitle("");
            builder.setMessage(getString(R.string.msg_no_history));
            builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    finish();
                }
            });
            builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
                @Override
                public void onCancel(DialogInterface dialog) {
                    finish();
                }
            });
            //builder.setView( listview );
            builder.show();
            noHistory = true;
            return null;
        }

        INetDriveUtils ndvUtils = DropboxUtils.getInstance(this);

        List<FileInfo> listFile = new ArrayList<FileInfo>();
        for (int i=0;i<fileHistory.size();i++) {
            String filename = fileHistory.get(i);
            File file = new File(filename);
            FileInfo fileInfo;
            if (ndvUtils.hasPrefix(filename)) {
                // dropbox file
                String localName = ndvUtils.convertToLocalName(filename);
                File localFile = new File( localName );
                fileInfo = new FileInfo(localFile.getName(), file);
                fileInfo.setFileSize(localFile.length());
                fileInfo.setModDate(localFile.lastModified());
            } else {
                // normal file
                fileInfo = new FileInfo(file.getName(), file);
            }
            fileInfo.setReaddate(fileHistory.getDateLong(i));
            listFile.add(fileInfo);
        }
        return listFile;
    }

    @CallSuper
    protected void setupReturnValues(String filename, Intent i){
        INetDriveUtils ndvUtils = DropboxUtils.getInstance(this);
        if (ndvUtils.hasPrefix(filename)){
            i.putExtra("remotename", filename.substring(4));
            filename = ndvUtils.convertToLocalName(filename);
        }

        super.setupReturnValues(filename, i);

        // auto選択で、すでにencodingが指定されていた場合はそれを利用
        if (rgEncoding.getCheckedRadioButtonId() == R.id.btn_auto){
            String encoding = fileHistory.getEncoding(clickedPosition);
            if (Utility.isNotEmpty(encoding)){
                i.putExtra("fileEncoding", encoding);
            }
        }
    }
}
