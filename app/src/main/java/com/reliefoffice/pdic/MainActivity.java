package com.reliefoffice.pdic;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.support.v4.app.Fragment;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.ListView;
import android.widget.SearchView;
import android.widget.Toast;

import com.reliefoffice.pdic.text.config;
import com.reliefoffice.pdic.text.pfs;

public class MainActivity extends AppCompatActivity implements SearchView.OnQueryTextListener {
    static final String PFS_RUNNING = "Running";

    private WordListAdapter wordListAdapter;

    PdicJni pdicJni;
    private JniCallback jniCallback;

    boolean prevRunning = false;
    boolean openPending = false;

    final Handler handler = new Handler();

    SharedPreferences pref;

    // NetDrive //
    INetDriveFileManager ndvFM;

    DictionaryManager dicMan;
    boolean dicOpened = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        if (savedInstanceState == null) {
//            getSupportFragmentManager().beginTransaction()
  //                  .add(R.id.container, new PlaceholderFragment())
    //                .commit();
        }

        // Create ListView for word list
        wordListAdapter = new WordListAdapter(this, R.layout.list_item_wordlist);

        ListView wordList = (ListView) findViewById(R.id.wordList);
        wordList.setAdapter(wordListAdapter);
        //wordListAdapter.notifyDataSetChanged();
        wordList.setOnScrollListener(new AbsListView.OnScrollListener() {
            boolean scrolling = false;
            int lastFirstVisibleItem = -1;
            int lastUpdateCounter = -1;
            @Override
            public void onScrollStateChanged(AbsListView absListView, int i) {
                //Log.i("PDD", "StateChanged:i="+i);
                if (i==SCROLL_STATE_TOUCH_SCROLL) {
                    scrolling = true;
                } else
                if (i==SCROLL_STATE_IDLE){
                    scrolling = false;
                }
            }

            @Override
            public void onScroll(AbsListView absListView, int firstVisibleItem, int visibleCount, int totalCount) {
                //Log.i("PDD", "Scroll: "+firstVisibleItem+" "+visibleCount+" "+totalCount);
                //TODO: consideration - We don't need these compelx processes if we can judge wheather foroward or backward scroll easily. (don't want to use lastFirstVisibleItem)
                if (jniCallback==null){
                    return;
                }
                boolean updating = jniCallback.isUpdating();
                if (jniCallback.getUpdateCounter()!=lastUpdateCounter){
                    lastUpdateCounter = jniCallback.getUpdateCounter();
                    updating = true;
                }
                if (updating) {
                    lastFirstVisibleItem = -1;
                    return;
                }

                if (lastFirstVisibleItem!=firstVisibleItem) {
                    //Log.d("PDD", "Scroll: "+firstVisibleItem+"/"+lastFirstVisibleItem+" "+visibleCount+" "+totalCount);
                    int backward;
                    int reqoffs;
                    if (lastFirstVisibleItem==-1){
                        // we cannot find forward or backward.
                        backward = -1;
                        reqoffs = 0;
                    } else
                    if (lastFirstVisibleItem < firstVisibleItem) {
                        // scroll up
                        reqoffs = firstVisibleItem + visibleCount;
                        backward = 0;
                    } else {
                        // scroll down
                        reqoffs = firstVisibleItem;
                        backward = 1;
                    }
                    lastFirstVisibleItem = firstVisibleItem;
                    //if (backward!=0) return;    // for debug
                    if (backward!=-1) {
                        if (pdicJni.requestScroll(reqoffs, backward, firstVisibleItem) > 0) {
                            Log.i("PDD", "scroll requested");
                            requestIdleProc();
                        }
                    }
                }
            }
        });

        // Initialize JNI.
        pdicJni = PdicJni.createInstance(this, getAssets());        // Create JNI callback

		jniCallback = JniCallback.createInstance();
        jniCallback.setWordListAdapter(wordList, wordListAdapter);

        int res = -1;
        if (pdicJni != null) {
            res = pdicJni.createFrame(jniCallback, 0);
        }

        // Setup Search View
        SearchView search = (SearchView) findViewById(R.id.searchView1);
        search.setIconifiedByDefault(false);
        search.setOnQueryTextListener(this);
        search.setSubmitButtonEnabled(true);
        search.setQueryHint(getString(R.string.msg_input_word));

        if (res != 0) {
            search.setQueryHint("JNI init/createFrame: res=" + Integer.toString(res));
        }
        //Log.i("PDD", "onCreate");

        pref = PreferenceManager.getDefaultSharedPreferences(this);
        prevRunning = pref.getBoolean(PFS_RUNNING, false);
        SharedPreferences.Editor edit = pref.edit();
        edit.putBoolean(PFS_RUNNING, true);
        edit.commit();

        ndvFM = DropboxFileManager.createInstance(this);

