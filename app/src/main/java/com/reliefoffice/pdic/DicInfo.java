package com.reliefoffice.pdic;

/**
 * Created by tnishi on 2015/06/25.
 */
public class DicInfo {
    public String upgradeKey;
    public String name;
    public String version;
    public String date;
    public String filename;
    public int fileSize;
    public String descriptoin;
    public String HPUrl;
    public String DLUrl;
    public int downloadCount;
    public String ndvPath;
    public String ndvRevision;
    //public boolean ndvDownloadRequest;
    //public boolean ndvUploadRequest;
    public boolean isTemp; // true when the file was copied from SAF to app-local temporary file

    final public boolean isDbx() { return Utility.isNotEmpty(ndvPath); }
}
