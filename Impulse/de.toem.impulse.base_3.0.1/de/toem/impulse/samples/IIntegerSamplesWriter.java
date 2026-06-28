/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.INumberSamplesWriter;
import java.math.BigInteger;

public interface IIntegerSamplesWriter
extends INumberSamplesWriter {
    public boolean write(long var1, boolean var3, int var4);

    public boolean write(long var1, boolean var3, long var4);

    public boolean write(long var1, boolean var3, BigInteger var4);

    @Override
    public boolean write(long var1, boolean var3, Number var4);

    public boolean write(long var1, boolean var3, int[] var4);

    public boolean write(long var1, boolean var3, long[] var4);

    public boolean write(long var1, boolean var3, BigInteger[] var4);

    public boolean writeInt(long var1, boolean var3, int var4);

    public boolean writeLong(long var1, boolean var3, long var4);

    public boolean writeBig(long var1, boolean var3, BigInteger var4);

    public boolean writeIntArray(long var1, boolean var3, int[] var4);

    public boolean writeIntArgs(long var1, boolean var3, int ... var4);

    public boolean writeLongArray(long var1, boolean var3, long[] var4);

    public boolean writeLongArgs(long var1, boolean var3, long ... var4);

    public boolean writeBigArray(long var1, boolean var3, BigInteger[] var4);

    public boolean writeBigArgs(long var1, boolean var3, BigInteger[] var4);
}

