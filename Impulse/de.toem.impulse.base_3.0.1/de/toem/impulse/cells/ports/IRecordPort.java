/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.toolkits.pattern.element.ICell;

public interface IRecordPort
extends ICell {
    public static final int NATURE_NONE = 0;
    public static final int NATURE_FLOATING = 1;
    public static final int NATURE_REFRESH_CONTINUOUS = 2;
    public static final int NATURE_REFRESH_REQUEST = 4;
    public static final int NATURE_REFRESH_IDLE = 6;
    public static final int NATURE_MASK_REFRESH = 6;
    public static final int NATURE_ITERATE_ONCE = 8;
    public static final int NATURE_ITERATE_MULTIPLE = 16;
    public static final int NATURE_ITERATE_BOTH = 24;
    public static final int NATURE_MASK_ITERATE = 24;
    public static final int NATURE_CONNECT = 32;
    public static final int NATURE_CURRENT_VALUE = 128;

    public int getNature();

    public boolean isPort();
}

