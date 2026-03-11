package com.reliefoffice.pdic;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.content.UriPermission;
import android.database.Cursor;
import android.net.Uri;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.webkit.MimeTypeMap;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;

/**
 * SAF-related utilities moved out of Utility.java
 */
public class SAFUtility {

    // File Selector //
    public static void showSelectSAFFile(Fragment fragment, int requestId, boolean isText){
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        if (isText) {
            intent.setType("text/plain"); // テキストファイルを対象
        } else {
            intent.setType("*/*");  // すべてのファイルを対象
            // または
//            intent.setType("application/octet-stream"); // .dic を含むバイナリファイルを対象
        }
        fragment.startActivityForResult(intent, requestId);
    }
    public static boolean isSAFFilename(String filename){
        return filename.startsWith("content:/");
    }
    public static boolean hasPersistablePermission(Uri uri, Context context){
        ContentResolver resolver = context.getContentResolver();
        String targetDocId = null;
        try {
            if (DocumentsContract.isDocumentUri(context, uri)) {
                targetDocId = DocumentsContract.getDocumentId(uri);
            }
        } catch (Exception e) {
            // ignore
        }

        for (UriPermission perm : resolver.getPersistedUriPermissions()) {
            Uri permUri = perm.getUri();
            // 完全一致（ファイルURIが直接保存されている場合）
            if (permUri.equals(uri)) {
                boolean canRead = perm.isReadPermission();
                boolean canWrite = perm.isWritePermission();
                Log.d("MyApp", "永続権限あり: read=" + canRead + ", write=" + canWrite);
                return true;
            }

            // ツリー権限が付与されている場合、そのツリー配下のファイルを許可する
            try {
                if (DocumentsContract.isTreeUri(permUri)) {
                    String treeDocId = DocumentsContract.getTreeDocumentId(permUri);
                    if (targetDocId == null) {
                        if (DocumentsContract.isDocumentUri(context, uri)) {
                            targetDocId = DocumentsContract.getDocumentId(uri);
                        }
                    }
                    if (targetDocId != null) {
                        // 同一ドキュメントID、またはツリーIDをプレフィックスとして含む場合は許可とみなす
                        if (targetDocId.equals(treeDocId) || targetDocId.startsWith(treeDocId + "/")) {
                            boolean canRead = perm.isReadPermission();
                            boolean canWrite = perm.isWritePermission();
                            Log.d("MyApp", "永続ツリー権限あり: tree=" + permUri + " read=" + canRead + ", write=" + canWrite);
                            return true;
                        }
                    }
                }
            } catch (Exception e) {
                // DocumentsContract の呼び出しで失敗しても無視して次へ
            }
        }
        return false;
    }

    /**
     * 指定したドキュメントURIが、保存されているどのツリーURI配下にあるかを返す。
     * 見つからなければ null を返す。
     */
    public static Uri findPersistedTreeUriForDocument(Uri docUri, Context context){
        if (docUri == null) return null;
        ContentResolver resolver = context.getContentResolver();
        String targetDocId = null;
        try {
            if (DocumentsContract.isDocumentUri(context, docUri)){
                targetDocId = DocumentsContract.getDocumentId(docUri);
            }
        } catch (Exception e){
            // ignore
        }

        for (UriPermission perm : resolver.getPersistedUriPermissions()){
            Uri permUri = perm.getUri();
            try {
                if (DocumentsContract.isTreeUri(permUri)){
                    String treeDocId = DocumentsContract.getTreeDocumentId(permUri);
                    if (treeDocId != null && targetDocId != null){
                        if (targetDocId.equals(treeDocId) || targetDocId.startsWith(treeDocId + "/")){
                            return permUri;
                        }
                    }
                }
            } catch (Exception e){
                // ignore and continue
            }
        }
        return null;
    }

