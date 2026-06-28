/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.toolkits.pattern.element.serializer.ICellReader;

public interface IRecordReader
extends ICellReader {
    public static final int CHANGED_VALUE = 1;
    public static final int CHANGED_CURRENT = 2;
    public static final int CHANGED_SIGNALS = 3;
    public static final int CHANGED_RECORD = 4;
    public static final int SUPPORT_INTRO = 8192;
}

