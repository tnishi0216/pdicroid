package com.reliefoffice.pdic;

import android.app.Service;
import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothHeadset;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.preference.PreferenceManager;

import com.reliefoffice.pdic.text.config;
import com.reliefoffice.pdic.text.pfs;

import java.io.IOException;

public class AudioPlayService extends Service {
    public static final String PlayStatusNotificationName = "play_status_notification";
    public boolean isPlayerOpened(){
        return mediaPlayer != null;
    }
    public int getAudioDuration(){
        return audioDuration;
    }
    public boolean isPlaying(){
        if (mediaPlayer == null) return false;
        return mediaPlayer.isPlaying();
    }
    public int getCurrentPosition(){
        return mediaPlayer.getCurrentPosition();
    }
    public void seekAudioPosition(int pos){
        mediaPlayer.seekTo(pos);
    }

    public MarkState getMarkState(){
        return markState;
    }
    public int getMarkPositionA(){
        return markPositionA;
    }
    public int getMarkPositionB(){
        return markPositionB;
    }
    public void setMarks(MarkState markState, int posA, int posB){
        this.markState = markState;
        markPositionA = posA;
        markPositionB = posB;
    }

    private final IBinder binder = new LocalBinder();
    public class LocalBinder extends Binder {
        AudioPlayService getService() {
            return AudioPlayService.this;
        }
    }

    private MediaPlayer mediaPlayer;

    // setting
    private boolean autoLooping = false;
    private boolean autoStartPlay = false;
    private boolean autoStartPlayMode = false;

    // working
    private int lastPlayPosition = 0;
    private int audioDuration = 0;
    private int audioDurationSec = 0;
    private boolean lastPlaying = false;

    enum MarkState {
        None, MarkA, MarkAB
    }
    private MarkState markState = MarkState.None;
    private int markPositionA;
    private int markPositionB;
//    final int minMarkABDuration = 1000; // [msec]

    @Override
    public void onCreate() {
        setupBluetooth();
        super.onCreate();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // 音楽を再生
        String filename = intent.getStringExtra("filename");
        if (Utility.isNotEmpty(filename))
            openAudioPlayer(filename);
        return START_STICKY; // システムによって終了された場合に再起動する
    }

    @Override
    public void onDestroy() {
        closeAudioPlayer();
        cleanupBluetooth();
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    SharedPreferences getPref(){
        return PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
    }

    public boolean openAudioPlayer(String filename) {
        closeAudioPlayer();
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
        audioDuration = mediaPlayer.getDuration();
        audioDurationSec = audioDuration / 1000;

        if (!autoLooping)
            mediaPlayer.setLooping(true);

        updateThread = new PositionUpdateThread();
        updateThread.start();

        audioPlayPause(!autoStartPlayMode && !autoStartPlay);

        clearAudioMark();

        return true;
    }

    void closeAudioPlayer(){
        clearAudioMark();

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
        }
    }
    void audioStepRewind(){
        int pos = mediaPlayer.getCurrentPosition();
        pos -= config.AudioStepRewindTime;
        if (pos < 0) pos = 0;
        mediaPlayer.seekTo(pos);
    }
    void audioPlayPause(){
        if (mediaPlayer == null) return;
        audioPlayPause(mediaPlayer.isPlaying());
    }
    boolean audioPlayPause(boolean pause){
        if (mediaPlayer == null) return false;
        if (pause){
            if (mediaPlayer.isPlaying())
                mediaPlayer.pause();
            lastPlaying = false;
        } else {
            if (!mediaPlayer.isPlaying())
                mediaPlayer.start();
            lastPlaying = true;
        }
        return true;    // status changed
    }
    // mark operation //
    void clearAudioMark(){
        markState = MarkState.None;
    }

    void notifyPlayPosition(){
        if (mediaPlayer == null) return;

        // Service内で再生中の位置を取得
        int currentPosition = mediaPlayer.getCurrentPosition();

        // Broadcastを作成して位置情報をセット
        Intent intent = new Intent("music_position");
        intent.putExtra("position", currentPosition);

        // Broadcastを送信
        sendBroadcast(intent);
    }

    // Position update thread //
    private PositionUpdateThread updateThread;
    SleepTimer slp;
    SleepTimer wku;

    public void setupSleepTimer()
    {
        SleepTimerConfig config = new SleepTimerConfig();
        SharedPreferences pref = getPref();
        config.load(pref);
        if (config.EnableSleepTimer) {
            if (slp == null || slp.isEnd()) {
                if (slp != null) slp.stop();
                AudioManager audioManager = (AudioManager) getApplicationContext().getSystemService(Context.AUDIO_SERVICE);
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
                AudioManager audioManager = (AudioManager) getApplicationContext().getSystemService(Context.AUDIO_SERVICE);
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

    public void clearSleepTimer()
    {
        if (slp != null){
            slp.stop();
            slp = null;
        }
        if (wku != null){
            wku.stop();
            wku = null;
        }
    }

    class PositionUpdateThread extends Thread {
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
            int position = msg.what;
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

            if (slp!=null){
                slp.handleTimer();
                if (slp.isEnd()){
                    slp.stop();
                    slp = null;
                    SleepTimerConfig config = new SleepTimerConfig();
                    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
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
                    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
                    config.load(pref);
                    config.EnableWakeupTimer = false;
                    config.save(pref);
                }
            }
        }
    };

    BluetoothManager bluetoothManager;
    boolean setupBluetooth(){
        if (bluetoothManager != null) return true;  // already setup
        bluetoothManager = new BluetoothManager();
        return true;
    }
    void cleanupBluetooth(){
        if (bluetoothManager == null) return;
        bluetoothManager.unregister(getApplicationContext());
        bluetoothManager = null;
    }
    class BluetoothManager {
        private final BroadcastReceiver btReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (action.equals(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED)){
                    int state = intent.getIntExtra(BluetoothA2dp.EXTRA_STATE, BluetoothA2dp.STATE_DISCONNECTED);
                    if (state == BluetoothA2dp.STATE_DISCONNECTED){
                        if (audioPlayPause(true)){
                            notifyStatus();
                        }
                    } else
                    if (state == BluetoothA2dp.STATE_CONNECTED){
                        SharedPreferences pref = getPref();
                        boolean bluetoothAutoPlay = pref.getBoolean(pfs.BLUETOOTH_AUTO_PLAY, config.defaultBluetoothAutoPlay);
                        if (bluetoothAutoPlay){
                            if (audioPlayPause(false)){
                                notifyStatus();
                            }
                        }
                    }
                }
            }
        };
        public BluetoothManager(){
            IntentFilter intentFilter = new IntentFilter(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED);
            intentFilter.addAction(BluetoothHeadset.ACTION_CONNECTION_STATE_CHANGED);
            getApplicationContext().registerReceiver(btReceiver, intentFilter);
        }
        public void unregister(Context context){
            context.unregisterReceiver(btReceiver);
        }
    }

    // statue:
    //  0: stop/pause
    //  1: playing
    public void notifyStatus(){
        int status = lastPlaying ? 1 : 0;
        Intent intent = new Intent(PlayStatusNotificationName);
        intent.putExtra("status",status);
        sendBroadcast(intent);
    }
}