    // デバッグ用: 永続化された UriPermission をログ出力する（必要に応じて有効化）
    /*
    public static void dumpPersistedUriPermissions(Context context){
        try {
            ContentResolver resolver = context.getContentResolver();
            List<UriPermission> perms = resolver.getPersistedUriPermissions();
            Log.d("Utility", "Persisted UriPermissions count=" + perms.size());
            for (UriPermission perm : perms){
                Uri u = perm.getUri();
                boolean isTree = false;
                String treeId = null;
                try {
                    isTree = DocumentsContract.isTreeUri(u);
                    if (isTree) treeId = DocumentsContract.getTreeDocumentId(u);
                } catch (Exception e){
                    // ignore
                }
                Log.d("Utility", "perm: uri=" + u + " isTree=" + isTree + " treeId=" + treeId + " read=" + perm.isReadPermission() + " write=" + perm.isWritePermission());
            }
        } catch (Exception e){
            Log.w("Utility", "dumpPersistedUriPermissions failed: " + e.getMessage());
        }
    }
    */
    public static String getFilePathFromUri(final Uri uri, final Context context) {
        String docId = DocumentsContract.getDocumentId(uri);

        // raw: パスが直接含まれているケース
        if (docId.startsWith("raw:")) {
            return docId.substring(4);
        }

        // msf: 形式など、数値に変換できない場合
        if (docId.contains(":")) {
            String[] parts = docId.split(":");
            if (parts.length >= 2) {
                String type = parts[0];
                String idPart = parts[1];

                // msfの場合 → fallbackとしてDownloadディレクトリを仮定
                if ("msf".equals(type)) {
                    File downloads = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
                    Cursor cursor = context.getContentResolver().query(uri,
                            new String[]{MediaStore.MediaColumns.DISPLAY_NAME},
                            null, null, null);
                    if (cursor != null && cursor.moveToFirst()) {
                        String fileName = cursor.getString(cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.DISPLAY_NAME));
                        cursor.close();
                        return new File(downloads, fileName).getAbsolutePath();
                    }
                }

                // downloads:1234 のような場合
                try {
                    Uri contentUri = ContentUris.withAppendedId(
                            Uri.parse("content://downloads/public_downloads"),
                            Long.parseLong(idPart));
                    Cursor cursor = context.getContentResolver().query(contentUri,
                            new String[]{MediaStore.Downloads.DATA},
                            null, null, null);
                    if (cursor != null && cursor.moveToFirst()) {
                        String path = cursor.getString(cursor.getColumnIndexOrThrow(MediaStore.Downloads.DATA));
                        cursor.close();
                        return path;
                    }
                    if (cursor != null) cursor.close();
                } catch (NumberFormatException e) {
                    Log.w("PathResolver", "Cannot parse docId: " + docId);
                } catch (Exception e) {
                    Log.w("PathResolver", "Failed to query downloads URI: " + e.getMessage());
                }
            }
        }

