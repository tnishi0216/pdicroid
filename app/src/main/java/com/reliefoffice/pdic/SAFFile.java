package com.reliefoffice.pdic;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

public class SAFFile {
    private Uri uri;

    public SAFFile(String fileName) {
        this.uri = Uri.parse(fileName);
    }
    public SAFFile(Uri uri){
        this.uri = uri;
    }
    public Uri getUri() {
        return uri;
    }
    public boolean isAvailable(Context context) {
        return SAFUtility.hasPersistablePermission(uri, context);
    }

    public void setPersistablePermission(int flags, Context context) {
        final int takeFlags = flags & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        context.getContentResolver().takePersistableUriPermission(uri, takeFlags);
    }
    public String getFileName(Context context) {
        return SAFUtility.getFileNameFromUri(uri, context);
    }
    public String getFullPath(Context context) {
        return SAFUtility.getFullPathNameFromUri(uri, context);
    }
    public long getFileSize(Context context) {
        return SAFUtility.getFileSizeFromUri(uri, context);
    }
    public Long getLastModified(Context context) {
        return SAFUtility.getLastModifiedFromUri(uri, context);
    }
    public String copyToTemporaryFile(Context context) {
        return SAFUtility.copySAFToTemporaryFile(uri, context);
    }
    public String copyToTemporaryFileIfNewer(Context context) {
        return SAFUtility.copySAFToTemporaryFileIfNewer(uri, context);
    }
    public static InputStream createInputStream(String filename, Context context) throws IOException {
        if (SAFUtility.isSAFFilename(filename)) {
            return context.getContentResolver().openInputStream(Uri.parse(filename));
        } else {
            return new FileInputStream(filename);
        }
    }
}
