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
    TextView fileSize;
    TextView modDate;

    public FileItemLayout(Context context, AttributeSet attrs){
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate(){
        super.onFinishInflate();
        imageView = (ImageView)findViewById(R.id.file_list_item_icon);
        textView  = (TextView)findViewById(R.id.file_list_item_text);
        fileSize = (TextView)findViewById(R.id.file_list_item_filesize);
        modDate = (TextView)findViewById(R.id.file_list_item_moddate);
    }

    public void bindView(FileInfo item){
        if (item.isDirectory()) {
            imageView.setImageDrawable(getContext().getResources().getDrawable(R.drawable.ic_folder));
        } else {
            if (item.m_mp3Exists) {
                imageView.setImageDrawable(getContext().getResources().getDrawable(R.drawable.ic_textmp3_file));
            } else
            if (!item.getName().toLowerCase().endsWith(".dic")){
                imageView.setImageDrawable(getContext().getResources().getDrawable(R.drawable.ic_text_file));
            }
            fileSize.setText(Utility.itocs(item.getFileSize()));
        }
        textView.setText(item.getName());
        if (item.getModDate()!=null) {
            SimpleDateFormat sdf = new SimpleDateFormat(" HH:mm:ss");
            DateFormat df = android.text.format.DateFormat.getDateFormat(getContext());
            modDate.setText(df.format(item.getModDate()) + sdf.format(item.getModDate()));
        }
    }
}
