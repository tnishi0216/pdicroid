package com.reliefoffice.pdic;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Environment;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

import com.reliefoffice.pdic.text.config;

import java.io.File;


public class SettingsActivity extends PreferenceActivity implements DropboxFileSelectionDialog.OnFileSelectListener {
    private static SettingsActivity This;   // アクセスする方法がわからないので。。
    SharedPreferences pref;
    INetDriveFileManager ndvFM;
    INetDriveUtils ndvUtils;
    CheckBoxPreference psbmSharing;
    EditTextPreference AudioFileFolder;
    CheckBoxPreference psbmDefCharset;

    static final public String getDefaultAudioFolder()
    {
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        This = this;

        addPreferencesFromResource(R.xml.preference);

        pref = PreferenceManager.getDefaultSharedPreferences(this);
        ndvFM = DropboxFileManager.createInstance(this);
        ndvUtils = DropboxUtils.getInstance(this);

        psbmSharing = (CheckBoxPreference) findPreference("PSBookmarkSharing");
        psbmSharing.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                if (psbmSharing.isChecked()) {
                    if (ndvFM.startAuth(false)) {
                        selectDropboxFile();
                    } else {
                        if (!ndvUtils.appKeysConfirmed) {
                            MyDropboxAppKeysDialog dlg = new MyDropboxAppKeysDialog();
                            dlg.show(getFragmentManager(), "dbx app keys");
                        } else {
                            startAuth();
                        }
                    }
                } else {
                    PSBookmarkFileManager psbmFM = PSBookmarkFileManager.createInstance(This, ndvFM);
                    psbmFM.changeFilename(null, null, null);
                    psbmFM.deleteInstance();
                }
                return true;
            }
        });
        if (psbmSharing.isChecked()) {
            PSBookmarkFileManager psbmFM = PSBookmarkFileManager.createInstance(this, ndvFM);
            psbmSharing.setSummary(psbmFM.getRemoteFilename());
            psbmFM.deleteInstance();
        }

        AudioFileFolder = (EditTextPreference) findPreference("AudioFileFolder");
        if (Utility.isEmpty(AudioFileFolder.getText())){
            String DefaultAudioFolder = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
            // DefaultAudioFolder = "/storage/emulated/0/Download";
            // DefaultAudioFolder = "/storage/sdcard0/Download";
            AudioFileFolder.setText(DefaultAudioFolder);
        }

        psbmDefCharset = (CheckBoxPreference) findPreference("DefCharset");
    }

    // summaryの動的変更
    private SharedPreferences.OnSharedPreferenceChangeListener listener =
            new SharedPreferences.OnSharedPreferenceChangeListener() {
                public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
                    AudioFileFolder.setSummary(AudioFileFolder.getText());
                }
            };

    public static class MyDropboxAppKeysDialog extends DropboxAppKeysDialog {
        @Override
        protected void onDropboxAppKeys(DropboxUtils.AppKeys appKeys){
            This.onDropboxAppKeys(appKeys);
        }
    }
    void onDropboxAppKeys(DropboxUtils.AppKeys appKeys){
        ndvUtils.setAppKeys(appKeys);
        startAuth();
    }
    void startAuth(){
        if (ndvFM.startAuth(false)){
            selectDropboxFile();
            ndvUtils.appKeysConfirmed = true;
        }
    }

    static final int REQUEST_CODE_SELECT_FILE_DBX = 3;

    // Dropbox serverからdownloadするファイルを選択する
    //DropboxFileSelectionDialog fileSelectionDialog;
    private void selectDropboxFile(){
        if (config.useOldFileSelection) {
            DropboxFileSelectionDialog dlg = DropboxFileSelectionDialog.createInstance(this, this, ndvFM, false);
            dlg.setOnCancelListener(new FileSelectionDialog.OnCancelListener() {
                @Override
                public void onCancel() {
                    psbmSharing.setChecked(false);
                }
            });
            String[] exts = {".txt"};
            dlg.show(new File(ndvUtils.getInitialDir()), exts);
        } else {
            Intent i = new Intent().setClassName(this.getPackageName(), Dropbox2FileSelectionActivity.class.getName());
            i.putExtra("onlySelection", true);
            String[] exts = {".txt"};
            i.putExtra("exts", exts);
            i.putExtra("no_encoding", true);
            startActivityForResult(i, REQUEST_CODE_SELECT_FILE_DBX);
        }
    }
    @Override
    public void onFileSelect(FileInfo file) {
        File fileDir = Utility.getWorkDirectory(this);
        if (fileDir==null) return;

        File toFile = new File(fileDir, file.getName());
        downloadFile(file.getAbsolutePath(), toFile);

        ndvUtils.setInitialDir(file.getParent() );
    }
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_CODE_SELECT_FILE_DBX) {
            if (resultCode == RESULT_OK) {
                Bundle bundle = data.getExtras();
                if (bundle != null) {
                    String filename = bundle.getString("filename");
                    if (Utility.isNotEmpty(filename)) {
                        File file = new File(filename);
                        String name;

                        // dropbox
                        //String remotename = bundle.getString("remotename");
                        // The file is selected to be added.
                        name = file.getName() + " [Dropbox]";

                        //fileEncoding = bundle.getString("fileEncoding");
                        FileInfo fileInfo = new FileInfo(name, file);
                        onFileSelect(fileInfo);
                    }
                }
            }
        }
    }

    void downloadFile(String from, File to){
        ndvFM.executeDownload(from, to, new INetDriveFileManager.OnExecuteListener() {
            @Override
            public void onPostExecute(boolean downloaded, String from, File to, String revision) {
                if (!downloaded){
                    Toast.makeText(getApplicationContext(), getString(R.string.msg_bookmark_file_downloaded) + to.getPath(), Toast.LENGTH_SHORT).show();
                    return;
                }

                Toast.makeText(getApplicationContext(), getString(R.string.msg_bookmark_file_downloaded) + to.getPath(), Toast.LENGTH_SHORT).show();
                Log.d("PDD", "File downloaded: " + to.getAbsolutePath() + " rev:" + revision);
                PSBookmarkFileManager psbmFM = PSBookmarkFileManager.createInstance(This, ndvFM);
                psbmFM.changeFilename(to.getAbsolutePath(), from, revision);
                psbmFM.deleteInstance();
                psbmSharing.setSummary(from);
                psbmSharing.setChecked(true);
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();

        getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(listener);

        if (ndvFM.authComplete(this)){
            selectDropboxFile();
        }

        PSBookmarkFileManager psbmFM = PSBookmarkFileManager.createInstance(this, ndvFM);
        boolean enabled = psbmFM.isRemoteEnabled();
        psbmFM.deleteInstance();
        if (!enabled){
            psbmSharing.setChecked(false);
        }
    }

    @Override
    protected void onPause(){
        super.onPause();
        getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(listener);
    }

    /*
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_settings, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
    */
}
