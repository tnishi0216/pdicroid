package com.reliefoffice.pdic;

import android.Manifest;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.provider.Settings;
import android.support.v4.app.ActivityCompat;
import android.support.v4.app.Fragment;
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
import android.util.Log;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.gms.common.GooglePlayServicesNotAvailableException;
import com.google.android.gms.common.GooglePlayServicesRepairableException;
import com.google.android.gms.security.ProviderInstaller;
import com.reliefoffice.pdic.text.config;
import com.reliefoffice.pdic.text.pfs;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.NoSuchAlgorithmException;
import java.text.NumberFormat;
import java.util.Calendar;
import java.util.List;
import java.util.UUID;

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
        // return Environment.getExternalStorageDirectory().getAbsolutePath();
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES).getAbsolutePath();
    }

    public static String getFileExtension(String filename)
    {
        int position = filename.lastIndexOf(".");
        if (position == -1){
            // 拡張子がない場合
            return "";
        }
        return filename.substring(position + 1);
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
        String audioFileName = changeExtension(filename, "mp3");
        boolean mp3Exists = fileExists(audioFileName);
        if (!mp3Exists){
            audioFileName = changePath(audioFileName, altAudioFolder);
            mp3Exists = fileExists(audioFileName);
        }
        return mp3Exists;
    }

    //TODO: .txtのLLM:1形式は未対応
    public static boolean isLLMFile(String filename)
    {
        String extension = getFileExtension(filename);
        return extension.toLowerCase().equals("llm");
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
    // 指定行へ移動＆行選択
    public static final void setCursorLineSelect(EditText editText, int line){
        Layout layout = editText.getLayout();
        int position;
        int pos2;
        try {
            position = layout.getOffsetForHorizontal(line, 0);
            pos2 = layout.getOffsetForHorizontal(line+1, 0);
        } catch(IndexOutOfBoundsException e){
            position = editText.length();
            pos2 = position;
        }
        editText.setSelection(position, pos2);
    }
    // 指定行へ移動＆行選択
    // 論理行指定ができないため、テキストによる移動を追加
    public static final int setCursorLineSelect(EditText editText, int line, String topOfLineText){
        Layout layout = editText.getLayout();
        int position;
        int pos2;
        try {
            while (true){
                position = layout.getOffsetForHorizontal(line, 0);
                pos2 = layout.getOffsetForHorizontal(line+1, 0);
                String s = editText.getText().toString().substring(position, pos2);
                if (s.startsWith(topOfLineText)){
                    break;
                }
                line++;
            }
        } catch(IndexOutOfBoundsException e){
            position = editText.length();
            pos2 = position;
        }
        editText.setSelection(position, pos2);
        return line;
    }

    public static final String getSelectedText(EditText editText){
        final int start = editText.getSelectionStart();
        final int end = editText.getSelectionEnd();
        return editText.getText().toString().substring(start, end);
    }

    // setup view style for EditText
    public static final void setTextStyle(EditText editText, int start, int end, int style, int color, String text) {
        if (text == null) {
            try {
                text = editText.getText().toString().substring(start, end);
            } catch(IndexOutOfBoundsException e){
                return;
            }
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

    // 自分のアプリが最前面にあるかどうかをチェックするメソッド
    public static boolean isAppInForeground(Context context) {
        // アクティビティマネージャーを取得
        ActivityManager activityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);

        // 実行中のアプリの情報を取得
        List<ActivityManager.RunningAppProcessInfo> runningAppProcesses = activityManager.getRunningAppProcesses();

        // プロセスが存在する場合、最初のプロセスのパッケージ名を取得
        if (runningAppProcesses != null && !runningAppProcesses.isEmpty()) {
            String packageName = runningAppProcesses.get(0).processName;

            // パッケージ名が自分のアプリのものと一致するかどうかをチェック
            return packageName.equals(context.getPackageName());
        }

        // プロセスが存在しない場合や情報が取得できない場合はfalseを返す
        return false;
    }

    public static final int REQUEST_CODE_PERMISSION = 181;
    public static final boolean permissionGranted(int[] grantResults){
        return grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED;
    }

    public static final boolean requestStoragePermission(Activity activity){
        if (ContextCompat.checkSelfPermission(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(activity,
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    REQUEST_CODE_PERMISSION);
            return false;
        }
        return true;
    }

    public static final boolean requestStorageAllPermission(Activity activity){
        if (Build.VERSION.SDK_INT >= 30){
            if(!Environment.isExternalStorageManager()){
                try {
                    Uri uri = Uri.parse("package:" + BuildConfig.APPLICATION_ID);
                    Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, uri);
                    activity.startActivity(intent);
                } catch (Exception ex) {
                    Intent intent = new Intent();
                    intent.setAction(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                    activity.startActivity(intent);
                }
            }
        } else {
            if (ContextCompat.checkSelfPermission(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(activity,
                        new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                        REQUEST_CODE_PERMISSION);
                return false;
            }
        }
        return true;
    }

    public static final boolean requestInternetPermission(Activity activity){
        if (ContextCompat.checkSelfPermission(activity, Manifest.permission.INTERNET) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(activity,
                    new String[]{Manifest.permission.INTERNET},
                    REQUEST_CODE_PERMISSION);
            return false;
        }
        return true;
    }
    public static final boolean requestBluetoothPermission(Activity activity){
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

    // File Selector //
    public static void showSelectSAFFile(Fragment fragment, int requestId, boolean isText){
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        if (isText) {
            intent.setType("text/plain"); // テキストファイルを対象
        } else {
            // intent.setType("*/*");  // すべてのファイルを対象
            // または
            intent.setType("application/octet-stream"); // .dic を含むバイナリファイルを対象
        }
        fragment.startActivityForResult(intent, requestId);
    }
    /**
     * SAFで取得したUriからファイル名を取得する
     */
    public static String getFileNameFromUri(Uri uri, Context context) {
        String result = null;
        if (uri.getScheme().equals("content")) {
            try (android.database.Cursor cursor = context.getContentResolver().query(uri, null, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    int nameIndex = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME);
                    if (nameIndex >= 0) {
                        result = cursor.getString(nameIndex);
                    }
                }
            }
        }
        if (result == null) {
            // contentスキームで取得できなかった場合はパスから取得
            result = uri.getLastPathSegment();
        }
        return result;
    }

    /**
     * SAFで取得したUriからフルパスを推測して取得する（取得できない場合はnull）
     */
    public static String getFullPathNameFromUri(Uri uri, Context context) {
        if ("file".equalsIgnoreCase(uri.getScheme())) {
            return uri.getPath();
        } else if ("content".equalsIgnoreCase(uri.getScheme())) {
            String[] projection = { android.provider.MediaStore.MediaColumns.DATA };
            try (android.database.Cursor cursor = context.getContentResolver().query(uri, projection, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    int columnIndex = cursor.getColumnIndexOrThrow(android.provider.MediaStore.MediaColumns.DATA);
                    return cursor.getString(columnIndex);
                }
            } catch (Exception e) {
                // 取得できない場合はnull
            }
        }
        // 取得できない場合はnull
        return null;
    }

    /**
     * SAFで取得したUriからファイルサイズ（バイト数）を取得する
     * 取得できない場合は-1を返す
     */
    public static long getFileSizeFromUri(Uri uri, Context context) {
        long size = -1;
        if ("content".equals(uri.getScheme())) {
            try (android.database.Cursor cursor = context.getContentResolver().query(
                    uri, new String[]{android.provider.OpenableColumns.SIZE}, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    int sizeIndex = cursor.getColumnIndex(android.provider.OpenableColumns.SIZE);
                    if (sizeIndex >= 0) {
                        size = cursor.getLong(sizeIndex);
                    }
                }
            }
        }
        return size;
    }


    //TODO: by GitHub Copilot, Please check.
    public static String getFileEncodingFromUri(Uri uri, Context context) {
        InputStream is = null;
        try {
            is = context.getContentResolver().openInputStream(uri);
            if (is == null) return null;
            // BOMチェック
            byte[] bom = new byte[3];
            int read = is.read(bom, 0, 3);
            if (read >= 2) {
                if (bom[0] == (byte)0xFF && bom[1] == (byte)0xFE) {
                    return "utf-16le";
                }
                if (bom[0] == (byte)0xFE && bom[1] == (byte)0xFF) {
                    return "utf-16be";
                }
            }
            if (read == 3) {
                if (bom[0] == (byte)0xEF && bom[1] == (byte)0xBB && bom[2] == (byte)0xBF) {
                    return "utf-8";
                }
            }
            // BOMがなければ、UTF-8かShift_JISかを簡易判定
            // 先頭数KBだけ読む
            int size = 4096;
            byte[] buf = new byte[size];
            System.arraycopy(bom, 0, buf, 0, read);
            int n = is.read(buf, read, size - read);
            int total = (n > 0 ? n : 0) + read;
            String encoding = detectEncoding(buf, total);
            return encoding;
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        } finally {
            if (is != null) {
                try { is.close(); } catch (IOException e) {}
            }
        }
    }
    
    //TODO: by GitHub Copilot, Please check.
    // 簡易エンコーディング判定（UTF-8判定のみ、他はShift_JISとする）
    public static String detectEncoding(byte[] buf, int len) {
        int i = 0;
        boolean utf8 = false;
        while (i < len) {
            byte b = buf[i];
            if ((b & 0x80) == 0) {
                i++;
                continue;
            }
            if ((b & 0xE0) == 0xC0 && i + 1 < len &&
                (buf[i + 1] & 0xC0) == 0x80) {
                utf8 = true;
                i += 2;
                continue;
            }
            if ((b & 0xF0) == 0xE0 && i + 2 < len &&
                (buf[i + 1] & 0xC0) == 0x80 &&
                (buf[i + 2] & 0xC0) == 0x80) {
                utf8 = true;
                i += 3;
                continue;
            }
            // それ以外はShift_JISとみなす
            return "ShiftJIS";
        }
        return utf8 ? "utf-8" : "ShiftJIS";
    }

    // SAFで取得したファイルを一時ファイルにコピーする関数
    // コピーされたファイルは最終的に削除する必要あり
    public static String copySAFToTemporaryFile(Uri safUri, Context context)
    {
        String safFileName = getFileNameFromUri(safUri, context);
        long filesize = getFileSizeFromUri(safUri, context);

        boolean use_external_storage = false;
        if (filesize > 0){
            // 空き容量のチェック
            try {
                StorageManager storageManager = context.getSystemService(StorageManager.class);
                UUID appSpecificInternalDirUuid = storageManager.getUuidForPath(context.getFilesDir());
                long availableBytes = storageManager.getAllocatableBytes(appSpecificInternalDirUuid);
                if (filesize > availableBytes) {
                    UUID appSpecificExternalDirUuid = storageManager.getUuidForPath(context.getExternalFilesDir(null));
                    availableBytes = storageManager.getAllocatableBytes(appSpecificExternalDirUuid);
                    if (filesize > availableBytes) {
                        Toast.makeText(context, context.getString(R.string.msg_not_enough_space), Toast.LENGTH_SHORT).show();
                        return null;
                    }
                    use_external_storage = true;  // 外部ストレージにコピーする
                }
            } catch (Exception e) {
                // e.printStackTrace();
                // Toast.makeText(getContext(), getString(R.string.msg_failed_to_check_available_space) + e.getMessage(), Toast.LENGTH_SHORT).show();
                return null;
            }
        }

        // SAF stream open
        InputStream in;
        try {
            in = context.getContentResolver().openInputStream(safUri);
            if (in == null) {
                Toast.makeText(context, context.getString(R.string.msg_failed_to_open_temporary_file) + safUri, Toast.LENGTH_SHORT).show();
                return null;
            }
        } catch (IOException e) {
            e.printStackTrace();
            Toast.makeText(context, context.getString(R.string.msg_failed_to_open_temporary_file) + e.getMessage(), Toast.LENGTH_SHORT).show();
            return null;
        }

        File tempFile = new File(use_external_storage ? context.getExternalFilesDir(null) : context.getFilesDir(), safFileName);  // 一時ファイルのパス：
        if (tempFile.exists()) {
            tempFile.delete();
        }
        try (OutputStream out = new FileOutputStream(tempFile)) {
            byte[] buf = new byte[8192];
            int len;
            while ((len = in.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
        } catch (IOException e) {
            e.printStackTrace();
            Toast.makeText(context, context.getString(R.string.msg_failed_to_open_temporary_file) + e.getMessage(), Toast.LENGTH_SHORT).show();
            return null;
        } finally {
            try {
                in.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return tempFile.getAbsolutePath();  // copyされた一時的なfull pathを返す
    }

    // Debug
    public static final void printDate(String tag, Calendar cal)
    {
        String s = Integer.toString(cal.get(Calendar.MONTH)+1);
        s += "/";
        s += Integer.toString(cal.get(Calendar.DAY_OF_MONTH));
        s += " ";
        s += Integer.toString(cal.get(Calendar.HOUR_OF_DAY));
        s += ":";
        s += Integer.toString(cal.get(Calendar.MINUTE));
        Log.d(tag, s);
    }
}
