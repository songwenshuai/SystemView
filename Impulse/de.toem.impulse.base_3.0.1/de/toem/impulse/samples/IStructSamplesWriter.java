/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.values.StructMember;

public interface IStructSamplesWriter
extends ISamplesWriter {
    public boolean write(long var1, boolean var3, StructMember[] var4);

    public boolean write(long var1, int var3, StructMember[] var4);

    public boolean write(long var1, boolean var3, int var4, int var5, int var6, StructMember[] var7);

    public boolean write(long var1, int var3, int var4, int var5, int var6, StructMember[] var7);

    public StructMember[] createMembers(int var1);

    public StructMember createMember(StructMember[] var1, int var2, String var3, int var4, String var5, int var6);
}

