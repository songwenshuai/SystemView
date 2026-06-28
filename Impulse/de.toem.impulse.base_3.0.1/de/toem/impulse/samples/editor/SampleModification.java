/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.editor;

import de.toem.impulse.values.CompoundValue;

public class SampleModification {
    static final int TYPE_INSERT = 1;
    static final int TYPE_REMOVE = 2;
    static final int TYPE_CHANGE = 3;
    static final int TYPE_MOVE = 4;
    static final int OPTION_MOVE_ALL = 1;
    int type;
    int idx;
    int options;
    CompoundValue value;
    long position;

    public void log() {
    }

    static SampleModification insert(CompoundValue value, int options) {
        SampleModification m = new SampleModification();
        m.type = 1;
        m.idx = value.getIndex();
        m.value = value;
        m.options = options;
        return m;
    }

    static SampleModification remove(int idx, int options) {
        SampleModification m = new SampleModification();
        m.type = 2;
        m.idx = idx;
        m.options = options;
        return m;
    }

    static SampleModification change(int idx, CompoundValue value, int options) {
        SampleModification m = new SampleModification();
        m.type = 3;
        m.idx = idx;
        m.value = value;
        m.options = options;
        return m;
    }

    static SampleModification move(int idx, long position, int options) {
        SampleModification m = new SampleModification();
        m.type = 4;
        m.idx = idx;
        m.position = position;
        m.options = options;
        return m;
    }

    public String toString() {
        return String.valueOf(this.type == 1 ? "I" : (this.type == 2 ? "R" : "C")) + this.idx + " " + this.options + " " + this.value + " " + this.position;
    }
}

