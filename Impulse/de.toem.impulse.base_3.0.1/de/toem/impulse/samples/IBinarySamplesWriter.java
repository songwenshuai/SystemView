/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.ISamplesWriter;

public interface IBinarySamplesWriter
extends ISamplesWriter {
    public boolean write(long var1, boolean var3, byte[] var4);

    public boolean write(long var1, boolean var3, byte[] var4, int var5, int var6);
}

