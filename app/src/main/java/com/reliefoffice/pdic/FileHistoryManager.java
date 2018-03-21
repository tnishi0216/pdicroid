package com.reliefoffice.pdic;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;

import com.reliefoffice.pdic.text.config;

import java.util.ArrayList;
import java.util.Date;

/**
 * Created by tnishi on 2015/09/12.
 */
public class FileHistoryManager {
    final static String PFS_FILEHISTORY = "filehistory";
    final static String PFS_FILEHISTORYDATE = "filehistorydate";
    final static String PFS_FILEENCODING = "fileencoding";
    SharedPreferences pref;

    class HistoryItem {
        String name;
        long date;
        String encoding;    // ShiftJIS utf8 utf16
        public HistoryItem(String name, long date, String encoding){
            this.name = name;
            this.date = date;
            this.encoding = encoding;
        }
        public HistoryItem(String name, String encoding){
            this.name = name;
            Date date = new Date();
            this.date = date.getTime();
            this.encoding = encoding;
        }
    }
    ArrayList<HistoryItem> fileList = new ArrayList<>();

    final static int maxNum = config.MaxFileHistoryNum;
    FileHistoryManager(Context context){
        pref = PreferenceManager.getDefaultSharedPreferences(context);
        load();
    }
    void load(){
        fileList.clear();
        for (int i=0;i<maxNum;i++){
            String pfsname = PFS_FILEHISTORY+i;
            String filename = pref.getString(pfsname, null);
            if (Utility.isEmpty(filename)){
                break;
            }
            long date = pref.getLong(PFS_FILEHISTORYDATE+i, 0);
            String encoding = pref.getString(PFS_FILEENCODING+i, "");
            fileList.add(new HistoryItem(filename, date, encoding));
            //Log.d("PDD", "filehist:"+ filename + " date:" + date);
        }
    }
    void save(){
        SharedPreferences.Editor edit = pref.edit();
        for (int i=0;i<maxNum;i++) {
            String pfsname = PFS_FILEHISTORY+i;
            if (i < fileList.size()) {
                HistoryItem item = fileList.get(i);
                edit.putString(pfsname, item.name);
                edit.putLong(PFS_FILEHISTORYDATE+i, item.date);
                edit.putString(PFS_FILEENCODING+i, item.encoding);
            } else {
                edit.putString(pfsname, null);
            }
        }
        edit.commit();
    }
    public int size(){
        return fileList.size();
    }
    public String get(int index){
        if (index<fileList.size()) {
            return fileList.get(index).name;
        }
        return null;
    }
    public Date getDate(int index){
        if (index<fileList.size()){
            return new Date(fileList.get(index).date);
        }
        return null;
    }
    public Long getDateLong(int index){
        if (index<fileList.size()){
            return new Long(fileList.get(index).date);
        }
        return null;
    }
    public String getEncoding(int index){
        if (index<fileList.size()) {
            return fileList.get(index).encoding;
        }
        return null;
    }
    public int find(String name){
        for (int i=0;i<fileList.size();i++){
            if (fileList.get(i).name.equals(name)) {
                return i;
            }
        }
        return -1;
    }
    public void add(String name, String encoding){
        int index = find(name);
        if (index == 0){
            fileList.get(0).date = new Date().getTime();
            fileList.get(0).encoding = encoding;
        } else {
            if (index != -1) {
                fileList.remove(index);
            }
            fileList.add(0, new HistoryItem(name, encoding));
            if (fileList.size() > maxNum) {
                fileList.remove(maxNum - 1);
            }
        }
        save();
    }
}
