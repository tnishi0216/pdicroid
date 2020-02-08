package com.reliefoffice.pdic;

import android.content.Context;
import android.graphics.Color;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.List;

/**
 * Created by tnishi on 2015/06/24.
 */
// DialogのListViewに適したものとなっているため、Activity上のListViewにはデザイン的に合わない
public class FileInfoArrayAdapter extends ArrayAdapter<FileInfo> {

    private List<FileInfo> m_listFileInfo;

    public FileInfoArrayAdapter(Context context,
                                    List<FileInfo> objects )
    {
        super( context, -1, objects );

        m_listFileInfo = objects;
    }

    @Override
    public FileInfo getItem( int position )
    {
        return m_listFileInfo.get( position );
    }

    @Override
    public View getView(	int position,
                            View convertView,
                            ViewGroup parent )
    {
        // generate layout
        if( null == convertView )
        {
            Context context = getContext();
            // layout
            LinearLayout layout = new LinearLayout( context );
            layout.setPadding( 10, 10, 10, 10 );
            layout.setBackgroundColor( Color.WHITE );
            convertView = layout;
            // text
            TextView textview = new TextView( context );
            textview.setTag( "text" );
            textview.setTextColor( Color.BLACK );
            textview.setPadding( 10, 10, 10, 10 );
            layout.addView( textview );
        }

        FileInfo fileinfo = m_listFileInfo.get( position );
        TextView textview = (TextView)convertView.findViewWithTag( "text" );
        if( fileinfo.isDirectory() )
        {
            textview.setText( fileinfo.getName() + "/" );
        }
        else
        {
            textview.setText( fileinfo.getName() );
        }

        return convertView;
    }
}
