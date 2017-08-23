package com.reliefoffice.pdic;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.graphics.Point;
import android.media.MediaPlayer;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.support.v7.app.ActionBarActivity;
import android.support.v4.app.Fragment;
import android.os.Bundle;
import android.content.ClipboardManager;
import android.text.Spannable;
import android.util.Log;
import android.view.ActionMode;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import com.reliefoffice.pdic.text.config;
import com.reliefoffice.pdic.text.pfs;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static java.lang.Math.abs;

//NOTE: popupList用のAdapterクラスは下の方に作ってある。必要な場合はそれを復活

public class PSWinActivity extends ActionBarActivity implements FileSelectionDialog.OnFileSelectListener, TextLoadTask.OnFileLoadListener, SaveFileTask.SaveFileTaskDone, GotoDialog.Listener, SeekBar.OnSeekBarChangeListener {
    final static boolean useTextLoadTask = true;
    final static boolean usePSBMforFileLoad = false;

    static PSWinActivity This;
    String orgTitle;
    String openedFilename;
    String remoteFilename;
    String psbmFilename;    // filename for PSBookmark

    //int remoteRevision;

    // swipe for editText
    class SwipeMove {
        int MOVE_MARGIN_X = 80;
        int MOVE_MARGIN_Y = 10;
        private  Point size = new Point();
        public LinearLayout layout;
        private int marginLeft = 0;
        private int startX = 0;
        private int startY = 0;
        private int delta = 0;
        private boolean movingX = false;
        private boolean movingY = false;
        SwipeMove(LinearLayout layout, int moveMarginX){
            this.layout = layout;
            MOVE_MARGIN_X = moveMarginX;
            WindowManager wm = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
            Display disp = wm.getDefaultDisplay();
            disp.getSize(size);
        }
        void start(MotionEvent e){
            startX = (int)e.getRawX();
            startY = (int)e.getRawY();
            delta = 0;
            movingX = false;
            movingY = false;
        }
        void move(MotionEvent e){
            {
                final int dY = (int) e.getRawY() - startY;
                if (!movingY) {
                    if ((dY >= 0 && dY < MOVE_MARGIN_Y) || (dY<0 && dY > -MOVE_MARGIN_Y)) {
                    } else {
                        movingY = true;
                        showPopupList(false);
                    }
                }

                final int d = (int) e.getRawX() - startX;
                if (marginLeft==0 && !movingX) {
                    if ((d >= 0 && d < MOVE_MARGIN_X) || (d<0 && d > -MOVE_MARGIN_X)) {   //TODO: 絶対値指定で大丈夫か？
                        return;
                    }
                    if (abs(dY) > abs(d)){
                        // Y方向の移動のほうが大きいときは対象外
                        return;
                    }
                }
                delta = d;
                movingX = true;
            }
            setLayout();
        }
        void end(MotionEvent e){
            marginLeft += delta;
            movingX = false;
            movingY = false;
            if ((marginLeft >= 0 && marginLeft < MOVE_MARGIN_X) || (marginLeft < 0 && marginLeft > -MOVE_MARGIN_X)){
                marginLeft = 0;
                delta = 0;
                setLayout();
            }
        }
        void setLayout(){
            final int scale = 1;    //TODO:
            LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                    (int) (size.x * scale),
                    LinearLayout.LayoutParams.WRAP_CONTENT  // important!!
            );
            //Log.d("PDD", "marginLeft="+marginLeft+" delta="+delta);
            params.setMargins(marginLeft+delta, 0, 0, 0);
            layout.setLayoutParams(params);
        }
    }
    private SwipeMove swipeMove;

    private EditText editText;
    private int savedEditTextHeight = -1;
    //private TextView popupText; // view by text
    private ListView popupList; // view by list view
    private ArrayAdapter<WordItem> popupListAdapter;
    private PdicJni pdicJni;
    private TWordListAdapter wordListAdapter;
    private JniCallback jniCallback;

    String fileEncoding;

    private DictionaryManager dicMan;

    PSBookmarkFileManager psbmFM;
    private boolean PSBookmarkReady = false;
    SharedPreferences pref;

    // Dropbox members //
    INetDriveUtils ndvUtils;
    INetDriveFileManager ndvFM;

    // for touch operation //
    float lastX, lastY;
    boolean moved;
    float startX;
    float startY;
    final int marginForMove = 16;

    //TODO: call setWordList() when deactivated.

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        This = this;
        //Log.i("PDD", "PSWinActivity.onCreate");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_pswin);
        if (savedInstanceState == null) {
            getSupportFragmentManager().beginTransaction()
                    .add(R.id.container, new PlaceholderFragment())
                    .commit();
        }
        pref = PreferenceManager.getDefaultSharedPreferences(this);

        // swipeによるEditText移動 //
        LinearLayout layoutEdit = (LinearLayout)findViewById(R.id.container_edit);
        swipeMove = new SwipeMove(layoutEdit, config.swipeMoveMargin);

        // EditText //
        editText = (EditText) findViewById(R.id.editText);
        /*editText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                System.out.println("onEditorAction");
                return false;
            }
        });*/
        /*editText.setOnTouchListener(new TextView.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction()==MotionEvent.ACTION_DOWN){
                    Log.i("PDP","onTouch - "+editText.getSelectionStart() + " " + editText.getSelectionEnd());
                }
                //System.out.println("onTouchListener");
                //Log.i("PDP","onTouchListener "+event.getAction());
                return false;
            }
        });*/
        editText.setOnClickListener(new TextView.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (moved) return;
                //Log.i("PDP", "onClick :" + editText.getSelectionStart() + " " + editText.getSelectionEnd());
                getWordText(editText.getSelectionStart(), editText.getSelectionEnd());
                closePSBookmarkEditWindow();
            }
        });
        editText.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent e) {
                switch (e.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        swipeMove.start(e);
                        moved = false;
                        startX = e.getX();
                        startY = e.getY();
                        Log.d("PDD", "ActionDown Y="+startY);
                        break;
                    case MotionEvent.ACTION_MOVE:
                        lastX = e.getX();
                        lastY = e.getY();
                        //Log.d("PDD", "X,Y=" + (int) lastX + "," + (int) lastY);
                        swipeMove.move(e);
                        break;
                    case MotionEvent.ACTION_UP:
                        lastX = e.getX();
                        lastY = e.getY();
                        Log.d("PDD", "ActionUp Y="+lastY);
                        if (abs(lastX-startX) > marginForMove || abs(lastY-startY) > marginForMove){
                            moved = true;
                            Log.d("PDD", "moved!");
                        }
                        swipeMove.end(e);
                        break;
                }
                return false;
            }
        });
        editText.setCustomSelectionActionModeCallback(new ActionMode.Callback() {
            @Override
            public boolean onCreateActionMode(ActionMode mode, Menu menu) {
                //Log.d("PDD", "onCreateActionMode");
                if (Utility.isEmpty(openedFilename)){
                    AlertDialog.Builder builder = new AlertDialog.Builder(This);
                    builder.setMessage(getString(R.string.msg_need_save_for_psbm))
                            .setPositiveButton("OK", null);
                    builder.show();
                    return true;
                }
                createPSBookmarkEditWindow();
                return true;
            }

            @Override
            public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
                return false;
            }

            @Override
            public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
                return false;
            }

            @Override
            public void onDestroyActionMode(ActionMode mode) {
                //closePSBookmarkEditWindow();
            }
        });

        // PopupText //
        //popupText = (TextView)findViewById(R.id.popupText);
        popupList = (ListView) findViewById(R.id.popupList);
        popupListAdapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1);
        popupList.setAdapter(popupListAdapter);
        popupList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                WordItem item = popupListAdapter.getItem(position);
                openNewPSWindow(item);
            }
        });
        showPopupList(false);

        // Initialize JNI.
        File tempPath = getExternalFilesDir(null);
        pdicJni = PdicJni.createInstance(getAssets(), tempPath.getAbsolutePath());        // Create JNI callback

		jniCallback = JniCallback.createInstance();
        wordListAdapter = new TWordListAdapter(popupList, popupListAdapter);

        if (pdicJni != null) {
            pdicJni.createFrame(jniCallback, 0);
        }

        dicMan = DictionaryManager.createInstance(this);

        // Dropbox //
        ndvUtils = DropboxUtils.getInstance(this);
        ndvFM = DropboxFileManager.createInstance(this);

        psbmFM = PSBookmarkFileManager.createInstance(this, ndvFM);

        // Audio Player setup //
        initAudioPlayer();

        // Initial Text //
        if (Utility.isEmpty(orgTitle)){
            orgTitle = getTitle().toString();
        }
        Intent i = getIntent();
        String word = i.getStringExtra("word");
        if (Utility.isNotEmpty(word)){
            String text = i.getStringExtra("text");
            editText.setText(text);
            setTitle(orgTitle + " - " + word);
        } else
        if (loadClipboardData()) {
            Toast.makeText(this, getString(R.string.msg_clipboard_loaded), Toast.LENGTH_SHORT).show();
        } else {
            // load the latest opened file
            HistoryFilename histName = getLatestHistoryName();
            if (histName!=null){
                fileEncoding = histName.encoding;
                loadFile(histName.filename, histName.remoteName);
            }
        }
    }

    class HistoryFilename {
        String filename;
        String remoteName;
        String encoding;
    }

    HistoryFilename getLatestHistoryName(){
        FileHistoryManager fileHistory = new FileHistoryManager(this);
        if (fileHistory.size()==0) return null;
        String filename = fileHistory.get(0);
        HistoryFilename histName = new HistoryFilename();
        if (ndvUtils.hasPrefix(filename)){
            histName.filename = ndvUtils.convertToLocalName(filename);
            histName.remoteName = filename.substring(4);
        } else {
            histName.filename = filename;
        }
        histName.encoding = fileHistory.getEncoding(0);
        return histName;
    }

    boolean loadClipboardData(){
        ClipboardManager cm = (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
        ClipData cd = cm.getPrimaryClip();
        if(cd != null){
            ClipData.Item item = cd.getItemAt(0);
            editText.setText(item.getText());
            return true;
        }
        return false;
    }

    PSBookmarkEditWindow psbEditWindow;

    void createPSBookmarkEditWindow() {
        if (psbEditWindow == null) {
            psbEditWindow = new PSBookmarkEditWindow(this, editText, psbmFilename) {
                @Override
                void notifyChanged(PSBookmarkItem item) {

                }
            };
            int x = (int)lastX;
            final int fontHeight = 32;  //TODO: １行分の高さ
            int y = ((int)lastY) - editText.getHeight() + fontHeight + 4;
            //Log.d("PDD", "y="+y+" lastY="+lastY+" editText.Height="+editText.getHeight()+" fontHeight="+fontHeight);
            psbEditWindow.show(editText, x, y);
        }
    }

    void closePSBookmarkEditWindow() {
        if (psbEditWindow != null) {
            psbEditWindow.close();
            psbEditWindow = null;
        }
    }

    private void openPSBookmark() {
        if (!PSBookmarkReady) {
            if (psbmFM.open()) {
                PSBookmarkReady = true;
            }
        }
    }

    void setupPopupMenuItem(View view, final int resource) {
        view.findViewById(resource).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setTextStyle(resource);
            }
        });
    }

    void setTextStyle(final int resource) {
        //Log.d("PDD", "resouce=" + resource);
        int start = editText.getSelectionStart();
        int end = editText.getSelectionEnd();
        int style = 0;
        int color = 0;
        if (resource == R.id.tv_normal_text) {

        } else if (resource == R.id.tv_bold_text) {
            style = PdicJni.BOLD;
        } else if (resource == R.id.tv_underline_text) {
            style = PdicJni.UNDERLINE;
        } else if (resource == R.id.tv_italic_text) {
            style = PdicJni.ITALIC;
        } else if (resource == R.id.tv_strikeout_text) {
            style = PdicJni.STRIKEOUT;
        } else if (resource == R.id.tv_hilite1_text) {
            color = Color.RED;
        } else if (resource == R.id.tv_hilite2_text) {
            color = Color.GREEN;
        } else if (resource == R.id.tv_hilite3_text) {
            color = Color.CYAN;
        }
        String text = editText.getText().toString().substring(start, end);
        setTextStyle(start, end, style, color, text);
        String comment = "";
        color &= ~0xFF000000;
        Log.d("PDD", "addPSBookmark");
        openPSBookmark();
        pdicJni.addPSBookmark(psbmFilename, start, end - start, style, color, text, comment);
    }

    void setTextStyle(int start, int end, int style, int color, String text) {
        Utility.setTextStyle(editText, start, end, style, color, text);
    }

    @Override
    protected void onResume() {
        super.onResume();
        //Log.i("PDD", "PSWinActivity.onResume");

        //if (jniCallback==null)
        //    jniCallback = JniCallback.getInstance();    // 2017.3.17 なぜかnullになるときがある？ための対症療法
        jniCallback.setWordList(wordListAdapter);
        if (PSBookmarkReady) {
            psbmFM.open();
        }

        if (ndvFM.authComplete(this)) {
            selectFileDropbox();
        }

        dicMan.openDictionary();
    }

    int lastPosition;

    @Override
    protected void onPause() {
        super.onPause();
        //Note: IdleConnectionHandlerが一分おきにonPauseさせる
        if (Utility.isNotEmpty(openedFilename)) {
            INetDriveFileInfo info = ndvFM.findByLocalName(openedFilename);
            if (info != null) {
                //remoteRevision = info.remoteRevision;
                Log.d("PDD", "onPause");
                if (lastPosition!=editText.getSelectionStart()) {
                    lastPosition = editText.getSelectionStart();
                    pdicJni.savePSFileInfo(psbmFilename, lastPosition, remoteFilename, info.remoteRevision);
                }
            }
        }
        jniCallback.setWordList(null);
        if (PSBookmarkReady) {
            psbmFM.close();
        }

        dicMan.closeDictionary();
    }

    @Override
    protected void onStop(){
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        closePSBookmarkEditWindow();
        super.onDestroy();
        closeAudioPlayer();
        if (PSBookmarkReady) {
            PSBookmarkReady = false;
            psbmFM.close();
        }
        if (psbmFM != null)
            psbmFM.deleteInstance();
        if (dicMan!=null){
            dicMan.deleteInstance();
            dicMan = null;
        }
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

    private String getWordText(int start, int end) {
        String searchText;
        String touchedText = null;
        int startPos = 0;

        if (start == end) {
            // no selection
            String text = editText.getText().toString();
            final int pick_range = 100;
            int start0 = start - pick_range;
            if (start0 < 0) {
                start0 = 0;
            }
            int end0 = end + pick_range;
            if (end0 > text.length()) {
                end0 = text.length();
            }
            String picked_text = text.substring(start0, end0);
            //popupText.setText(picked_text);

            final boolean findByGetWords = true;

            if (findByGetWords) {
                // find the words for the touched word from the dictionry.
                int longest = 1;
                int maxwords = 15;  // max number of the candidated words.
                int about = 1;      // what's on earth?
                int pos = start - start0;
                pdicJni.getLPWords(picked_text, pos, longest, maxwords, about);
                Log.d("PDP", "start=" + pdicJni.startPos + " end=" + pdicJni.endPos + " prevstart=" + pdicJni.prevStartPos);
                searchText = picked_text.substring(pdicJni.prevStartPos, pdicJni.endPos);
                startPos = pdicJni.startPos - pdicJni.prevStartPos;

                // touchした単語のみを抽出
                String s = picked_text.substring(pdicJni.startPos, pdicJni.endPos);
                String[] words = s.split("[^\\w]", 1);
                int index = s.indexOf(' ');
                if (index>=0){
                    touchedText = s.substring(0, index);
                } else {
                    touchedText = s;
                }
            } else {
                // Find the previous word that locates on the touched word.
                int wordtop = -1;
                int lastword = -1;
                int prev_wordtop = -1;
                for (int i = 0; i < start - start0; i++) {
                    char c = picked_text.charAt(i);
                    if (c <= ' ') {
                        // delimiter
                        lastword = -1;
                    } else {
                        // word character
                        if (lastword == -1) {
                            if (wordtop != -1) {
                                prev_wordtop = wordtop;
                            }
                            lastword = i;
                            wordtop = i;
                        }
                    }
                }
                Log.d("PDP", "prev_word=" + prev_wordtop + " wordtop=" + wordtop);

                int pos = prev_wordtop == -1 ? wordtop == -1 ? 0 : wordtop : prev_wordtop;
                searchText = picked_text.substring(pos);
                startPos = prev_wordtop == -1 ? 0 : pos - prev_wordtop;
            }
            //popupText.setText(searchText);
        } else {
            // uses selected text
            searchText = "";
        }

        //popupText.setText("");
        popupListAdapter.clear();
        int curpos = 0;
        int flags = 0;  // depends on flags in JNI.
        int ret = pdicJni.searchLongestWord(searchText, startPos, curpos, flags);
        Log.d("PDP", "ret=" + ret);
        if (ret==0){
            // no hit words
            if (true) {
                if (pdicJni.incSearch(searchText.substring(startPos)) >= 0) {
                    final int max_count = 8;
                    for (int i = 0; i < max_count; i++) {
                        if (pdicJni.idleProc(0) != 0) {
                            jniCallback.requestUpdate();
                            break;
                        }
                    }
                }
            }
            if (false) {
                if (popupListAdapter.getCount() == 0) {
                    WordItem item = new WordItem(searchText.substring(startPos), null);
                    popupListAdapter.add(item);
                }
            }
        }
        showPopupList(ret>0);
        if (ret==0){
            String word;
            if (Utility.isNotEmpty(touchedText)){
                word = " - "+touchedText;
            } else {
                word = "";
            }
            Toast.makeText(this, getString(R.string.msg_no_hit_word) + word, Toast.LENGTH_SHORT).show();
        }
        return "";
    }

    void showPopupList(boolean on){
        if (popupList.getVisibility() == View.GONE) {
            // to visible
            if (!on) return;
            ViewGroup.LayoutParams params;
            params = editText.getLayoutParams();
            params.height = savedEditTextHeight;
            editText.setLayoutParams(params);
            popupList.setVisibility(View.VISIBLE);
        } else {
            // to invisible
            if (on) return;
            popupList.setVisibility(View.GONE);
            ViewGroup.LayoutParams params;
            params = editText.getLayoutParams();
            if (savedEditTextHeight < 0)
                savedEditTextHeight = params.height;
            params.height = MATCH_PARENT;
            editText.setLayoutParams(params);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_pswin, menu);
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
            startActivity(new Intent().setClassName(this.getPackageName(), SettingsActivity.class.getName()));
            return true;
        } else if (id == R.id.action_loadfile) {
            selectFile();
        } else if (id == R.id.action_loadfile_history) {
            selectFileFromHistory();
        } else if (id == R.id.action_loadfile_dropbox) {
            downloadFileDropbox(true);
        } else if (id == R.id.action_savefile){
            saveFileUI();
        } else if (id == R.id.action_goto){
            moveCursor();
        } else if (id == R.id.action_viewpsb) {
            viewPSBookmarkList();
        }

        return super.onOptionsItemSelected(item);
    }

    //TODO: move to Util
    private String getText(InputStream is) throws IOException {
        ByteArrayOutputStream bs = new ByteArrayOutputStream();
        byte[] bytes = new byte[4096];
        int len;
        while ((len = is.read(bytes)) > 0) {
            bs.write(bytes, 0, len);
        }
        return new String(bs.toByteArray(), "UTF8");
    }

    static final int REQUEST_CODE_PSB = 0;
    static final int REQUEST_CODE_SAVE = 1;
    static final int REQUEST_CODE_SELECT_FILE = 2;
    static final int REQUEST_CODE_SELECT_FILE_DBX = 3;

    boolean fromDropbox = false;
    private String m_strInitialDir = "/";

    void selectFile() {
        if (config.useOldFileSelection) {
            FileSelectionDialog dlg = new FileSelectionDialog(this, this, false);
            String[] exts = {".txt"};
            m_strInitialDir = pref.getString(pfs.PSINITIALDIR, m_strInitialDir);
            dlg.show(new File(m_strInitialDir), exts);
            fromDropbox = false;
        } else {
            Intent i = new Intent().setClassName(this.getPackageName(), FileDirSelectionActivity.class.getName());
            i.putExtra(pfs.INITIALDIR, pref.getString(pfs.PSINITIALDIR, m_strInitialDir));
            startActivityForResult(i, REQUEST_CODE_SELECT_FILE);
        }
    }

    String downloadedRemoteName;

    private void loadFile(String filename, String downloadedRemoteName) {
        if (useTextLoadTask) {
            this.downloadedRemoteName = downloadedRemoteName;
            String defCharset = pref.getBoolean(pfs.DEFCHARSET, false) ? config.defaultCharsetEncoding : null;
            TextLoadTask textLoadTask = new TextLoadTask(this, this, 0, defCharset);
            textLoadTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, filename, fileEncoding);
            createProgressDialog(false);
        } else {
            LoadFileTask loadTask = new LoadFileTask(filename, downloadedRemoteName);
            createProgressDialog(false);
            loadTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }
    }

    // TextLoadTask callbacks //
    @Override
    public void onPreFileLoad() {
    }

    @Override
    public void onFileLoaded(StringBuilder result, String filename, String charset, int linebreak, int mOffset) {
        if (result != null) {
            editText.setText(result);
            //editText.setChanged(false);
            loadFilePost(filename, downloadedRemoteName);
        }
    }

    // my original load file task
    //TODO: LoadFileTaskに移動できないか？
    class LoadFileTask extends AsyncTask {
        String filename;
        String remoteName;
        String text;
        boolean ok = false;

        public LoadFileTask(String filename, String remotename) {
            super();
            this.filename = filename;
            this.remoteName = remotename;
        }

        @Override
        protected Object doInBackground(Object[] param) {
            try {
                FileInputStream fis = new FileInputStream(filename);
                text = getText(fis);
                ok = true;
            } catch (IOException e) {
                //Toast.makeText(this, "File load failed : " + filename, Toast.LENGTH_SHORT).show();
                //e.printStackTrace();
            }
            if (!ok) {
                AssetManager asset = getAssets();
                try {
                    InputStream is = asset.open(filename);
                    text = getText(is);
                    ok = true;
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            return null;
        }

        @Override
        protected void onPostExecute(Object result) {
            onPostLoadFile(this);
        }
    }

    private void onPostLoadFile(LoadFileTask loadTask) {
        if (loadTask.ok) {
            editText.setText(loadTask.text);
            loadFilePost(loadTask.filename, loadTask.remoteName);
        } else {
            deleteProgressDialog();
            Toast.makeText(this, getString(R.string.msg_file_load_failed)+" : " + loadTask.filename, Toast.LENGTH_LONG).show();
            loadFileFailed();
        }
    }

    void loadFilePost(String filename, String remotename) {
        // setup remote filename
        if (Utility.isNotEmpty(remotename)) {
            remoteFilename = remotename;
        } else {
            if (ndvUtils.isLocalFileForRemote(filename)) {
                remoteFilename = ndvUtils.convertToRemoteName(filename);
            } else {
                remoteFilename = null;
            }
        }
        openedFilename = filename;
        psbmFilename = PSBookmarkFileManager.buildFileName(openedFilename, remoteFilename, ndvUtils.prefix);

        // set title
        File file = new File(filename);
        setTitle(orgTitle + " - " + file.getName());

        // load PSBookmark
        PdicJni.PSFileInfo info = loadPSBookmarks(psbmFilename);
        //remoteRevision = info.revision;

        if (Utility.isNotEmpty(remoteFilename) && info!=null) {
            // add to ndvFM
            ndvFM.add(openedFilename, remoteFilename, info.revision, null);  //TODO: 現在はdownloadのみだからいいけど。。
        }
        int position = 0;
        if (info!=null) {
            position = info.position;
            if (position >= 0) {
                try {
                    editText.setSelection(position);
                } catch (Exception e){
                    e.printStackTrace();
                    position = 0;
                }
            } else
            if (position < 0) position = editText.getSelectionStart();
        }
        lastPosition = position;
        //pdicJni.savePSFileInfo(psbmFilename, position, remoteFilename, info != null ? info.revision : null);   // update the date
        deleteProgressDialog();
        //Toast.makeText(this, getString(R.string.msg_file_loaded)+" : " + filename, Toast.LENGTH_SHORT).show();
        FileHistoryManager mgr = new FileHistoryManager(this);
        mgr.add(psbmFilename, fileEncoding);

        // setup Audio Player //
        String audioFileName = Utility.changeExtension(openedFilename, "mp3");
        boolean audioOk = openAudioPlayer(audioFileName);
        if (!audioOk){
            audioFileName = Utility.changePath(audioFileName, altAudioFolder);
            audioOk = openAudioPlayer(audioFileName);
        }
        showAudio(audioOk);
    }

    // file load エラー後処理
    void loadFileFailed(){
        closeAudioPlayer();
    }

    void saveFileUI(){
        AlertDialog.Builder builder = new AlertDialog.Builder(This);
        builder.setMessage("UNDERCONSTRUCTION - This will be implemented in the near future")
                .setPositiveButton("OK", null);
        builder.show();
        return;
        //Intent i = new Intent().setClassName(this.getPackageName(), FileSaveActivity.class.getName());
        //startActivityForResult(i, REQUEST_CODE_SAVE);
    }

    String saveFileName;
    void saveFile(File file){
        try {
            saveFileName = file.getAbsolutePath();
            FileOutputStream fos = new FileOutputStream(saveFileName);    // throw IOException
            SaveFileTask saveTask = new SaveFileTask(fos, editText.getText().toString(), this);
            createProgressDialog(true);
            saveTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        } catch (IOException e){
            Toast.makeText(this, getString(R.string.toast_save_failed) + " : " + file.getAbsolutePath(), Toast.LENGTH_SHORT).show();
            e.printStackTrace();
        }
    }

    @Override
    public void onPostSaveFile(SaveFileTask saveTask){
        if (saveTask.ok) {
            openedFilename = saveFileName;
            psbmFilename = PSBookmarkFileManager.buildFileName(openedFilename, remoteFilename, ndvUtils.prefix);
            remoteFilename = "";
            deleteProgressDialog();
        } else {
            deleteProgressDialog();
            Toast.makeText(this, getString(R.string.msg_file_save_failed)+" : " + saveFileName, Toast.LENGTH_LONG).show();
        }
    }

    ProgressDialog progress;

    void createProgressDialog(boolean save) {
        progress = new ProgressDialog(this);
        if (save){
            progress.setTitle(getString(R.string.title_prog_file_saving));
            progress.setMessage(getString(R.string.msg_prog_file_saving));
        } else {
            progress.setTitle(getString(R.string.title_prog_file_loading));
            progress.setMessage(getString(R.string.msg_prog_file_loading));
        }
        progress.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        progress.show();
    }

    void deleteProgressDialog() {
        if (progress == null) return;
        progress.dismiss();
        progress = null;
    }

    private void downloadFileDropbox(boolean withAuth) {
        if (ndvFM.startAuth(false)) {
            ndvUtils.appKeysConfirmed = true;
            selectFileDropbox();
        } else {
            if (withAuth) {
                if (!ndvUtils.appKeysConfirmed) {
                    MyDropboxAppKeysDialog dlg = new MyDropboxAppKeysDialog();
                    dlg.show(getFragmentManager(), "dbx app keys");
                }
            }
        }
    }

    @Override
    public void onFileSelect(FileInfo file) {
        if (fromDropbox) {
            ndvUtils.setInitialDir(file.getParent());
            String toFileName = ndvUtils.convertToLocalName(file.getAbsolutePath());
            File toFile = new File(toFileName);
            ndvUtils.makeDir(this, toFile);
            executeDownloadFile(file.getAbsolutePath(), toFile);
        } else {
            loadFile(file.getPath(), null);

            m_strInitialDir = file.getParent();
            SharedPreferences.Editor edit = pref.edit();
            edit.putString(pfs.PSINITIALDIR, m_strInitialDir);
            edit.commit();
        }
    }

    public static class MyDropboxAppKeysDialog extends DropboxAppKeysDialog {
        @Override
        protected void onDropboxAppKeys(DropboxUtils.AppKeys appKeys) {
            This.onDropboxAppKeys(appKeys);
        }
    }

    void onDropboxAppKeys(DropboxUtils.AppKeys appKeys) {
        ndvUtils.setAppKeys(appKeys);
        downloadFileDropbox(false);
    }

    private void selectFileDropbox() {
        if (config.useOldFileSelection) {
            DropboxFileSelectionDialog dlg = DropboxFileSelectionDialog.createInstance(this, this, ndvFM, false);
            String[] exts = {".txt"};
            dlg.show(new File(ndvUtils.getInitialDir()), exts);
        } else {
            Intent i = new Intent().setClassName(this.getPackageName(), Dropbox2FileSelectionActivity.class.getName());
            i.putExtra("onlySelection", true);
            startActivityForResult(i, REQUEST_CODE_SELECT_FILE_DBX);
        }
        fromDropbox = true;
    }

    void executeDownloadFile(String from, File to) {
        ndvFM.executeDownload(from, to, new INetDriveFileManager.OnExecuteListener(){
            @Override
            public void onPostExecute(boolean downloaded, String from, File to, String revision) {
                deleteProgressDialog();
                if (!downloaded) return;
                loadFile(to.getAbsolutePath(), from);
            }
        });
        createProgressDialog(false);
    }

    private void saveToFile(String filename) {
        FileOutputStream outputTextStream;
        try {
            outputTextStream = openFileOutput(filename, MODE_PRIVATE);
            outputTextStream.write(editText.getText().toString().getBytes());
            outputTextStream.close();
        } catch (FileNotFoundException ex) {
            Log.e("FileNotFoundException", ex.getMessage());
        } catch (IOException ex) {
            Log.e("IOException", ex.getMessage());
        }
    }

    // --------------------------------------- //
    // Move Cursor
    // --------------------------------------- //
    void moveCursor()
    {
        GotoDialog dialog = new GotoDialog();
        dialog.setListener(this);
        dialog.show(getFragmentManager(), "goto_dialog");
    }
    @Override
    public void onGotoClicked(Bundle args){
        int value;
        try {
            value = Integer.parseInt(args.getString("value"));
        } catch (NumberFormatException e){
            value = 0;
        }
        if (args.getInt("selection")==0) {
            // use percentage
            if (value>100) value = 100;
            int position = (int)(editText.length() * value / 100);
            editText.setSelection(position, position);
        } else {
            // use line number
            Utility.setCursorLine(editText, value);
        }
    }

    // --------------------------------------- //
    // Bookmark
    // --------------------------------------- //
    int psbmGeneration;

    private void viewPSBookmarkList() {
        Intent i = new Intent().setClassName(this.getPackageName(), PSBookmarkActivity.class.getName());
        i.putExtra("filename", psbmFilename);
        startActivityForResult(i, REQUEST_CODE_PSB);
        psbmGeneration = pdicJni.psbmGeneration;
    }

    private void selectFileFromHistory() {
        if (config.useOldFileSelection) {
            if (usePSBMforFileLoad) {
                Intent i = new Intent().setClassName(this.getPackageName(), PSBookmarkActivity.class.getName());
                startActivityForResult(i, REQUEST_CODE_PSB);
                psbmGeneration = pdicJni.psbmGeneration;
            } else {
                Intent i = new Intent().setClassName(this.getPackageName(), FileHistoryActivity.class.getName());
                startActivityForResult(i, REQUEST_CODE_PSB);
            }
        } else {
            Intent i = new Intent().setClassName(this.getPackageName(), FileHistorySelectionActivity.class.getName());
            startActivityForResult(i, REQUEST_CODE_PSB);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_CODE_PSB) {
            if (resultCode == RESULT_OK) {
                Bundle bundle = data.getExtras();
                if (bundle != null) {
                    String filename = bundle.getString("filename");
                    String remotename = bundle.getString("remotename");
                    if (Utility.isEmpty(filename)) {
                        if (psbmGeneration!=pdicJni.psbmGeneration)
                            reloadPSBookmarks(psbmFilename);

                        int start = bundle.getInt("start");
                        int length = bundle.getInt("length");
                        //editText.setSelection(start, start + length); // これだとscrollしたときbookmarkした項目が表示されるようにscrollが戻ってしまう。(touchで何かを検索すると直る)
                        editText.setSelection(start, start);
                    } else {
                        // File Load from History
                        fileEncoding = bundle.getString("fileEncoding");
                        loadFile(filename, remotename);
                    }
                }
            } else
            if (resultCode==RESULT_CANCELED){
                if (psbmGeneration!=pdicJni.psbmGeneration)
                    reloadPSBookmarks(psbmFilename);
            }
        } else
        if (requestCode == REQUEST_CODE_SAVE){
            if (resultCode == RESULT_OK){
                Bundle bundle = data.getExtras();
                if (bundle != null){
                    String path = bundle.getString("path");
                    String filename = bundle.getString("filename");
                    File file = new File(path, filename);
                    saveFile(file);
                }
            }
        } else
        if (requestCode == REQUEST_CODE_SELECT_FILE || requestCode == REQUEST_CODE_SELECT_FILE_DBX) {
            if (resultCode == RESULT_OK) {
                Bundle bundle = data.getExtras();
                if (bundle != null) {
                    String filename = bundle.getString("filename");
                    if (Utility.isNotEmpty(filename)) {
                        File file = new File(filename);
                        String name;
                        if (requestCode == REQUEST_CODE_SELECT_FILE){
                            name = file.getName();
                        } else {
                            // dropbox
                            //String remotename = bundle.getString("remotename");
                            // The file is selected to be added.
                            name = file.getName() + " [Dropbox]";
                        }
                        fileEncoding = bundle.getString("fileEncoding");
                        FileInfo fileInfo = new FileInfo(name, file);
                        onFileSelect(fileInfo);
                    }
                }
            }
        }
    }

    IPSBookmarkListAdapter ipsBookmarkListAdapter;

    PdicJni.PSFileInfo loadPSBookmarks (String psbmName) {
        openPSBookmark();
        if (ipsBookmarkListAdapter == null) {
            ipsBookmarkListAdapter = new IPSBookmarkListAdapter() {
                @Override
                public int addItem(int position, int length, int style, int color, String markedWord, String comment) {
                    if (color != 0) color |= 0xFF000000;    //TODO: alpha support
                    setTextStyle(position, position + length, style, color, null);
                    return 0;
                }

                @Override
                public int addFile(String filename) {
                    return 0;
                }
            };
        }
        JniCallback callback = JniCallback.createInstance();
        callback.setPSBookmarkListAdapter(ipsBookmarkListAdapter);
        pdicJni.loadPSBookmark(psbmName);
        callback.setPSBookmarkListAdapter(null);

        callback.deleteInstance();

        PdicJni.PSFileInfo info = pdicJni.loadPSFileInfo(psbmName);
        return info;
    }

    void reloadPSBookmarks(String psbmName){
        String text = editText.getText().toString();
        Spannable word = Utility.removeSpannable(text);
        editText.setText(word, TextView.BufferType.SPANNABLE);
        loadPSBookmarks(psbmName);
    }

    void openNewPSWindow(WordItem item){
        Intent i = new Intent().setClassName(this.getPackageName(), PSWinActivity.class.getName());
        i.putExtra("word", item.word);
        i.putExtra("text", item.trans);
        startActivity(i);
    }

    void checkPSBookmarkStatus(){
        // conflictが発生していたらdialog boxで動作の選択肢を表示
    }

    // --------------------------------------- //
    // Audio Player
    // --------------------------------------- //
    private LinearLayout audioLayout;
    private SeekBar audioSlider;
    private Button btnStepRewind;
    private Button btnPlayPause;
    private TextView tvPosition;
    private MediaPlayer mediaPlayer;
    // play status
    private int lastPlayPosition = 0;
    private int audioDuration = 0;
    private int audioDurationSec = 0;
    // settings
    private int stepRewindTime = 5000; // [msec]
    private String altAudioFolder = "/storage/sdcard0/Download";

    private AudioSliderUpdateThread updateThread;
    void initAudioPlayer(){
        audioLayout = (LinearLayout)findViewById(R.id.container_audio);
        audioSlider = (SeekBar)findViewById(R.id.audioSeekBar);
        audioSlider.setOnSeekBarChangeListener(this);
        btnStepRewind = (Button)findViewById(R.id.btn_step_rewind);
        btnPlayPause = (Button)findViewById(R.id.btn_play_stop);
        btnStepRewind.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v){
                audioStepRewind();
            }
        });
        btnPlayPause.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                audioPlayPause();
            }
        });
        tvPosition = (TextView)findViewById(R.id.text_position);
        showAudio(false);
    }
    boolean openAudioPlayer(String filename){
        closeAudioPlayer(false);
        if (!Utility.fileExists(filename))
            return false;
        mediaPlayer = new MediaPlayer();
        try {
            mediaPlayer.setDataSource(filename);
        } catch (IOException e) {
            e.printStackTrace();
            closeAudioPlayer();
            return false;
        }
        try {
            mediaPlayer.prepare();
        } catch (IOException e) {
            e.printStackTrace();
            closeAudioPlayer();
            return false;
        }
        audioSlider.setProgress(lastPlayPosition);
        audioDuration = mediaPlayer.getDuration();
        audioDurationSec = audioDuration / 1000;
        audioSlider.setMax(audioDuration);

        mediaPlayer.setLooping(true);
        mediaPlayer.start();

        updateThread = new AudioSliderUpdateThread();
        updateThread.start();
        btnPlayPause.setText(R.string.label_pause);
        tvPosition.setText("sss");

        return true;
    }
    void closeAudioPlayer(){
        closeAudioPlayer(true);
    }
    void closeAudioPlayer(boolean showControl){
        if (updateThread != null){
            updateThread.runnable = false;
            updateThread.interrupt();
            try {
                updateThread.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            updateThread = null;
        }
        if (mediaPlayer != null){
            if (mediaPlayer.isPlaying())
                mediaPlayer.stop();
            mediaPlayer.release();
            mediaPlayer = null;
            if (showControl) {
                showAudio(false);
            }
        }
    }
    void audioStepRewind(){
        int pos = mediaPlayer.getCurrentPosition();
        pos -= stepRewindTime;
        if (pos < 0) pos = 0;
        mediaPlayer.seekTo(pos);
        audioSlider.setProgress(pos);
    }
    void audioPlayPause(){
        if (mediaPlayer == null) return;
        if (mediaPlayer.isPlaying()){
            mediaPlayer.pause();
            btnPlayPause.setText(R.string.label_play);
        } else {
            mediaPlayer.start();
            btnPlayPause.setText(R.string.label_pause);
        }
    }

    void showAudio(boolean on){
        if (audioLayout.getVisibility()==View.GONE){
            // to visible
            if (!on) return;
            audioLayout.setVisibility(View.VISIBLE);
        } else {
            // to invisible
            if (on) return;
            audioLayout.setVisibility(View.GONE);
        }
    }

    // SeekBar.OnSeekBarChangeListener overridden members //
    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (fromUser) {
            // ユーザーがpositionを変更した
            mediaPlayer.seekTo(progress);
            seekBar.setProgress(progress);
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {

    }

    // SeekBar update thread //
    class AudioSliderUpdateThread extends Thread {
        public boolean runnable = true;
        @Override
        public void run(){
            try {
                while (runnable){
                    if (mediaPlayer != null) {
                        int currentPosition = mediaPlayer.getCurrentPosition();    //現在の再生位置を取得
                        Message msg = new Message();
                        msg.what = currentPosition;
                        threadHandler.sendMessage(msg);                        //ハンドラへのメッセージ送信
                    }
                    Thread.sleep(200);
                }
            } catch (InterruptedException e) {
                //return;
            }
        }
    }
    private Handler threadHandler = new Handler() {
        public void handleMessage(Message msg) {
            audioSlider.setProgress(msg.what);
            int sec = msg.what / 1000;
            String text = String.format("%d:%02d/%d:%02d", sec/60, sec % 60, audioDurationSec/60, audioDurationSec%60);
            tvPosition.setText(text);
            //tvPosition.setText( Integer.toString(sec/60) + ":" + Integer.toString(sec % 60) + "/" + Integer.toString(audioDurationSec/60) + ":" + Integer.toString(audioDurationSec%60));
        }
    };

    /**
     * A placeholder fragment containing a simple view.
     */
    public static class PlaceholderFragment extends Fragment {

        public PlaceholderFragment() {
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                                 Bundle savedInstanceState) {
            View rootView = inflater.inflate(R.layout.fragment_pswin, container, false);
            return rootView;
        }
    }
}

/* 必要になれば復活
class PSWordItemLayout extends LinearLayout {
    TextView textView;

    public PSWordItemLayout(Context context, AttributeSet attrs){
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate(){
        super.onFinishInflate();
        textView  = (TextView)findViewById(R.id.file_list_item_text);
    }

    public void bindView(FileInfo item){
        textView.setText(item.getName());
    }
}
class PSWordListAdapter extends ArrayAdapter<WordItem> {
    private List<WordItem> wordList;

    LayoutInflater layoutInflater;
    int resource;

    PSWordListAdapter(Context context, int resource, List<WordItem> objects) {
        super(context, resource);

        layoutInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        this.resource = resource;
        wordList = objects;
    }

    @Override
    public int getCount() { return wordList.size(); }

    @Override
    public WordItem getItem( int position )
    {
        return wordList.get( position );
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent){
        final PSWordItemLayout view;
        if (convertView==null){
            view = (PSWordItemLayout)layoutInflater.inflate(resource, null);
        } else {
            view = (PSWordItemLayout)convertView;
        }
        //view.bindView(getItem(position));
        return view;
    }
}
*/

class TWordListAdapter implements IWordListAdapter {

    //private TextView popupText;
    private ListView popupList;
    private ArrayAdapter<WordItem> popupListAdapter;

    public TWordListAdapter(ListView _popupList, ArrayAdapter<WordItem> _popupListAdapter) {
        //popupText = _popupText;
        popupList = _popupList;
        popupListAdapter = _popupListAdapter;
    }

    @Override
    public void add(WordItem item) {
        //popupText.setText(text+word);
        //popupListAdapter.add(text+item.word);
        popupListAdapter.add(item);
    }
}
