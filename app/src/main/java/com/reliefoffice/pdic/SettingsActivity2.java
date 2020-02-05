package com.reliefoffice.pdic;

import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceActivity;

public class SettingsActivity2 extends PreferenceActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getFragmentManager().beginTransaction()
                .replace(android.R.id.content, new SettingsFragment())
                .commit();
    }

    static final public String getDefaultAudioFolder()
    {
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
    }
}
