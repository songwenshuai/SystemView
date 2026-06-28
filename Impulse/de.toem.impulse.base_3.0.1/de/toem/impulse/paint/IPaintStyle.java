/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint;

import de.toem.toolkits.pattern.element.Link;

public interface IPaintStyle {
    public int getType();

    public int getFormat();

    public int getValueColumnFormat();

    public boolean hasMod(int var1);

    public int getMods();

    public boolean hasValueAxis();

    public double getScaleFrom();

    public double getScaleTo();

    public int getScaleType();

    public String getScaleUnit();

    public Link getDescriptor();

    public String[][] getAdditional();

    public IPaintStyle clone();
}

