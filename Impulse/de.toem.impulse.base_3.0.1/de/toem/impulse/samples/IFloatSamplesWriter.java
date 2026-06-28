/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.INumberSamplesWriter;
import java.math.BigDecimal;

public interface IFloatSamplesWriter
extends INumberSamplesWriter {
    public boolean write(long var1, double var3);

    public boolean write(long var1, boolean var3, float var4);

    public boolean write(long var1, boolean var3, double var4);

    public boolean write(long var1, boolean var3, BigDecimal var4);

    @Override
    public boolean write(long var1, boolean var3, Number var4);

    public boolean write(long var1, boolean var3, float[] var4);

    public boolean write(long var1, boolean var3, double[] var4);

    public boolean write(long var1, boolean var3, BigDecimal[] var4);

    public boolean writeFloat(long var1, boolean var3, float var4);

    public boolean writeDouble(long var1, boolean var3, double var4);

    public boolean writeBig(long var1, boolean var3, BigDecimal var4);

    public boolean writeFloatArray(long var1, boolean var3, float[] var4);

    public boolean writeFloatArgs(long var1, boolean var3, float ... var4);

    public boolean writeDoubleArray(long var1, boolean var3, double[] var4);

    public boolean writeDoubleArgs(long var1, boolean var3, double ... var4);

    public boolean writeBigArray(long var1, boolean var3, BigDecimal[] var4);

    public boolean writeBigArgs(long var1, boolean var3, BigDecimal ... var4);
}