        // 最後の手段：URIを直接問い合わせてファイル名から推定
        Cursor cursor = context.getContentResolver().query(uri,
                new String[]{MediaStore.MediaColumns.DISPLAY_NAME},
                null, null, null);
        if (cursor != null && cursor.moveToFirst()) {
            String fileName = cursor.getString(cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.DISPLAY_NAME));
            cursor.close();
            File downloads = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
            return new File(downloads, fileName).getAbsolutePath();
        }
        if (cursor != null) cursor.close();
        return null;
    }
    /**
     * SAFで取得したUriからファイル名を取得する
     */
    public static String getFileNameFromUri(Uri uri, Context context) {
        String result = null;
        if (uri.getScheme().equals("content")) {
            try (Cursor cursor = context.getContentResolver().query(uri, null, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    int nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                    if (nameIndex >= 0) {
                        result = cursor.getString(nameIndex);
                    }
                    // 拡張子がない場合、MIMEタイプから拡張子を追加
                    if (result != null && !result.contains(".")) {
                        int mimeIndex = cursor.getColumnIndex(MediaStore.MediaColumns.MIME_TYPE);
                        if (mimeIndex >= 0) {
                            String mimeType = cursor.getString(mimeIndex);
                            String extension = getExtensionFromMimeType(mimeType);
                            if (extension != null) {
                                result += "." + extension;
                            }
                        }
                    }
                }
            } catch (SecurityException e) {
                // ExternalStorageProviderなど、直接クエリできないプロバイダー対応
                // パスから取得できるかトライ
                String lastSegment = uri.getLastPathSegment();
                if (lastSegment != null && lastSegment.contains("%3A")) {
                    // エンコードされたパス（例：primary%3ADownload%2Fpdic%2Ffile.mp3）
                    // %3A と %2F をデコード
                    result = lastSegment.replace("%3A", ":").replace("%2F", "/");
                    // 最後のパスセグメントのみを取得
                    int lastSlash = result.lastIndexOf("/");
                    if (lastSlash >= 0) {
                        result = result.substring(lastSlash + 1);
                    }
                } else {
                    result = lastSegment;
                }
            } catch (Exception e) {
                // その他の例外
                e.printStackTrace();
            }
        }
        if (result == null) {
            // contentスキームで取得できなかった場合はパスから取得
            result = uri.getLastPathSegment();
        }
        return result;
    }

    private static String getExtensionFromMimeType(String mimeType) {
        if (mimeType == null) return null;
        MimeTypeMap mimeTypeMap = MimeTypeMap.getSingleton();
        return mimeTypeMap.getExtensionFromMimeType(mimeType);
    }
    public static String getBaseName(String filename, Context context) {
        if (isSAFFilename(filename)) {
            return getFileNameFromUri(Uri.parse(filename), context);
        } else {
            File file = new File(filename);
            return file.getName();
        }
    }
    public static String getFilePathFromDownloadsUri(Uri uri, Context context) {
        String docId = DocumentsContract.getDocumentId(uri);
        if (docId.startsWith("raw:")) {
            return docId.substring(4);
        }

        Cursor cursor = context.getContentResolver().query(uri,
                new String[]{MediaStore.Downloads.DATA},
                null, null, null);

        if (cursor != null && cursor.moveToFirst()) {
            String path = cursor.getString(cursor.getColumnIndexOrThrow(MediaStore.Downloads.DATA));
            cursor.close();
            return path;
        }
        if (cursor != null) cursor.close();
        return null;
    }
    /**
     * SAFで取得したUriからPath+ファイル名を取得する
     */
    public static String getFilePathFromDocumentsUri(Uri uri, Context context) {
        if (DocumentsContract.isDocumentUri(context, uri)) {
            String docId = DocumentsContract.getDocumentId(uri);
            String[] split = docId.split(":");
            String type = split[0];
            String relativePath = split.length > 1 ? split[1] : "";

            if ("primary".equalsIgnoreCase(type)) {
                return Environment.getExternalStorageDirectory() + "/" + relativePath;
            } else {
                // SDカードなど外部ストレージの場合
                return "/storage/" + type + "/" + relativePath;
            }
        }
        return null;
    }
    /**
     * Full pathからSAFのUriを取得する
     */
    public static Uri getSAFUriFromFilename(String fileString){
        return Uri.parse(fileString);
    }
    // URIオブジェクトに永続的アクセス権を設定（URIを文字列で保存したあとでも使用するため）
    public static void setPersistableUriPermission(Uri uri, int flags, Context context) {
        final int takeFlags = flags & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        context.getContentResolver().takePersistableUriPermission(uri, takeFlags);
    }

    /**
     * SAFで取得したUriからフルパスを推測して取得する（取得できない場合はnull）
     * これは正しく動作しない？
     */
    public static String getFullPathNameFromUri(Uri uri, Context context) {
        if ("file".equalsIgnoreCase(uri.getScheme())) {
            return uri.getPath();
        } else if ("content".equalsIgnoreCase(uri.getScheme())) {
            String[] projection = { MediaStore.MediaColumns.DATA };
            try (Cursor cursor = context.getContentResolver().query(uri, projection, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    int columnIndex = cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.DATA);
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
            try (Cursor cursor = context.getContentResolver().query(
                    uri, new String[]{OpenableColumns.SIZE}, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    int sizeIndex = cursor.getColumnIndex(OpenableColumns.SIZE);
                    if (sizeIndex >= 0) {
                        size = cursor.getLong(sizeIndex);
                    }
                }
            } catch (SecurityException se) {
                // 権限がない場合はフォールバックして -1 を返す
                Log.w("Utility", "getFileSizeFromUri: SecurityException querying uri=" + uri + " : " + se.getMessage());
                return -1;
            }
        }
        return size;
    }
    // FileクラスのlastModified()に相当する値をSAF Uriから取得する
    public static long getLastModifiedFromUri(Uri uri, Context context) {
        long lastModified = 0;

        // Document URIの場合（SAF経由）
        if (DocumentsContract.isDocumentUri(context, uri)) {
            Cursor cursor = null;
            try {
                cursor = context.getContentResolver().query(
                        uri,
                        new String[]{DocumentsContract.Document.COLUMN_LAST_MODIFIED},
                        null,
                        null,
                        null
                );
                if (cursor != null) {
                    if (cursor.moveToFirst()) {
                        int index = cursor.getColumnIndex(DocumentsContract.Document.COLUMN_LAST_MODIFIED);
                        if (index != -1) {
                            lastModified = cursor.getLong(index);
                        }
                    }
                }
            } catch (SecurityException se) {
                Log.w("Utility", "getLastModifiedFromUri: SecurityException querying document uri=" + uri + " : " + se.getMessage());
                // 権限がない場合は lastModified を 0 のままにしてフォールバックへ
            } finally {
                if (cursor != null) cursor.close();
            }
        }

        // 取得できなかった場合は、fallbackとしてMediaStoreを試す
        if (lastModified == 0) {
            Cursor cursor = null;
            try {
                cursor = context.getContentResolver().query(
                        uri,
                        new String[]{MediaStore.MediaColumns.DATE_MODIFIED},
                        null,
                        null,
                        null
                );
                if (cursor != null) {
                    if (cursor.moveToFirst()) {
                        int index = cursor.getColumnIndex(MediaStore.MediaColumns.DATE_MODIFIED);
                        if (index != -1) {
                            // DATE_MODIFIEDは「秒」単位なので「ミリ秒」に変換
                            lastModified = cursor.getLong(index) * 1000L;
                        }
                    }
                }
            } catch (SecurityException se) {
                Log.w("Utility", "getLastModifiedFromUri: SecurityException querying mediastore uri=" + uri + " : " + se.getMessage());
            } finally {
                if (cursor != null) cursor.close();
            }
        }

        return lastModified; // File.lastModified() と同じミリ秒単位
    }

    // by GitHub Copilot
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
            // BOMがなければ、nullを返す。
            return null;
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        } finally {
            if (is != null) {
                try { is.close(); } catch (IOException e) {}
            }
        }
    }
    public static boolean mp3FileExistsFromUri(Uri uri, Context context, String altAudioFolder){
        Uri _uri = Mp3Finder.findSiblingMp3(context, uri);
        return _uri != null;
    }
    public static void registerFileToMediaStore(String filePath, Context context){
        if (filePath == null) {
            Log.w("SAFUtility", "registerFileToMediaStore: filePath is null");
            return;
        }
        File f = new File(filePath);
        Log.d("PDX", "exists=" + f.exists() + "  path=" + filePath);

        android.media.MediaScannerConnection.scanFile(
                context,
                new String[]{filePath},  // .mp3 の実ファイルパス
                new String[]{"audio/mpeg"},
                (path, uri) -> Log.d("PDD", "Scanned: " + path + " -> " + uri)
        );
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
    // Returns the File object that would be used as temporary copy for the given SAF URI
    public static File getTemporaryFileForSAF(Uri safUri, Context context)
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

        File tempFile = new File(use_external_storage ? context.getExternalFilesDir(null) : context.getFilesDir(), safFileName);  // 一時ファイルのパス：
        return tempFile;
    }

    public static String copySAFToTemporaryFile(Uri safUri, Context context)
    {
        File tempFile = getTemporaryFileForSAF(safUri, context);
        if (tempFile == null) {
            return null; // 空き容量不足などで一時ファイルが作成できない場合は null を返す
        }

        // SAF stream open
        InputStream in;
        try {
            in = context.getContentResolver().openInputStream(safUri);
            if (in == null) {
                Toast.makeText(context, context.getString(R.string.msg_failed_to_open_temporary_file) + safUri, Toast.LENGTH_SHORT).show();
                return null;
            }
        } catch (SecurityException e) {
            // ExternalStorageProvider など、永続的なアクセス権がない場合
            Log.w("Utility", "No persistent permission for URI: " + safUri + ", Error: " + e.getMessage());
            e.printStackTrace();
            Toast.makeText(context, context.getString(R.string.msg_failed_to_open_temporary_file) + "Permission Denied: " + safUri, Toast.LENGTH_SHORT).show();
            return null;
        } catch (IOException e) {
            e.printStackTrace();
            Toast.makeText(context, context.getString(R.string.msg_failed_to_open_temporary_file) + e.getMessage(), Toast.LENGTH_SHORT).show();
            return null;
        }

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

    // コピー先のファイルが古い場合、コピーし直す
    public static String copySAFToTemporaryFileIfNewer(Uri safUri, Context context)
    {
        // 永続的なアクセス権がない場合のチェック (SAF URIの場合のみ)
        if (DocumentsContract.isDocumentUri(context, safUri) && !hasPersistablePermission(safUri, context)) {
            Log.w("Utility", "No persistent permission for SAF URI: " + safUri);
            // 権限がない場合、エラーメッセージを表示して null を返す
            Toast.makeText(context, "このファイルへのアクセス権がありません。もう一度ファイルを選択してください。", Toast.LENGTH_LONG).show();
            return null;
        }
        
        String safFileName = getFileNameFromUri(safUri, context);
        Long sourceLastModified = getLastModifiedFromUri(safUri, context);
        
        boolean use_external_storage = false;
        File tempFile = new File(context.getFilesDir(), safFileName);
        File externalTempFile = new File(context.getExternalFilesDir(null), safFileName);
        
        // 既存のファイルを確認（内部ストレージを優先）
        File existingFile = null;
        if (tempFile.exists()) {
            existingFile = tempFile;
        } else if (externalTempFile.exists()) {
            existingFile = externalTempFile;
            use_external_storage = true;
        }
        
        // コピー先のファイルが新しい場合はそのまま返す
        if (existingFile != null && sourceLastModified != null && existingFile.lastModified() >= sourceLastModified) {
            return existingFile.getAbsolutePath();
        }
        // コピー先空き容量を正確に計算するために削除
        if (existingFile != null && existingFile.exists()) {
            existingFile.delete();
        }
        
        // コピーし直す必要があるため、copySAFToTemporaryFileを呼び出す
        return copySAFToTemporaryFile(safUri, context);
    }
}
