
include env.mk

.PHONY: build clean javah deploy

APP_BUILD_SCRIPT_PATH=./Android.mk

# Detect platform
UNAME_S := $(shell uname -s 2>/dev/null)
IS_WINDOWS :=
ifneq (,$(filter MINGW% MSYS% CYGWIN%,$(UNAME_S)))
  IS_WINDOWS := 1
endif

# NDK build tool resolution (override with `make NDK_BUILD=/path/to/ndk-build ...`)
NDK_BUILD ?=
NDK_PATH_NORM := $(patsubst %/,%,$(NDK_PATH))

# If NDK_BUILD is not explicitly set, derive it from NDK_PATH (if provided);
# otherwise fall back to whatever is on PATH. On Windows-like shells, use .cmd.
ifeq ($(strip $(NDK_BUILD)),)
  ifneq ($(strip $(NDK_PATH_NORM)),)
    NDK_BUILD := $(NDK_PATH_NORM)/ndk-build$(if $(IS_WINDOWS),.cmd,)
  else
    NDK_BUILD := ndk-build$(if $(IS_WINDOWS),.cmd,)
  endif
endif

build:
	$(MAKE) mkdirs
	$(NDK_BUILD) NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=$(APP_BUILD_SCRIPT_PATH) APP_PLATFORM=android-21 NDK_OUT=./app/build/intermediates/ndk/debug/obj NDK_LIBS_OUT=./app/build/intermediates/ndk/debug/lib APP_STL=$(APP_STL_VAL) APP_ABI=armeabi-v7a,arm64-v8a,x86,x86_64
	$(CP) app/build/intermediates/ndk/debug/lib/armeabi-v7a/*.so app/src/main/jniLibs/armeabi-v7a/
	$(CP) app/build/intermediates/ndk/debug/lib/arm64-v8a/*.so app/src/main/jniLibs/arm64-v8a/
	$(CP) app/build/intermediates/ndk/debug/lib/x86/*.so app/src/main/jniLibs/x86/
	$(CP) app/build/intermediates/ndk/debug/lib/x86_64/*.so app/src/main/jniLibs/x86_64/

# for xperia
buildx:
	$(MAKE) mkdirs
	$(NDK_BUILD) NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=$(APP_BUILD_SCRIPT_PATH) APP_PLATFORM=android-21 NDK_OUT=./app/build/intermediates/ndk/debug/obj NDK_LIBS_OUT=./app/build/intermediates/ndk/debug/lib APP_STL=$(APP_STL_VAL) APP_ABI=arm64-v8a
	$(CP) app/build/intermediates/ndk/debug/lib/arm64-v8a/*.so app/src/main/jniLibs/arm64-v8a/

build8664:
	$(MAKE) mkdirs
	$(NDK_BUILD) NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=$(APP_BUILD_SCRIPT_PATH) APP_PLATFORM=android-21 NDK_OUT=./app/build/intermediates/ndk/debug/obj NDK_LIBS_OUT=./app/build/intermediates/ndk/debug/lib APP_STL=$(APP_STL_VAL) APP_ABI=x86_64
	$(CP) app/build/intermediates/ndk/debug/lib/x86_64/*.so app/src/main/jniLibs/x86_64/

buildunzip:
	$(NDK_BUILD) NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=./litezip/LiteUnzip/Android.mk APP_PLATFORM=android-21 NDK_OUT=./litezip/build/obj NDK_LIBS_OUT=./litezip/build/lib APP_STL=$(APP_STL_VAL) APP_ABI=armeabi-v7a,arm64-v8a,x86,x86_64

clean:
	$(NDK_BUILD) NDK_PROJECT_PATH=null APP_BUILD_SCRIPT=$(APP_BUILD_SCRIPT_PATH) APP_PLATFORM=android-21 NDK_OUT=./app/build/intermediates/ndk/debug/obj NDK_LIBS_OUT=./app/build/intermediates/ndk/debug/lib APP_STL=$(APP_STL_VAL) APP_ABI=armeabi-v7a,arm64-v8a,x86,x86_64 clean
	$(RM) app/src/main/jniLibs/armeabi-v7a/*.so
	$(RM) app/src/main/jniLibs/arm64-v8a/*.so
	$(RM) app/src/main/jniLibs/x86/*.so
	$(RM) app/src/main/jniLibs/x86_64/*.so

javah:
	javah -classpath "bin/classes;E:\src\Android\sdk\platforms\android-21\data\layoutlib.jar;E:\src\Android\MyApplication\app\build\intermediates\classes\release\com\example\tnishi\myapplication" com.reliefoffice.pdic.MainActivity

mkdirs:
	mkdir -p app/src/main/jniLibs/armeabi-v7a
	mkdir -p app/src/main/jniLibs/arm64-v8a
	mkdir -p app/src/main/jniLibs/x86
	mkdir -p app/src/main/jniLibs/x86_64

zip:
	zip -r src . -i *.java *.c *.cpp *.h *.xml *.png Makefile *.mk *.gradle *.properties *.bat

deploy:
	$(CP) app\release\app-release.apk pdicroid.apk
	perl -S deploy.pl pdicroid.apk -vapp/build.gradle -hs:\web\sakura-pdic\android\index.html -hkey:Apk @sakura
	-mkdir release
	perl -S deploy.pl pdicroid.apk -vapp/build.gradle -rename -copy release
#	pause zip
#	make zip
#	zr.bat
#	del zr.bat

deploydbg:
	copy app\release\app-release.apk S:\web\sakura-pdic\android\pdicroid-0.8.27.apk
#	echo upftp > S:\www\cgi-bin\cmdsvr\cmd.txt
