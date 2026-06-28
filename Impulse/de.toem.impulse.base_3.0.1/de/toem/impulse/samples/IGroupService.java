/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import java.util.Iterator;

public interface IGroupService {
    public int[] availableLayers();

    public IGroupIterator activeGroupIterator(int var1, int var2, int var3);

    public int size();

    public int[] rangeOf(int var1);

    public static interface IGroupIterator
    extends Iterator<Integer> {
        public boolean hasNextBefore(int var1);

        public int currentGroup();

        public int currentGroupFirstIdx();

        public int currentGroupLayer();

        public int currentGroupLastIdx();
    }
}

