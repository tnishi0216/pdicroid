package com.reliefoffice.pdic;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.net.Uri;
import android.preference.PreferenceManager;
import android.support.v4.app.Fragment;
import android.os.Bundle;
import android.support.v4.app.FragmentManager;
import android.support.v4.view.GravityCompat;
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

    Fragment curFragment;

    boolean prevRunning = false;
    boolean openPending = false;

    SharedPreferences pref;

    NavigationView navigationView;

    int lastNavItem = -1;    // 設定以外の最後の選択item
    boolean lastNavSetting = false;

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
        toolbar.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                onToolbarClicked();
            }
        });

        // Navigation Drawere //
        DrawerLayout drawer = findViewById(R.id.drawer_layout);
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(
                this, drawer, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close);
        drawer.addDrawerListener(toggle);
        toggle.syncState();

        navigationView = findViewById(R.id.nav_view);

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

        // 起動時の初期選択item
        int navItem = pref.getInt(pfs.LAST_NAV_ITEM, -1);
        switch (navItem){
            case R.id.nav_main:
            case R.id.nav_touch_search:
            case R.id.nav_clip_search:
                break;
            default:
                navItem = R.id.nav_main;
                break;
        }

        displaySelectedScreenById(navigationView, navItem);
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

        if (lastNavSetting)
            setupViewFlags();
        lastNavSetting = false;

        boolean needAllPop = false;

        switch(menuItem.getItemId()) {
            case R.id.nav_main:
            default:
                fragmentClass = IncrSrchFragment.class;
                lastNavItem = menuItem.getItemId();
                break;
            case R.id.nav_touch_search:
                fragment = TouchSrchFragment.newInstance(null, null, true);
                if (lastNavItem == R.id.nav_clip_search) needAllPop = true;
                lastNavItem = menuItem.getItemId();
                break;
            case R.id.nav_clip_search:
                fragment = TouchSrchFragment.newInstance("\\\\clip", null, true);
                if (lastNavItem == R.id.nav_touch_search) needAllPop = true;
                lastNavItem = menuItem.getItemId();
                break;
            case R.id.nav_settings:
                fragmentClass = SettingsFragmentCompat.class;
                lastNavSetting = true;
                break;
            case R.id.nav_dic_setting:
                fragmentClass = DicSettingFragment.class;
                lastNavSetting = true;
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

        if (needAllPop){
            // タッチ検索→drill mode→クリップ検索にした場合、popBackStackImmediate()でタッチ検索が再生成され、非同期なfile loadが走り、ここでの切り替え処理が終わった後、タッチ検索のfile load完了処理が走るため
            TouchSrchFragment.setCancel(true);
            while (fragmentManager.getBackStackEntryCount()>0) {
                fragmentManager.popBackStackImmediate();
            }
            TouchSrchFragment.setCancel(false);
        }

        curFragment = fragment;
        fragmentManager.beginTransaction().replace(R.id.content_frame, fragment).commit();

        // Highlight the selected item has been done by NavigationView
        menuItem.setChecked(true);
        // Set action bar title
        setTitle(menuItem.getTitle());
        // Close the navigation drawer
        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        drawer.closeDrawers();
    }

    // fragmentでtoolbar(actionbar) clickを受け取る正式な方法が見つからなかったので
    void onToolbarClicked()
    {
        if (curFragment != null && curFragment.getClass() == TouchSrchFragment.class){
            ((TouchSrchFragment)curFragment).onToolbarClicked();
        }
    }

    @Override
    public void onBackPressed() {
        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        if (drawer.isDrawerOpen(GravityCompat.START)) {
            drawer.closeDrawer(GravityCompat.START);
        } else {
            if (lastNavSetting)
                displaySelectedScreenById(navigationView, lastNavItem);
            else {
                boolean processed = false;
                Fragment fragment = getSupportFragmentManager().findFragmentById(R.id.content_frame);
                if (fragment instanceof OnBackPressedListener) {
                    processed = ((OnBackPressedListener) fragment).onBackPressed();
                }
                if (!processed)
                    super.onBackPressed();
            }
        }
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

        setupViewFlags();
    }

    void setupViewFlags(){
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
                            displaySelectedScreenById(navigationView, R.id.nav_dic_setting);
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
        edit.putInt(pfs.LAST_NAV_ITEM, lastNavItem);
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
    public void onFragmentInteraction(Uri uri) {

    }

    //TODO: これいる？
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
