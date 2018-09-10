package com.reliefoffice.pdic;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

import java.io.File;


public class SettingsActivity extends PreferenceActivity implements DropboxFileSelectionDialog.OnFileSelectListener {
    private static SettingsActivity This;   // アクセスする方法がわからないので。。
    SharedPreferences pref;
    INetDriveFileManager ndvFM;
    INetDriveUtils ndvUtils;
    CheckBoxPreference psbmSharing;
    EditTextPreference AudioFileFolder;
    CheckBoxPreference psbmDefCharset;
    static public String DefaultAudioFolder = "/storage/sdcard0/Download";

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

    // Dropbox serverからdownloadするファイルを選択する
    //DropboxFileSelectionDialog fileSelectionDialog;
    private void selectDropboxFile(){
        DropboxFileSelectionDialog dlg = DropboxFileSelectionDialog.createInstance( this, this, ndvFM, false);
        dlg.setOnCancelListener(new FileSelectionDialog.OnCancelListener() {
            @Override
            public void onCancel() {
                psbmSharing.setChecked(false);
            }
        });
        String[] exts = {".txt"};
        dlg.show(new File(ndvUtils.getInitialDir()), exts);
    }
    @Override
    public void onFileSelect(FileInfo file) {
        File fileDir = Utility.getWorkDirectory(this);
        if (fileDir==null) return;

        File toFile = new File(fileDir, file.getName());
        downloadFile(file.getAbsolutePath(), toFile);
        psbmSharing.setSummary(file.getAbsolutePath());
        psbmSharing.setChecked(true);   // �F�؂��o�R�����check����Ȃ����߁i�ΏǗÖ@�j

        ndvUtils.setInitialDir(file.getParent() );
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
