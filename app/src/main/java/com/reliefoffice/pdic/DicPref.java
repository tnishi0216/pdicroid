package com.reliefoffice.pdic;

import android.content.SharedPreferences;
import android.util.Log;

import java.util.ArrayList;

/**
 * Created by tnishi on 2015/06/28.
 */
public class DicPref {
    public static final String PFS_DIC_PREF = "DictionaryPref";

    static final int groupIndex = 0;

    SharedPreferences pref;
    public DicPref(SharedPreferences pref){
        this.pref = pref;
    }

    String getKeyName(){
        return PFS_DIC_PREF + groupIndex + "_";
    }

    public int getNum() {
        int i;
        for (i=0;;i++){
            String s = pref.getString(getKeyName() + i, "");
            if (s.isEmpty())
                break;
        }
        return i;
    }

    // Build parameters for openDictionary()
    public String[] getNamesWithParams(){
        int num = getNum();
        String[] ret = new String[num];
        for (int i=0;i<num;i++){
            ret[i] = getNameWithParams(i);
        }
        return ret;
    }
    public String getNameWithParams(int index){
        DicInfo dicInfo = loadDicInfo(index);
        if (dicInfo==null) return null;
        String ret = "|";
        ret += "r1";
        ret += "|" + dicInfo.filename;
        return ret;
    }

    public ArrayList<String> getDictionaryPath() {
        //return getPrefArray(PFS_DICTIONARY_PATH);
        ArrayList<String> array  = new ArrayList<String>();
        for (int i=0;;i++){
            DicInfo info = loadDicInfo(i);
            if (info==null) break;
            array.add(info.filename);
        }
        return array;
    }
    public ArrayList<String> getDictionaryName(){
        //return getPrefArray(PFS_DICTIONARY_NAME);
        ArrayList<String> array  = new ArrayList<String>();
        for (int i=0;;i++){
            DicInfo info = loadDicInfo(i);
            if (info==null) break;
            array.add(info.name);
        }
        return array;
    }

    public String getDictionaryPath(int index){
        DicInfo info = loadDicInfo(index);
        if (info==null) return "";
        return info.filename;
    }

    public void remove(int index){
        int src = 0;
        int dst = 0;
        SharedPreferences.Editor edit = pref.edit();
        for (;;){
            String s = pref.getString(getKeyName() + src, "");
            if (s.isEmpty())
                break;
            if (src==index) {
                // remove index
            } else {
                edit.putString(getKeyName() + dst, s);
                dst++;
            }
            src++;
        }
        edit.remove(getKeyName()+dst);
        edit.commit();
    }

    public void exchange(int index1, int index2){
        String s1 = pref.getString(getKeyName()+index1, "");
        String s2 = pref.getString(getKeyName()+index2, "");
        SharedPreferences.Editor edit = pref.edit();
        edit.putString(getKeyName()+index2, s1);
        edit.putString(getKeyName()+index1, s2);
        edit.commit();
    }

    public DicInfo loadDicInfo(int index){
        String _s = loadDicPref(index);
        if (_s.isEmpty()) return null;

        String[] s = _s.split("\t");
        DicInfo info = new DicInfo();
        int length = s.length;
        if (length>0){ info.filename = s[0];
        if (length>1){ info.name = s[1];
        if (length>2){ info.upgradeKey = s[2];
        if (length>3){ info.version = s[3];
        if (length>4){ info.date = s[4];
        if (length>5){ info.descriptoin = s[5];
        if (length>6){ info.HPUrl = s[6];
        if (length>7){ info.DLUrl = s[7];
        if (length>8){ info.ndvPath = s[8];
        if (length>9){ info.ndvRevision = s[9];
        }}}}}}}}}}
        return info;
    }
    public void addDicInfo(DicInfo info){
        saveDicInfo(getNum(), info);
    }
    final private static String nonull(String s){
        if (Utility.isEmpty(s)) return "";
        return s;
    }
    public void saveDicInfo(int index, DicInfo info){
        saveDicPref(index, info.filename + "\t" + nonull(info.name) + "\t" + nonull(info.upgradeKey) + "\t" + nonull(info.version) + "\t" + nonull(info.date) + "\t" + nonull(info.descriptoin) + "\t" + nonull(info.HPUrl) + "\t" + nonull(info.DLUrl) + "\t" + nonull(info.ndvPath) + "\t" + nonull(info.ndvRevision));
    }

    // dicpref format
    // path[\t]name[\t]upgradekey[\t]version[\t]date[\t]description[\t]HPUrl[\t]DLUrl[\t]remotePath[\t]revision
    private String loadDicPref(int index){
        return pref.getString(getKeyName()+index, "");
    }
    private void saveDicPref(int index, String prefs){
        Log.d("PDP", "saveDicPref:"+prefs);
        SharedPreferences.Editor edit = pref.edit();
        edit.putString(getKeyName()+index, prefs);
        edit.commit();
    }

    public int findDictionaryIndex(String filename){
        for (int i=0;;i++){
            DicInfo info = loadDicInfo(i);
            if (info==null) return -1;
            if (info.filename.equals(filename)){
                return i;
            }
        }
    }
}
