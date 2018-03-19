package com.reliefoffice.pdic;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;
import android.widget.EditText;

/**
 * Created by tnishi on 2015/08/02.
 */
//TODO: TextInputDialogÇ©ÇÁÇÃîhê∂Ç…ÇµÇΩÇ¢Ç™ÅAconstructorÇ≈getString(R.strings.xxx)ÇÇ‚ÇÈÇ∆ActivityÇ≈ÇÕÇ»Ç¢Ç∆åæÇÌÇÍÇƒóéÇøÇÈÅionCreateDialog()Ç≈ÇÕìÆçÏÇµÇƒÇ¢ÇÈÇÃÇ…Åj
public class CommentInputDialog extends DialogFragment {
    CommentInputCallback callback;
    EditText editText;
    String initialText;
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState){
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        editText = new EditText(getActivity());
        if (initialText!=null) editText.setText(initialText);
        builder.setMessage(getString(R.string.title_input_comment))
                .setView(editText)
                .setPositiveButton("OK",new DialogInterface.OnClickListener(){
                    public void onClick(DialogInterface dialog, int id){
                        if (callback!=null) callback.onCommentInputClickOk();
                    }
                })
                .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        if (callback!=null) callback.onCommentInputClickCancel();
                    }
                });
        return builder.create();
    }
    public void setCallback(CommentInputCallback callback){
        this.callback = callback;
    }
    public void setText(String text){
        if (editText==null)
            initialText = text;
        else
            editText.setText(text);
    }
    public String getText(){
        return editText.getText().toString();
    }
}
