/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.base;

public class SamplesStat {
    public int count;
    public static final int FLAGS_HAS_TAG = 1;
    public static final int FLAGS_HAS_NON_TAG = 2;
    public static final int FLAGS_HAS_CHANGES = 4;
    public static final int FLAGS_HAS_NONE = 8;
    public static final int FLAGS_HAS_VALUES = 16;
    public short flags;
    public byte tag;
    public float first = 0.0f;
    public float min = Float.MAX_VALUE;
    public float med = 0.0f;
    public float max = -3.4028235E38f;
    public float last = 0.0f;
    public Object value;
}

