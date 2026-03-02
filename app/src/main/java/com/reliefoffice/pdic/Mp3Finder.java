package com.reliefoffice.pdic;

import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.MediaStore;
import android.util.Log;

public class Mp3Finder {

    private static final String TAG = "Mp3Finder";

    public static Uri   findSiblingMp3(Context context, Uri txtUri) {
        String txtFileName = null;
        String relativePath = null;

        // ① 選択された .txt の display name と relative path を取得
        try (Cursor cursor = context.getContentResolver().query(
                txtUri,
                new String[]{
                        MediaStore.Files.FileColumns.DISPLAY_NAME,
                        MediaStore.Files.FileColumns.RELATIVE_PATH
                },
                null,
                null,
                null
        )) {
            if (cursor != null && cursor.moveToFirst()) {
                txtFileName = cursor.getString(cursor.getColumnIndexOrThrow(MediaStore.Files.FileColumns.DISPLAY_NAME));
                relativePath = cursor.getString(cursor.getColumnIndexOrThrow(MediaStore.Files.FileColumns.RELATIVE_PATH));
            }
        } catch (Exception e) {
            Log.e(TAG, "query txtUri failed", e);
        }

        if (txtFileName == null) {
            Log.w(TAG, "Failed to get DISPLAY_NAME for " + txtUri);
            return null;
        }

        //TDBG: debug code
        {
            Cursor c = context.getContentResolver().query(
                MediaStore.Files.getContentUri("external"),
                new String[]{
                    MediaStore.Files.FileColumns.DISPLAY_NAME,
                    MediaStore.Files.FileColumns.MIME_TYPE,
                    MediaStore.Files.FileColumns.DATA
                },
                MediaStore.Files.FileColumns.DISPLAY_NAME + " LIKE ?",
                new String[]{"%anki-enge-voice.mp3%"},
                null
            );

            while (c != null && c.moveToNext()) {
                Log.d("PDX", "Found: " +
                    c.getString(0) + " | " +
                    c.getString(1) + " | " +
                    c.getString(2));
            }
            if (c != null) c.close();

        }


        // 対象のmp3ファイルをMediaStoreに登録する
        String txtFullPath = SAFUtility.getFilePathFromUri(txtUri, context);
        if (txtFullPath != null) {
            String mp3FullPath = txtFullPath.replaceAll("\\.txt$", ".mp3");
            SAFUtility.registerFileToMediaStore(mp3FullPath, context);
        }

        // ② ファイル名から拡張子を差し替え
        String mp3Name = txtFileName.replaceAll("\\.txt$", ".mp3");

        // ③ 同じ relative path 内で mp3 を検索
        Uri collection = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
//        Uri collection = MediaStore.Audio.Media.getContentUri(MediaStore.VOLUME_EXTERNAL);

        String selection;
        String[] selectionArgs;

        if (relativePath != null) {
            selection = MediaStore.Audio.Media.RELATIVE_PATH + "=? AND " +
                    MediaStore.Audio.Media.DISPLAY_NAME + "=?";
            selectionArgs = new String[]{relativePath, mp3Name};
        } else {
            // RELATIVE_PATHが取れなかった場合はDISPLAY_NAMEだけで検索
            selection = MediaStore.Audio.Media.DISPLAY_NAME + "=?";
            selectionArgs = new String[]{mp3Name};
        }

        try (Cursor cursor = context.getContentResolver().query(
                collection,
                new String[]{MediaStore.Audio.Media._ID},
                selection,
                selectionArgs,
                null
        )) {
            if (cursor != null && cursor.moveToFirst()) {
                long id = cursor.getLong(cursor.getColumnIndexOrThrow(MediaStore.Audio.Media._ID));
                Uri mp3Uri = ContentUris.withAppendedId(collection, id);
                Log.d(TAG, "Found mp3 file: " + mp3Uri);
                return mp3Uri;
            }
        } catch (Exception e) {
            Log.e(TAG, "query mp3 failed", e);
        }

        Log.d(TAG, "No mp3 found matching " + mp3Name);
        return null;
    }
}
