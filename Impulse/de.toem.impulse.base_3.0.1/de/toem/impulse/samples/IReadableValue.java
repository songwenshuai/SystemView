/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.values.IAttachment;
import java.util.List;

public interface IReadableValue
extends ISample {
    public boolean isSample();

    public boolean isGroup();

    public long getUnits();

    public DomainValue getPosition();

    public int getIndex();

    public int getGroup();

    public String format(int var1);

    public List<IAttachment> attachments(int var1);

    public boolean isTagged();

    public int getTag();

    public ISamples.TagDomain getTagDomain();
}

