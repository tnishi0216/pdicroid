package com.reliefoffice.pdic;

import java.util.Arrays;

public class LLMManager {
    int[] timestamps;

    //TODO: EditTextに論理行への移動、あるいは何か論理行返還の妙案がない限り、テキストによる比較検索
    String[] shortLines;
    final int numShortChar = 10;    // 10文字で比較

    LLMManager()
    {
    }
    // LLM fileのdurationを読み込みtimestamp - 行番号 mapを作成
    String setup(String text)
    {
        String[] lines = text.split("\n");
        timestamps = new int[lines.length];
        shortLines = new String[lines.length];
        int index = 0;
        int timestamp = 0;
        int linenum = 0;
        String newtext = "";
        for (String line : lines){
            int pos = line.indexOf(",");
            int dur = 0;
            if (pos>=1){
                dur = Integer.parseInt(line.substring(0, pos));
                pos++;
                newtext += line.substring(pos);
            } else {
                newtext += line;
                pos = 0;
            }
            newtext += "\n";
            timestamps[index] = timestamp;  // msec単位のtimestamp
            shortLines[index] = line.substring(pos, pos+numShortChar);
            timestamp += dur;
            index++;
            linenum++;
        }
        return newtext;
    }
    public void clear()
    {
        timestamps = null;
        shortLines = null;
    }
    public int timestampToLine(int timestamp)
    {
        if (timestamps == null) return -1;
        int index = Arrays.binarySearch(timestamps, timestamp);
        if (index < 0){
            index = ~index;
            if (index >= 1) index--;
        }
        return index;
    }
    public String getLineText(int line)
    {
        return shortLines[line];
    }
}
