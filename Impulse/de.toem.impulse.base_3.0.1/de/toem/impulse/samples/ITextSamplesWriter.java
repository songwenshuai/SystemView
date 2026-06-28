/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.ISamplesWriter;

public interface ITextSamplesWriter
extends ISamplesWriter {
    public boolean write(long var1, boolean var3, String var4);

    public boolean write(long var1, boolean var3, String[] var4);

    public boolean writeString(long var1, boolean var3, String var4);

    public boolean writeStringArray(long var1, boolean var3, String[] var4);

    public boolean writeStringArgs(long var1, boolean var3, String ... var4);
}

