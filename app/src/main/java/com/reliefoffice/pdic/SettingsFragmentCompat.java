package com.reliefoffice.pdic;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.support.v7.preference.CheckBoxPreference;
import android.support.v7.preference.EditTextPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;
import android.util.Log;
import android.widget.Toast;

import com.reliefoffice.pdic.text.pfs;

import java.io.File;

public class SettingsFragmentCompat extends PreferenceFragmentCompat implements DropboxFileSelectionDialog.OnFileSelectListener {
    private static SettingsFragmentCompat This;   // アクセスする方法がわからないので。。
    INetDriveFileManager ndvFM;
    INetDriveUtils ndvUtils;
    CheckBoxPreference psbmSharing;
    EditTextPreference AudioFileFolder;
    CheckBoxPreference psbmDefCharset;

    public SettingsFragmentCompat(){

    }

    public static SettingsFragmentCompat newInstance(String param1, String param2) {
        SettingsFragmentCompat fragment = new SettingsFragmentCompat();
        Bundle args = new Bundle();
        //args.putString(ARG_PARAM1, param1);
        //args.putString(ARG_PARAM2, param2);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        This = this;

        addPreferencesFromResource(R.xml.preference);

        ndvFM = DropboxFileManager.createInstance(getActivity());
        ndvUtils = DropboxUtils.getInstance(getActivity());

        psbmSharing = (CheckBoxPreference) findPreference("PSBookmarkSharing");

        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(getContext());
        final boolean debug = pref.getBoolean(pfs.DEBUG, false);
        if (debug) {
            psbmSharing.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {
                    if (psbmSharing.isChecked()) {
                        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(getContext());
                        boolean debug = pref.getBoolean(pfs.DEBUG, false);
                        if (debug) {
                            if (ndvFM.startAuth(false)) {
                                selectDropboxFile();
                            } else {
                                if (!ndvUtils.appKeysConfirmed) {
                                    SettingsFragmentCompat.MyDropboxAppKeysDialog dlg = new SettingsFragmentCompat.MyDropboxAppKeysDialog();
                                    dlg.parent = This;
                                    dlg.show(getFragmentManager(), "dbx app keys");
                                } else {
                                    startAuth();
                                }
                            }
                        }
                    } else {
                        PSBookmarkFileManager psbmFM = PSBookmarkFileManager.createInstance(getActivity(), ndvFM);
                        psbmFM.changeFilename(null, null, null);
                        psbmFM.deleteInstance();
                    }
                    return true;
                }
            });
            if (psbmSharing.isChecked()) {
                PSBookmarkFileManager psbmFM = PSBookmarkFileManager.createInstance(getActivity(), ndvFM);
                psbmSharing.setSummary(psbmFM.getRemoteFilename());
                psbmFM.deleteInstance();
            }
        } else {
            psbmSharing.setVisible(false);
        }

        AudioFileFolder = (EditTextPreference) findPreference("AudioFileFolder");
        if (Utility.isEmpty(AudioFileFolder.getText())){
            String DefaultAudioFolder = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
            // DefaultAudioFolder = "/storage/emulated/0/Download";
            // DefaultAudioFolder = "/storage/sdcard0/Download";
            AudioFileFolder.setText(DefaultAudioFolder);
        }
        AudioFileFolder.setSummary(AudioFileFolder.getText());

        psbmDefCharset = (CheckBoxPreference) findPreference("DefCharset");
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {

    }

    // summaryの動的変更
    private SharedPreferences.OnSharedPreferenceChangeListener listener =
            new SharedPreferences.OnSharedPreferenceChangeListener() {
                public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
                    AudioFileFolder.setSummary(AudioFileFolder.getText());
                }
            };

    public static class MyDropboxAppKeysDialog extends DropboxAppKeyDialogCompat {
        public SettingsFragmentCompat parent;
        @Override
        protected void onDropboxAppKeys(DropboxUtils.AppKeys appKeys){
            parent.onDropboxAppKeys(appKeys);
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
        Intent i = new Intent().setClassName(getActivity().getPackageName(), Dropbox2FileSelectionActivity.class.getName());
        i.putExtra("onlySelection", true);
        String[] exts = {".txt"};
        i.putExtra("exts", exts);
        i.putExtra("no_encoding", true);
        startActivityForResult(i, REQUEST_CODE_SELECT_FILE_DBX);
    }
    @Override
    public void onFileSelect(FileInfo file) {
        File fileDir = Utility.getWorkDirectory(getActivity());
        if (fileDir==null) return;

        File toFile = new File(fileDir, file.getName());
        downloadFile(file.getAbsolutePath(), toFile);

        ndvUtils.setInitialDir(file.getParent() );
    }
    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_CODE_SELECT_FILE_DBX) {
            if (resultCode == getActivity().RESULT_OK) {
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
                    Toast.makeText(getActivity(), getString(R.string.msg_bookmark_file_downloaded) + to.getPath(), Toast.LENGTH_SHORT).show();
                    return;
                }

                Toast.makeText(getActivity(), getString(R.string.msg_bookmark_file_downloaded) + to.getPath(), Toast.LENGTH_SHORT).show();
                Log.d("PDD", "File downloaded: " + to.getAbsolutePath() + " rev:" + revision);
                PSBookmarkFileManager psbmFM = PSBookmarkFileManager.createInstance(getActivity(), ndvFM);
                psbmFM.changeFilename(to.getAbsolutePath(), from, revision);
                psbmFM.deleteInstance();
                psbmSharing.setSummary(from);
                psbmSharing.setChecked(true);
            }
        });
    }

    @Override
    public void onResume() {
        super.onResume();

        getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(listener);

        if (ndvFM.authComplete(getActivity())){
            selectDropboxFile();
        }

        PSBookmarkFileManager psbmFM = PSBookmarkFileManager.createInstance(getActivity(), ndvFM);
        boolean enabled = psbmFM.isRemoteEnabled();
        psbmFM.deleteInstance();
        if (!enabled){
            psbmSharing.setChecked(false);
        }
    }

    @Override
    public void onPause(){
        super.onPause();
        getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(listener);
    }
}
