/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint;

import de.toem.toolkits.ui.tlk.controls.ITlkObject;

public interface ITheme
extends ITlkObject {
    public static final int COLOR_ALL = -1;
    public static final int COLOR_PLOT_BACKGROUND = 0;
    public static final int COLOR_PLOT_FOREGROUND = 1;
    public static final int COLOR_PLOT_DIFF = 2;
    public static final int COLOR_PLOT_TEXT = 3;
    public static final int COLOR_RELATION_FOREGROUND = 4;
    public static final int COLOR_RELATION_INFO = 5;
    public static final int COLOR_LABEL_TEXT = 6;
    public static final int COLOR_SCALE_BACKGROUND = 7;
    public static final int COLOR_SCALE_TICK_MAJOR = 8;
    public static final int COLOR_SCALE_TICK_MINOR = 9;
    public static final int COLOR_SCALE_TEXT = 10;
    public static final int COLOR_TAG_1 = 11;
    public static final int COLOR_TAG_2 = 12;
    public static final int COLOR_TAG_3 = 13;
    public static final int COLOR_TAG_4 = 14;
    public static final int COLOR_TAG_5 = 15;
    public static final int COLOR_TAG_6 = 16;
    public static final int COLOR_TAG_7 = 17;
    public static final int COLOR_TAG_8 = 18;
    public static final int[] COLOR_TAG_N;
    public static final int COLOR_SHADE_I0 = 24;
    public static final int COLOR_SHADE_I1 = 25;
    public static final int COLOR_SHADE_I2 = 26;
    public static final int COLOR_SHADE_I3 = 27;
    public static final int COLOR_SHADE_I4 = 28;
    public static final int COLOR_SHADE_I5 = 29;
    public static final int COLOR_SHADE_I6 = 30;
    public static final int COLOR_SHADE_I7 = 31;
    public static final int COLOR_SHADE_I_COUNT = 8;
    public static final int COLOR_VERT_SCALE_TEXT = 28;
    public static final int COLOR_PALETTE_0 = 32;
    public static final int COLOR_PALETTE_31 = 47;
    public static final int COLOR_PALETTE_COUNT = 32;
    public static final int COLOR_LIST_FOREGROUND = 64;
    public static final int COLOR_LIST_BACKGROUND = 65;
    public static final int COLOR_LIST_STRIPED_BACKGROUND = 66;
    public static final int COLOR_LIST_INACTIVE_BACKGROUND = 67;
    public static final int COLOR_LIST_OBJECT = 68;
    public static final int COLOR_LIST_SELECTION_FOREGROUND = 70;
    public static final int COLOR_LIST_SELECTION_BACKGROUND = 71;
    public static final int COLOR_LIST_INACTIVE_SELECTION_FOREGROUND = 72;
    public static final int COLOR_LIST_INACTIVE_SELECTION_BACKGROUND = 73;
    public static final int COLOR_LIST_FOCUS_BORDER = 74;
    public static final int COLOR_LIST_HOVER_BACKGROUND = 77;
    public static final int COLOR_COUNT = 80;
    public static final int STYLE_NONE = 0;
    public static final int STYLE_ACTIVE = 1;
    public static final int STYLE_SELECTED = 2;
    public static final int STYLE_NOAXIS = 4;
    public static final int STYLE_FOCUS = 8;
    public static final int STYLE_RESIZE = 16;
    public static final int STYLE_DROP_ON = 32;
    public static final int STYLE_DROP_BEFORE = 64;
    public static final int STYLE_DROP_AFTER = 128;
    public static final int STYLE_SHOW_ICONS = 256;
    public static final int STYLE_SHOW_NAMES_RIGHTALLIGNED = 512;
    public static final int STYLE_SHOW_VALUE_COLUMN = 1024;
    public static final int STYLE_HIGHLIGHT = 2048;
    public static final int STYLE_DIM = 4096;
    public static final int STYLE_HOVER = 2;
    public static final int STYLE_XSTYLE = 4;
    public static final int STYLE_BASE = 240;
    public static final int STYLE_ZOOM_IN = 16;
    public static final int STYLE_ZOOM_OUT = 32;
    public static final int STYLE_QUICK_MEASURE = 48;
    public static final int STYLE_MOVE = 3840;
    public static final int STYLE_MOVE_LEFT = 256;
    public static final int STYLE_MOVE_LEFT_FAST = 512;
    public static final int STYLE_MOVE_LEFT_BOOST = 768;
    public static final int STYLE_MOVE_RIGHT = 1024;
    public static final int STYLE_MOVE_RIGHT_FAST = 1280;
    public static final int STYLE_MOVE_RIGHT_BOOST = 1536;
    public static final int STYLE_MEASURE = 28672;
    public static final int STYLE_MEASURE_SHARED_TO_LEFT = 4096;
    public static final int STYLE_MEASURE_SHARED_TO_RIGHT = 8192;
    public static final int STYLE_MEASURE_SHARED = 12288;
    public static final int STYLE_MEASURE_NORMAL = 16384;
    public static final int STYLE_HIDDEN = 32768;
    public static final int STYLE_TAG_TO_LEFT = 65536;
    public static final int STYLE_MULTI = 262144;
    public static final int STYLE_LOOSE = 0x10000000;
    public static final int STYLE_MASK_DIR = 3;
    public static final int STYLE_DIR_UP = 1;
    public static final int STYLE_DIR_DOWN = 2;
    public static final String Font = "Font";
    public static final String MarkerFont = "MarkerFont";
    public static final String Color = "Color";
    public static final String[] Contents;

    static {
        int[] nArray = new int[16];
        nArray[1] = 11;
        nArray[2] = 12;
        nArray[3] = 13;
        nArray[4] = 14;
        nArray[5] = 15;
        nArray[6] = 16;
        nArray[7] = 17;
        nArray[8] = 18;
        nArray[9] = 18;
        nArray[10] = 18;
        nArray[11] = 18;
        nArray[12] = 18;
        nArray[13] = 18;
        nArray[14] = 18;
        nArray[15] = 18;
        COLOR_TAG_N = nArray;
        Contents = new String[]{Font, MarkerFont, Color};
    }

    public Object getFont();

    public String getFontName();

    public float getFontHeight();

    public void setFont(Object var1);

    public Object getMarkerFont();

    public void setMarkerFont(Object var1);

    public int getLineWidth();

    public void setLineWidth(int var1);

    public void setColor(int var1, Object var2);

    public Object getColor(int var1);

    public void setPrintMode(boolean var1);

    public boolean isPrintMode();

    public int getMinimumSampleSize(int var1, int var2);

    public void overideMinimumSampleSize(int var1, int var2);

    public static interface IPointsToResult {
        public double fuzzy();

        public int idx();
    }
}

