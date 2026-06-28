/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cell.parts;

import de.toem.impulse.cells.preferences.AbstractSubViewPreferenceCell;
import de.toem.toolkits.pattern.element.CellAnnotation;

@CellAnnotation(type="impulse.subview.samples", dynamicChildOf={"preferences.impulse.parts"})
public class SamplesViewPreferences
extends AbstractSubViewPreferenceCell {
    public static final String TYPE = "impulse.subview.samples";
    public static final int ALIGNMENT_LEFT = 0;
    public static final int ALIGNMENT_CENTER = 1;
    public static final int ALIGNMENT_RIGHT = 2;
    public static final int UNIT_MODE_RAW = 0;
    public static final int UNIT_MODE_PREFFERED = 1;
    public static final int UNIT_MODE_AUTO = 2;
    public static final int SHOW_DONT = 0;
    public static final int SHOW_ALLWAYS = 1;
    public static final int SHOW_CONDITIONAL = 2;
    public int domainAlignment = 2;
    public boolean domainFixedFont = true;
    public int domainUnitMode = 1;
    public int valueShowMode = 2;
    public int valueAlignment = 0;
    public boolean valueFixedFont = false;
    public boolean valueWrap = false;
    public int valueFormat = -1;
    public int signalShowMode = 2;
    public int signalAlignment = 1;
    public boolean signalFixedFont = false;
    public int groupShowMode = 2;
    public int groupAlignment = 1;
    public boolean groupFixedFont = false;
    public int orderShowMode = 2;
    public int orderAlignment = 1;
    public boolean orderFixedFont = false;
    public int samplesShowMode = 2;
    public int samplesAlignment = 0;
    public boolean samplesFixedFont = false;
    public int labelShowMode = 0;
    public int labelAlignment = 1;
    public boolean labelFixedFont = false;
    public int relationShowMode = 0;
    public int relationAlignment = 0;
    public boolean relationFixedFont = false;
    public int tagShowMode = 0;
    public int tagAlignment = 0;
    public boolean tagFixedFont = false;
    public boolean tagBackground = true;

    public static long alignment2TableFormat(int alignment) {
        if (alignment == 0) {
            return 1L;
        }
        if (alignment == 1) {
            return 2L;
        }
        if (alignment == 2) {
            return 3L;
        }
        return 0L;
    }

    public static long fixedFont2TableFormat(boolean fixedFont) {
        if (fixedFont) {
            return 16L;
        }
        return 0L;
    }

    public static long wrap2TableFormat(boolean wrap) {
        if (wrap) {
            return 32L;
        }
        return 0L;
    }

    public static int unitMode2Style(int mode) {
        if (mode == 0) {
            return 0;
        }
        if (mode == 1) {
            return 4;
        }
        if (mode == 2) {
            return 2;
        }
        return 0;
    }
}

