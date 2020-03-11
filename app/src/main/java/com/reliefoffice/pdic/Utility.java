package com.reliefoffice.pdic;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Typeface;
import android.os.Environment;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.text.Editable;
import android.text.Layout;
import android.text.Selection;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.BackgroundColorSpan;
import android.text.style.CharacterStyle;
import android.text.style.StrikethroughSpan;
import android.text.style.StyleSpan;
import android.text.style.UnderlineSpan;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.gms.common.GooglePlayServicesNotAvailableException;
import com.google.android.gms.common.GooglePlayServicesRepairableException;
import com.google.android.gms.security.ProviderInstaller;
import com.reliefoffice.pdic.text.config;
import com.reliefoffice.pdic.text.pfs;

import java.io.File;
import java.security.NoSuchAlgorithmException;
import java.text.NumberFormat;

import javax.net.ssl.SSLContext;

/**
 * Created by nishikawat on 2015/07/29.
 */
public class Utility {
    public static final boolean isEmpty(String s){
        return s==null || s.isEmpty();
    }
    public static final boolean isNotEmpty(String s){
        return s!=null && !s.isEmpty();
    }

    // ３桁区切り
    public static final String itocs(int value){
        return NumberFormat.getNumberInstance().format(value);
    }
    public static final String itocs(long value){
        return NumberFormat.getNumberInstance().format(value);
    }

    public static final int getWordCount(String words){
        final String SEPARATOR = "(\\s+?|\\.|,|;)";
        return words.split(SEPARATOR).length;
    }

    public static String initialFileDirectory(){
        //return "/storage/emulated/0";
        return Environment.getExternalStorageDirectory().getAbsolutePath();
    }

    // ファイル名の拡張子を変更する
    public static final String changeExtension(String filename, String extention){
        File in = new File(filename);
        String fileName = in.getName(); // フルパスからファイル名だけを取得

        // 拡張子の文字位置を取得
        int postion = fileName.lastIndexOf(".");

        if (postion == -1) {
            // 拡張子がない場合
            return filename + "." + extention; //
        } else {
            // 拡張子がある場合
            int postionOfFullPath = filename.lastIndexOf(".");
            String pathWithoutExt = filename.substring(0, postionOfFullPath);
            return pathWithoutExt + "." + extention;
        }
    }

    // ファイル名のパスを変更する
    public static final String changePath(String filename, String path){
        File in = new File(filename);
        String fileName = in.getName(); // フルパスからファイル名だけを取得

        File out = new File(path, fileName);
        return out.getAbsolutePath();
    }

    public static boolean fileExists(String filename){
        File file = new File(filename);
        return file.exists();
    }

    // audio folderのpathを返す
    public static String altAudioFolder(SharedPreferences pref){
        String altAudioFolder = pref.getString(pfs.AUDIOFILEFOLDER, config.getDefaultAudioFolder());
        if (Utility.isEmpty(altAudioFolder))
            altAudioFolder = config.getDefaultAudioFolder();
        return altAudioFolder;
    }

    // filenameに対するmp3ファイルが存在するか？
    public static boolean mp3Exists(String filename, String altAudioFolder){
        String audioFileName = Utility.changeExtension(filename, "mp3");
        boolean mp3Exists = Utility.fileExists(audioFileName);
        if (!mp3Exists){
            audioFileName = Utility.changePath(audioFileName, altAudioFolder);
            mp3Exists = Utility.fileExists(audioFileName);
        }
        return mp3Exists;
    }

    // general purpose for EditText
    // 現在行を返す
    //TODO: 動作未確認
    public static final int getCurrentCursorLine(EditText editText){
        int selectionStart = Selection.getSelectionStart(editText.getText());
        Layout layout = editText.getLayout();
        if (selectionStart != -1) {
            return layout.getLineForOffset(selectionStart);
        }
        return -1;
    }
    // 指定行へ移動
    // 超えていた場合は最後
    public static final void setCursorLine(EditText editText, int line){
        Layout layout = editText.getLayout();
        int position;
        try {
            position = layout.getOffsetForHorizontal(line, 0);
        } catch(IndexOutOfBoundsException e){
            position = editText.length();
        }
        editText.setSelection(position);
    }

