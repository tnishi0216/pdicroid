package com.reliefoffice.pdic;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.util.Log;

import java.io.File;

public class VupUtility {
    static public class VupInfo {
        public String Version;
        public String Location;
    }
    public static final String getCurrentVersion(Context context)
    {
        String currentVersionName = "0.0.0";
        try {
            PackageInfo packageInfo = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
            currentVersionName = packageInfo.versionName;
            // strip anything after the numeric version (major.minor.release)
            // e.g. "0.8.38 alpha" -> "0.8.38"
            currentVersionName = sanitizeVersionString(currentVersionName);
        } catch (PackageManager.NameNotFoundException e) {
            Log.e("VupUtility", "Failed to get package info", e);
        }
        return currentVersionName;
    }
    static final String URL_UPGRADE_SERVER = "https://pdic.sakura.ne.jp/cgi-bin/upgrade/download.cgi";

    public static final String GetVupBaseUrl()
    {
        return URL_UPGRADE_SERVER;
    }
    public static final String GetVupUrl(String currentVersionName)
    {
        return GetVupBaseUrl() + "?server=verinfo&app=pdicroid&version=" + currentVersionName;
    }
    public static final VupInfo parseVupInfoFile(File filename)
    {
        VupInfo info = new VupInfo();
        try {
            javax.xml.parsers.DocumentBuilderFactory dbf = javax.xml.parsers.DocumentBuilderFactory.newInstance();
            javax.xml.parsers.DocumentBuilder db = dbf.newDocumentBuilder();
            org.w3c.dom.Document doc = db.parse(filename.getAbsoluteFile());
            doc.getDocumentElement().normalize();

            org.w3c.dom.NodeList versionNodes = doc.getElementsByTagName("Version");
            if (versionNodes.getLength() > 0) {
                info.Version = versionNodes.item(0).getTextContent().trim();
            }
            org.w3c.dom.NodeList locationNodes = doc.getElementsByTagName("Location");
            if (locationNodes.getLength() > 0) {
                info.Location = locationNodes.item(0).getTextContent().trim();
            }
        } catch (Exception e) {
            Log.w("VupUtility", "Failed to parse version info", e);
            return null;
        }
        return info;
    }

    /**
     * Compare two version strings formatted as major.minor.release.
     *
     * @return negative if v1 &lt; v2, zero if equal, positive if v1 &gt; v2
     */
    public static final int compareVersion(String v1, String v2) {
        if (v1 == null) return -1;
        if (v2 == null) return 1;
        String[] a1 = v1.split("\\.");
        String[] a2 = v2.split("\\.");
        int len = Math.max(a1.length, a2.length);
        for (int i = 0; i < len; i++) {
            int n1 = i < a1.length ? parseIntSafe(a1[i]) : 0;
            int n2 = i < a2.length ? parseIntSafe(a2[i]) : 0;
            if (n1 != n2) return n1 - n2;
        }
        return 0;
    }

    public static final int parseIntSafe(String s) {
        try {
            return Integer.parseInt(s);
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    /**
     * Trim version string to numeric parts only (major.minor.release).
     * Removes any suffixes or non-digit characters after the three components.
     */
    public static final String sanitizeVersionString(String v) {
        if (v == null) return v;
        // grab first match of digits separated by dots up to three segments
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("^(\\d+(?:\\.\\d+){0,2})").matcher(v);
        if (m.find()) {
            return m.group(1);
        }
        return v;
    }
}



