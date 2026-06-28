/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

public interface IMemberDescriptor {
    public int getId();

    public int getParentId();

    public String getName();

    public String getPath();

    public String getContent();

    public int getFormat();

    public int defaultFormat();

    public int getType();

    public int getRawType();

    public boolean isHidden();

    public boolean isValidUntilChange();
}

