/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.utils;

import de.toem.impulse.i18n.I18n;
import de.toem.toolkits.core.Utils;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

public class Log {
    public static final int TAG_NONE = 0;
    public static final int TAG_FATAL = 1;
    public static final int TAG_ERROR = 2;
    public static final int TAG_WARNING = 3;
    public static final int TAG_SUCCESS = 4;
    public static final int TAG_INFO = 5;
    public static final int TAG_DEBUG = 6;
    public static final int TAG_TRACE = 7;
    public static final String[] TAGS = new String[]{"", I18n.General_Fatal, I18n.General_Error, I18n.General_Warning, I18n.General_Success, I18n.General_Info, I18n.General_Debug, I18n.General_Trace};
    public static final String LOGGER = "Logger";
    public static final String TIMESTAMP = "Timestamp";
    public static final String LEVEL = "Level";
    public static final String THREAD = "Thread";
    public static final String MESSAGE = "Message";
    public static final String CLASS = "Class";
    public static final String METHOD = "Method";
    public static final String FILE = "File";
    public static final String LINE = "Line";
    public static final String THROWABLE = "Throwable";
    public static final String NDC = "NDC";
    public static final String TEXT = "Text";
    public static final String RECPOS = "RecPos";
    public static final String[] MEMBERS = new String[]{"Logger", "Timestamp", "Level", "Thread", "Message", "Class", "Method", "File", "Line", "Throwable", "NDC", "Text", "RecPos"};
    public static final int[] MEMBER_TYPES = new int[]{7, 1, 7, 7, 1, 7, 7, 7, 3, 1, 1, 1, 3};
    public static final int[] MEMBER_FORMATS = new int[MEMBERS.length];
    public static final String[] MEMBER_CONTENTS = new String[MEMBERS.length];
    private static final List<String> MEMBERLIST;

    static {
        int n = 0;
        while (n < MEMBERS.length) {
            Log.MEMBER_FORMATS[n] = MEMBER_TYPES[n] == 7 ? 6 : -1;
            Log.MEMBER_CONTENTS[n] = null;
            ++n;
        }
        MEMBERLIST = new ArrayList<String>();
        String[] stringArray = MEMBERS;
        int n2 = MEMBERS.length;
        int n3 = 0;
        while (n3 < n2) {
            String m = stringArray[n3];
            MEMBERLIST.add(m);
            ++n3;
        }
    }

    public static boolean contains(String member) {
        return MEMBERLIST.contains(member);
    }

    public static int memberType(String member) {
        int n = MEMBERLIST.indexOf(member);
        if (n >= 0) {
            return MEMBER_TYPES[n];
        }
        return 1;
    }

    public static int memberFormat(String member) {
        int n = MEMBERLIST.indexOf(member);
        if (n >= 0) {
            return MEMBER_FORMATS[n];
        }
        return -1;
    }

    public static String memberContent(String member) {
        int n = MEMBERLIST.indexOf(member);
        if (n >= 0) {
            return MEMBER_CONTENTS[n];
        }
        return null;
    }

    public static void sort(List<String> members) {
        members.sort(new Comparator<String>(){

            @Override
            public int compare(String arg0, String arg1) {
                int idx0 = MEMBERLIST.indexOf(arg0);
                int idx1 = MEMBERLIST.indexOf(arg1);
                if (idx0 < 0) {
                    idx0 = MEMBERS.length;
                }
                if (idx1 < 0) {
                    idx1 = MEMBERS.length;
                }
                return Utils.compare(idx0, idx1, 1);
            }
        });
    }
}

