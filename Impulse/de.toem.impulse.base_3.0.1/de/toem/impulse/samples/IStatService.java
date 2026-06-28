/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.base.SamplesStat;
import de.toem.impulse.samples.convert.SampleConverterConfiguration;

public interface IStatService {
    public static final int CONTENT_FLAGS_TAG = 1;
    public static final int CONTENT_FLAGS_CHANGE = 2;
    public static final int CONTENT_FLAGS_NONE = 4;
    public static final int CONTENT_FLAGS = 7;
    public static final int CONTENT_NUM_MINMAX = 8;
    public static final int CONTENT_NUM_MEDIAN = 16;
    public static final int CONTENT_NUM = 24;

    public boolean init(SampleConverterConfiguration var1, int var2);

    public SamplesStat getStat(int var1, int var2);

    public int getContent();

    public SampleConverterConfiguration getConverterConfiguration();
}

