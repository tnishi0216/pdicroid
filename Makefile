
# Android.mkÇÃoriginalÇÕrootÇ…Ç†ÇÈ"Android-local.mk"ÇæÇ¡ÇΩÅI

build:
	$(MAKE) copymk
	ndk-build.cmd NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=.\app\build\intermediates\ndk\debug\Android.mk APP_PLATFORM=android-21 NDK_OUT=.\app\build\intermediates\ndk\debug\obj NDK_LIBS_OUT=.\app\build\intermediates\ndk\debug\lib APP_STL=gnustl_shared APP_ABI=armeabi,mips,armeabi-v7a,arm64-v8a,x86,x86_64
	copy app\build\intermediates\ndk\debug\lib\armeabi\*.so app\src\main\jniLibs\armeabi\\
	copy app\build\intermediates\ndk\debug\lib\armeabi-v7a\*.so app\src\main\jniLibs\armeabi-v7a\\
	copy app\build\intermediates\ndk\debug\lib\arm64-v8a\*.so app\src\main\jniLibs\arm64-v8a\\
	copy app\build\intermediates\ndk\debug\lib\mips\*.so app\src\main\jniLibs\mips\\
	copy app\build\intermediates\ndk\debug\lib\x86\*.so app\src\main\jniLibs\x86\\
	copy app\build\intermediates\ndk\debug\lib\x86_64\*.so app\src\main\jniLibs\x86_64\\

# for xperia
buildx:
	ndk-build.cmd NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=.\app\build\intermediates\ndk\debug\Android.mk APP_PLATFORM=android-21 NDK_OUT=.\app\build\intermediates\ndk\debug\obj NDK_LIBS_OUT=.\app\build\intermediates\ndk\debug\lib APP_STL=gnustl_shared APP_ABI=arm64-v8a
	copy app\build\intermediates\ndk\debug\lib\arm64-v8a\*.so app\src\main\jniLibs\arm64-v8a\\

build8664:
	ndk-build.cmd NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=.\app\build\intermediates\ndk\debug\Android.mk APP_PLATFORM=android-21 NDK_OUT=.\app\build\intermediates\ndk\debug\obj NDK_LIBS_OUT=.\app\build\intermediates\ndk\debug\lib APP_STL=gnustl_shared APP_ABI=x86_64
	copy app\build\intermediates\ndk\debug\lib\x86_64\*.so app\src\main\jniLibs\x86_64\\

buildunzip:
	ndk-build.cmd NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=.\litezip\LiteUnzip\Android.mk APP_PLATFORM=android-21 NDK_OUT=.\litezip\build\obj NDK_LIBS_OUT=.\litezip\build\lib APP_STL=gnustl_shared APP_ABI=armeabi,mips,armeabi-v7a,arm64-v8a,x86,x86_64

clean:
	ndk-build.cmd NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=.\app\build\intermediates\ndk\debug\Android.mk APP_PLATFORM=android-21 NDK_OUT=.\app\build\intermediates\ndk\debug\obj NDK_LIBS_OUT=.\app\build\intermediates\ndk\debug\lib APP_STL=gnustl_shared APP_ABI=armeabi,mips,armeabi-v7a,arm64-v8a,x86,x86_64 clean

javah:
	javah -classpath "bin/classes;E:\src\Android\sdk\platforms\android-21\data\layoutlib.jar;E:\src\Android\MyApplication\app\build\intermediates\classes\release\com\example\tnishi\myapplication" com.reliefoffice.pdic.MainActivity

# app/build/intermediates/ndk/debug/Android.mkÇÃLOCAL_SRC_FILESÇÃÉpÉXÇ©ÇÁ\Ç/Ç…ïœä∑Ç∑ÇÈ
mkpath:
	perl -S android.pl mkpath app/build/intermediates/ndk/debug/Android.mk

copymk:
	-mkdir app\build\intermediates\ndk
	-mkdir app\build\intermediates\ndk\debug
	-mkdir app\src\main\jniLibs\armeabi
	-mkdir app\src\main\jniLibs\armeabi-v7a
	-mkdir app\src\main\jniLibs\arm64-v8a
	-mkdir app\src\main\jniLibs\mips
	-mkdir app\src\main\jniLibs\x86
	-mkdir app\src\main\jniLibs\x86_64
	copy Android-local.mk app\build\intermediates\ndk\debug\Android.mk

zip:
	zip -r src . -i *.java *.c *.cpp *.h *.xml *.png Makefile *.mk *.gradle *.properties *.bat

deploy:
	copy app\release\app-release.apk pdicroid.apk
	perl -S deploy.pl pdicroid.apk -vapp/build.gradle -hs:\web\NIFTY\android\index.html -hkey:Apk @sakura
	perl -S deploy.pl pdicroid.apk -vapp/build.gradle -rename -copy release
#	pause zipÇµÇ‹Ç∑
#	make zip
#	zr.bat
#	del zr.bat
