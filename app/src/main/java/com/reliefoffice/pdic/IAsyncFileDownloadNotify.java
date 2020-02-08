package com.reliefoffice.pdic;

/**
 * Created by tnishi on 2015/06/24.
 */
public interface IAsyncFileDownloadNotify {
    void finished(boolean result, String errMsg);
}
