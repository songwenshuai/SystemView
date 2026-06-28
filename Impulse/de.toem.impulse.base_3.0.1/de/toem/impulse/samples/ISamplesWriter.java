/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IPackedSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesLegend;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.toolkits.pattern.element.exploits.Marker;
import de.toem.toolkits.pattern.pageable.Pageable;

public interface ISamplesWriter
extends IPackedSamples {
    public static final long NEXT_UNITS = Long.MIN_VALUE;

    public void setDomainBase(IDomainBase var1);

    public void setTagDomain(ISamples.TagDomain var1);

    public boolean isEmpty();

    public int getCount();

    public int getGroups();

    public boolean isOpen();

    public boolean open(long var1);

    public boolean open(long var1, Pageable<byte[]> var3);

    public boolean open(long var1, int var3, int var4, Pageable<byte[]> var5);

    public boolean open(long var1, long var3, int var5, int var6, Pageable<byte[]> var7);

    public boolean open(long var1, long var3, long var5, int var7, int var8, Pageable<byte[]> var9);

    public long getMaxUnits();

    public void close();

    public void close(long var1);

    public void flush(long var1);

    public void flush();

    public boolean apply(Signal var1);

    public void setLegend(ISamplesLegend var1);

    @Deprecated
    public int addMember(String var1, String var2, int var3);

    public boolean setMember(int var1, String var2, String var3, int var4);

    public boolean setMember(int var1, String var2, int var3, String var4, int var5);

    public boolean setMember(int var1, int var2, String var3, int var4, String var5, int var6);

    public boolean setEnum(int var1, String var2, int var3);

    public boolean addMarker(Marker var1);

    public boolean attachRelation(String var1, String var2, long var3);

    public boolean attachRelation(int var1, String var2, String var3, long var4);

    public boolean attachRelation(int var1, String var2, String var3, long var4, IDomainBase var6);

    public boolean attachRelation(int var1, String var2, String var3, long var4, IDomainBase var6, int var7, int var8);

    public boolean attachRelation(int var1, int var2, long var3);

    public boolean attachRelation(int var1, int var2, int var3, long var4);

    public boolean attachRelation(int var1, int var2, int var3, long var4, int var6);

    public boolean attachRelation(int var1, int var2, int var3, long var4, int var6, int var7, int var8);

    public boolean attachLabel(String var1);

    public boolean attachLabel(int var1);

    public boolean write(long var1, boolean var3);

    public boolean write(long var1, int var3);

    public boolean writeNone(long var1, boolean var3);

    public boolean writeNone(long var1, int var3);

    public boolean writeSample(long var1, byte var3);

    public boolean writeSample(long var1, byte var3, byte var4);

    public boolean writeSample(long var1, byte var3, byte[] var4, int var5, int var6);

    public boolean writeSample(long var1, byte var3, int var4, int var5, byte[] var6, int var7, int var8);

    public boolean writeSample(CompoundPack var1);

    public boolean writeSample(CompoundValue var1);

    public boolean writeSample(long var1, boolean var3, Object var4);

    public boolean writeSample(long var1, int var3, Object var4);

    public boolean writeSample(long var1, boolean var3, int var4, int var5, int var6, Object var7);

    public boolean writeSample(long var1, int var3, int var4, int var5, int var6, Object var7);
}

