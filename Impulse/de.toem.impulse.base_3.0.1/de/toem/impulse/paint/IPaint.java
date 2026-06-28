/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint;

import de.toem.toolkits.pattern.js.JsType;
import org.eclipse.swt.graphics.Rectangle;

@JsType
public interface IPaint {
    public static final int DIAGRAM_NONE = 0;
    public static final int DIAGRAM_LOGIC = 1;
    public static final int DIAGRAM_VECTOR = 2;
    public static final int DIAGRAM_EVENT = 3;
    public static final int DIAGRAM_LINE = 4;
    public static final int DIAGRAM_TRANSACTION = 5;
    public static final int DIAGRAM_LOG = 6;
    public static final int DIAGRAM_IMAGE = 7;
    public static final int DIAGRAM_CHART = 8;
    public static final int DIAGRAM_AREA = 9;
    public static final int DIAGRAM_GANTT = 10;
    public static final int DIAGRAM_MAX = 10;
    public static final int FOLDER_NORMAL = 0;
    public static final int FOLDER_ACCORDION = 1;
    public static final int DIAGRAM_MOD_INTERPOLATION = 16;
    public static final int DIAGRAM_MOD_ANNOTATION = 32;
    public static final int DIAGRAM_MOD_SCALE = 64;
    public static final int DIAGRAM_MOD_AXIS = 128;
    public static final int DIAGRAM_MOD_STACKED = 256;
    public static final int DIAGRAM_MOD_TRANSPARENT = 512;
    public static final int DIAGRAM_MOD_SLOPE_BASE = 1024;
    public static final int DIAGRAM_MOD_MULTI_COLOR = 2048;
    public static final int DIAGRAM_MOD_RELATION = 4096;
    public static final int DIAGRAM_MOD_LABEL = 8192;
    public static final int DIAGRAM_MOD_HIGHLIGHT = 16384;
    public static final int MARKER_NONE = 0;
    public static final int MARKER_ABOVE = 1;
    public static final int MARKER_WITHIN = 2;
    public static final int PAINT_BASE_MASK = 61440;
    public static final int PAINT_PAINT_MASK = 65280;
    public static final int PAINT_MOD_MASK = 255;
    public static final int PAINT_MOD_NOT_TAGGED = 0;
    public static final int PAINT_MOD_TAG_1 = 1;
    public static final int PAINT_MOD_TAG_MAX = 15;
    public static final int PAINT_MOD_TAG_MASK = 15;
    public static final int PAINT_MOD_ANNOTATION = 16;
    public static final int PAINT_PAR_SCALE_NONE = 0;
    public static final int PAINT_PAR_SCALE_SIMPLE = 1;
    public static final int PAINT_PAR_SCALE_KEEPASPECT = 2;
    public static final int PAINT_PAR_ALLIGN_LEFT = 0;
    public static final int PAINT_PAR_ALLIGN_CENTER = 1;
    public static final int PAINT_PAR_ALLIGN_RIGHT = 2;
    public static final int PAINT_PAR_ALLIGN_TOP = 0;
    public static final int PAINT_PAR_ALLIGN_MIDDLE = 1;
    public static final int PAINT_PAR_ALLIGN_BOTTOM = 2;
    public static final int PAINT_NONE = 0;
    public static final int PAINT_LOGIC = 4096;
    public static final int PAINT_LOGIC_0 = 4352;
    public static final int PAINT_LOGIC_1 = 4608;
    public static final int PAINT_LOGIC_U = 4864;
    public static final int PAINT_LOGIC_X = 5120;
    public static final int PAINT_VECTOR = 8192;
    public static final int PAINT_EVENT = 12288;
    public static final int PAINT_LINE = 16384;
    public static final int PAINT_LINE_POINT = 16640;
    public static final int PAINT_LINE_RANGE = 16896;
    public static final int PAINT_LINE_IMPULSE = 17408;
    public static final int PAINT_TRANSACTION = 20480;
    public static final int PAINT_LOG = 24576;
    public static final int PAINT_IMAGE = 28672;
    public static final int PAINT_CHART = 32768;
    public static final int PAINT_AREA = 36864;
    public static final int PAINT_GANT = 40960;
    public static final int PAINT_ANNOTATION = 57344;
    public static final int PAINT_RELATION = 57600;
    public static final int PAINT_LABEL = 57856;
    public static final int PAINT_VOLATILE = 61440;
    public static final int MODE_STRICT = 0;
    public static final int MODE_LOOSE = 1;
    public static final int MODE_RESIZE = 2;
    public static final int MODE_ZOOM = 4;
    public static final int MODE_LEFT_SLOW = 16;
    public static final int MODE_LEFT_MED = 32;
    public static final int MODE_LEFT_FAST = 48;
    public static final int MODE_RIGHT_SLOW = 64;
    public static final int MODE_RIGHT_MED = 80;
    public static final int MODE_RIGHT_FAST = 96;

    public static class CachedImage {
        public Object image;
        public Rectangle bounds;
        public Rectangle area;
        public int index;
    }
}

