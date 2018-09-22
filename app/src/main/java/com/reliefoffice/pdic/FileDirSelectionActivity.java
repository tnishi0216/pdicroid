package com.reliefoffice.pdic;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;

import java.util.List;

/**
 * Created by nishikawat on 2016/10/12.
 */

public class FileDirSelectionActivity extends FileSelectionActivity {
    static final String PFS_INITIALDIR = "FileSelInitialDir";
    String m_strInitialDir = Utility.initialFileDirectory();

    public void setSaveMode(){
        modeForSave = true;
    }
    private boolean modeForSave;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    protected boolean retryFromRoot(){
        if (m_fileDirectory.getPath().equals("/")) {
            // already rooted. Show the advisable message.
            return false;
        } else {
            Log.d("PDD", "retryFromRoot");
            m_fileDirectory.setRoot();
            show(m_fileDirectory, m_exts);
            return true;
        }
    }

    protected void showPost(FileInfo fileDirectory, List<FileInfo> listFileInfo) {
        textPath.setText(fileDirectory.getAbsolutePath());

        if( null != fileDirectory.getParent() ){
            listFileInfo.add( 0, new FileInfo( "..", true, fileDirectory.getParent() ) );
        }

        super.showPost(fileDirectory, listFileInfo);
    }

//    @Override
//    protected void prepareParams(){
//        super.prepareParams();
//        if (Utility.isNotEmpty(m_fileDirectory.getAbsolutePath())){
//            setFileDirectory( pref.getString(PFS_INITIALDIR, m_strInitialDir) );
//        }
//    }
}
