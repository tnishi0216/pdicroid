package com.reliefoffice.pdic;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothHeadset;
import android.content.BroadcastReceiver;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.graphics.Point;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.preference.PreferenceManager;
import android.speech.tts.TextToSpeech;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.text.Spannable;
import android.util.Log;
import android.view.ActionMode;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
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
import java.util.ArrayList;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static java.lang.Math.abs;

/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * {@link TouchSrchFragment.OnFragmentInteractionListener} interface
 * to handle interaction events.
 * Use the {@link TouchSrchFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class TouchSrchFragment extends Fragment implements FileSelectionDialog.OnFileSelectListener, TextLoadTask.OnFileLoadListener, SaveFileTask.SaveFileTaskDone, GotoDialog.Listener, SeekBar.OnSeekBarChangeListener, OnBackPressedListener {
    private static final String ARG_PARAM1 = "param1";
    private static final String ARG_PARAM2 = "param2";
    private static final String ARG_PARAM3 = "param3";

    private String mParam1;
    private String mParam2;
    private boolean fromMain;   //TODO: 動作未確認

    private OnFragmentInteractionListener mListener;

    final static boolean useTextLoadTask = true;

    static TouchSrchFragment This;
    String orgTitle;
    String openedFilename;
    String remoteFilename;
    String psbmFilename;    // filename for PSBookmark
    String lastFileName;

    boolean initialLoading = false;

    static boolean cancel = false;

    // swipe for editText
    class SwipeMove {
        int MOVE_MARGIN_X = 80;
        int MOVE_MARGIN_Y = 10;
        private Point size = new Point();
        public LinearLayout layout;
        private int marginLeft = 0;
        private int startX = 0;
        private int startY = 0;
        private int delta = 0;
        private boolean movingX = false;
        private boolean movingY = false;
        private int prevMargin = 0;     //TODO: Fragmentにしてから、下スクロールをするとend()の時点で元に戻ってしまう（原因不明）そのための対症療法 (git:c44ce89a311daef812aadd4cce283d423cfa1ad1)
        SwipeMove(LinearLayout layout, int moveMarginX){
            this.layout = layout;
            MOVE_MARGIN_X = moveMarginX;
            WindowManager wm = (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
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
            if (prevMargin != marginLeft + delta) {
                final int scale = 1;
                LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                        (int) (size.x * scale),
                        LinearLayout.LayoutParams.WRAP_CONTENT  // important!!
                );
                //Log.d("PDD", "marginLeft="+marginLeft+" delta="+delta);
                prevMargin = marginLeft + delta;
                int selStart = editText.getSelectionStart();    //TODO: Fragmentにしたらうまくうごかない対症療法(prevMarginと同じ）
                int selEnd = editText.getSelectionEnd();
                params.setMargins(prevMargin, 0, 0, 0);
                layout.setLayoutParams(params);
                editText.setSelection(selStart, selEnd);
            }
        }
    }
    private SwipeMove swipeMove;

    private LinearLayout llEdit;

    private EditText editText;
    private int savedEditTextHeight = -1;
    private ListView popupList; // view by list view
    private ArrayAdapter<WordItem> popupListAdapter;
    private PdicJni pdicJni;
    private TWordListAdapter wordListAdapter;
    private JniCallback jniCallback;

    String fileEncoding;

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

    // Audio Mark Position History //
    MarkPositioinHistory markPositionHistory;

    // Bluetooth Manager //
    BluetoothManager bluetoothManager;

    int prevStart, prevEnd;

    static boolean newPSWindow;
    static ArrayList<Integer> lastWordModePositions = new ArrayList<>();

    final static int id_google_translate = 111;
    final static int id_tts = 112;

    // TTS
    TextToSpeech tts;

    // WPM
    WpmController wpm;

    public TouchSrchFragment() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @param param1 Parameter 1.
     * @param param2 Parameter 2.
     * @return A new instance of fragment TouchSrchFragment.
     */
    // TODO: Rename and change types and number of parameters
    public static TouchSrchFragment newInstance(String param1, String param2, boolean fromMain) {
        TouchSrchFragment fragment = new TouchSrchFragment();
        Bundle args = new Bundle();
        args.putString(ARG_PARAM1, param1);
        args.putString(ARG_PARAM2, param2);
        args.putBoolean(ARG_PARAM3, fromMain);
        fragment.setArguments(args);
        return fragment;
    }

    SleepTimer slp;
    SleepTimer wku;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        This = this;

        if (getArguments() != null) {
            mParam1 = getArguments().getString(ARG_PARAM1);
            mParam2 = getArguments().getString(ARG_PARAM2);
            fromMain = getArguments().getBoolean(ARG_PARAM3);
        }

        // 画面ON/OFFのBroadcastReceiverを登録
        screenOnReceiver = new ScreenOnReceiver();
        IntentFilter filter = new IntentFilter(Intent.ACTION_SCREEN_ON);
        getContext().registerReceiver(screenOnReceiver, filter);

        initServiceNotification();
        startAudioPlayService(null);
    }

    private ScreenOnReceiver screenOnReceiver;
    // 画面ONのBroadcastReceiverクラス
    private class ScreenOnReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(Intent.ACTION_SCREEN_ON)) {
                // 画面がONになったときの処理をここに記述
                // 例えば、最前面に自分のアプリがあるかどうかをチェックし、あれば処理を実行
                if (Utility.isAppInForeground(getContext())) {
                    // 自分のアプリが最前面にある場合の処理
                    // LLMファイルである場合、現在再生中のところへカーソル移動
                    moveCursorPlayingLine();
                }
            }
        }
    }

    @Override
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        setHasOptionsMenu(true);    //TODO: OptionsMenuがここでやらなくなったら削除
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_touch_srch, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        /*
        //TODO: これは何のため？？？
        if (savedInstanceState == null) {
            getSupportFragmentManager().beginTransaction()
                    .add(R.id.container, new PlaceholderFragment())
                    .commit();
        }
         */

        pref = PreferenceManager.getDefaultSharedPreferences(getContext());

        // swipeによるEditText移動 //
        LinearLayout layoutEdit = view.findViewById(R.id.container_edit);
        swipeMove = new SwipeMove(layoutEdit, config.swipeMoveMargin);

        // EditText //
        editText = view.findViewById(R.id.editText);
        editText.setKeyListener(null);  // Android 8.0でkeyboardを表示させないようにするため
        editText.setOnClickListener(new TextView.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (moved) return;
                //Log.i("PDP", "onClick :" + editText.getSelectionStart() + " " + editText.getSelectionEnd());
                popupWordText(editText.getSelectionStart(), editText.getSelectionEnd());
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
                menu.add(Menu.NONE, id_google_translate, Menu.NONE, getString(R.string.action_google_translate));
                menu.add(Menu.NONE, id_tts, Menu.NONE, getString(R.string.action_tts));
                if (tts == null) {
                    // なるべく必要なときだけ生成するように
                    // 本当は再生直前にnewしたいが、初期化処理がbackgroundのようで、すぐには再生開始できない
                    tts = new TextToSpeech(getActivity(), null);
                }
                if (getBackStackEntryCount() > 0)
                    return true;
                if (Utility.isEmpty(openedFilename)){
                    // dialogを閉じても再び呼ばれてしまうため
                    int start = editText.getSelectionStart();
                    int end = editText.getSelectionEnd();
                    if (prevStart == start && prevEnd == end)
                        return true;
                    prevStart = start;
                    prevEnd = end;

                    if (false) {
                        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
                        builder.setMessage(getString(R.string.msg_need_save_for_psbm))
                                .setPositiveButton("OK", null);
                        builder.show();
                    }
                    return true;
                }
                prevStart = prevEnd = 0;
                createPSBookmarkEditWindow();
                wpm.pause();
                return true;
            }

            @Override
            public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
                return false;
            }

            @Override
            public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
                int id = item.getItemId();
                switch (id){
                    case id_google_translate:
                    case id_tts:
                        closePSBookmarkEditWindow();
                        String text = Utility.getSelectedText(This.editText);
                        if (id == id_google_translate) {
                            doGoogleTranslate(text);
                        } else {
                            doTTS(text);
                        }
                        return true;
                }
                return false;
            }

            @Override
            public void onDestroyActionMode(ActionMode mode) {
                //closePSBookmarkEditWindow();
                wpm.restart();
            }
        });

        llEdit = view.findViewById(R.id.container_edit);
        ViewTreeObserver.OnGlobalLayoutListener globalLayoutListener = new ViewTreeObserver.OnGlobalLayoutListener(){
            @Override
            public void onGlobalLayout() {
                if (popupList.getVisibility() == View.GONE) {
                    int height = llEdit.getHeight();
                    savedEditTextHeight = height * 2 / 3;
                }
            }
        };
        llEdit.getViewTreeObserver().addOnGlobalLayoutListener(globalLayoutListener);

        // PopupText //
        //popupText = (TextView)findViewById(R.id.popupText);
        popupList = view.findViewById(R.id.popupList);
        popupListAdapter = new ArrayAdapter<>(getContext(), android.R.layout.simple_list_item_1);
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
        pdicJni = PdicJni.getInstance();

        jniCallback = JniCallback.createInstance();
        wordListAdapter = new TWordListAdapter(popupList, popupListAdapter);

        if (pdicJni != null) {
            pdicJni.setCallback(jniCallback, 0);
        }

        // WPM
        wpm = new WpmController(editText) {
            @Override
            protected void updateProcess(int wpm, int wordCount)
            {
                String wpms = Integer.toString(wpm);
                String title = "WPM: ";
                if (running && !paused)
                    title += " << ";
                title += wpms + "  (" + wordCount + "words)";
                getActivity().setTitle(title);
            }
        };

        // Dropbox //
        ndvUtils = DropboxUtils.getInstance(getContext());
        ndvFM = DropboxFileManager.createInstance(getContext());

        psbmFM = PSBookmarkFileManager.createInstance(getContext(), ndvFM);

        // Audio Player setup //
        initAudioPlayer(view);

        // Initial Text //
        if (Utility.isEmpty(orgTitle)){
            orgTitle = getActivity().getTitle().toString();
        }

        if (getBackStackEntryCount() > 0 && !isWordMode()){
            // タッチ検索/クリップ検索で深い検索に入り、そこから別画面に移り、戻ってきた場合
            faked = true;
            mParam1 = pref.getString(pfs.LAST_TOUCH_SRCH_WORD, null);
            mParam2 = pref.getString(pfs.LAST_TOUCH_SRCH_TRANS, null);
        }

        if (!cancel) {
            if (isWordMode()) {
                editText.setText(mParam1 + " " + mParam2);
                if (getBackStackEntryCount() > 0) {
                    SharedPreferences.Editor edit = pref.edit();
                    edit.putString(pfs.LAST_TOUCH_SRCH_WORD, mParam1);
                    edit.putString(pfs.LAST_TOUCH_SRCH_TRANS, mParam2);
                    edit.commit();
                }
            } else if (isClipMode()) {
            } else {
                // load the latest opened file
                HistoryFilename histName = getLatestHistoryName();
                if (histName != null) {
                    fileEncoding = histName.encoding;
                    autoStartPlayMode = false;
                    loadFirstFile(histName.filename, histName.remoteName);
                }
            }
        }

        editText.requestFocus();

        // Audio Mark Position History //
        markPositionHistory = new MarkPositioinHistory(pref);

        if (!use_service) {
            // Bluetooth Manager //
            bluetoothManager = new BluetoothManager();
        }
    }

    public void onToolbarClicked()
    {
        if (wpm == null || !isNormalMode())
            return;
        if (wpm.isRunning()){
            if (wpm.isPaused()){
                wpm.stop();
                if (Utility.isNotEmpty(lastFileName)){
                    getActivity().setTitle(lastFileName);
                }
            } else {
                wpm.pause();
            }
        } else {
            wpm.start();
        }
    }

    boolean faked = false;
    // LLM関連
    boolean LLM = false;
    LLMManager llmManager = new LLMManager();

    boolean isNormalMode(){
        return Utility.isEmpty(mParam1) && Utility.isEmpty(mParam2);
    }
    boolean isWordMode(){
        return Utility.isNotEmpty(mParam1) && !isClipMode();
    }
    boolean isClipMode(){
        return mParam1 == "\\\\clip";
    }
    boolean isLLMode() { return LLM; }

    int getBackStackEntryCount(){
        return getFragmentManager().getBackStackEntryCount();
    }

    static public void setCancel(boolean cancel){
        TouchSrchFragment.cancel = cancel;
    }

    class HistoryFilename {
        String filename;
        String remoteName;
        String encoding;
    }

    HistoryFilename getLatestHistoryName(){
        FileHistoryManager fileHistory = new FileHistoryManager(getContext());
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

    int lastClipLength = 0;

    // 語数を返す
    int loadClipboardData(){
        ClipboardManager cm = (ClipboardManager) getContext().getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData cd = cm.getPrimaryClip();
        if(cd != null){
            ClipData.Item item = cd.getItemAt(0);
            CharSequence text = item.getText();
            if (text != null) {
                editText.setText(text);
                lastClipLength = text.length();
                if (lastClipLength <= config.AutoClipMaxTextLen) {
                    int wordCount = Utility.getWordCount(text.toString());
                    return wordCount == 0 ? 1 : wordCount;
                } else {
                    return config.AutoClipMaxWordCount + 1;
                }
            } else {
                Toast.makeText(getContext(), getString(R.string.toast_cannot_get_clipboard), Toast.LENGTH_LONG).show();
            }
        }
        return 0;
    }

    PSBookmarkEditWindow psbEditWindow;

    void createPSBookmarkEditWindow() {
        if (psbEditWindow == null) {
            psbEditWindow = new PSBookmarkEditWindow(getActivity(), editText, psbmFilename) {
                @Override
                void notifyChanged(PSBookmarkItem item) {
                }
                @Override
                void closeNotify(){
                    psbEditWindow = null;
                    This.editText.requestFocus(View.FOCUS_UP);
                }
            };
            int x = (int)lastX;
            final int fontHeight = 32;  //TODO: １行分の高さ
            int y;
            if (popupList.getVisibility() == View.GONE){
                // popup is visible
                //TODO: こちらはまだおかしい
                //y = - ((int)lastY) + fontHeight + 4;
                y = ((int)lastY) - editText.getHeight() + fontHeight + 4;
            } else {
                // popup is invisible
                y = ((int)lastY) - editText.getHeight() + fontHeight + 4;
            }
            Log.d("PDD", "y="+y+" lastY="+lastY+" editText.Height="+editText.getHeight()+" fontHeight="+fontHeight);
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
    public void onResume() {
        super.onResume();

        if (cancel)
            return;

        jniCallback.setWordList(wordListAdapter);
        if (PSBookmarkReady) {
            psbmFM.open();
        }

        if (ndvFM.authComplete(getContext())) {
            selectFileDropbox();
        }

        if (isWordMode()){
            if (newPSWindow){
                editText.setSelection(0);
                newPSWindow = false;
            } else {
                int size = lastWordModePositions.size();
                if (size > 0){
                    int pos = lastWordModePositions.get(size-1);
                    try {
                        editText.setSelection(pos);
                    } catch(Exception e){
                        Log.e("PDP", "out of selection");
                    }
                    // pop last position
                    lastWordModePositions.remove(size - 1);
                }
            }
        } else
        if (isClipMode()) {
            // 他のアプリからの切り替えに対応するため、onCreateではなくここで。
            int wordCount = loadClipboardData();
            if (wordCount > 0) {
                Toast.makeText(getContext(), getString(R.string.msg_clipboard_loaded), Toast.LENGTH_LONG).show();
                // 前回と長さが同じ場合はpositionを移動
                int lastLength = pref.getInt(pfs.LAST_CLIP_LENGTH, 0);
                if (lastLength == lastClipLength) {
                    int position = pref.getInt(pfs.LAST_CURSOR_POS, 0);
                    try {
                        editText.setSelection(position);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
                // config.AutoClipWordCount以下の場合は自動的に検索開始
                if (wordCount <= config.AutoClipMaxWordCount){
                    savedEditTextHeight = 200;  //TODO: タイミングの違いにより正しい値を取得できないため、ここで固定値を設定
                    popupWordText(0, 0);
                }
            } else {
                editText.setText(R.string.msg_need_clipboard_data);
            }
        }

        // Bluetooth Manager //
        Utility.requestBluetoothPermission(getActivity());

        fromMain = false;

        InputMethodManager inputMethodManager = (InputMethodManager)getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        if (inputMethodManager != null)
            inputMethodManager.hideSoftInputFromWindow(editText.getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);

        setupSleepTimer();
    }

    void setupSleepTimer()
    {
        if (use_service){
            if (audioPlayService != null)
                audioPlayService.setupSleepTimer();
            return;
        }
        SleepTimerConfig config = new SleepTimerConfig();
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(getContext());
        config.load(pref);
        if (config.EnableSleepTimer) {
            if (slp == null || slp.isEnd()) {
                if (slp != null) slp.stop();
                AudioManager audioManager = (AudioManager) getContext().getSystemService(Context.AUDIO_SERVICE);
                slp = new SleepTimer(audioManager);
                slp.startByAfter(config.SLPAfter, config.SLPTransTime, config.SLPTargetVolume);
            }
        } else {
            if (slp != null){
                slp.stop();
                slp = null;
            }
        }
        if (config.EnableWakeupTimer){
            if (wku == null || wku.isEnd()) {
                if (wku != null) wku.stop();
                AudioManager audioManager = (AudioManager) getContext().getSystemService(Context.AUDIO_SERVICE);
                wku = new SleepTimer(audioManager);
                wku.startByTime(config.getWKUFromByHM(), config.WKUTransTime, config.WKUTargetVolume);
            }
        } else {
            if (wku != null){
                wku.stop();
                wku = null;
            }
        }
    }

    void clearSleepTimer()
    {
        if (use_service){
            audioPlayService.clearSleepTimer();
            return;
        }
        if (slp != null){
            slp.stop();
            slp = null;
        }
        if (wku != null){
            wku.stop();
            wku = null;
        }
    }

    // MainActivityのonBackPressedから呼ばれる(OnBackPressedListener)
    @Override
    public boolean onBackPressed() {
        if (faked) {
            getFragmentManager().beginTransaction().remove(this).commit();
        }
        return false;
    }

    int lastPosition;

    @Override
    public void onPause() {
        if (Utility.isNotEmpty(openedFilename)) {
            INetDriveFileInfo info = ndvFM.findByLocalName(openedFilename);
            if (info != null) {
                //remoteRevision = info.remoteRevision;
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

        if (lastClipLength>0) {
            SharedPreferences.Editor edit = pref.edit();
            edit.putInt(pfs.LAST_CLIP_LENGTH, lastClipLength);
            int position = editText.getSelectionStart();
            edit.putInt(pfs.LAST_CURSOR_POS, position);
            edit.commit();
        }

        if (isWordMode()){
            if (newPSWindow) {
                int pos = editText.getSelectionStart();
                lastWordModePositions.add(pos);
            }
        }

        super.onPause();
    }

    @Override
    public void onStop() {
        saveMarkPosition();
        super.onStop();
    }

    @Override
    public void onDestroyView() {
        if (wpm != null){
            wpm.clear();
        }
        if (tts != null) {
            tts.shutdown();
            tts = null;
        }
        if (!use_service){
            if (bluetoothManager != null) {
                bluetoothManager.unregister(getContext());
                bluetoothManager = null;
            }
        }
        closePSBookmarkEditWindow();
        hideAudioPlayer();
        if (PSBookmarkReady) {
            PSBookmarkReady = false;
            psbmFM.close();
        }
        if (psbmFM != null)
            psbmFM.deleteInstance();
        if (jniCallback != null) {
            jniCallback.deleteInstance();
            jniCallback = null;
        }
        if (pdicJni != null){
            pdicJni.deleteInstance();
            pdicJni = null;
        }

        super.onDestroyView();
    }

    @Override
    public void onDestroy() {
        releaseAudioPlayer();
        super.onDestroy();

        // BroadcastReceiverの解除
        if (screenOnReceiver != null) {
            getContext().unregisterReceiver(screenOnReceiver);
        }
    }

    // TODO: Rename method, update argument and hook method into UI event
    public void onButtonPressed(Uri uri) {
        if (mListener != null) {
            mListener.onFragmentInteraction(uri);
        }
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnFragmentInteractionListener) {
            mListener = (OnFragmentInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnFragmentInteractionListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    /**
     * This interface must be implemented by activities that contain this
     * fragment to allow an interaction in this fragment to be communicated
     * to the activity and potentially other fragments contained in that
     * activity.
     * <p>
     * See the Android Training lesson <a href=
     * "http://developer.android.com/training/basics/fragments/communicating.html"
     * >Communicating with Other Fragments</a> for more information.
     */
    public interface OnFragmentInteractionListener {
        // TODO: Update argument type and name
        void onFragmentInteraction(Uri uri);
    }

    @Override
    public void onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);
        MenuItem item = menu.findItem(R.id.action_goto_play_line);
        // fragmentの切替え時、このitemがnullになるときがあるため（原因不明）
        if (item != null) {
            // 動的に切り替わるメニュー
            item.setVisible(isLLMode());
        }
    }

    private String popupWordText(int start, int end) {
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
                Log.d("PDP", "start=" + pdicJni.startPos + " end=" + pdicJni.endPos + " prevstart=" + pdicJni.prevStartPos + " picked_text="+picked_text.length());
                if (pdicJni.endPos > picked_text.length()) pdicJni.endPos = picked_text.length();   // 改行の数え方の違いなのか、overするときがあるため、その対症療法
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
        Log.d("PDP", "searchLongestWord ret=" + ret);
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
            Toast.makeText(getContext(), getString(R.string.msg_no_hit_word) + word, Toast.LENGTH_SHORT).show();
        }
        return "";
    }

    void doGoogleTranslate(String text)
    {
        String apikey = pref.getString(pfs.GOOGLETRANSAPIKEY, "");
        if (Utility.isEmpty(apikey)){
            Toast.makeText(getContext(), getString(R.string.msg_require_google_trans_api_key), Toast.LENGTH_LONG).show();
            return;
        }

        startGoogleTranslate(text, apikey);
    }

    void doTTS(String text)
    {
//        if (Build.VERSION.SDK_INT >= 21){
//            // SDK 21 以上
//            CharSequence seq = text;
//            tts.speak(seq, TextToSpeech.QUEUE_FLUSH, null, "messageID");
//        } else
        {
            tts.speak(text, TextToSpeech.QUEUE_FLUSH, null);
        }
    }

    void startGoogleTranslate(String text, String apikey)
    {
        GoogleTranslator trans = new GoogleTranslator(text, apikey){
            @Override
            protected void onTranslated(String translatedText, String debugText){
                postGoogleTranslate(translatedText, debugText);
            }
        };
        trans.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    void postGoogleTranslate(String text, String debugText)
    {
        if (Utility.isEmpty(text)){
            String msg = getString(R.string.msg_cannot_get_or_connect) + " " + debugText;
            Toast.makeText(getContext(), msg, Toast.LENGTH_LONG).show();
            return;
        }
        popupListAdapter.clear();
        WordItem item = new WordItem("", text);
        popupListAdapter.add(item);
        showPopupList(true);
    }

    void showPopupList(boolean on){
        if (popupList.getVisibility() == View.GONE) {
            // to visible
            if (!on) return;
            ViewGroup.LayoutParams params;
            params = editText.getLayoutParams();
            params.height = savedEditTextHeight;
            editText.setLayoutParams(params);
            popupList.setSelection(0);
            popupList.setVisibility(View.VISIBLE);
        } else {
            // to invisible
            if (on){
                popupList.setSelection(0);
                return;
            }
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
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);
        if (isNormalMode()) {
            inflater.inflate(R.menu.menu_touch_srch, menu);
            boolean debug = pref.getBoolean(pfs.DEBUG, false);
            if (!debug){
                menu.findItem(R.id.action_loadfile_dropbox).setVisible(false);
                menu.findItem(R.id.action_savefile).setVisible(false);
                menu.findItem(R.id.action_sleep_timer).setVisible(false);
            }
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_loadfile) {
            selectFile();
        } else if (id == R.id.action_loadfile_history) {
            selectFileFromHistory();
        } else if (id == R.id.action_loadfile_dropbox) {
            downloadFileDropbox(true);
        } else if (id == R.id.action_reload_file){
            reloadFile();
        } else if (id == R.id.action_savefile){
            saveFileUI();
        } else if (id == R.id.action_goto){
            moveCursor();
        } else if (id == R.id.action_goto_play_line){
            moveCursorPlayingLine();
        } else if (id == R.id.action_viewpsb) {
            viewPSBookmarkList();
        } else if (id == R.id.action_wpm) {
            if (wpm.isRunning())
                wpm.stop();
            else
                wpm.start();
        } else if (id == R.id.action_sleep_timer) {
            clearSleepTimer();
            SleepTimerSettingFragment fragment = new SleepTimerSettingFragment();
            FragmentTransaction transaction = getFragmentManager().beginTransaction();
            transaction.addToBackStack(null);
            transaction.replace(R.id.content_frame, fragment);
            transaction.commit();
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
    private String m_strInitialDir;

    private static final int OPEN_DOCUMENT_REQUEST = 121;
    void selectFile() {
        if (false){
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.setType("*.*");
            startActivityForResult(intent, OPEN_DOCUMENT_REQUEST);
        } else {
            Intent i = new Intent().setClassName(getContext().getPackageName(), FileDirSelectionActivity.class.getName());
            i.putExtra(pfs.INITIALDIR, pref.getString(pfs.PSINITIALDIR, Utility.initialFileDirectory()));
            i.putExtra("exts", config.TextExtensions);
            startActivityForResult(i, REQUEST_CODE_SELECT_FILE);
        }
    }

    String downloadedRemoteName;

    private void loadFirstFile(String filename, String downloadedRemoteName) {
        loadFile(filename, downloadedRemoteName);
        initialLoading = true;
    }

    private void loadFile(String filename, String downloadedRemoteName) {
        initialLoading = false;
        if (useTextLoadTask) {
            this.downloadedRemoteName = downloadedRemoteName;
            String defCharset = pref.getBoolean(pfs.DEFCHARSET, false) ? config.defaultCharsetEncoding : null;
            TextLoadTask textLoadTask = new TextLoadTask(getActivity(), this, 0, defCharset);
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
            if (Utility.isLLMFile(filename)){
                editText.setText( llmManager.setup(result.toString()) );
            } else {
                editText.setText(result);
            }
            //editText.setChanged(false);
            loadFilePost(filename, downloadedRemoteName);
        }
    }

    // my original load file task
    //TODO: LoadFileTaskに移動できないか？
    class LoadFileTask extends AsyncTask {
        Context context;
        String filename;
        String remoteName;
        String text;
        boolean ok = false;

        public LoadFileTask(String filename, String remotename) {
            super();
            this.context = getContext();
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
                AssetManager asset = context.getAssets();   //TODO: これはいいんだっけ？
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
            if (Utility.isLLMFile(loadTask.filename)){
                editText.setText( llmManager.setup(loadTask.text) );
            } else {
                editText.setText(loadTask.text);
            }
            loadFilePost(loadTask.filename, loadTask.remoteName);
        } else {
            deleteProgressDialog();
            Toast.makeText(getContext(), getString(R.string.msg_file_load_failed)+" : " + loadTask.filename, Toast.LENGTH_LONG).show();
            loadFileFailed();
            llmManager.clear();
        }
    }

    void loadFilePost(String filename, String remotename) {
        deleteProgressDialog();

        // 回転するとactivityがnullで動作するための対症療法
        if (getActivity() == null){
            Log.i("PDD", "loadFilePost: activity is null");
            return;
        }

        saveMarkPosition();

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
        lastFileName = file.getName();
        getActivity().setTitle(lastFileName);

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
        //Toast.makeText(getContext(), getString(R.string.msg_file_loaded)+" : " + filename, Toast.LENGTH_SHORT).show();
        FileHistoryManager mgr = new FileHistoryManager(getContext());
        mgr.add(psbmFilename, fileEncoding);

        // setup Audio Player //
        loadFilePostAudio();

        wpm.clear();
    }
    void loadFilePostAudio()
    {
        if (use_service){
            // audioPLayServiceが有効になるまで時間がかかるための処理
            if (audioPlayService == null){
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        loadFilePostAudio();
                    }
                }, 10);
                return;
            }
        }

        boolean need_setup = true;
        boolean audioOk = false;
        if (initialLoading){
            initialLoading = false;
            if (use_service){
                if (isPlaying()){
                    // 継続再生する
                    need_setup = false;
                    audioOk = true;
                }
            }
        }

        if (need_setup){
            String audioFileName = Utility.changeExtension(openedFilename, "mp3");
            audioOk = openAudioPlayer(audioFileName);
            if (!audioOk){
                String altAudioFolder = pref.getString(pfs.AUDIOFILEFOLDER, config.getDefaultAudioFolder());
                if (Utility.isEmpty(altAudioFolder))
                    altAudioFolder = config.getDefaultAudioFolder();
                audioFileName = Utility.changePath(audioFileName, altAudioFolder);
                audioOk = openAudioPlayer(audioFileName);
            }
        } else {
            reopenAudioPlayer();
            updatePlayPause();
        }
        showAudio(audioOk);

        LLM = Utility.isLLMFile(openedFilename) && audioOk;
        if (!LLM){
            llmManager.clear();
        }

        reloadMarkPosition();
    }

    // file load エラー後処理
    void loadFileFailed(){
        closeAudioPlayer();
    }

    void saveFileUI(){
        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        builder.setMessage("UNDERCONSTRUCTION - This will be implemented in the near future")
                .setPositiveButton("OK", null);
        builder.show();
        return;
        //Intent i = new Intent().setClassName(getContext().getPackageName(), FileSaveActivity.class.getName());
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
            Toast.makeText(getContext(), getString(R.string.toast_save_failed) + " : " + file.getAbsolutePath(), Toast.LENGTH_SHORT).show();
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
            Toast.makeText(getContext(), getString(R.string.msg_file_save_failed)+" : " + saveFileName, Toast.LENGTH_LONG).show();
        }
    }

    ProgressDialog progress;

    void createProgressDialog(boolean save) {
        progress = new ProgressDialog(getContext());
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
                    dlg.show(getActivity().getFragmentManager(), "dbx app keys");
                }
            }
        }
    }

    private void reloadFile(){
        if (Utility.isEmpty(openedFilename)) return;
        if (ndvUtils.isLocalFileForRemote(openedFilename)) {
            fromDropbox = true;
            FileInfo file = new FileInfo(ndvUtils.convertToRemoteName(openedFilename));
            onFileSelect(file);
        } else {
            fromDropbox = false;
            FileInfo file = new FileInfo(openedFilename);
            onFileSelect(file);
        }
    }

    @Override
    public void onFileSelect(FileInfo file) {
        autoStartPlayMode = true;
        if (fromDropbox) {
            ndvUtils.setInitialDir(file.getParent());
            String toFileName = ndvUtils.convertToLocalName(file.getAbsolutePath());
            File toFile = new File(toFileName);
            ndvUtils.makeDir(getContext(), toFile);
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
        Intent i = new Intent().setClassName(getContext().getPackageName(), Dropbox2FileSelectionActivity.class.getName());
        i.putExtra("onlySelection", true);
        i.putExtra("exts",config.TextExtensions);
        startActivityForResult(i, REQUEST_CODE_SELECT_FILE_DBX);
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
            outputTextStream = getContext().openFileOutput(filename, Context.MODE_PRIVATE);
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
        dialog.show(getActivity().getFragmentManager(), "goto_dialog");
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
    void moveCursorPlayingLine()
    {
        if (!isLLMode() || !isPlayerOpened()) return;
        int linenum = llmManager.timestampToLine(getAudioCurrentPosition());
        if (linenum < 0) return;
        Utility.setCursorLineSelect(editText, linenum, llmManager.getLineText(linenum));
    }

    // --------------------------------------- //
    // Bookmark
    // --------------------------------------- //
    int psbmGeneration;

    private void viewPSBookmarkList() {
        Intent i = new Intent().setClassName(getContext().getPackageName(), PSBookmarkActivity.class.getName());
        i.putExtra("filename", psbmFilename);
        startActivityForResult(i, REQUEST_CODE_PSB);
        psbmGeneration = pdicJni.psbmGeneration;
    }

    private void selectFileFromHistory() {
        Intent i = new Intent().setClassName(getContext().getPackageName(), FileHistorySelectionActivity.class.getName());
        startActivityForResult(i, REQUEST_CODE_PSB);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == OPEN_DOCUMENT_REQUEST){
            if (resultCode == Activity.RESULT_OK){
                Uri uri = data.getData();
                Log.i("PDD", "uri="+uri.toString());
                Log.i("PDD", "filename="+uri.getPath());
//                getActivity().getContentResolver().query()
            }
        } else
        if (requestCode == REQUEST_CODE_PSB) {
            if (resultCode == Activity.RESULT_OK) {
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
                        autoStartPlayMode = true;
                        loadFile(filename, remotename);
                    }
                }
            } else
            if (resultCode==Activity.RESULT_CANCELED){
                if (psbmGeneration!=pdicJni.psbmGeneration) {
                    int start  = editText.getSelectionStart();  // 選択状態の保存
                    int end = editText.getSelectionEnd();
                    reloadPSBookmarks(psbmFilename);
                    editText.setSelection(start, end);          // 選択状態の復帰
                }
            }
        } else
        if (requestCode == REQUEST_CODE_SAVE){
            if (resultCode == Activity.RESULT_OK){
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
            if (resultCode == Activity.RESULT_OK) {
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
        FragmentManager fragmentManager = getFragmentManager();

        if(fragmentManager != null){
            FragmentTransaction fragmentTransaction = fragmentManager.beginTransaction();
            fragmentTransaction.replace(R.id.content_frame, this.newInstance(item.word, item.trans, false));
            fragmentTransaction.addToBackStack(null);   // BackStackを設定
            fragmentTransaction.commit();
            newPSWindow = true;
        }
    }

    void checkPSBookmarkStatus(){
        // conflictが発生していたらdialog boxで動作の選択肢を表示
    }

    // --------------------------------------- //
    // Audio Play Service
    // --------------------------------------- //
    boolean use_service = true;
    static Intent audioPlayServiceIntent;
    AudioPlayService audioPlayService;
    ServiceConnection serviceConnection;
    //Note: 必要なときにだけserviceを起動したいけど、audioPlayServiceがnon nullになるのが非同期のため難しい
    boolean startAudioPlayService(String filename) {
        if (!use_service) return false;

        // Broadcast Receiver
        if (audioPlayServiceIntent == null){
            Intent intent = new Intent(getActivity(), AudioPlayService.class);
            if (filename != null)
                intent.putExtra("filename", filename);
            getActivity().startService(intent);
            audioPlayServiceIntent = intent;
        }
        if (serviceConnection == null){
            serviceConnection = new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName componentName, IBinder iBinder) {
                    AudioPlayService.LocalBinder binder = (AudioPlayService.LocalBinder) iBinder;
                    audioPlayService = binder.getService();
                    Log.i("PDD", "AudioPlayService connected.");
                }

                @Override
                public void onServiceDisconnected(ComponentName componentName) {
                    audioPlayService = null;
                }
            };
            getActivity().bindService(audioPlayServiceIntent, serviceConnection, Context.BIND_AUTO_CREATE);
        }
        return true;
    }
    void stopAudioPlayService(){
        if (!use_service) return;
        if (audioPlayServiceIntent != null) {
            getActivity().stopService(audioPlayServiceIntent);
            audioPlayServiceIntent = null;
            audioPlayService = null;
        }
    }
    // use_service == trueのときのみ生成
    void initServiceNotification() {
        if (!use_service) return;
        getActivity().registerReceiver(playStatusNotification, new IntentFilter(AudioPlayService.PlayStatusNotificationName));
    }
    void cleanupServiceNotification(){
        if (!use_service) return;
        getActivity().unregisterReceiver(playStatusNotification);
    }
    private BroadcastReceiver playStatusNotification = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(AudioPlayService.PlayStatusNotificationName)) {
                int status = intent.getIntExtra("status", 0);
                if (status == 0){
                    lastPlaying = false;
                } else {
                    lastPlaying = true;
                }
                updatePlayPause();
            }
        }
    };

    // --------------------------------------- //
    // Audio Player
    // --------------------------------------- //
    private boolean autoLooping = false;
    private boolean autoStartPlay = false;
    private boolean autoStartPlayMode = false;
    private LinearLayout audioLayout;
    private SeekBar audioSlider;
    private Button btnStepRewind;
    private Button btnPlayPause;
    private Button btnMark;
    private TextView tvPosition;
    private MediaPlayer mediaPlayer;
    // play status
    private int lastPlayPosition = 0;
    private int audioDuration = 0;
    private int audioDurationSec = 0;
    private boolean lastPlaying = false;
    // mark
    enum MarkState {
        None, MarkA, MarkAB
    }
    private MarkState markState = MarkState.None;
    private int markPositionA;
    private int markPositionB;
    final int minMarkABDuration = 1000; // [msec]

    private AudioSliderUpdateThread updateThread;
    boolean isPlayerClosed()
    {
        if (use_service) return false;
        return mediaPlayer == null;
    }
    // isPlayerClosedと微妙にニュアンスが違うので注意！
    // こちらのほうが正しい実装に近い（user_serviceがなくなるまでの暫定的矛盾）
    boolean isPlayerOpened()
    {
        if (use_service) return audioPlayService != null && audioPlayService.isPlayerOpened();
        return mediaPlayer != null;
    }
    void initAudioPlayer(View view){
        audioLayout = view.findViewById(R.id.container_audio);
        audioSlider = view.findViewById(R.id.audioSeekBar);
        audioSlider.setOnSeekBarChangeListener(this);
        btnStepRewind = view.findViewById(R.id.btn_step_rewind);
        btnPlayPause = view.findViewById(R.id.btn_play_stop);
        btnMark = view.findViewById(R.id.btn_mark);
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
        btnMark.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                toggleAudioMark();
            }
        });
        tvPosition = view.findViewById(R.id.text_position);
        showAudio(false);
    }
    boolean openAudioPlayer(String filename) {
        closeAudioPlayer(false);
        if (!Utility.fileExists(filename))
            return false;

        if (use_service){
            if (!startAudioPlayService(filename)){
                Log.e("PDD", "startAudioPlayService failed");
                return false;
            }
            if (audioPlayService == null){
                Log.e("PDD", "audioPlayService is still null!?!?");
                return false;
            }
            audioPlayService.openAudioPlayer(filename);
        } else {
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
            if (!autoLooping)
                mediaPlayer.setLooping(true);
        }

        reopenAudioPlayer();

        audioPlayPause(!autoStartPlayMode && !autoStartPlay);

        clearAudioMark();

        return true;
    }
    // openはすでにしているけど、open状態にしたい場合
    void reopenAudioPlayer()
    {
        if (use_service){
            audioDuration = audioPlayService.getAudioDuration();
        } else {
            audioDuration = mediaPlayer.getDuration();
        }

        audioSlider.setProgress(lastPlayPosition);
        audioDurationSec = audioDuration / 1000;
        audioSlider.setMax(audioDuration);

        updateThread = new AudioSliderUpdateThread();
        updateThread.start();
        tvPosition.setText("sss");
    }

    // viewは破棄、serviceは終了しない
    void releaseAudioPlayer(){
        if (use_service){
            hideAudioPlayer();
            if (serviceConnection != null) {
                // Service起動中
                boolean playing = isPlaying();
                getActivity().unbindService(serviceConnection);
                serviceConnection = null;
                if (!playing){
                    // 停止しているときのみService終了
                    stopAudioPlayService();
                }
            }
            cleanupServiceNotification();
        } else {
            closeAudioPlayer();
        }
    }
    void closeAudioPlayer(){
        closeAudioPlayer(true);
    }
    void hideAudioPlayer(){
        audioLayout = null;
        btnMark = null;
        audioSlider = null;
        tvPosition = null;
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
    }
    // 完全にaudioをclose
    void closeAudioPlayer(boolean showControl){
        clearAudioMark();

        if (use_service){
            if (audioPlayService != null)
                audioPlayService.closeAudioPlayer();
            if (showControl) {
                showAudio(false);
            }
        } else {
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
    }

    int getAudioCurrentPosition(){
        if (use_service){
            if (audioPlayService == null) return 0;
            return audioPlayService.getCurrentPosition();
        } else {
            return mediaPlayer.getCurrentPosition();
        }
    }
    void seekAudioPosition(int pos){
        if (use_service){
            if (audioPlayService == null){
                Log.e("PDD", "audioPlayService is still null!?!?");
                return;
            }
            if (audioPlayService.isPlayerOpened())
                audioPlayService.seekAudioPosition(pos);
        } else {
            mediaPlayer.seekTo(pos);
        }
    }
    void audioStepRewind(){
        int pos = getAudioCurrentPosition();
        pos -= config.AudioStepRewindTime;
        if (pos < 0) pos = 0;
        seekAudioPosition(pos);
        audioSlider.setProgress(pos);
    }
    void audioPlayPause(){
        if (isPlayerClosed()) return;
        audioPlayPause(isPlaying());
    }
    // 互換性関数
    boolean isPlaying(){
        if (use_service){
            if (audioPlayService == null) return false;
            return audioPlayService.isPlaying();
        } else {
            if (mediaPlayer == null) return false;
            return mediaPlayer.isPlaying();
        }
    }
    MarkState getMarkState()
    {
        if (use_service){
            switch (audioPlayService.getMarkState()){
                case None: default: return MarkState.None;
                case MarkA: return MarkState.MarkA;
                case MarkAB: return MarkState.MarkAB;
            }
        } else {
            return markState;
        }
    }
    AudioPlayService.MarkState convertMarkState(MarkState markstate){
        switch (markstate){
            case MarkA: return AudioPlayService.MarkState.MarkA;
            case MarkAB: return AudioPlayService.MarkState.MarkAB;
            default: return AudioPlayService.MarkState.None;
        }
    }
    int getMarkPositionA(){
        if (use_service){
            return audioPlayService.getMarkPositionA();
        } else {
            return markPositionA;
        }
    }
    int getMarkPositionB(){
        if (use_service){
            return audioPlayService.getMarkPositionB();
        } else {
            return markPositionB;
        }
    }
    void setMarks(MarkState markState, int posA, int posB)
    {
        if (use_service){
            audioPlayService.setMarks(convertMarkState(markState), posA, posB);
        } else {
            this.markState = markState;
            markPositionA = posA;
            markPositionB = posB;
        }
    }
    // End of 互換性関数
    void audioPlayPause(boolean pause){
        if (use_service){
            if (audioPlayService != null)
                audioPlayService.audioPlayPause(pause);
        } else {
            if (mediaPlayer == null) return;
            if (pause){
                if (mediaPlayer.isPlaying())
                    mediaPlayer.pause();
            } else {
                if (!mediaPlayer.isPlaying())
                    mediaPlayer.start();
            }
        }
        updatePlayPause();
    }
    void updatePlayPause(){
        if (isPlaying()){
            btnPlayPause.setText(R.string.label_pause);
            lastPlaying = true;
        } else {
            btnPlayPause.setText(R.string.label_play);
            lastPlaying = false;
        }
    }
    // mark operation //
    void clearAudioMark(){
        if (use_service){
            if (audioPlayService != null)
                audioPlayService.clearAudioMark();
        } else {
            markState = MarkState.None;
        }
        if (btnMark != null)
            btnMark.setText("A");
    }
    void toggleAudioMark(){
        if (isPlayerClosed()) return;
        switch (getMarkState()){
            case None:
            default:
                setAudioMark(MarkState.MarkA, getAudioCurrentPosition(), -1, false);
                break;
            case MarkA:
                setAudioMark(MarkState.MarkAB, getMarkPositionA(), getAudioCurrentPosition(), false);
                break;
            case MarkAB:
                clearAudioMark();
                break;
        }
    }
    boolean setAudioMark(MarkState newMarkState, int newMarkA, int newMarkB, boolean force){
        if (isPlayerClosed()) return false;
        if (!force && getMarkState() == newMarkState) return false;
        int markPositionA = getMarkPositionA();
        int markPositionB = getMarkPositionB();
        switch (newMarkState){
            case MarkA:
                markPositionA = newMarkA;
                btnMark.setText("B");
                break;
            case MarkAB:
                markPositionA = newMarkA;
                int markA, markB;
                if (newMarkB  > markPositionA){
                    markA = markPositionA;
                    markB = newMarkB;
                } else {
                    markA = newMarkB;
                    markB = markPositionA;
                }
                if (markB - markA < minMarkABDuration){
                    btnMark.setText("B");
                    return false;  // ignored
                }
                markPositionA = markA;
                markPositionB = markB;
                btnMark.setText("AB");
                break;
            default:
                clearAudioMark();
                return true;
        }
        setMarks(newMarkState, markPositionA, markPositionB);
        return true;
    }

    void saveMarkPosition(){
        if (!isPlayerClosed() && Utility.isNotEmpty(openedFilename)){
            SharedPreferences.Editor edit = pref.edit();
            if (getMarkState() != MarkState.None) {
                // mark設定されている場合
                int markA = getMarkPositionA();
                int markB = getMarkPositionB();
                switch (getMarkState()) {
                    case MarkAB:
                        break;
                    default:
                        markA = -1;
                    case MarkA:
                        markB = -1;
                }
                edit.putString(pfs.LAST_AUDIOFILE, openedFilename);
                edit.putInt(pfs.LAST_AUDIO_MARK_A, markA);
                edit.putInt(pfs.LAST_AUDIO_MARK_B, markB);
                markPositionHistory.save(openedFilename, markA, markB);
            } else {
                // markが解除された場合
                String lastAudioFile = pref.getString(pfs.LAST_AUDIOFILE, "");
                if (lastAudioFile.equals(openedFilename)) {
                    edit.remove(pfs.LAST_AUDIO_MARK_A);
                    edit.remove(pfs.LAST_AUDIO_MARK_B);
                }
                edit.putString(pfs.LAST_AUDIOFILE_FOR_POS, openedFilename);
                if (isPlayerOpened()){
                    edit.putInt(pfs.LAST_AUDIO_POS, getAudioCurrentPosition());
                }
                markPositionHistory.remove(openedFilename);
            }
            edit.commit();
        }
    }
    void reloadMarkPosition(){
        if (Utility.isNotEmpty(openedFilename)) {
            int pos = -1;
            // markAB
            String lastAudioFile = pref.getString(pfs.LAST_AUDIOFILE, "");
            if (Utility.isNotEmpty(lastAudioFile)) {
                if (lastAudioFile.equals(openedFilename)) {
                    final int markA = pref.getInt(pfs.LAST_AUDIO_MARK_A, -1);
                    final int markB = pref.getInt(pfs.LAST_AUDIO_MARK_B, -1);
                    pos = setupMarkAB(markA, markB);
                }
            }

            if (pos == -1){
                final MarkPositionItem markPosItem = markPositionHistory.load(openedFilename);
                if (markPosItem != null){
                    markPositionHistory.save(openedFilename, markPosItem.markA, markPosItem.markB);
                    pos = setupMarkAB(markPosItem.markA, markPosItem.markB);
                }
            }

            if (pos == -1) {
                // audio position
                lastAudioFile = pref.getString(pfs.LAST_AUDIOFILE_FOR_POS, "");
                if (Utility.isNotEmpty(lastAudioFile)) {
                    if (lastAudioFile.equals(openedFilename)) {
                        pos = pref.getInt(pfs.LAST_AUDIO_POS, -1);
                    }
                }
            }
            if (pos>=0) {
                seekAudioPosition(pos);
            }
        }
    }
    int setupMarkAB(int markA, int markB)
    {
        MarkState markState;
        if (markA >= 0) {
            if (markB >= 0) {
                markState = MarkState.MarkAB;
            } else {
                markState = MarkState.MarkA;
            }
            if (setAudioMark(markState, markA, markB, true)) {
                seekAudioPosition(markPositionA);
            }
            return markA;
        }
        return -1;  // invalid mark
    }

    void showAudio(boolean on){
        if (audioLayout == null)
            return;

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
            seekAudioPosition(progress);
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
                    if (use_service){
                        if (!audioPlayService.isPlayerOpened()) break;  // 非同期で終了した？
                        int currentPosition = getAudioCurrentPosition();
                        Message msg = new Message();
                        msg.what = currentPosition;
                        threadHandler.sendMessage(msg);                        //ハンドラへのメッセージ送信
                    } else {
                        if (mediaPlayer != null) {
                            int currentPosition = mediaPlayer.getCurrentPosition();    //現在の再生位置を取得
                            Message msg = new Message();
                            msg.what = currentPosition;
                            threadHandler.sendMessage(msg);                        //ハンドラへのメッセージ送信
                        }
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
            int position = msg.what;
            if (!use_service){
                if (lastPlaying){
                    if (autoLooping) {
                        if (position >= audioDuration) {
                            position = 0;
                            mediaPlayer.seekTo(0);
                            mediaPlayer.start();
                        }
                    }
                    // Mark AB
                    if (markState == MarkState.MarkAB){
                        if (position >= markPositionB){
                            mediaPlayer.seekTo(markPositionA);
                            position = markPositionA;
                        }
                    }
                }
            }

            if (audioSlider != null){
                audioSlider.setProgress(position);
                int sec = position / 1000;
                String text = String.format("%d:%02d/%d:%02d", sec/60, sec % 60, audioDurationSec/60, audioDurationSec%60);
                tvPosition.setText(text);
                //tvPosition.setText( Integer.toString(sec/60) + ":" + Integer.toString(sec % 60) + "/" + Integer.toString(audioDurationSec/60) + ":" + Integer.toString(audioDurationSec%60));
            }

            if (!use_service){
                if (slp!=null){
                    slp.handleTimer();
                    if (slp.isEnd()){
                        slp.stop();
                        slp = null;
                        SleepTimerConfig config = new SleepTimerConfig();
                        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(getContext());
                        config.load(pref);
                        config.EnableSleepTimer = false;
                        config.save(pref);
                    }
                }
                if (wku!=null){
                    wku.handleTimer();
                    if (wku.isEnd()){
                        wku.stop();
                        wku = null;
                        SleepTimerConfig config = new SleepTimerConfig();
                        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(getContext());
                        config.load(pref);
                        config.EnableWakeupTimer = false;
                        config.save(pref);
                    }
                }
            }
        }
    };

    // !use_serviceのときのみ使用
    class BluetoothManager {
        private final BroadcastReceiver btReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (action.equals(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED)){
                    int state = intent.getIntExtra(BluetoothA2dp.EXTRA_STATE, BluetoothA2dp.STATE_DISCONNECTED);
                    if (state == BluetoothA2dp.STATE_DISCONNECTED){
                        audioPlayPause(true);
                    } else
                    if (state == BluetoothA2dp.STATE_CONNECTED){
                        boolean bluetoothAutoPlay = pref.getBoolean(pfs.BLUETOOTH_AUTO_PLAY, config.defaultBluetoothAutoPlay);
                        if (bluetoothAutoPlay){
                            audioPlayPause(false);
                        }
                    }
                }
            }
        };
        public BluetoothManager(){
            IntentFilter intentFilter = new IntentFilter(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED);
            intentFilter.addAction(BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED);
            getContext().registerReceiver(btReceiver, intentFilter);
        }
        public void unregister(Context context){
            context.unregisterReceiver(btReceiver);
        }
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
