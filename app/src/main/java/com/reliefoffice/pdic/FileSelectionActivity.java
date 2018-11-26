package com.reliefoffice.pdic;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.annotation.CallSuper;
import android.support.v7.app.ActionBarActivity;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.ListView;
import android.widget.RadioGroup;
import android.widget.TextView;

import com.reliefoffice.pdic.text.pfs;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class FileSelectionActivity extends ActionBarActivity implements FileSelectionInterface.FileListListener {
    SharedPreferences pref;

    protected  String[] m_exts = {".dic", ".txt"};
    protected FileInfo m_fileDirectory;
    void setFileDirectory(String dir){
        m_fileDirectory = new FileInfo(dir);
    }
    String getCurrentDir(){
        return m_fileDirectory.getPath();
    }

    FileListAdapter fileListAdapter;
    //FileInfoArrayAdapter fileListAdapter;

    TextView textPath;
    RadioGroup rgEncoding;
    int clickedPosition;

    enum SortType {
        Name, NameR,
        Date, DateR,
        Read, ReadR,
        Size, SizeR
    }
    SortType lastSortType = SortType.Name;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_file_selection);

        pref = PreferenceManager.getDefaultSharedPreferences(this);

        // TextView path //
        textPath = (TextView)findViewById(R.id.text_path);

        // ListView //
        final ListView fileList = (ListView) findViewById(R.id.fileList);
        fileList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                clickedPosition = position;
                FileInfo info = fileListAdapter.getItem(position);
                onFileItemClick(info);
            }
        });

        // Sort buttons //
        Button btnName = (Button)findViewById(R.id.btn_name);
        btnName.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (lastSortType==SortType.Name) {
                    lastSortType = SortType.NameR;
                } else {
                    lastSortType = SortType.Name;
                }
                sortCommon(true);
            }
        });
        Button btnDate = (Button)findViewById(R.id.btn_date);
        btnDate.setOnClickListener(new View.OnClickListener() {
            @Override
                public void onClick(View v) {
                if (lastSortType==SortType.DateR) {
                    lastSortType = SortType.Date;
                } else {
                    lastSortType = SortType.DateR;
                }
                sortCommon(true);
            }
        });
        Button btnRead = (Button)findViewById(R.id.btn_read);
        btnRead.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (lastSortType==SortType.ReadR) {
                    lastSortType = SortType.Read;
                } else {
                    lastSortType = SortType.ReadR;
                }
                sortCommon(true);
            }
        });
        Button btnSize = (Button)findViewById(R.id.btn_size);
        btnSize.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (lastSortType==SortType.SizeR) {
                    lastSortType = SortType.Size;
                } else {
                    fileListAdapter.sortBySizeR();
                    lastSortType = SortType.SizeR;
                }
                sortCommon(true);
            }
        });

        rgEncoding = (RadioGroup)findViewById(R.id.rg_encoding);
        rgEncoding.check(R.id.btn_auto);

        prepareParams();
    }

    void sortCommon(boolean save){
        if (lastSortType == SortType.Name){
            fileListAdapter.sortByName();
        } else
        if (lastSortType == SortType.NameR){
            fileListAdapter.sortByNameR();
        } else
        if (lastSortType == SortType.Date){
            fileListAdapter.sortByDate();
        } else
        if (lastSortType == SortType.DateR){
            fileListAdapter.sortByDateR();
        } else
        if (lastSortType == SortType.Read){
            fileListAdapter.sortByRead();
        } else
        if (lastSortType == SortType.ReadR){
            fileListAdapter.sortByReadR();
        } else
        if (lastSortType == SortType.Size){
            fileListAdapter.sortBySize();
        } else
        if (lastSortType == SortType.SizeR){
            fileListAdapter.sortBySizeR();
        }

        fileListAdapter.notifyDataSetChanged();

        if (save) {
            SharedPreferences.Editor edit = pref.edit();
            edit.putInt("sortType", lastSortType.ordinal());
            edit.commit();
        }
    }

    @Override
    protected void onResume(){
        super.onResume();
        Utility.requestPermision(this);
        if (checkStartSelectFile()){
            startSelectFile();
        }
    }

    @Override
    protected void onStop(){
        super.onStop();
        saveParameters();
    }

    protected void prepareParams(){
        Intent i = getIntent();
        String initialDir = i.getStringExtra(pfs.INITIALDIR);
        if (Utility.isNotEmpty(initialDir))
            setFileDirectory(initialDir);
        else
            setFileDirectory("/");

        lastSortType = SortType.values()[pref.getInt("sortType", SortType.Name.ordinal())];
    }

    protected void saveParameters(){
    }

    protected boolean checkStartSelectFile(){
        return true;
    }

    TextInputDialog pathDialog;
    protected void startSelectFile(){
        Log.d("PDD", "fileDirectory = "+m_fileDirectory.getPath());

        List<FileInfo> listFileInfo = getListFileInfo(m_fileDirectory.getAbsolutePath());
        if (listFileInfo == null || listFileInfo.size() <= 1){
            String rootStr = "/";
            //Note: rootStr == m_fileDirectory.getAbsolutePath()では正常に動かない
            if (rootStr.compareTo(m_fileDirectory.getAbsolutePath())==0
                    || Utility.initialFileDirectory().compareTo(m_fileDirectory.getAbsolutePath())==0){
                pathDialog = new TextInputDialog();
                pathDialog.titleText = getString(R.string.title_enter_path);
                pathDialog.setText(m_fileDirectory.getPath());
                pathDialog.setCallback(new TextInputCallback() {
                    @Override
                    public void onTextInputClickOk() {
                        m_fileDirectory = new FileInfo(pathDialog.getText());
                        pathDialog.dismiss();
                        pathDialog = null;
                        startSelectFile();
                    }
                    @Override
                    public void onTextInputClickCancel() {
                        pathDialog.dismiss();
                        pathDialog = null;
                        finish();
                    }
                });
                pathDialog.show(getFragmentManager(), "test");    //TODO: what is the second argument?
                return;
            } else {
                return; // error or pending
            }
        }

        showPost(m_fileDirectory, listFileInfo);
    }

    // Cancel Listener //
    //TODO: cancel要らないかも？
    interface OnCancelListener {
        void onCancel();
    }
    FileSelectionDialog.OnCancelListener onCancelListener;
    public void setOnCancelListener(FileSelectionDialog.OnCancelListener listener){
        onCancelListener = listener;
    }
    // 呼び出し側にcancelをさせる
    protected void cancel(){
        if (onCancelListener!=null) onCancelListener.onCancel();
        finish();
    }


    void show(File file, final String[] exts){
        show(new FileInfo(file.getName(), file), exts);
    }

    void show( FileInfo fileDirectory, final String[] exts )
    {
        m_fileDirectory = fileDirectory;
        m_exts = exts;
        startSelectFile();
    }

    protected List<FileInfo> getListFileInfo(String fileDirectory){
        File file = new File(fileDirectory);
        File[] aFile = file.listFiles();
        List<FileInfo> listFileInfo = new ArrayList<FileInfo>();
        if( null != aFile ){
            String altAudioFolder = Utility.altAudioFolder(pref);
            for( File fileTemp : aFile ){
                boolean mp3Exists = false;
                if (m_exts!=null && m_exts.length!=0 && !fileTemp.isDirectory()){
                    String lcName = fileTemp.getName().toLowerCase();
                    boolean found = false;
                    for ( String ext : m_exts ){
                        if (lcName.endsWith(ext)){
                            found = true;
                            break;
                        }
                    }
                    if (!found) continue;

                    // 音声ファイルがあるか？
                    mp3Exists = Utility.mp3Exists(fileTemp.getName(), altAudioFolder);
                }
                listFileInfo.add( new FileInfo( fileTemp.getName(), fileTemp, mp3Exists ) );
            }
            Collections.sort(listFileInfo);
        }
        return listFileInfo;
    }

    @Override   // FileListListener
    public void onFileListReady(List<FileInfo> listFile)
    {
        // file listが準備できた
        showPost(m_fileDirectory, listFile);
    }

    protected void showPost(FileInfo fileDirectory, List<FileInfo> listFileInfo){
        ListView listview = (ListView) findViewById(R.id.fileList);
        //listview.setScrollingCacheEnabled(false);
        //listview.setOnItemClickListener( this );
        //fileListAdapter = new FileInfoArrayAdapter(this, listFileInfo);
        fileListAdapter = new FileListAdapter(this, R.layout.list_item_filelist, listFileInfo);
        listview.setAdapter( fileListAdapter );
        sortCommon(false);
    }

    protected void onFileItemClick(FileInfo fileinfo) {
        if( true == fileinfo.isDirectory() ){
            if (fileinfo.getName().equals("..")) {
                File file = fileinfo.getFile();
                if (file == null) {
                    file = new File(fileinfo.getParent());
                }
                show(file, m_exts);
            } else {
                show(fileinfo, m_exts);
            }
        } else {
            onFileSelect( fileinfo );
        }
    }

    protected void onFileSelect(FileInfo fileInfo){
        Intent i = new Intent();
        setupReturnValues(fileInfo.getPath(), i);
        setResult(Activity.RESULT_OK, i);
        finish();
    }

    @CallSuper
    protected void setupReturnValues(String filename, Intent i){
        i.putExtra("filename", filename);

        int id = rgEncoding.getCheckedRadioButtonId();
        String encoding;
        switch (id){
            case R.id.btn_shiftjis: encoding = "ShiftJIS"; break;
            case R.id.btn_utf8: encoding = "utf-8"; break;
            case R.id.btn_utf16: encoding = "utf-16"; break;
            default:
                return; // return!!
        }
        i.putExtra("fileEncoding", encoding);
    }
}
