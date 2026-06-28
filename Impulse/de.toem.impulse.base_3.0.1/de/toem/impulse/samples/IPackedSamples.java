/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ISamples;
import de.toem.toolkits.pattern.pageable.Pageable;

public interface IPackedSamples
extends ISamples {
    public static final int ID = 2069;
    public static final int VERSION = 6;
    public static final int VERSION_3 = 3;
    public static final int VERSION_4 = 4;
    public static final int HEAD_ID_POS = 0;
    public static final int HEAD_VERSION_POS = 2;
    public static final int HEAD_FLAGS_POS = 3;
    public static final int HEAD_SAMPLES_PER_FRAGMENT_POS = 4;
    public static final int HEAD_BLOCKSIZE = 256;
    public static final int HEAD_ID_LENGTH = 2;
    public static final int HEAD_VERSION_LENGTH = 1;
    public static final int HEAD_FLAGS_LENGTH = 1;
    public static final int HEAD_FRAGSIZE_LENGTH = 1;
    public static final int HEAD_LENGTH = 6;
    public static final int HEADV2_LENGTH = 4;

    public int getPackVersion();

    public IDomainBase getSamplesDomainBase();

    public Pageable<byte[]> getSamples();
}