    // setup view style for EditText
    public static final void setTextStyle(EditText editText, int start, int end, int style, int color, String text) {
        if (text == null) {
            text = editText.getText().toString().substring(start, end);
        }
        Spannable word = getStyleSpan(style, color, text);

        //Log.d("PDD", "style=" + style + " color=" + color);
        Editable editable = editText.getEditableText();
        editable.replace(start, end, word);
    }
    public static final void setTextStyle(TextView textView, int style, int color, String text){
        Spannable word = getStyleSpan(style, color, text);
        textView.setText(word, TextView.BufferType.SPANNABLE);
    }
    public static final Spannable getStyleSpan(int style, int color, String text){
        Spannable word = new SpannableString(text);
        CharacterStyle styleSpan = null;

        if (style == PdicJni.BOLD) {
            styleSpan = new StyleSpan(Typeface.BOLD);
        } else if (style == PdicJni.UNDERLINE) {
            styleSpan = new UnderlineSpan();
        } else if (style == PdicJni.ITALIC) {
            styleSpan = new StyleSpan(Typeface.ITALIC);
        } else if (style == PdicJni.STRIKEOUT) {
            styleSpan = new StrikethroughSpan();
        } else if (style==0){
            removeSpannable(word);
        }
        if (color != 0) {
            styleSpan = new BackgroundColorSpan(color);
        } else {
            word.removeSpan(new BackgroundColorSpan(0));
        }
        if (styleSpan != null) {
            word.setSpan(styleSpan, 0, text.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        return word;
    }

    public static final Spannable removeSpannable(String text){
        Spannable word = new SpannableString(text);
        removeSpannable(word);
        word.removeSpan(new BackgroundColorSpan(0));
        return word;
    }
    public static final void removeSpannable( Spannable word){
        word.removeSpan(new StyleSpan(Typeface.BOLD));
        word.removeSpan(new StyleSpan(Typeface.ITALIC));
        word.removeSpan(new UnderlineSpan());
        word.removeSpan(new StrikethroughSpan());
    }

    public static final File getWorkDirectory(Context context){
        File fileDir = context.getExternalFilesDir(null);
        if (!fileDir.exists()){
            if (!fileDir.mkdir()){
                Toast ts = Toast.makeText(context, context.getString(R.string.failed_to_make_dir)+fileDir, Toast.LENGTH_LONG);
                ts.show();
                return null;
            }
        }
        return fileDir;
    }

    /*
    public static final String UriToFile(Uri uri, Context context){
        String scheme = uri.getScheme();
        String path = "";
        if ("file".equals(scheme)) {
            path = uri.getPath();
        } else if("content".equals(scheme)) {
            ContentResolver contentResolver = context.getContentResolver();
            Cursor cursor = contentResolver.query(uri, new String[] { MediaStore.MediaColumns.DATA }, null, null, null);
            if (cursor != null) {
                cursor.moveToFirst();
                path = cursor.getString(0);
                cursor.close();
            }
        }
        return path;
    }
    */

    public static final int REQUEST_CODE_PERMISSION = 181;
    public static final boolean permissionGranted(int[] grantResults){
        return grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED;
    }

    public static final boolean requestStoragePermision(Activity activity){
        if (ContextCompat.checkSelfPermission(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(activity,
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    REQUEST_CODE_PERMISSION);
            return false;
        }
        return true;
    }

    public static final boolean requestInternetPermision(Activity activity){
        if (ContextCompat.checkSelfPermission(activity, Manifest.permission.INTERNET) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(activity,
                    new String[]{Manifest.permission.INTERNET},
                    REQUEST_CODE_PERMISSION);
            return false;
        }
        return true;
    }
    public static final boolean requestBluetoothPermision(Activity activity){
        if (ContextCompat.checkSelfPermission(activity, Manifest.permission.BLUETOOTH) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(activity,
                    new String[]{Manifest.permission.BLUETOOTH},
                    REQUEST_CODE_PERMISSION);
            return false;
        }
        return true;
    }

    // Network
    public static final void initializeSSLContext(Context mContext){
        try {
            SSLContext.getInstance("TLSv1.2");
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        }
        try {
            ProviderInstaller.installIfNeeded(mContext.getApplicationContext());
        } catch (GooglePlayServicesRepairableException e) {
            e.printStackTrace();
        } catch (GooglePlayServicesNotAvailableException e) {
            e.printStackTrace();
        }
    }
}
