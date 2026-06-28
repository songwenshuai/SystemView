/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.axis;

public interface IValueAxis {
    public static final int AXIS_TYPE_LINEAR = 0;
    public static final int AXIS_TYPE_LOG10 = 1;

    public int getType();

    public String getClazz();

    public double getMin();

    public double getMax();

    public String getUnit();

    public double norm(double var1);
}

