package com.reliefoffice.pdic;

import java.io.File;

/**
 * Created by tnishi on 2015/06/24.
 */
public class FileInfo implements Comparable<FileInfo> {

    private String	m_strName;	// display name
    private File	m_file;	// file object (can be null)

    private boolean m_isDir;
    private String m_parent;

    long m_size;
    Long m_modDate;
    Long m_readDate;

    boolean m_mp3Exists = false;

    // constructor
    public FileInfo( String strName, File file )
    {
        initialize(strName, file);
    }
    // constructor
    public FileInfo( String strName, File file, boolean mp3Exists )
    {
        initialize(strName, file);
        m_mp3Exists = mp3Exists;
    }
    // constructor
    public FileInfo( String strName, boolean isDir, String parent )
    {
        m_strName = strName;
        m_isDir = isDir;
        m_parent = parent;
    }
    // constructor
    public FileInfo( String strName, boolean isDir, String parent, boolean mp3Exists )
    {
        m_strName = strName;
        m_isDir = isDir;
        m_parent = parent;
        m_mp3Exists = mp3Exists;
    }
    // constructor
    public FileInfo(String filename){
        File file = new File(filename);
        initialize(file.getName(), file);
    }
    private void initialize(String strName, File file) {
        m_strName = strName;
        m_file = file;
        m_size = file.length();
        m_modDate = file.lastModified();
    }
    public boolean isDirectory(){ return m_file!=null ? m_file.isDirectory() : m_isDir; }

    public String getName()
    {
        return m_strName;
    }
    public String getAbsolutePath() { return m_file!=null ? m_file.getAbsolutePath() : m_parent + m_strName; }
    public String getParent() { return m_file!=null ? m_file.getParent() : m_parent; }
    public String getPath() { return m_file!=null ? m_file.getPath() : m_parent; }

    public File getFile(){
        return m_file;  //Note: can be null
    }

    public long getFileSize() { return m_size; }
    public Long getModDate() { return m_modDate; }
    public Long getReadDate() { return m_readDate; }
    public void setFileSize(long size) { m_size = size; }
    public void setModDate(Long date) { m_modDate = date; }
    public void setReadDate(Long date) { m_readDate = date; }

    public void setRoot(){
        m_strName = "";
        m_isDir = true;
        m_parent = "/";
        m_file = null;
    }

    // Compare
    public int compareTo( FileInfo another )
    {
        // directory < file order
        if( true == isDirectory() && false == another.isDirectory() )
        {
            return -1;
        }
        if( false == isDirectory() && true == another.isDirectory() )
        {
            return 1;
        }

        return getName().toLowerCase().compareTo( another.getName().toLowerCase() );
    }
}
