package com.reliefoffice.pdic;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;
import android.widget.EditText;

import org.w3c.dom.Comment;

/**
 * Created by tnishi on 2015/08/02.
 */
public class CommentInputDialog extends TextInputDialog {
    public CommentInputDialog()
    {
        titleText = getString(R.string.title_input_comment);
    }
}
