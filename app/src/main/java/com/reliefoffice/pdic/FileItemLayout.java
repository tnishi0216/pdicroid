package com.reliefoffice.pdic;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.text.DateFormat;
import java.text.SimpleDateFormat;

/**
 * Created by nishikawat on 2016/10/12.
 */

public class FileItemLayout extends LinearLayout {
    ImageView imageView;
    TextView textView;
    TextView nrBM;  // number of bookmarks
    TextView fileSize;
    TextView modDate;

    public FileItemLayout(Context context, AttributeSet attrs){
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate(){
        super.onFinishInflate();
        imageView = findViewById(R.id.file_list_item_icon);
        textView  = findViewById(R.id.file_list_item_text);
        nrBM = findViewById(R.id.file_list_item_nbm);
        fileSize = findViewById(R.id.file_list_item_filesize);
        modDate = findViewById(R.id.file_list_item_moddate);
    }

    public void bindView(FileInfo item){
        if (item.isDirectory()) {
            imageView.setImageDrawable(getContext().getResources().getDrawable(R.drawable.ic_folder));
            fileSize.setText("");
        } else {
            if (item.m_mp3Exists) {
                imageView.setImageDrawable(getContext().getResources().getDrawable(R.drawable.ic_textmp3_file));
            } else
            if (item.getName().toLowerCase().endsWith(".dic")){
                imageView.setImageDrawable(getContext().getResources().getDrawable(R.mipmap.ic_launcher));
            } else {
                imageView.setImageDrawable(getContext().getResources().getDrawable(R.drawable.ic_text_file));
            }
            fileSize.setText(Utility.itocs(item.getFileSize()));
        }
        textView.setText(item.getName());
        if (item.m_nrBM>0){
            nrBM.setText(" ["+String.valueOf(item.m_nrBM)+"BM]");
        } else {
            nrBM.setText("");
        }
        if (item.getModDate()!=null) {
            SimpleDateFormat sdf = new SimpleDateFormat(" HH:mm:ss");
            DateFormat df = android.text.format.DateFormat.getDateFormat(getContext());
            modDate.setText(df.format(item.getModDate()) + sdf.format(item.getModDate()));
        } else {
            modDate.setText("");
        }
    }
}
