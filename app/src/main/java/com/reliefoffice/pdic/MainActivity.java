package com.reliefoffice.pdic;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.support.v4.app.Fragment;
import android.os.Bundle;
import android.support.v4.app.FragmentManager;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;
import android.support.design.widget.NavigationView;

import com.reliefoffice.pdic.text.config;
import com.reliefoffice.pdic.text.pfs;

public class MainActivity extends AppCompatActivity implements IncrSrchFragment.OnFragmentInteractionListener, TouchSrchFragment.OnFragmentInteractionListener, DicSettingFragment.OnFragmentInteractionListener {
    static final String PFS_RUNNING = "Running";

    PdicJni pdicJni;

    boolean prevRunning = false;
    boolean openPending = false;

    final Handler handler = new Handler();

    SharedPreferences pref;

    // NetDrive //
    INetDriveFileManager ndvFM;

    DictionaryManager dicMan;
    boolean dicOpened = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        // Navigation Drawere //
        DrawerLayout drawer = findViewById(R.id.drawer_layout);
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(
                this, drawer, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close);
        drawer.addDrawerListener(toggle);
        toggle.syncState();

        NavigationView navigationView = findViewById(R.id.nav_view);
        //TODO: navigationView.setNavigationItemSelectedListener(this);

        // Initialize JNI.
        pdicJni = PdicJni.createInstance(this, getAssets());        // Create JNI callback
        if (pdicJni != null) {
            pdicJni.createFrame(null, 0);
        }

        pref = PreferenceManager.getDefaultSharedPreferences(this);
        prevRunning = pref.getBoolean(PFS_RUNNING, false);
        SharedPreferences.Editor edit = pref.edit();
        edit.putBoolean(PFS_RUNNING, true);
        edit.commit();

        ndvFM = DropboxFileManager.createInstance(this);

        dicMan = DictionaryManager.createInstance(this);

        setupDrawerContent(navigationView);

        displaySelectedScreenById(navigationView, R.id.nav_main);
    }

    private void setupDrawerContent(NavigationView navigationView) {
        navigationView.setNavigationItemSelectedListener(
                new NavigationView.OnNavigationItemSelectedListener() {
                    @Override
                    public boolean onNavigationItemSelected(MenuItem menuItem) {
                        displaySelectedScreen(menuItem);
                        return true;
                    }
                });
    }

    public void displaySelectedScreenById(NavigationView navView, int itemId){
        Menu menu = navView.getMenu();
        MenuItem menuItem = menu.findItem(itemId);
        displaySelectedScreen(menuItem);
    }

    public void displaySelectedScreen(MenuItem menuItem) {
        // Create a new fragment and specify the fragment to show based on nav item clicked
        Fragment fragment = null;
        Class fragmentClass = null;

        switch(menuItem.getItemId()) {
            case R.id.nav_main:
            default:
                fragmentClass = IncrSrchFragment.class;
                break;
            case R.id.nav_touch_search:
                fragment = TouchSrchFragment.newInstance(null, null);
                break;
            case R.id.nav_clip_search:
                fragment = TouchSrchFragment.newInstance("clip", null);
                break;
            case R.id.nav_settings:
                fragmentClass = SettingsFragmentCompat.class;
                break;
            case R.id.nav_dic_setting:
                fragmentClass = DicSettingFragment.class;
                break;
        }

        if (fragment == null) {
            try {
                fragment = (Fragment) fragmentClass.newInstance();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        // Insert the fragment by replacing any existing fragment
        FragmentManager fragmentManager = getSupportFragmentManager();
        fragmentManager.beginTransaction().replace(R.id.content_frame, fragment).commit();

        // Highlight the selected item has been done by NavigationView
        menuItem.setChecked(true);
        // Set action bar title
        setTitle(menuItem.getTitle());
        // Close the navigation drawer
        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        drawer.closeDrawers();
    }

    @Override
    protected void onResume() {
        super.onResume();

        if (!dicMan.isDicOpened()) {
            if (prevRunning) {
                prevRunning = false;
                showQueryOpen();
            }
            if (!openPending) {
                openDictionary();
            }
        }

        // Setup JNI config //
        int viewFlags = 0xFFFF;
        boolean showPronExp = pref.getBoolean(pfs.SHOW_PRONEXP, config.defaultShowPronExp);
        if (!showPronExp)
            viewFlags &= ~(PdicJni.VF_PRON | PdicJni.VF_EXP);
        pdicJni.config(viewFlags);
    }

    void openDictionary(){
        if (dicOpened) return;

        int num = dicMan.getDictionaryNum();
        if (num==0){
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setMessage(getString(R.string.msg_nodictionary))
                    .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int id) {
                            actionDictionarySetting();
                        }
                    });
            builder.show();
            return;
        }

        int nindex = dicMan.openDictionary();
        if (dicMan.isDicOpened()) dicOpened = true;
        else {
            String name = dicMan.getDictionaryPath(-nindex-1);
            Toast ts = Toast.makeText(this, getString(R.string.msg_dic_not_opened) + "\n" + name, Toast.LENGTH_LONG);
            ts.show();
        }
    }

    void closeDictionary(){
        if (dicOpened) {
            dicMan.closeDictionary();
            dicOpened = false;
        }
    }

    void showQueryOpen(){
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(this);
        DicPref dicPref = new DicPref(pref);
        if (dicPref.getNum()==0) return;    // no dictionary

        openPending = true;
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);
        alertDialogBuilder.setTitle("Open Dictionary");
        alertDialogBuilder.setMessage( getString(R.string.msg_open_dictionary_query));
        alertDialogBuilder.setPositiveButton("Yes",
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        openPending = false;
                        openDictionary();
                    }
                });
        alertDialogBuilder.setNegativeButton("No",
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        openPending = false;
                    }
                });
        alertDialogBuilder.setCancelable(true);
        AlertDialog alertDialog = alertDialogBuilder.create();
        alertDialog.show();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onStop() {
        super.onStop();

        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(this);
        SharedPreferences.Editor edit = pref.edit();
        edit.putBoolean(PFS_RUNNING, false);
        edit.commit();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        closeDictionary();

        if (!dicMan.isDicOpened())
            ndvFM.removeAll();
        pdicJni.deleteFrame();
        if (pdicJni != null){
            pdicJni.deleteInstance();
            pdicJni = null;
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id==R.id.action_pswin){
            startTouchSearch();
        } else
        if (id==R.id.action_dictionary){
            actionDictionarySetting();
        } else
        if (id == R.id.action_settings) {
            startSettings();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    private final void startTouchSearch(){
        startActivity(new Intent().setClassName(this.getPackageName(), PSWinActivity.class.getName()));
    }

    private final void startSettings(){
        startActivity(new Intent().setClassName(this.getPackageName(), SettingsActivity2.class.getName()));
    }

    void actionDictionarySetting(){
        startActivity(new Intent().setClassName(this.getPackageName(), DicSettingActivity.class.getName()));
    }

    @Override
    public void onFragmentInteraction(Uri uri) {

    }

    //TODO: Ç±ÇÍÇ¢ÇÈÅH
    /**
     * A placeholder fragment containing a simple view.
     */
    public static class PlaceholderFragment extends Fragment {

        public PlaceholderFragment() {
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                                 Bundle savedInstanceState) {
            View rootView = inflater.inflate(R.layout.fragment_main, container, false);

            return rootView;
        }
    }
}
