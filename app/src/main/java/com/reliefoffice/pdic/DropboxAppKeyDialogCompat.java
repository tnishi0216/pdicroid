package com.reliefoffice.pdic;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;

public class DropboxAppKeyDialogCompat extends DialogFragment {
    protected EditText edAppKey;
    protected EditText edAppSecret;
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState){
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        LayoutInflater inflater = (LayoutInflater)getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View content = inflater.inflate(R.layout.dialog_dbxkeys, null);
        DropboxUtils.AppKeys appKeys = DropboxUtils.getInstance(null).getAppKeys();
        edAppKey = (EditText)content.findViewById(R.id.edit_text_app_key);
        edAppKey.setText(appKeys.key);
        builder.setView(content);
        builder.setMessage("Dropbox App Keys")
                .setNegativeButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        //View view = getView();
                        //EditText edAppKey = (EditText) view.findViewById(R.id.edit_text_app_key);
                        //EditText edAppSecret = (EditText) view.findViewById(R.id.edit_text_app_secret);
                        String appKey = edAppKey.getText().toString();
                        String appSecret = "";
                        DropboxUtils.AppKeys appKeys = new DropboxUtils.AppKeys(appKey, appSecret);
                        onDropboxAppKeys(appKeys);
                        dismiss();
                    }
                });
        return builder.create();
    }

    protected void onDropboxAppKeys(DropboxUtils.AppKeys appKeys){}
}
