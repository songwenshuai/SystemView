/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.values.IAttachment;
import java.util.List;

public interface IPointer
extends IReadableSamples {
    public ISamples getReference();

    public boolean goPrev();

    public boolean goNext();

    public boolean hasPrev();

    public boolean hasNext();

    public boolean goPos1();

    public boolean goEnd();

    public void setIndex(int var1);

    public int getIndex();

    public int getMaxIndex();

    public int getMinIndex();

    public long getUnits();

    public long getUnits(boolean var1);

    public DomainValue getPosition();

    public DomainValue getPosition(boolean var1);

    public void setDelta(long var1);

    public long getDelta();

    public void setUnits(long var1);

    public void setPosition(DomainValue var1);

    public boolean hasIndexChanged();

    public void setChanged(boolean var1);

    public IPointer relative(String var1);

    public IPointer relative(DomainValue var1);

    public IPointer relative(long var1);

    public Object val();

    public List<IAttachment> attachments(int var1);

    public String format(int var1);

    public int defaultFormat();
}

