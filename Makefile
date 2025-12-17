
include env.mk

APP_BUILD_SCRIPT_PATH=.\Android.mk

build:
	$(MAKE) mkdirs
	# for 29.0.14206865
	$(NDK_PATH)ndk-build.cmd NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=$(APP_BUILD_SCRIPT_PATH) APP_PLATFORM=android-21 NDK_OUT=.\app\build\intermediates\ndk\debug\obj NDK_LIBS_OUT=.\app\build\intermediates\ndk\debug\lib APP_STL=$(APP_STL_VAL) APP_CPPFLAGS='-std=c++14' APP_ABI=armeabi-v7a,arm64-v8a,x86,x86_64
	# for 21.0.6113669
#	$(NDK_PATH)ndk-build.cmd NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=$(APP_BUILD_SCRIPT_PATH) APP_PLATFORM=android-21 NDK_OUT=.\app\build\intermediates\ndk\debug\obj NDK_LIBS_OUT=.\app\build\intermediates\ndk\debug\lib APP_STL=$(APP_STL_VAL) APP_ABI=armeabi-v7a,arm64-v8a,x86,x86_64
	$(CP) app/build/intermediates/ndk/debug/lib/armeabi-v7a/*.so app/src/main/jniLibs/armeabi-v7a/
	$(CP) app/build/intermediates/ndk/debug/lib/arm64-v8a/*.so app/src/main/jniLibs/arm64-v8a/
	$(CP) app/build/intermediates/ndk/debug/lib/x86/*.so app/src/main/jniLibs/x86/
	$(CP) app/build/intermediates/ndk/debug/lib/x86_64/*.so app/src/main/jniLibs/x86_64/

# for xperia
buildx:
	$(NDK_PATH)ndk-build.cmd NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=$(APP_BUILD_SCRIPT_PATH) APP_PLATFORM=android-21 NDK_OUT=.\app\build\intermediates\ndk\debug\obj NDK_LIBS_OUT=.\app\build\intermediates\ndk\debug\lib APP_STL=$(APP_STL_VAL) APP_ABI=arm64-v8a
	copy app\build\intermediates\ndk\debug\lib\arm64-v8a\*.so app\src\main\jniLibs\arm64-v8a\\

build8664:
	$(NDK_PATH)ndk-build.cmd NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=$(APP_BUILD_SCRIPT_PATH) APP_PLATFORM=android-21 NDK_OUT=.\app\build\intermediates\ndk\debug\obj NDK_LIBS_OUT=.\app\build\intermediates\ndk\debug\lib APP_STL=$(APP_STL_VAL) APP_ABI=x86_64
	copy app\build\intermediates\ndk\debug\lib\x86_64\*.so app\src\main\jniLibs\x86_64\\

buildunzip:
	$(NDK_PATH)ndk-build.cmd NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=.\litezip\LiteUnzip\Android.mk APP_PLATFORM=android-21 NDK_OUT=.\litezip\build\obj NDK_LIBS_OUT=.\litezip\build\lib APP_STL=$(APP_STL_VAL) APP_ABI=armeabi-v7a,arm64-v8a,x86,x86_64

clean:
	$(NDK_PATH)ndk-build.cmd NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=$(APP_BUILD_SCRIPT_PATH) APP_PLATFORM=android-21 NDK_OUT=.\app\build\intermediates\ndk\debug\obj NDK_LIBS_OUT=.\app\build\intermediates\ndk\debug\lib APP_STL=$(APP_STL_VAL) APP_ABI=armeabi-v7a,arm64-v8a,x86,x86_64 clean
	$(RM) app/src/main/jniLibs/armeabi-v7a/*.so
	$(RM) app/src/main/jniLibs/arm64-v8a/*.so
	$(RM) app/src/main/jniLibs/x86/*.so
	$(RM) app/src/main/jniLibs/x86_64/*.so

javah:
	javah -classpath "bin/classes;E:\src\Android\sdk\platforms\android-21\data\layoutlib.jar;E:\src\Android\MyApplication\app\build\intermediates\classes\release\com\example\tnishi\myapplication" com.reliefoffice.pdic.MainActivity

mkdirs:
	-mkdir app\src\main\jniLibs
	-mkdir app\src\main\jniLibs\armeabi-v7a
	-mkdir app\src\main\jniLibs\arm64-v8a
	-mkdir app\src\main\jniLibs\x86
	-mkdir app\src\main\jniLibs\x86_64

zip:
	zip -r src . -i *.java *.c *.cpp *.h *.xml *.png Makefile *.mk *.gradle *.properties *.bat

deploy:
	copy app\release\app-release.apk pdicroid.apk
	perl -S deploy.pl pdicroid.apk -vapp/build.gradle -hs:\web\sakura-pdic\android\index.html -hkey:Apk @sakura
	perl -S deploy.pl pdicroid.apk -vapp/build.gradle -rename -copy release
#	pause zip‚µ‚Ü‚·
#	make zip
#	zr.bat
#	del zr.bat

deploydbg:
	copy app\release\app-release.apk S:\web\sakura-pdic\android\pdicroid-0.8.27.apk
#	echo upftp > S:\www\cgi-bin\cmdsvr\cmd.txt
