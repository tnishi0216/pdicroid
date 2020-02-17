package com.reliefoffice.pdic;

import android.content.Intent;
import android.content.IntentSender;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Log;
import android.widget.Toast;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.common.api.PendingResult;
import com.google.android.gms.common.api.ResultCallback;
import com.google.android.gms.drive.Drive;
import com.google.android.gms.drive.DriveApi;
import com.google.android.gms.drive.DriveFile;
import com.google.android.gms.drive.DriveFolder;
import com.google.android.gms.drive.DriveId;
import com.google.android.gms.drive.DriveResource;
import com.google.android.gms.drive.Metadata;
import com.google.android.gms.drive.OpenFileActivityBuilder;
import com.reliefoffice.pdic.text.config;

import java.io.File;

/**
 * Created by nishikawat on 2016/05/25.
 */
public class GoogleDriveDownloadActivity extends NetDriveDownloadActivity implements GoogleApiClient.ConnectionCallbacks, GoogleApiClient.OnConnectionFailedListener {
    private static final String TAG = "PDD";
    private static final int REQUEST_CODE_OPENER = 3;

    GoogleDriveFileManager gdvFM;
    DriveId mCurrentDriveId;

    boolean first = true;

    @Override
    void createNetDriveComponents() {
        ndvUtils = GoogleDriveUtils.getInstance(getApplicationContext());
        ndvFM = GoogleDriveFileManager.getInstance(this);
        gdvFM = (GoogleDriveFileManager)ndvFM;
    }


    @Override
    void selectFile() {
        final boolean use_api = true;
        if (use_api){
            if (!first) return;
            first = false;
            IntentSender i = gdvFM.createFileSelectionIntent();
            try {
                startIntentSenderForResult(i, REQUEST_CODE_OPENER, null, 0, 0, 0);
            } catch (IntentSender.SendIntentException e) {
                Log.w(TAG, "Unable to send intent", e);
            }
        } else {
            GoogleDriveFileSelectionDialog dlg = new GoogleDriveFileSelectionDialog(this, this, (GoogleDriveFileManager) ndvFM, false);
            dlg.setOnCancelListener(new GoogleDriveFileSelectionDialog.OnCancelListener() {
                @Override
                public void onCancel() {
                    finish();
                }
            });
            dlg.show(new File(ndvUtils.getInitialDir()), config.DicTextExtensions);
        }
        fromNetDrive = true;
    }

    @Override
    protected void onResume(){
        super.onResume();
        ndvFM.startAuth(false);
    }

    @Override
    protected void onPause(){
        ndvFM.disconnect();
        super.onPause();
    }

    // Google Drive listeners //

    private static final int REQUEST_CODE_CAPTURE_IMAGE = 1;
    private static final int REQUEST_CODE_CREATOR = 2;
    private static final int REQUEST_CODE_RESOLUTION = 3;

    @Override
    public void onConnected(@Nullable Bundle bundle) {
        Log.i("PDD", "API client connected.");
        selectFile();
    }

    @Override
    public void onConnectionSuspended(int i) {

    }

    @Override
    public void onConnectionFailed(@NonNull ConnectionResult result) {
        Log.i(TAG, "GoogleApiClient connection failed: " + result.toString());
        if (!result.hasResolution()) {
            // show the localized error dialog.
            GoogleApiAvailability.getInstance().getErrorDialog(this, result.getErrorCode(), 0).show();
            return;
        }
        // The failure has a resolution. Resolve it.
        // Called typically when the app is not yet authorized, and an
        // authorization
        // dialog is displayed to the user.
        try {
            result.startResolutionForResult(this, REQUEST_CODE_RESOLUTION);
        } catch (IntentSender.SendIntentException e) {
            Log.e(TAG, "Exception while starting resolution activity", e);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode,
                                    Intent data) {
        switch (requestCode) {
            case REQUEST_CODE_CREATOR:
                if (resultCode == RESULT_OK) {
                    mCurrentDriveId = (DriveId) data.getParcelableExtra(
                            OpenFileActivityBuilder.EXTRA_RESPONSE_DRIVE_ID);
                    //refresh();
                }
                break;
            case REQUEST_CODE_OPENER:
                if (resultCode == RESULT_OK) {
                    String str = data.getDataString();
                    if (Utility.isNotEmpty(str)) {
                        String filePath = str.replace("file://", "");
                        Log.d(TAG, filePath);
                    }
                    mCurrentDriveId = (DriveId) data.getParcelableExtra(
                            OpenFileActivityBuilder.EXTRA_RESPONSE_DRIVE_ID);
                    fileSelected();
                } else {
                    finish();
                }
                break;
            case 102:
                Log.d(TAG, "Selected");
                break;
            default:
                super.onActivityResult(requestCode, resultCode, data);
        }
    }

    private void fileSelected() {
        Log.d(TAG, "Retrieving...");
        DriveFile file = mCurrentDriveId.asDriveFile();
        final PendingResult<DriveResource.MetadataResult>
                metadataResult = file.getMetadata(gdvFM.getApi());

        metadataResult.setResultCallback(new ResultCallback<DriveResource.MetadataResult>() {
            @Override
            public void onResult(@NonNull DriveResource.MetadataResult metadataResult) {
                if (metadataResult.getStatus().isSuccess()){
                    Metadata metadata = metadataResult.getMetadata();
                    String s = metadata.getTitle() + " " + metadata.getOriginalFilename() + " " + metadata.getFileExtension() + " " + metadata.getMimeType() + " " + metadata.getFileSize();
                    Log.d(TAG, "fileSelected: "+s);

                    FileInfo fileInfo = new FileInfo(null, null);
                    onFileSelect(fileInfo);
                }
            }
        });
    }
}
