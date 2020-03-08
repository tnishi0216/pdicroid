package com.reliefoffice.pdic;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.PopupMenu;
import android.widget.Toast;

import com.reliefoffice.pdic.text.pfs;

import java.io.File;
import java.util.ArrayList;

import static android.app.Activity.RESULT_OK;

/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * {@link DicSettingFragment.OnFragmentInteractionListener} interface
 * to handle interaction events.
 * Use the {@link DicSettingFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class DicSettingFragment extends Fragment implements FileSelectionDialog.OnFileSelectListener {
    // TODO: Rename parameter arguments, choose names that match
    // the fragment initialization parameters, e.g. ARG_ITEM_NUMBER
    private static final String ARG_PARAM1 = "param1";
    private static final String ARG_PARAM2 = "param2";

    // TODO: Rename and change types of parameters
    private String mParam1;
    private String mParam2;

    private OnFragmentInteractionListener mListener;

    public DicSettingFragment() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @param param1 Parameter 1.
     * @param param2 Parameter 2.
     * @return A new instance of fragment DicSettingFragment.
     */
    // TODO: Rename and change types and number of parameters
    public static DicSettingFragment newInstance(String param1, String param2) {
        DicSettingFragment fragment = new DicSettingFragment();
        Bundle args = new Bundle();
        args.putString(ARG_PARAM1, param1);
        args.putString(ARG_PARAM2, param2);
        fragment.setArguments(args);
        return fragment;
    }

    static DicSettingFragment This;

    SharedPreferences pref;
    DicPref dicPref;
    ListView lvDicList;
    ArrayAdapter<String> adpDicList;
    int lastSelDic;

    private String m_strInitialDir;


    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (getArguments() != null) {
            mParam1 = getArguments().getString(ARG_PARAM1);
            mParam2 = getArguments().getString(ARG_PARAM2);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_dic_setting, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        This = this;

        lvDicList = view.findViewById(R.id.dicList);
        adpDicList = new ArrayAdapter<String>(getContext(), android.R.layout.simple_selectable_list_item);
        lvDicList.setAdapter(adpDicList);

        lvDicList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                int num = adpDicList.getCount();
                final boolean debug = pref.getBoolean(pfs.DEBUG, false);
                if (!debug) num++;
                if (position == num - 3) {
                    // download
                    actionDownload();
                } else if (position == num - 2) {
                    // add file
                    actionAddFile();
                } else if (position == num - 1) {
                    // add file from dropbox
                    actionAddFileFromDropbox();
                } else if (position == num - 0) {
                    // add file from Google Drive
                    actionAddFileFromGoogleDrive();
                } else {
                    // context menu //
                    lastSelDic = position;
                    contextMenu(view);
                }
            }
        });

        pref = PreferenceManager.getDefaultSharedPreferences(getContext());
        dicPref = new DicPref(pref);

        updateList();
    }

    // TODO: Rename method, update argument and hook method into UI event
    public void onButtonPressed(Uri uri) {
        if (mListener != null) {
            mListener.onFragmentInteraction(uri);
        }
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnFragmentInteractionListener) {
            mListener = (OnFragmentInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnFragmentInteractionListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    @Override
    public void onResume() {
        super.onResume();

        updateList();
        m_strInitialDir = pref.getString(pfs.INITIALDIR, Utility.initialFileDirectory());
    }

    /**
     * This interface must be implemented by activities that contain this
     * fragment to allow an interaction in this fragment to be communicated
     * to the activity and potentially other fragments contained in that
     * activity.
     * <p>
     * See the Android Training lesson <a href=
     * "http://developer.android.com/training/basics/fragments/communicating.html"
     * >Communicating with Other Fragments</a> for more information.
     */
    public interface OnFragmentInteractionListener {
        // TODO: Update argument type and name
        void onFragmentInteraction(Uri uri);
    }

    void contextMenu(View view){
        // PopupMenu : refer to http://techbooster.org/android/ui/3056/
        PopupMenu popup = new PopupMenu(getContext(), view);

        popup.getMenuInflater().inflate(R.menu.dic_context_menu, popup.getMenu());

        popup.show();

        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
            public boolean onMenuItemClick(MenuItem item) {
                if (item.getItemId() == R.id.showDicInfo) {
                    showDicInfo(lastSelDic);
                } else if (item.getItemId() == R.id.removeItem) {
                    removeDic(lastSelDic);
                } else if (item.getItemId() == R.id.moveToUp) {
                    moveToUp(lastSelDic);
                } else if (item.getItemId() == R.id.moveToDown) {
                    moveToDown(lastSelDic);
                } else {
                    Toast.makeText(getContext(), "Clicked : " + item.getTitle(), Toast.LENGTH_SHORT).show();
                }
                return true;
            }
        });
    }

    void updateList(){
        adpDicList.clear();

        //ArrayList<String> dicPath = getDictionaryPath();
        ArrayList<String> dicName = dicPref.getDictionaryName();

        for (int i=0;i<dicName.size();i++) {
            if (dicName.get(i)!=null)
                adpDicList.add(dicName.get(i));
            else
                adpDicList.add("(null)");
        }

        adpDicList.add(getString(R.string.label_download));
        adpDicList.add(getString(R.string.label_add_file));
        final boolean debug = pref.getBoolean(pfs.DEBUG, false);
        if (debug) {
            adpDicList.add(getString(R.string.label_add_file_dropbox));
        }
        //adpDicList.add(getString(R.string.label_add_file_googledrive));
    }

    void actionDownload(){
        startActivity(new Intent().setClassName(getActivity().getPackageName(), DicDownloadList.class.getName()));
    }
    static final int REQUEST_CODE_ADD_FILE = 1;
    static final int REQUEST_CODE_ADD_FILE_DBX = 2;
    static final int REQUEST_CODE_ADD_FILE_GDV = 3;
    void actionAddFile(){
        Intent i = new Intent().setClassName(getActivity().getPackageName(), FileDirSelectionActivity.class.getName());
        i.putExtra(pfs.INITIALDIR, m_strInitialDir);
        startActivityForResult(i, REQUEST_CODE_ADD_FILE);
    }
    void actionAddFileFromDropbox(){
        Intent i = new Intent().setClassName(getActivity().getPackageName(), Dropbox2FileSelectionActivity.class.getName());
        startActivityForResult(i, REQUEST_CODE_ADD_FILE_DBX);
    }
    void actionAddFileFromGoogleDrive(){
        Intent i = new Intent().setClassName(getActivity().getPackageName(), GoogleDriveDownloadActivity.class.getName());
        startActivityForResult(i, REQUEST_CODE_ADD_FILE_GDV);
    }

    // Activity result handler //
    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == RESULT_OK) {
            Bundle bundle = data.getExtras();
            if (bundle != null) {
                String filename = bundle.getString("filename");
                if (Utility.isNotEmpty(filename)) {
                    if (requestCode == REQUEST_CODE_ADD_FILE) {
                        FileInfo fileInfo = new FileInfo(filename);
                        onFileSelect(fileInfo);
                    } else
                    if (requestCode == REQUEST_CODE_ADD_FILE_DBX) {
                        //String remotename = bundle.getString("remotename");
                        // The file is selected to be added.
                        File file = new File(filename);
                        String name = file.getName() + " [Dropbox]";
                        addDictionaryFile(filename, name);
                    }
                }
            }
        }
    }

    @Override
    public void onFileSelect(FileInfo file) {
        addDictionaryFile(file.getPath(), file.getName());

        m_strInitialDir = file.getParent();
        SharedPreferences.Editor edit = pref.edit();
        edit.putString(pfs.INITIALDIR, m_strInitialDir);
        edit.commit();
    }

    void addDictionaryFile(String filename, String name){
        DicInfo info = new DicInfo();
        info.filename = filename;
        info.name = name;
        if (Utility.isEmpty(info.name)){
            File file = new File(filename);
            info.name = file.getName();
        }
        dicPref.addDicInfo(info);
        Toast.makeText(getContext(), getString(R.string.msg_dictionary_added) + " : " + filename, Toast.LENGTH_SHORT).show();
        //adpDicList.insert(0, file.getParent());
        updateList();
    }


    void showDicInfo(int index){
        DicInfo info = dicPref.loadDicInfo(index);
        Toast.makeText(getContext(), "UpgradeKey="+info.upgradeKey+" URL="+info.HPUrl+" "+info.descriptoin, Toast.LENGTH_LONG).show();
    }
    void removeDic(int index){
        DicInfo info = dicPref.loadDicInfo(index);
        File file = new File(info.filename);
        //file.delete();
        dicPref.remove(index);
        adpDicList.remove((String)lvDicList.getItemAtPosition(index));
        Toast.makeText(getContext(), getString(R.string.msg_dictionary_removed)+" "+(info.name!=null ? info.name : ""), Toast.LENGTH_SHORT).show();
    }
    void moveToUp(int index){
        if (index==0) return;
        dicPref.exchange(index-1, index);
        updateList();
    }
    void moveToDown(int index){
        if (index==dicPref.getNum()-1) return;
        dicPref.exchange(index, index+1);
        updateList();
    }
}
