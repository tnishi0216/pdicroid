package com.reliefoffice.pdic;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.PopupMenu;
import android.widget.Toast;

import com.reliefoffice.pdic.text.config;
import com.reliefoffice.pdic.text.pfs;

import java.io.File;
import java.util.ArrayList;


public class DicSettingActivity extends Activity implements FileSelectionDialog.OnFileSelectListener {

    static DicSettingActivity This;

    SharedPreferences pref;
    DicPref dicPref;
    ListView lvDicList;
    ArrayAdapter<String> adpDicList;
    int lastSelDic;

    private String m_strInitialDir = "/";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        This = this;

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_dic_setting);

        lvDicList = (ListView)findViewById(R.id.dicList);
        adpDicList = new ArrayAdapter<String>(this, android.R.layout.simple_selectable_list_item);
        lvDicList.setAdapter(adpDicList);

        lvDicList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                int num = adpDicList.getCount();
                if (position == num - 3) {
                    // download
                    actionDownload();
                } else if (position == num - 2) {
                    // add file
                    actionAddFile();
                } else if (position == num - 1) {
                    // add file from dropbox
                    actionAddFileFromDropbox();
                } else if (position == num - 0) {
                    // add file from Google Drive
                    actionAddFileFromGoogleDrive();
                } else {
                    // context menu //
                    lastSelDic = position;
                    contextMenu(view);
                }
            }
        });

        pref = PreferenceManager.getDefaultSharedPreferences(this);
        dicPref = new DicPref(pref);

        updateList();
    }

    void contextMenu(View view){
        // PopupMenu : refer to http://techbooster.org/android/ui/3056/
        PopupMenu popup = new PopupMenu(this, view);

        popup.getMenuInflater().inflate(R.menu.dic_context_menu, popup.getMenu());

        popup.show();

        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
            public boolean onMenuItemClick(MenuItem item) {
                if (item.getItemId() == R.id.showDicInfo) {
                    showDicInfo(lastSelDic);
                } else if (item.getItemId() == R.id.removeItem) {
                    removeDic(lastSelDic);
                } else if (item.getItemId() == R.id.moveToUp) {
                    moveToUp(lastSelDic);
                } else if (item.getItemId() == R.id.moveToDown) {
                    moveToDown(lastSelDic);
                } else {
                    Toast.makeText(DicSettingActivity.this, "Clicked : " + item.getTitle(), Toast.LENGTH_SHORT).show();
                }
                return true;
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        updateList();
        m_strInitialDir = pref.getString(pfs.INITIALDIR, m_strInitialDir);
    }

    void updateList(){
        adpDicList.clear();

        //ArrayList<String> dicPath = getDictionaryPath();
        ArrayList<String> dicName = dicPref.getDictionaryName();

        for (int i=0;i<dicName.size();i++) {
            if (dicName.get(i)!=null)
                adpDicList.add(dicName.get(i));
            else
                adpDicList.add("(null)");
        }

        adpDicList.add(getString(R.string.label_download));
        adpDicList.add(getString(R.string.label_add_file));
        adpDicList.add(getString(R.string.label_add_file_dropbox));
        //adpDicList.add(getString(R.string.label_add_file_googledrive));
    }

    void actionDownload(){
        startActivity(new Intent().setClassName(this.getPackageName(), DicDownloadList.class.getName()));
    }
    static final int REQUEST_CODE_ADD_FILE = 1;
    static final int REQUEST_CODE_ADD_FILE_DBX = 2;
    static final int REQUEST_CODE_ADD_FILE_GDV = 3;
    void actionAddFile(){
        if (config.useOldFileSelection) {
            FileSelectionDialog dlg = new FileSelectionDialog(this, this, false);
            String[] exts = {".txt", ".dic"};
            dlg.show(new File(m_strInitialDir), exts);
        } else {
            Intent i = new Intent().setClassName(this.getPackageName(), FileDirSelectionActivity.class.getName());
            i.putExtra(pfs.INITIALDIR, m_strInitialDir);
            startActivityForResult(i, REQUEST_CODE_ADD_FILE);
        }
    }
    void actionAddFileFromDropbox(){
        Intent i;
        if (config.useOldFileSelection) {
            i = new Intent().setClassName(this.getPackageName(), DropboxDownloadActivity.class.getName());
        } else {
            i = new Intent().setClassName(this.getPackageName(), Dropbox2FileSelectionActivity.class.getName());
        }
        startActivityForResult(i, REQUEST_CODE_ADD_FILE_DBX);
    }
    void actionAddFileFromGoogleDrive(){
        Intent i = new Intent().setClassName(this.getPackageName(), GoogleDriveDownloadActivity.class.getName());
        startActivityForResult(i, REQUEST_CODE_ADD_FILE_GDV);
    }

    // Activity result handler //
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == RESULT_OK) {
            Bundle bundle = data.getExtras();
            if (bundle != null) {
                String filename = bundle.getString("filename");
                if (Utility.isNotEmpty(filename)) {
                    if (requestCode == REQUEST_CODE_ADD_FILE) {
                        FileInfo fileInfo = new FileInfo(filename);
                        onFileSelect(fileInfo);
                    } else
                    if (requestCode == REQUEST_CODE_ADD_FILE_DBX) {
                        //String remotename = bundle.getString("remotename");
                        // The file is selected to be added.
                        File file = new File(filename);
                        String name = file.getName() + " [Dropbox]";
                        addDictionaryFile(filename, name);
                    }
                }
            }
        }
    }

    @Override
    public void onFileSelect(FileInfo file) {
        addDictionaryFile(file.getPath(), file.getName());

        m_strInitialDir = file.getParent();
        SharedPreferences.Editor edit = pref.edit();
        edit.putString(pfs.INITIALDIR, m_strInitialDir);
        edit.commit();
    }

    void addDictionaryFile(String filename, String name){
        DicInfo info = new DicInfo();
        info.filename = filename;
        info.name = name;
        if (Utility.isEmpty(info.name)){
            File file = new File(filename);
            info.name = file.getName();
        }
        dicPref.addDicInfo(info);
        Toast.makeText(this, getString(R.string.msg_dictionary_added) + " : " + filename, Toast.LENGTH_SHORT).show();
        //adpDicList.insert(0, file.getParent());
        updateList();
    }


    void showDicInfo(int index){
        DicInfo info = dicPref.loadDicInfo(index);
        Toast.makeText(DicSettingActivity.this, "UpgradeKey="+info.upgradeKey+" URL="+info.HPUrl+" "+info.descriptoin, Toast.LENGTH_LONG).show();
    }
    void removeDic(int index){
        DicInfo info = dicPref.loadDicInfo(index);
        File file = new File(info.filename);
        //file.delete();
        dicPref.remove(index);
        adpDicList.remove((String)lvDicList.getItemAtPosition(index));
        Toast.makeText(DicSettingActivity.this, getString(R.string.msg_dictionary_removed)+" "+(info.name!=null ? info.name : ""), Toast.LENGTH_SHORT).show();
    }
    void moveToUp(int index){
        if (index==0) return;
        dicPref.exchange(index-1, index);
        updateList();
    }
    void moveToDown(int index){
        if (index==dicPref.getNum()-1) return;
        dicPref.exchange(index, index+1);
        updateList();
    }

    /*
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_dic_setting, menu);
        return true;
    }
    */

    /*
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
