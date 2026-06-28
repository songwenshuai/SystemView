/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.record;

import de.toem.toolkits.pattern.element.Cell;

public class RecordContent
extends Cell {
    public String description;
    public static final int DIFF_MATCH = 0;
    public static final int DIFF_MODIFIED = 1;
    public static final int DIFF_INSERTED = 2;
    public static final int DIFF_REMOVED = 3;
    public int diff;
}

