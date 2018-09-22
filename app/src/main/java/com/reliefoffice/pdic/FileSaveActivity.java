package com.reliefoffice.pdic;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;


public class FileSaveActivity extends ActionBarActivity implements FileSelectionDialog.OnFileSelectListener {

    // Preferences
    static final String PFS_PSINITIALDIR = "PSInitialDir";
    SharedPreferences pref;

    private String m_strInitialDir;
    private String filename = "";

    // Dropbox members //
    INetDriveUtils ndvUtils;

    // UI component //
    EditText edFilename;
    TextView tvFolderPath;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_file_save);

        pref = PreferenceManager.getDefaultSharedPreferences(this);

        m_strInitialDir = pref.getString(PFS_PSINITIALDIR, Utility.initialFileDirectory());

        // Dropbox //
        ndvUtils = DropboxUtils.getInstance(this);

        Button btnSelectFolder = (Button)findViewById(R.id.select_folder_button);
        btnSelectFolder.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                selectFolder();
            }
        });

        Button btnSave = (Button)findViewById(R.id.save_file_button);
        btnSave.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                saveFile();
            }
        });

        edFilename = (EditText)findViewById(R.id.filename_edit);
        tvFolderPath = (TextView)findViewById(R.id.folder_text);

        tvFolderPath.setText(m_strInitialDir);
        //edFilename.setText()
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_file_save, menu);
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

    boolean fromDropbox = false;
    void selectFolder(){
        FileSelectionDialog dlg = new FileSelectionDialog(this, this, true);
        String[] exts = {".txt"};
        dlg.show(new File(m_strInitialDir), exts);
        fromDropbox = false;
    }

    void saveFile(){
        Intent i = new Intent();
        i.putExtra("path", m_strInitialDir);
        i.putExtra("filename", edFilename.getText().toString());
        setResult(Activity.RESULT_OK, i);
        finish();
    }

    // FileSelectionDialog return
    // file.getFile()!=null → ファイルが選択されている
    // file.getFile()==null → direcotoryの選択
    @Override
    public void onFileSelect(FileInfo file) {
        if (fromDropbox) {
            ndvUtils.setInitialDir(file.getParent());

            String toFileName = ndvUtils.convertToLocalName(file.getAbsolutePath());
            File toFile = new File(toFileName);
            File parent = toFile.getParentFile();
            if (!parent.exists()) {
                if (!parent.mkdirs()) {
                    Toast.makeText(this, "Failed to mkdirs : " + parent.getAbsolutePath(), Toast.LENGTH_SHORT).show();
                    Log.w("PDD", "Failed to mkdirs: " + parent.getAbsolutePath());
                }
            }

            executeUploadFile(file.getAbsolutePath(), toFile);
        } else {
            m_strInitialDir = file.getParent();
            SharedPreferences.Editor edit = pref.edit();
            edit.putString(PFS_PSINITIALDIR, m_strInitialDir);
            edit.commit();

            tvFolderPath.setText(m_strInitialDir);

            File _file = file.getFile();
            if (_file != null){
                filename = _file.getName();
            } else {
                filename = "";
            }
            edFilename.setText(filename);
        }
    }

    // Dropbox //
    void executeUploadFile(String from, File to) {

    }
}
