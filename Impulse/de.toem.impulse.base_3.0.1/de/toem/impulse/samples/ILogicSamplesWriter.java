/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.values.Logic;

public interface ILogicSamplesWriter
extends ISamplesWriter {
    public boolean write(long var1, boolean var3, byte var4);

    public boolean write(long var1, boolean var3, byte var4, byte[] var5, int var6, int var7);

    public boolean write(long var1, boolean var3, byte var4, String var5);

    public boolean write(long var1, boolean var3, Logic var4);

    @Deprecated
    public boolean write(long var1, Logic var3);

    public int getBitWidth();

    public boolean write(long var1, boolean var3, int var4, byte var5);

    public boolean write(long var1, boolean var3, int var4, byte var5, byte var6);

    public boolean write(long var1, boolean var3, int var4, byte var5, byte[] var6, int var7, int var8);

    public boolean writeByte(long var1, boolean var3, byte var4);

    public boolean writeBytesP(long var1, boolean var3, byte var4, byte[] var5, int var6, int var7);

    public boolean writeStringP(long var1, boolean var3, byte var4, String var5);

    public boolean writeByteS(long var1, boolean var3, int var4, byte var5);

    public boolean writeByteSP(long var1, boolean var3, int var4, byte var5, byte var6);

    public boolean writeBytesSP(long var1, boolean var3, int var4, byte var5, byte[] var6, int var7, int var8);

    public boolean writeLogic(long var1, boolean var3, Logic var4);

    @Deprecated
    public boolean writeLogic(long var1, Logic var3);
}