        dicMan = DictionaryManager.createInstance(this);
    }

    /*
    @Override
    protected void onStart() {
        super.onStart();
        Log.i("PDD", "onStart");
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        Log.i("PDD", "onRestart");
    }*/

    @Override
    protected void onResume() {
        super.onResume();

        //Log.i("PDD", "Main.onResume");
        jniCallback.setWordList(null);
        if (!dicMan.isDicOpened()) {
            if (prevRunning) {
                prevRunning = false;
                showQueryOpen();
            }
            if (!openPending) {
                openDictionary();
            }
        }

        // Setup JNI config //
        int viewFlags = 0xFFFF;
        boolean showPronExp = pref.getBoolean(pfs.SHOW_PRONEXP, config.defaultShowPronExp);
        if (!showPronExp)
            viewFlags &= ~(PdicJni.VF_PRON | PdicJni.VF_EXP);
        pdicJni.config(viewFlags);
    }

    void openDictionary(){
        if (dicOpened) return;

        int num = dicMan.getDictionaryNum();
        if (num==0){
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setMessage(getString(R.string.msg_nodictionary))
                    .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            actionDictionarySetting();
                        }
                    });
            builder.show();
            return;
        }

        int nindex = dicMan.openDictionary();
        if (dicMan.isDicOpened()) dicOpened = true;
        else {
            String name = dicMan.getDictionaryPath(-nindex-1);
            Toast ts = Toast.makeText(this, getString(R.string.msg_dic_not_opened) + "\n" + name, Toast.LENGTH_LONG);
            ts.show();
        }
    }

    void closeDictionary(){
        if (dicOpened) {
            dicMan.closeDictionary();
            dicOpened = false;
        }
    }

    void showQueryOpen(){
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(this);
        DicPref dicPref = new DicPref(pref);
        if (dicPref.getNum()==0) return;    // no dictionary

        openPending = true;
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);
        alertDialogBuilder.setTitle("Open Dictionary");
        alertDialogBuilder.setMessage( getString(R.string.msg_open_dictionary_query));
        alertDialogBuilder.setPositiveButton("Yes",
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        openPending = false;
                        openDictionary();
                    }
                });
        alertDialogBuilder.setNegativeButton("No",
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        openPending = false;
                    }
                });
        alertDialogBuilder.setCancelable(true);
        AlertDialog alertDialog = alertDialogBuilder.create();
        alertDialog.show();
    }

    // Idle Proc //
    void requestIdleProc(){
        //Log.d("PDD", "requestIdleProc");
        handler.postDelayed(idleProc, 0);
    }

    private final Runnable idleProc = new Runnable() {
        @Override
        public void run() {
            final int maxLoop = 100;
            for (int i=0;i<maxLoop;i++){
                if (pdicJni.idleProc(0)==1) {
                    // end of idle
                    //Log.d("PDD", "End idle ");
                    jniCallback.requestUpdate();
                    if (i>0) {
                        handler.postDelayed(idleProc, 1000);
                    }
                    return;
                }
            }
            requestIdleProc();  // continue to run idle proc
        }
    };

    @Override
    protected void onPause() {
        super.onPause();
        Log.i("PDD", "onPause");
    }

    @Override
    protected void onStop() {
        super.onStop();
        //Log.i("PDD", "onStop");

        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(this);
        SharedPreferences.Editor edit = pref.edit();
        edit.putBoolean(PFS_RUNNING, false);
        edit.commit();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d("PDD", "Main.onDestroy");

        closeDictionary();

        if (!dicMan.isDicOpened())
            ndvFM.removeAll();
        pdicJni.deleteFrame();
        if (jniCallback != null) {
            jniCallback.deleteInstance();
            jniCallback = null;
        }
        if (pdicJni != null){
            pdicJni.deleteInstance();
            pdicJni = null;
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id==R.id.action_pswin){
            startActivity(new Intent().setClassName(this.getPackageName(), PSWinActivity.class.getName()));
        } else
        if (id==R.id.action_dictionary){
            actionDictionarySetting();
        } else
        if (id == R.id.action_settings) {
            startActivity(new Intent().setClassName(this.getPackageName(), SettingsActivity2.class.getName()));
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    void actionDictionarySetting(){
        startActivity(new Intent().setClassName(this.getPackageName(), DicSettingActivity.class.getName()));
    }

    @Override
    public boolean onQueryTextSubmit(String query) {
        return false;
    }

    @Override
    public boolean onQueryTextChange(String newText) {
        if (TextUtils.isEmpty(newText)) {
            //Log.d("PDD", "empty");
            pdicJni.incSearch("");
            wordListAdapter.clear();
            wordListAdapter.notifyDataSetChanged();
        } else {
            //Log.d("PDD", "call incSearch1");
            //wordListAdapter.clear();
            //wordListAdapter.notifyDataSetChanged();
            pdicJni.incSearch("");
            wordListAdapter.clear();
            pdicJni.incSearch(newText);
        }
        //Log.d("PDD", "Start first idle");
        final int firstIdleCount = 30;  //TODO: need to adjust
        boolean done = false;
        for (int i = 0; i < firstIdleCount; i++) {
            if (pdicJni.idleProc(0) != 0) {
                Log.d("PDD", "End first idle");
                jniCallback.requestUpdate();
                done = true;
                break;
            }
        }
        if (!done){
            //Log.i("PDD", "Not done - ");
            requestIdleProc();
        }
        return true;
    }

    /**
     * A placeholder fragment containing a simple view.
     */
    public static class PlaceholderFragment extends Fragment {

        public PlaceholderFragment() {
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                                 Bundle savedInstanceState) {
            View rootView = inflater.inflate(R.layout.fragment_main, container, false);

            return rootView;
        }
    }
}
