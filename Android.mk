LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := pdjni
LOCAL_LDLIBS := \
	-llog \
	-landroid \

JNI_PATH=./app/src/main/jni
APP_SRC_PATH=.\app\src

LOCAL_SRC_FILES := \
	$(JNI_PATH)/pdjni.cpp \
	$(JNI_PATH)/tnlib2/android.cpp \
	$(JNI_PATH)/tnlib2/bifile.cpp \
	$(JNI_PATH)/tnlib2/biofile.cpp \
	$(JNI_PATH)/tnlib2/bocu1.cpp \
	$(JNI_PATH)/tnlib2/bocu1_utf8.cpp \
	$(JNI_PATH)/tnlib2/bofile.cpp \
	$(JNI_PATH)/tnlib2/buffer.cpp \
	$(JNI_PATH)/tnlib2/fexist.cpp \
	$(JNI_PATH)/tnlib2/file.cpp \
	$(JNI_PATH)/tnlib2/filestr.cpp \
	$(JNI_PATH)/tnlib2/fixchar.cpp \
	$(JNI_PATH)/tnlib2/flexobj.cpp \
	$(JNI_PATH)/tnlib2/newstr.cpp \
	$(JNI_PATH)/tnlib2/ofile.cpp \
	$(JNI_PATH)/tnlib2/rexpgen.cpp \
	$(JNI_PATH)/tnlib2/strlib.cpp \
	$(JNI_PATH)/tnlib2/tfilebuf.cpp \
	$(JNI_PATH)/tnlib2/tifile.cpp \
	$(JNI_PATH)/tnlib2/timex_linux.cpp \
	$(JNI_PATH)/tnlib2/tiofile.cpp \
	$(JNI_PATH)/tnlib2/tnarray.cpp \
	$(JNI_PATH)/tnlib2/tnassert.cpp \
	$(JNI_PATH)/tnlib2/tndefs.cpp \
	$(JNI_PATH)/tnlib2/tnstr.cpp \
	$(JNI_PATH)/tnlib2/tnstrbuf.cpp \
	$(JNI_PATH)/tnlib2/tofile.cpp \
	$(JNI_PATH)/tnlib2/unix.cpp \
	$(JNI_PATH)/tnlib2/utf.cpp \
	$(JNI_PATH)/tnlib2/uustr.cpp \
	$(JNI_PATH)/tnlib2/vbuffer.cpp \
	$(JNI_PATH)/tnlib2/cbxe/stdafx.cpp \
	$(JNI_PATH)/commonLib/BiPool.cpp \
	$(JNI_PATH)/commonLib/defs.cpp \
	$(JNI_PATH)/commonLib/defs_android.cpp \
	$(JNI_PATH)/commonLib/fileio.cpp \
	$(JNI_PATH)/commonLib/filestr.cpp \
	$(JNI_PATH)/commonLib/jtype.cpp \
	$(JNI_PATH)/commonLib/jtype1.cpp \
	$(JNI_PATH)/commonLib/jtype3.cpp \
	$(JNI_PATH)/commonLib/md5.cpp \
	$(JNI_PATH)/commonLib/pdconfig.cpp \
	$(JNI_PATH)/commonLib/pdstrlib.cpp \
	$(JNI_PATH)/commonLib/pdtime.cpp \
	$(JNI_PATH)/commonLib/RangeCodec.cpp \
	$(JNI_PATH)/commonLib/TextResource.cpp \
	$(JNI_PATH)/commonLib/wordcount.cpp \
	$(JNI_PATH)/dicLib/Dic.cpp \
	$(JNI_PATH)/dicLib/Dic3.cpp \
	$(JNI_PATH)/dicLib/DicConv.cpp \
	$(JNI_PATH)/dicLib/dicdata.cpp \
	$(JNI_PATH)/dicLib/DicDec.cpp \
	$(JNI_PATH)/dicLib/Dicgrp.cpp \
	$(JNI_PATH)/dicLib/Dicinx.cpp \
	$(JNI_PATH)/dicLib/Dicixdt.cpp \
	$(JNI_PATH)/dicLib/diclist.cpp \
	$(JNI_PATH)/dicLib/dicmix.cpp \
	$(JNI_PATH)/dicLib/dicname.cpp \
	$(JNI_PATH)/dicLib/dicobj.cpp \
	$(JNI_PATH)/dicLib/dictype.cpp \
	$(JNI_PATH)/dicLib/dicUtil.cpp \
	$(JNI_PATH)/dicLib/faststr.cpp \
	$(JNI_PATH)/dicLib/filebuf.cpp \
	$(JNI_PATH)/dicLib/Hdicdata.cpp \
	$(JNI_PATH)/dicLib/Hdicinx.cpp \
	$(JNI_PATH)/dicLib/japa.cpp \
	$(JNI_PATH)/dicLib/japa1.cpp \
	$(JNI_PATH)/dicLib/kstr.cpp \
	$(JNI_PATH)/dicLib/MainDic.cpp \
	$(JNI_PATH)/dicLib/multiwd.cpp \
	$(JNI_PATH)/dicUtil/dictext.cpp \
	$(JNI_PATH)/dicUtil/utydic.cpp \
	$(JNI_PATH)/History/histfile.cpp \
	$(JNI_PATH)/LangProc/CCTable.cpp \
	$(JNI_PATH)/LangProc/IrregDic.cpp \
	$(JNI_PATH)/LangProc/ktable.cpp \
	$(JNI_PATH)/LangProc/LangProc.cpp \
	$(JNI_PATH)/LangProc/LangProcBase.cpp \
	$(JNI_PATH)/LangProc/LangProcDef.cpp \
	$(JNI_PATH)/LangProc/LangProcMan.cpp \
	$(JNI_PATH)/LangProc/LangProcStd.cpp \
	$(JNI_PATH)/LangProc/LPTable.cpp \
	$(JNI_PATH)/pdcom/msgbox.cpp \
	$(JNI_PATH)/pdcom/mstr.cpp \
	$(JNI_PATH)/pdcom/pddefs.cpp \
	$(JNI_PATH)/pdcom/pdfilestr.cpp \
	$(JNI_PATH)/pdcom/pfs.cpp \
	$(JNI_PATH)/pdcom/regs.cpp \
	$(JNI_PATH)/pdcom/SrchPatParser.cpp \
	$(JNI_PATH)/pdcom/txr.cpp \
	$(JNI_PATH)/pdcom/android/msgbox.cpp \
	$(JNI_PATH)/pdcom2/textext.cpp \
	$(JNI_PATH)/PdicMain/Dicproc.cpp \
	$(JNI_PATH)/PdicMain/android/UIMain.cpp \
	$(JNI_PATH)/Popup/PSBookmark.cpp \
	$(JNI_PATH)/profile/android/pdprof_android.cpp \
	$(JNI_PATH)/Search/srchcom.cpp \
	$(JNI_PATH)/Search/SrchMed.cpp \
	$(JNI_PATH)/Search/srchstat.cpp \
	$(JNI_PATH)/Square/android/MouseCapCommon.cpp \
	$(JNI_PATH)/Square/android/MouseCapture.cpp \
	$(JNI_PATH)/Square/Pool.cpp \
	$(JNI_PATH)/Square/PrimaryPool.cpp \
	$(JNI_PATH)/Square/android/SquFrm.cpp \
	$(JNI_PATH)/Square/SquInterface.cpp \
	$(JNI_PATH)/Square/SquItemView.cpp \
	$(JNI_PATH)/Square/android/SquView.cpp \
	$(JNI_PATH)/Square/SquWidthAttr.cpp \
	$(JNI_PATH)/Square/srchproc.cpp \
	$(JNI_PATH)/Square/wdicsqu.cpp \
	$(JNI_PATH)/Square/winsqu.cpp \
	$(JNI_PATH)/Square/winsqu_android.cpp \
	$(JNI_PATH)/Square/wsdisp.cpp \
	$(JNI_PATH)/Square/wsinc.cpp \
	$(JNI_PATH)/Square/wspaint.cpp \
	$(JNI_PATH)/Square/Wsqusrch.cpp \
	$(JNI_PATH)/Square/Wsscroll.cpp \
	$(JNI_PATH)/zipLib/zipDll.cpp \
	$(JNI_PATH)/zipLib/LiteUnzip/LiteUnzip.c \

LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\bookmark
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\commonLib
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\dicLib
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\dicUtil
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\History
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\LangProc
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\pdcom
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\pdcom2
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\pdic
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\pdicLib
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\PdicMain
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\PdicMain\android
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\Popup
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\profile
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\profile\android
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\tnlib2
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\tnlib2\cbxe
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\Search
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\Square
LOCAL_C_INCLUDES += $(APP_SRC_PATH)\main\jni\Square\android

include $(BUILD_SHARED_LIBRARY)
