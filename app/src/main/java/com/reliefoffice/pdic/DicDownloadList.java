package com.reliefoffice.pdic;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.os.AsyncTask;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.util.Xml;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import org.xmlpull.v1.XmlPullParser;

import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;


public class DicDownloadList extends AppCompatActivity implements IAsyncFileDownloadNotify {

    ListView lvDicList;
    ArrayAdapter<DicInfo> adpDicList;
    File dwnFile;
    ArrayList<DicInfo> dicInfoList;
    DicInfo dwnDicInfo;
    boolean dicDownMode;

    public ArrayList<DicInfo> addedDicList = new ArrayList<DicInfo>();
    SharedPreferences pref;
    DicPref dicPref;

    class DicListAdapter extends ArrayAdapter<DicInfo> {
        private LayoutInflater layoutInflater;
        public DicListAdapter(Context context) {
            super(context, 0);
            layoutInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            DicInfo item = (DicInfo)getItem(position);

            if (null == convertView) {
                convertView = layoutInflater.inflate(R.layout.list_item_diclist, null);
            }

            TextView textView;
            // name //
            textView = (TextView)convertView.findViewById(R.id.diclist_name);
            textView.setText(item.name);
            // text //
            textView = (TextView)convertView.findViewById(R.id.diclist_text);
            textView.setText(
                    getString(R.string.label_size) + " : " + Utility.itocs(item.fileSize) + " Bytes\n"
                  + getString(R.string.label_count)+" : " + Utility.itocs(item.downloadCount)
            );
            // URL //
            textView = (TextView)convertView.findViewById(R.id.diclist_url);
            textView.setText(item.HPUrl);

            return convertView;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_dic_download_list);

        pref = PreferenceManager.getDefaultSharedPreferences(this);
        dicPref = new DicPref(pref);

        lvDicList = (ListView) findViewById(R.id.dicDownloadList);
        adpDicList = new DicListAdapter(this);
        lvDicList.setAdapter(adpDicList);

