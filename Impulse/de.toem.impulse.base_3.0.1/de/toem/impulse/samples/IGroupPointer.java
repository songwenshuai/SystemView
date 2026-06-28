/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.IPointer;
import de.toem.impulse.samples.IReadableMembers;
import de.toem.impulse.values.GroupedValue;
import de.toem.impulse.values.IAttachment;
import java.util.List;

public interface IGroupPointer
extends IPointer,
IReadableMembers {
    @Override
    public GroupedValue val();

    @Override
    public List<IAttachment> attachments(int var1);

    @Override
    public String format(int var1);

    @Override
    public int defaultFormat();
}

