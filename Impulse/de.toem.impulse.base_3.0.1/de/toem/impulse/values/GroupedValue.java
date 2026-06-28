/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.IReadableGroup;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.convert.ConvertedMembers;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.IAttachment;
import java.util.ArrayList;
import java.util.List;

public abstract class GroupedValue
extends ConvertedMembers
implements IReadableGroup {
    List<CompoundValue> events = new ArrayList<CompoundValue>();

    public GroupedValue(List<? extends CompoundValue> events) {
        this.events.addAll(events);
    }

    @Override
    public int noOfEvents() {
        return this.events.size();
    }

    @Override
    public GroupedValue val() {
        return this;
    }

    @Override
    public List<? extends CompoundValue> compounds() {
        return this.events;
    }

    @Override
    public List<IAttachment> attachments(int type) {
        ArrayList<IAttachment> attachments = new ArrayList<IAttachment>();
        for (CompoundValue c : this.events) {
            List<IAttachment> a;
            if (c == null || (a = c.attachments(type)) == null) continue;
            attachments.addAll(a);
        }
        return attachments;
    }

    @Override
    public Object valOfEvent(int event) {
        return this.events.get(event) != null ? this.events.get(event).val() : null;
    }

    @Override
    public CompoundValue compoundOfEvent(int event) {
        return this.events.get(event);
    }

    @Deprecated
    public CompoundValue get(int idx) {
        return this.events.get(idx);
    }

    @Deprecated
    public List<? extends CompoundValue> getEvents() {
        return this.events;
    }

    @Override
    public int getIndex() {
        return this.getGroup();
    }

    public long getStartIndex() {
        return this.events != null && this.events.size() >= 1 ? this.events.get(0).getIndex() : -1;
    }

    @Override
    public int getGroup() {
        return this.events != null && this.events.size() >= 1 ? this.events.get(0).getGroup() : -1;
    }

    @Override
    public int getLayer() {
        return this.events != null && this.events.size() >= 1 ? this.events.get(0).getLayer() : 0;
    }

    @Override
    public DomainValue getStart() {
        return this.events != null && this.events.size() >= 1 ? this.events.get(0).getPosition() : null;
    }

    @Override
    public final DomainValue getPosition() {
        return this.getStart();
    }

    @Override
    public DomainValue getEnd() {
        return this.events != null && this.events.size() >= 1 ? this.events.get(this.events.size() - 1).getPosition() : null;
    }

    @Override
    public long getStartUnits() {
        return this.events != null && this.events.size() >= 1 ? this.events.get(0).getUnits() : Long.MIN_VALUE;
    }

    @Override
    public final long getUnits() {
        return this.getStartUnits();
    }

    @Override
    public long getEndUnits() {
        return this.events != null && this.events.size() >= 1 ? this.events.get(this.events.size() - 1).getUnits() : Long.MIN_VALUE;
    }

    @Deprecated
    public boolean hasConflict() {
        for (CompoundValue event : this.events) {
            if (!event.isTagged()) continue;
            return true;
        }
        return false;
    }

    @Override
    public boolean hasTag() {
        for (CompoundValue event : this.events) {
            if (!event.isTagged()) continue;
            return true;
        }
        return false;
    }

    @Override
    public final boolean isTagged() {
        return this.hasTag();
    }

    @Override
    public int getTag() {
        int tag = 0;
        for (CompoundValue event : this.events) {
            if (event.getTag() <= 0 || tag != 0 && event.getTag() >= tag) continue;
            tag = event.getTag();
        }
        return tag;
    }

    @Override
    public ISamples.TagDomain getTagDomain() {
        return this.events != null && this.events.size() >= 1 ? this.events.get(0).getTagDomain() : ISamples.TagDomain.Unknown;
    }

    @Override
    public abstract int defaultFormat();
}

