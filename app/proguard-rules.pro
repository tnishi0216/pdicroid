# Add project specific ProGuard rules here.
# By default, the flags in this file are appended to flags specified
# in E:\src\Android\sdk/tools/proguard/proguard-android.txt
# You can edit the include path and order by changing the proguardFiles
# directive in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# Add any project specific keep options here:

-dontwarn com.dropbox.**
-dontwarn okio.**
-dontwarn okhttp3.**
-keepattributes EnclosingMethod
-keepclassmembers class com.reliefoffice.pdic.JniCallback {
	public *;
}
-keepclassmembers class com.reliefoffice.pdic.PdicJni {
	public *;
}

# Preserve Fragment classes that are instantiated dynamically via navigation
-keep class com.reliefoffice.pdic.SettingsFragmentCompat { *; }
-keep class com.reliefoffice.pdic.SettingsFragmentCompat$* { *; }
-keep class com.reliefoffice.pdic.DicSettingFragment { *; }
-keep class com.reliefoffice.pdic.IncrSrchFragment { *; }
-keep class com.reliefoffice.pdic.TouchSrchFragment { *; }
-keep class com.reliefoffice.pdic.PlaceholderFragment { *; }

# Preserve dialog-related classes
-keep class com.reliefoffice.pdic.*Dialog* { *; }
-keep class com.reliefoffice.pdic.*Dialog*$* { *; }

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}
