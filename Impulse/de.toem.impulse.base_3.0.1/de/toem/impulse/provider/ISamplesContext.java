/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.provider;

import de.toem.impulse.provider.ISamplesCache;
import de.toem.toolkits.pattern.provider.IContext;

public interface ISamplesContext
extends IContext {
    public static final int MODE_FETCH = 0;
    public static final int MODE_UPDATE_ALL = 1;
    public static final int MODE_UPDATE_SAMPLES = 2;

    public ISamplesCache getSamplesCache();

    default public int getSamplesMode() {
        return 0;
    }
}

