package com.reliefoffice.pdic;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.PopupWindow;
import android.widget.TextView;

//Note:
// 使用終了時に、uninit()を呼び出さないと、PSBookmarkFileManagerのinstanceが解放されない
abstract public class PSBookmarkEditWindow {
    Activity activity;
    String editFilename;    //Note: psbmFilename

    INetDriveFileManager ndvFM;
    PSBookmarkFileManager psbmFM;
    PSBookmarkItem psbmItem;    // optional, can be null
    IPSBookmarkEditor  editText;  // optional

    PopupWindow popupWindow;
    private PdicJni pdicJni;

    //Note: editFilename should be dbx:/dropbox/... if remote file exists.
    public PSBookmarkEditWindow(Activity activity, PSBookmarkItem item, String editFilename) {
        this.activity = activity;
        //psbmItem = item;   // should not be null.
        this.editFilename = editFilename;

        init(activity);

        psbmFM.open();
        psbmItem = pdicJni.getPSBookmarkItem(editFilename, item.position);
        psbmFM.close();
        if (psbmItem==null){
            psbmItem = new PSBookmarkItem(item);
            psbmItem.filename = editFilename;
        }
    }
    //Note: editFilename should be dbx:/dropbox/... if remote file exists.
    public PSBookmarkEditWindow(Activity activity, EditText editText, String editFilename) {
        this.activity = activity;
        this.editText = new PSBookmarkEditor(editText);
        this.editFilename = editFilename;

        init(activity);

        int start = editText.getSelectionStart();
        int end = editText.getSelectionEnd();
        psbmFM.open();
        psbmItem = pdicJni.getPSBookmarkItem(editFilename, start);
        psbmFM.close();
        if (psbmItem==null){
            psbmItem = new PSBookmarkItem();
            psbmItem.filename = editFilename;
            psbmItem.position = start;
            psbmItem.length = end - start;
            psbmItem.markedWord = this.editText.getText();
            psbmItem.comment = "";
        }
    }
    @Override
    protected void finalize() throws Throwable {
        try {
            uninit();
            super.finalize();
        } finally {
        }
    }
    void init(Context context){
        ndvFM = DropboxFileManager.createInstance(context);
        psbmFM = PSBookmarkFileManager.createInstance(context, ndvFM);

        // Initialize JNI.
        pdicJni = PdicJni.createInstance(null, null);   // ほかですでにinstance化されている前提

        popupWindow = new PopupWindow(activity);
        View view = activity.getLayoutInflater().inflate(R.layout.popup_menu, null);
        view.findViewById(R.id.edit_comment).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                editComment();
            }
        });
        view.findViewById(R.id.close_button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (popupWindow.isShowing()) {
                    close();
                }
            }
        });

        setupPopupMenuItem(view, R.id.tv_normal_text);
        setupPopupMenuItem(view, R.id.tv_bold_text);
        setupPopupMenuItem(view, R.id.tv_underline_text);
        setupPopupMenuItem(view, R.id.tv_italic_text);
        setupPopupMenuItem(view, R.id.tv_strikeout_text);
        setupPopupMenuItem(view, R.id.tv_strikeout_text);
        setupPopupMenuItem(view, R.id.tv_hilite_off_text);
        setupPopupMenuItem(view, R.id.tv_hilite1_text);
        setupPopupMenuItem(view, R.id.tv_hilite2_text);
        setupPopupMenuItem(view, R.id.tv_hilite3_text);
        setupPopupMenuItem(view, R.id.tv_hilite4_text);

        popupWindow.setContentView(view);

        popupWindow.setBackgroundDrawable(activity.getResources().getDrawable(R.drawable.popup_background));

        // for outside click
        popupWindow.setTouchable(true);
        //popupMenu.setFocusable(true);

        float width = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 300, activity.getResources().getDisplayMetrics());
        popupWindow.setWindowLayoutMode((int) width, WindowManager.LayoutParams.WRAP_CONTENT);
        popupWindow.setWidth((int) width);
        popupWindow.setHeight(WindowManager.LayoutParams.WRAP_CONTENT);
    }
    public void uninit()
    {
        if (psbmFM!=null) {
            psbmFM.deleteInstance();
            psbmFM = null;
        }
        if (pdicJni!=null)
            pdicJni.deleteInstance();
    }
    public void show(View anchorView){
        popupWindow.showAsDropDown(anchorView);
    }
    public void show(View anchorView, int x, int y){
        if (x==0 && y==0)
            popupWindow.showAtLocation(anchorView, Gravity.CENTER, x, y);
        else {
            popupWindow.showAsDropDown(anchorView, x, y);
        }
    }

    abstract void notifyChanged(PSBookmarkItem item);

    void setupPopupMenuItem(View view, final int resource){
        view.findViewById(resource).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setTextStyle(resource);
            }
        });
    }
    void setTextStyle(final int resource) {
        //Log.d("PDD", "resouce=" + resource);
        int style = psbmItem.style;
        int color = psbmItem.color;
        if (color!=0) color |= 0xFF000000;  //TODO: alpha support
        if (resource==R.id.tv_normal_text){
            style = 0;
        } else
        if (resource==R.id.tv_hilite_off_text){
            color = 0;
        } else
        if (resource==R.id.tv_bold_text){
            style = PdicJni.BOLD;
        } else
        if (resource==R.id.tv_underline_text){
            style = PdicJni.UNDERLINE;
        } else
        if (resource==R.id.tv_italic_text){
            style = PdicJni.ITALIC;
        } else
        if (resource==R.id.tv_strikeout_text){
            style = PdicJni.STRIKEOUT;
        } else
        if (resource==R.id.tv_hilite1_text){
            color = Color.rgb(0xff, 0x80, 0x80);
        } else
        if (resource==R.id.tv_hilite2_text){
            color = Color.GREEN;
        } else
        if (resource==R.id.tv_hilite3_text){
            color = Color.CYAN;
        } else
        if (resource==R.id.tv_hilite4_text){
            color = Color.YELLOW;
        }

        if (editText!=null) {
            int start = editText.getSelectionStart();
            int end = editText.getSelectionEnd();
            String text = editText.getText();
            setTextStyle(start, end, style, color, text);

            if (start!=psbmItem.position){
                //TODO: Is selection moved?
            }
            psbmItem.position = start;
            psbmItem.length = end - start;
            psbmItem.markedWord = text;
        }

        psbmItem.style = style;
        psbmItem.color = color & ~0xFF000000;   //TODO: alpha support
        psbmFM.open();
        pdicJni.addPSBookmark(editFilename, psbmItem);
        psbmFM.close();

        if (psbmItem.color!=0) psbmItem.color |= 0xFF000000;   //TODO: alpha support
        notifyChanged(psbmItem);
    }

    void setTextStyle(int start, int end, int style, int color, String text) {
        EditText editor = editText.getEditText();
        if (editor != null)
            Utility.setTextStyle(editor, start, end, style, color, text);
        else {
            TextView textView = editText.getTextView();
            if (textView != null)
                Utility.setTextStyle(textView, style, color, text);
            notifyChanged(psbmItem);
        }
    }

    CommentInputDialog commentInputDialog;
    void editComment(){
        commentInputDialog = new CommentInputDialog();
        commentInputDialog.setText(psbmItem.comment);
        commentInputDialog.setCallback(new CommentInputCallback() {
            @Override
            public void onCommentInputClickOk() {
                psbmItem.comment = commentInputDialog.getText();
                psbmItem.color &= ~0xFF000000;   //TODO: alpha support
                psbmFM.open();
                pdicJni.addPSBookmark(editFilename, psbmItem);
                psbmFM.close();
                psbmItem.color |= 0xFF000000;   //TODO: alpha support
                notifyChanged(psbmItem);
            }

            @Override
            public void onCommentInputClickCancel() {
                commentInputDialog.dismiss();
                commentInputDialog = null;
            }
        });
        commentInputDialog.show(activity.getFragmentManager(), "test");    //TODO: what is the second argument?
    }

    public void close(){
        if (popupWindow!=null){
            popupWindow.dismiss();
            popupWindow = null;
        }
        closeNotify();
    }

    abstract void closeNotify();
}
