package com.reliefoffice.pdic;

import android.app.Activity;
import android.content.Intent;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.PopupMenu;

import java.io.File;


public class PSBookmarkActivity extends ActionBarActivity {
    String filename;    // File selection list if filename is null.
                        //Note: psbm filename

    PdicJni pdicJni;
    INetDriveFileManager ndvFM;
    PSBookmarkFileManager psbmFM;

    ListView psbList;
    private PSBookmarkListAdapter psbListAdapter;
    int lastSel;
    boolean savedResult;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_psbookmark);

        Intent i = getIntent();
        filename = i.getStringExtra("filename");

        if (Utility.isEmpty(filename)) {
            setTitle(getString(R.string.title_activity_loadfile_history));
        }

        File tempPath = getExternalFilesDir(null);
        pdicJni = PdicJni.createInstance(getAssets(), tempPath.getAbsolutePath());
        ndvFM = DropboxFileManager.createInstance(this);
        psbmFM = PSBookmarkFileManager.createInstance(this, ndvFM);

        // Create ListView for bookmark list
        psbListAdapter = new PSBookmarkListAdapter(this, R.layout.list_item_psbookmarklist);
        psbList = (ListView) findViewById(R.id.psbookmarkList);
        psbList.setAdapter(psbListAdapter);
        psbList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View view, int position, long l) {
                exit(position);
            }
        });
        psbList.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            @Override
            public boolean onItemLongClick(AdapterView<?> adapterView, View view, int position, long l) {
                if (Utility.isNotEmpty(filename)) {
                    lastSel = position;
                    contextMenu(view);
                }
                return true;
            }
        });

        loadPSBookmark(filename);
    }

    void exit(int position){
        Intent i = new Intent();
        PSBookmarkItem item = (PSBookmarkItem)psbListAdapter.getItem(position);
        if (Utility.isNotEmpty(item.filename)){
            INetDriveUtils ndvUtils = DropboxUtils.getInstance(this);
            if (ndvUtils.hasPrefix(item.filename)){
                i.putExtra("filename", ndvUtils.convertToLocalName(item.filename));
                i.putExtra("remotename", item.filename.substring(4));
            } else {
                i.putExtra("filename", item.filename);
            }
        } else {
            i.putExtra("start", item.position);
            i.putExtra("length", item.length);
        }
        setResult(Activity.RESULT_OK, i);
        savedResult = true;
        finish();
    }

    boolean PSBookmarkReady;
    void loadPSBookmark(final String filename){
        if (!PSBookmarkReady) {
            if (psbmFM.open()) {
                PSBookmarkReady = true;
            } else {
                //TODO: error handling
                return;
            }
        }
        IPSBookmarkListAdapter ipsBookmarkListAdapter = new IPSBookmarkListAdapter() {
            @Override
            public int addItem(int position, int length, int style, int color, String markedWord, String comment) {
                if (color!=0) color |= 0xFF000000;    //TODO: alpha channel support
                psbListAdapter.add(new PSBookmarkItem(null, position, length, style, color, markedWord, comment));
                return 0;
            }

            @Override
            public int addFile(String filename) {
                psbListAdapter.add(new PSBookmarkItem(filename));
                return 0;
            }
        };
        JniCallback callback = JniCallback.createInstance();
        callback.setPSBookmarkListAdapter(ipsBookmarkListAdapter);
        if (Utility.isEmpty(filename)) {
            pdicJni.loadPSBookmarkFiles();
        } else {
            pdicJni.loadPSBookmark(filename);
        }
        callback.setPSBookmarkListAdapter(null);
        callback.deleteInstance();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_psbookmark, menu);
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

    void contextMenu(final View view){
        // PopupMenu : refer to http://techbooster.org/android/ui/3056/
        PopupMenu popup = new PopupMenu(this, view);
        popup.getMenuInflater().inflate(R.menu.psb_context_menu, popup.getMenu());
        popup.show();
        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
            public boolean onMenuItemClick(MenuItem item) {
                if (item.getItemId() == R.id.editPSBookmark) {
                    editPSBookmark(lastSel, view);
                } else if (item.getItemId() == R.id.removeItem) {
                    removeItem(lastSel);
                }
                return true;
            }
        });
    }

    PSBookmarkEditWindow psbEditWindow;
    void editPSBookmark(final int index, View itemView){
        if (!PSBookmarkReady) return;
        PSBookmarkItem item = psbListAdapter.getItem(index);
        psbEditWindow = new PSBookmarkEditWindow(this, item, filename){
            @Override
            void notifyChanged(PSBookmarkItem item){
                PSBookmarkItem i = psbListAdapter.getItem(index);
                i.style = item.style;
                i.color = item.color;
                i.comment = item.comment;
                psbListAdapter.notifyDataSetChanged();
            }
        };
        psbEditWindow.show(itemView);
    }
    void removeItem(int index){
        if (!PSBookmarkReady) return;
        PSBookmarkItem item = psbListAdapter.getItem(index);
        // delete from file
        if (pdicJni.deletePSBookmark(filename, item.position)) {
            // succeeded
            psbListAdapter.remove(item);
        } else {
            Log.w("PDD", "Failed to delete PSBookmark: " + filename + " " + item.position);
        }
    }

    @Override
    protected void onResume(){
        super.onResume();
        if (PSBookmarkReady){
            psbmFM.open();
        }
    }

    @Override
    protected void onPause(){
        super.onPause();
        if (PSBookmarkReady){
            psbmFM.close();
        }
    }

    @Override
    protected void onDestroy(){
        super.onDestroy();
        if (PSBookmarkReady){
            PSBookmarkReady = false;
            psbmFM.close();
        }
        if (psbmFM != null)
            psbmFM.deleteInstance();
        if (pdicJni != null)
            pdicJni.deleteInstance();
    }
}
