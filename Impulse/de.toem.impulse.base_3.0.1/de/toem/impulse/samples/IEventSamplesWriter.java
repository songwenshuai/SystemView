/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.values.Enumeration;

public interface IEventSamplesWriter
extends ISamplesWriter {
    @Override
    public boolean write(long var1, boolean var3);

    public boolean write(long var1, boolean var3, int var4);

    public boolean write(long var1, boolean var3, String var4);

    public boolean write(long var1, boolean var3, Enumeration var4);

    public boolean write(long var1, boolean var3, int[] var4);

    public boolean write(long var1, boolean var3, String[] var4);

    public boolean write(long var1, boolean var3, Object[] var4);

    public boolean write(long var1, boolean var3, Enumeration[] var4);

    public boolean writeInt(long var1, boolean var3, int var4);

    public boolean writeString(long var1, boolean var3, String var4);

    public boolean writeEnum(long var1, boolean var3, Enumeration var4);

    public boolean writeIntArray(long var1, boolean var3, int[] var4);

    public boolean writeIntArgs(long var1, boolean var3, int ... var4);

    public boolean writeStringArray(long var1, boolean var3, String[] var4);

    public boolean writeStringArgs(long var1, boolean var3, String ... var4);

    public boolean writeEnumArray(long var1, boolean var3, Enumeration[] var4);

    public boolean writeEnumArgs(long var1, boolean var3, Enumeration ... var4);
}

