package com.reliefoffice.pdic.text;

import android.os.Environment;

/**
 * Created by nishikawat on 2015/12/17.
 */
public class config {
    public static final String defaultCharsetEncoding = "ShiftJIS";
    public static final boolean defaultShowPronExp = false; // change R.xml.preference also.
    public static final int swipeMoveMargin = 120;
    public static final int MaxFileHistoryNum = 100;
    public static final String[] DicTextExtensions = {".dic", ".txt", ".md"};
    public static final String[] TextExtensions = {".txt", ".md"};
    public static final int AudioStepRewindTime = 5000; // [msec]
    public static final String getDefaultAudioFolder()
    {
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
    }
    // この数以下ならクリップ検索を自動的に開始
    public static final int AutoClipMaxTextLen = 200;
    public static final int AutoClipMaxWordCount = 8;
}