        lvDicList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                dicDownMode = true;
                dwnDicInfo = dicInfoList.get(position);
                startFileDownload(dwnDicInfo.filename, dwnDicInfo.DLUrl);
            }
        });

        dicDownMode = false;
    }

    @Override
    protected void onResume() {
        super.onResume();

        if (!Utility.requestInternetPermision(this)){
            return;
        }

        startFileDownload("list.xml", getString(R.string.dic_list_url));
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == Utility.REQUEST_CODE_PERMISSION){
            if (!Utility.permissionGranted(grantResults)){
                finish();
            }
            return;
        }
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }

    void updateList(){
        adpDicList.clear();
        if (dwnFile==null){ return; }

        dicInfoList = analyzeXmlDicList(dwnFile);

        for (int i=0;i<dicInfoList.size();i++){
            DicInfo dicInfo = dicInfoList.get(i);
            adpDicList.add(dicInfo);
        }
    }

    ArrayList<DicInfo> analyzeXmlDicList(File file){
        ArrayList<DicInfo> dicInfoList = new ArrayList<DicInfo>();

        // analyze the downloaded list file
        try {
            InputStream is = new FileInputStream(file);
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(is, "UTF-8");
            int eventType;
            boolean inDictionary = false;

            DicInfo dicInfo = null;

            while ((eventType=parser.next())!=XmlPullParser.END_DOCUMENT){
                String name = parser.getName();
                if (eventType==XmlPullParser.START_TAG) {
                    if (name != null) {
                        if (name.equals("dictionary")){
                            inDictionary = true;
                            dicInfo = new DicInfo();
                        } else
                        if (inDictionary) {
                            String text = parser.nextText();
                            if (name.equals("upgradekey")) {
                                dicInfo.upgradeKey = text;
                            } else if (name.equals("name")) {
                                dicInfo.name = text;
                            } else if (name.equals("version")) {
                                dicInfo.version = text;
                            } else if (name.equals(("date"))) {
                                dicInfo.date = text;
                            } else if (name.equals("filename")) {
                                dicInfo.filename = text;
                            } else if (name.equals("filesize")) {
                                dicInfo.fileSize = Integer.parseInt(text);
                            } else if (name.equals("description")) {
                                dicInfo.descriptoin = text;
                            } else if (name.equals("hp_url")) {
                                dicInfo.HPUrl = text;
                            } else if (name.equals("dl_url")) {
                                dicInfo.DLUrl = text;
                            } else if (name.equals("dl_count")) {
                                dicInfo.downloadCount = Integer.parseInt(text);
                            }
                        }
                    }
                } else
                if (eventType==XmlPullParser.END_TAG) {
                    if (name.equals("dictionary")){
                        if (inDictionary) {
                            inDictionary = false;
                            dicInfoList.add(dicInfo);
                        }
                    }
                }
            }
        } catch (IOException e){
            e.printStackTrace();
        } catch (Exception e){
            e.printStackTrace();
        }
        return dicInfoList;
    }

    private String loadFile(String filename){
        AssetManager asset = getAssets();
        InputStream is = null;
        String text = "";
        try {
            is = asset.open(filename);
            text = getStreamText(is);
        } catch (IOException e){
            e.printStackTrace();
        }
        return text;
    }

    private String getStreamText(InputStream is) throws IOException {
        ByteArrayOutputStream bs = new ByteArrayOutputStream();
        byte[] bytes = new byte[4096];
        int len;
        while ((len=is.read(bytes))>0){
            bs.write(bytes, 0, len);
        }
        return new String(bs.toByteArray(), "UTF8");
    }

    void expandDic(){
        Log.d("PDP", "downloaded:" + dwnFile.getAbsolutePath());
        try {
            ZipInputStream zip = new ZipInputStream(new FileInputStream(dwnFile.getAbsolutePath()));
            ZipEntry entry;
            while ((entry=zip.getNextEntry())!=null){
                String name = entry.getName().trim();
                Log.d("PDP", "unzipping "+name);
                File file = new File(name);
                int expPos = file.getName().lastIndexOf(".");
                if (expPos==-1) continue;
                String ext = file.getName().substring(expPos+1);
                Log.d("PDP", "ext="+ext);
                if (ext.equalsIgnoreCase("dic")){
                    Log.d("PDP", "install dictionary");
                    // install dictionary
                    //TODO: overwrite ok?
                    File outFile = new File(getFilesDir(), name);
                    BufferedOutputStream out = new BufferedOutputStream(new FileOutputStream(outFile));
                    byte[] buffer = new byte[1024];
                    int len;
                    while ((len = zip.read(buffer)) != -1){
                        out.write(buffer, 0, len);
                    }
                    zip.closeEntry();
                    out.close();
                    DicInfo info = dwnDicInfo;
                    info.filename = outFile.getAbsolutePath();
                    addedDicList.add(info);
                }
            }
            finish();
        } catch (FileNotFoundException e){
            e.printStackTrace();
        } catch (IOException e){
            e.printStackTrace();
        }
    }

    @Override
    protected void onPause(){
        super.onPause();
        cancelLoad();
        //Log.d("PDP", "onPause");
        //ArrayList<String> path = dicPref.getDictionaryPath();
        //ArrayList<String> name = dicPref.getDictionaryName();
        int num = dicPref.getNum();
        for (int i=0;i<addedDicList.size();i++){
            //Log.i("PDP", "name:"+addedDicList.get(i).name+" path:"+addedDicList.get(i).filename);
            dicPref.saveDicInfo(num, addedDicList.get(i));
            num++;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (dwnFile!=null){
            dwnFile.delete();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_dic_download_list, menu);
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

    AsyncFileDownload asyncFileDownload;
    ProgressHandler progressHandler;

    void startFileDownload(String filename, String url){
        // dictionary list download //
        progressHandler = new ProgressHandler();
        File dicDir = DicUtils.getDictionaryPath(this);
        if (!dicDir.exists()){
            if (!dicDir.mkdir()){
                Toast ts = Toast.makeText(this, getString(R.string.failed_to_make_dir)+dicDir, Toast.LENGTH_LONG);
                ts.show();
                return;
            }
        }
        dwnFile = new File(dicDir, filename);
        asyncFileDownload = new AsyncFileDownload(this, url, dwnFile);
        asyncFileDownload.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);

        createProgressDialog();
        progressHandler.progressDialog = progress;
        progressHandler.asyncfiledownload = asyncFileDownload;
        if (progress != null && asyncFileDownload != null){
            progress.setProgress(0);
            progressHandler.sendEmptyMessage(0);
        }else{
            Toast ts = Toast.makeText(this, "NULL error", Toast.LENGTH_LONG);
            ts.show();
        }
    }

    ProgressDialog progress;
    void createProgressDialog(){
        progress = new ProgressDialog(this);
        progress.setTitle(getString(R.string.title_downloading_file));
        progress.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        progress.setButton(DialogInterface.BUTTON_POSITIVE, "Hide",
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                    }
                });

        progress.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel",
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        cancelLoad();
                    }
                });
        progress.show();
    }
    void deleteProgressDialog(){
        if (progress == null) return;
        progress.dismiss();
        progress = null;
    }

    private void cancelLoad()
    {
        if(asyncFileDownload != null){
            asyncFileDownload.cancel(true);
        }
        deleteProgressDialog();
    }

    @Override
    public void finished(boolean result, String errMsg) {
        Log.d("PDP", "finished:" + result);
        if (asyncFileDownload.isScceeded()) {
            if (dicDownMode) {
                expandDic();
            } else {
                updateList();
            }
            dwnFile.delete();
        }
        if (Utility.isNotEmpty(errMsg)){
            Toast.makeText(this, errMsg, Toast.LENGTH_LONG).show();
            Utility.initializeSSLContext(this);
        }
    }
}
