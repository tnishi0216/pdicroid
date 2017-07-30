package com.reliefoffice.pdic;

import android.util.Log;
import android.widget.ArrayAdapter;
import android.widget.ListView;

import java.io.PrintStream;

/**
 * Created by tnishi on 2015/04/11.
 */
public class JniCallback {
    static JniCallback This;
    static int refCounter = 0;
    //MainActivity mainActivity;
    ListView wordList;
    WordListAdapter wordListAdapter;
    IWordListAdapter iwordListAdapter;
    boolean updated;
    int updateCounter;
    int savedFirstItem = -1;
    int savedOffset;

    IPSBookmarkListAdapter ipsBookmarkListAdapter;

    static public JniCallback createInstance(){
        if (This==null){
            This = new JniCallback();
        }
        refCounter++;
        return This;
    }
    public void deleteInstance(){
        if (refCounter>0){
            refCounter--;
            if (refCounter==0){
                This = null;
            }
        }
    }

    private JniCallback(){
        updated = false;
    }
    private JniCallback(IWordListAdapter wordListAdapter){
        this.iwordListAdapter = wordListAdapter;
        updated = false;
    }
    public void setWordListAdapter(ListView wordList, WordListAdapter wordListAdapter){
        this.wordList = wordList;
        this.wordListAdapter = wordListAdapter;
    }
    public void setWordList(IWordListAdapter adapter)
    {
        //Log.d("PDD", "setWordList: "+(adapter!=null?"NOT NULL":"null"));
        iwordListAdapter = adapter;
        updated = false;
    }
    public boolean isUpdating()
    {
        return updated;
    }
    public int getUpdateCounter() { return updateCounter; }
    public void requestUpdate()
    {
        if (updated){
            wordListAdapter.notifyDataSetChanged();
            if (savedFirstItem>=0) {
                Log.i("PDD", "Restore: "+savedFirstItem+" "+savedOffset);
                wordList.setSelectionFromTop(savedFirstItem, savedOffset);
                savedFirstItem = -1;
            }
            updated = false;
            updateCounter++;
        }
    }
    public void clearWords()
    {
        //Log.d("PDD", "clearWords\n");
        if (wordListAdapter != null)
            wordListAdapter.clear();
        savedFirstItem = -1;
        updated = true;
    }
    public int addWord(String word, String trans)
    {
        //Log.d("PDD", "addWord:" + wordListAdapter.getCount() + "\n");
        WordItem item = new WordItem(word, trans);
        if (iwordListAdapter!=null){
            iwordListAdapter.add(item);
        } else {
            wordListAdapter.add(item);
        }
        updated = true;
        return 0;
    }
    public int insertWord(int index, String word, String trans)
    {
        Log.d("PDD", "insertWord: " + index);
        if (!updated){
            saveOffset();
        }
        WordItem item = new WordItem(word, trans);
        wordListAdapter.insert(item, index);
        if (savedFirstItem>=0 && index<=savedFirstItem)
            savedFirstItem++;
        updated = true;
        return 0;
    }
    public void deleteWord(int index)
    {
        Log.d("PDD", "deleteWord: " + index + " num="+wordListAdapter.getCount());
        if (!updated){
            saveOffset();
        }
        if (index < wordListAdapter.getCount()) {
            // 2017.3.2 はっきりとした原因は掴んでいないが、ヒットしない単語を連打していると、
            // addWordでiwordListAdapter != nullのcaseが呼ばれ、JNI側の状態とJava側の状態が食い違い、
            // JNI側がpool満杯状態、Java側はなしとなってしまい、wordListAdapter.getItem(index)で落ちる
            // 対症療法として条件を追加した。
            // 再現手順：
            // この条件を外し、"ubernerd"などの辞書にない単語を連打する
            WordItem item = wordListAdapter.getItem(index);
            wordListAdapter.remove(item);
            if (savedFirstItem >= 0 && index <= savedFirstItem) {
                savedFirstItem--;
            }
            updated = true;
        }
    }
    public void select(int index, int rev)
    {
        Log.d("PDD", "select: " + index + " rev=" + rev + "\n");
        if (wordList==null) return;
        if (rev>0) {
            wordList.setSelection(index);
        }
    }

    private void saveOffset(){
        if (wordList.getCount()>0) {
            savedFirstItem = wordList.getFirstVisiblePosition();
            savedOffset = wordList.getChildAt(0).getTop();
            Log.i("PDD", "Saved: "+savedFirstItem+" "+savedOffset);
        } else {
            savedFirstItem = -1;
        }
    }

    // PSBoomark //
    PSBookmarkItem psbItem;
    public void setPSBookmarkListAdapter(IPSBookmarkListAdapter adapter){
        ipsBookmarkListAdapter = adapter;
    }
    public void setPSBookmarkItem(PSBookmarkItem item){
        psbItem = item;
    }
    public int enumPSBookmarkItem(int position, int length, int style, int color, String marked_word, String comment){
        //Log.d("PDD", "enumPSBookmarkItem callback");
        if (psbItem!=null){
            psbItem.position = position;
            psbItem.length = length;
            psbItem.style = style;
            psbItem.color = color;
            psbItem.markedWord = marked_word;
            psbItem.comment = comment;
        }
        if (ipsBookmarkListAdapter==null) return 1; // end
        return ipsBookmarkListAdapter.addItem(position, length, style, color, marked_word, comment);
    }
    public int enumPSBookmarkFile(String filename){
        if (ipsBookmarkListAdapter==null) return 1; // end
        return ipsBookmarkListAdapter.addFile(filename);
    }

}
