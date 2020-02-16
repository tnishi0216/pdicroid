package com.reliefoffice.pdic;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.ListView;
import android.widget.SearchView;

import com.reliefoffice.pdic.text.pfs;

/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * {@link IncrSrchFragment.OnFragmentInteractionListener} interface
 * to handle interaction events.
 * Use the {@link IncrSrchFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class IncrSrchFragment extends Fragment implements SearchView.OnQueryTextListener {
    private static final String ARG_PARAM1 = "param1";
    private static final String ARG_PARAM2 = "param2";

    private String mParam1;
    private String mParam2;

    SearchView searchView;

    private OnFragmentInteractionListener mListener;

    private WordListAdapter wordListAdapter;

    PdicJni pdicJni;
    private JniCallback jniCallback;

    final Handler handler = new Handler();

    public IncrSrchFragment() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @param param1 Parameter 1.
     * @param param2 Parameter 2.
     * @return A new instance of fragment IncrSrchFragment.
     */
    // TODO: Rename and change types and number of parameters
    public static IncrSrchFragment newInstance(String param1, String param2) {
        IncrSrchFragment fragment = new IncrSrchFragment();
        Bundle args = new Bundle();
        args.putString(ARG_PARAM1, param1);
        args.putString(ARG_PARAM2, param2);
        fragment.setArguments(args);
        return fragment;
    }

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
        // Create ListView for word list
        return inflater.inflate(R.layout.fragment_incr_srch, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        wordListAdapter = new WordListAdapter(getActivity(), R.layout.list_item_wordlist);

        ListView wordList =  view.findViewById(R.id.wordList);
        wordList.setAdapter(wordListAdapter);
        //wordListAdapter.notifyDataSetChanged();
        wordList.setOnScrollListener(new AbsListView.OnScrollListener() {
            boolean scrolling = false;
            int lastFirstVisibleItem = -1;
            int lastUpdateCounter = -1;
            @Override
            public void onScrollStateChanged(AbsListView absListView, int i) {
                //Log.i("PDD", "StateChanged:i="+i);
                if (i==SCROLL_STATE_TOUCH_SCROLL) {
                    scrolling = true;
                } else
                if (i==SCROLL_STATE_IDLE){
                    scrolling = false;
                }
            }

            @Override
            public void onScroll(AbsListView absListView, int firstVisibleItem, int visibleCount, int totalCount) {
                //Log.i("PDD", "Scroll: "+firstVisibleItem+" "+visibleCount+" "+totalCount);
                //TODO: consideration - We don't need these compelx processes if we can judge wheather foroward or backward scroll easily. (don't want to use lastFirstVisibleItem)
                if (jniCallback==null){
                    return;
                }
                boolean updating = jniCallback.isUpdating();
                if (jniCallback.getUpdateCounter()!=lastUpdateCounter){
                    lastUpdateCounter = jniCallback.getUpdateCounter();
                    updating = true;
                }
                if (updating) {
                    lastFirstVisibleItem = -1;
                    return;
                }

                if (lastFirstVisibleItem!=firstVisibleItem) {
                    //Log.d("PDD", "Scroll: "+firstVisibleItem+"/"+lastFirstVisibleItem+" "+visibleCount+" "+totalCount);
                    int backward;
                    int reqoffs;
                    if (lastFirstVisibleItem==-1){
                        // we cannot find forward or backward.
                        backward = -1;
                        reqoffs = 0;
                    } else
                    if (lastFirstVisibleItem < firstVisibleItem) {
                        // scroll up
                        reqoffs = firstVisibleItem + visibleCount;
                        backward = 0;
                    } else {
                        // scroll down
                        reqoffs = firstVisibleItem;
                        backward = 1;
                    }
                    lastFirstVisibleItem = firstVisibleItem;
                    //if (backward!=0) return;    // for debug
                    if (backward!=-1) {
                        if (pdicJni.requestScroll(reqoffs, backward, firstVisibleItem) > 0) {
                            Log.i("PDD", "scroll requested");
                            requestIdleProc();
                        }
                    }
                }
            }
        });

        // Initialize JNI.
        pdicJni = PdicJni.getInstance();

        jniCallback = JniCallback.createInstance();
        jniCallback.setWordListAdapter(wordList, wordListAdapter);

        int res = -1;
        if (pdicJni != null) {
            res = pdicJni.setCallback(jniCallback, 0);
        }

        // Setup Search View
        searchView = view.findViewById(R.id.searchView);
        searchView.setIconifiedByDefault(false);
        searchView.setOnQueryTextListener(this);
        searchView.setSubmitButtonEnabled(true);
        searchView.setQueryHint(getString(R.string.msg_input_word));

        if (res != 0) {
            searchView.setQueryHint("JNI init/createFrame: res=" + Integer.toString(res));
        }
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

        jniCallback.setWordList(null);

        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(getContext());
        String text = pref.getString(pfs.LAST_SRCH_WORD, "");
        if (Utility.isNotEmpty(text)) {
            searchView.setQuery(text, true);
        }
    }

    @Override
    public void onStop() {
        super.onStop();

        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(getContext());
        SharedPreferences.Editor edit = pref.edit();
        String text = searchView.getQuery().toString();
        edit.putString(pfs.LAST_SRCH_WORD, text);
        edit.commit();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();

        if (jniCallback != null) {
            jniCallback.deleteInstance();
            jniCallback = null;
        }
        if (pdicJni != null){
            pdicJni.deleteInstance();
            pdicJni = null;
        }
    }

    @Override
    public boolean onQueryTextSubmit(String s) {
        return false;
    }

    @Override
    public boolean onQueryTextChange(String newText) {
        if (TextUtils.isEmpty(newText)) {
            //Log.d("PDD", "empty");
            pdicJni.incSearch("");
            wordListAdapter.clear();
            wordListAdapter.notifyDataSetChanged();
        } else {
            //Log.d("PDD", "call incSearch1");
            //wordListAdapter.clear();
            //wordListAdapter.notifyDataSetChanged();
            pdicJni.incSearch("");
            wordListAdapter.clear();
            pdicJni.incSearch(newText);
        }
        //Log.d("PDD", "Start first idle");
        final int firstIdleCount = 30;  //TODO: need to adjust
        boolean done = false;
        for (int i = 0; i < firstIdleCount; i++) {
            if (pdicJni.idleProc(0) != 0) {
                Log.d("PDD", "End first idle");
                jniCallback.requestUpdate();
                done = true;
                break;
            }
        }
        if (!done){
            //Log.i("PDD", "Not done - ");
            requestIdleProc();
        }
        return true;
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

    // Idle Proc //
    void requestIdleProc(){
        //Log.d("PDD", "requestIdleProc");
        handler.postDelayed(idleProc, 0);
    }

    private final Runnable idleProc = new Runnable() {
        @Override
        public void run() {
            final int maxLoop = 100;
            for (int i=0;i<maxLoop;i++){
                if (pdicJni.idleProc(0)==1) {
                    // end of idle
                    //Log.d("PDD", "End idle ");
                    jniCallback.requestUpdate();
                    if (i>0) {
                        handler.postDelayed(idleProc, 1000);
                    }
                    return;
                }
            }
            requestIdleProc();  // continue to run idle proc
        }
    };
}
