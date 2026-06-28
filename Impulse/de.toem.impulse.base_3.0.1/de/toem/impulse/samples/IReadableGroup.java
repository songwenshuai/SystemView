/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.IReadableValue;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.GroupedValue;
import de.toem.impulse.values.IAttachment;
import java.util.List;

public interface IReadableGroup
extends IReadableValue {
    public int noOfEvents();

    public GroupedValue val();

    public List<? extends CompoundValue> compounds();

    @Override
    public List<IAttachment> attachments(int var1);

    public Object valOfEvent(int var1);

    public CompoundValue compoundOfEvent(int var1);

    @Override
    public int getGroup();

    public int getLayer();

    public DomainValue getStart();

    public DomainValue getEnd();

    public long getStartUnits();

    public long getEndUnits();

    public boolean hasTag();

    @Override
    public int getTag();

    @Override
    public String format(int var1);

    public int defaultFormat();
}

