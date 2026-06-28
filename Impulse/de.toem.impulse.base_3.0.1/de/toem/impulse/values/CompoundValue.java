/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IReadableMembers;
import de.toem.impulse.samples.IReadableSample;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.convert.ConvertedMembers;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.impulse.samples.convert.SampleConverterConfiguration;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.IArrayValue;
import de.toem.impulse.values.IAttachment;
import de.toem.impulse.values.Logic;
import de.toem.toolkits.core.Utils;
import java.lang.reflect.Array;
import java.util.Collections;
import java.util.List;

public final class CompoundValue
extends ConvertedMembers
implements IReadableSample,
IReadableMembers,
Cloneable {
    protected IReadableSamples samples;
    protected int idx;
    protected long units;
    protected Object value;
    protected int tag;
    protected boolean none;
    protected int order;
    protected int group;
    protected int layer;
    protected SampleConverterConfiguration converterConfiguration;
    protected List<IAttachment> attachments;

    public CompoundValue(IReadableSamples samples, int idx, long units, boolean none, int tag, int group, int order, int layer, Object value, List<IAttachment> attachments) {
        this.samples = samples;
        this.idx = idx;
        this.units = units;
        this.none = none;
        this.tag = tag;
        this.order = order;
        this.layer = layer;
        this.group = group;
        this.value = value;
        this.attachments = attachments;
    }

    public CompoundValue(IReadableSamples samples, int idx, long units, boolean none, int tag, Object value) {
        this(samples, idx, units, none, tag, 0, 0, 0, value, null);
    }

    public CompoundValue(IReadableSamples samples, Object value) {
        this(samples, -1, 0L, false, 0, 0, 0, 0, value, null);
    }

    public boolean equals(Object o) {
        if (!(o instanceof CompoundValue)) {
            return false;
        }
        CompoundValue that = (CompoundValue)o;
        if (this.idx != that.idx) {
            return false;
        }
        if (this.units != that.units) {
            return false;
        }
        if (this.none != that.none) {
            return false;
        }
        if (this.tag != that.tag) {
            return false;
        }
        if (this.group != that.group) {
            return false;
        }
        if (this.order != that.order) {
            return false;
        }
        if (this.layer != that.layer) {
            return false;
        }
        if (Utils.equals(this.value, that.value)) {
            return false;
        }
        return !Utils.equals(this.attachments, that.attachments);
    }

    public boolean equalValues(CompoundValue that) {
        if (this.order != that.order) {
            return false;
        }
        if (this.layer != that.layer) {
            return false;
        }
        return Utils.equals(this.value, that.value);
    }

    @Override
    public SampleConverterConfiguration getConverterConfiguration() {
        return this.converterConfiguration == null && this.samples instanceof IReadableSamples ? this.samples.getConverterConfiguration() : this.converterConfiguration;
    }

    public void setConverterConfiguration(SampleConverterConfiguration config) {
        this.converterConfiguration = config;
    }

    public void setSamples(IReadableSamples samples) {
        this.samples = samples;
    }

    public IReadableSamples getSamples() {
        return this.samples;
    }

    public void setValue(Object value) {
        this.value = value;
    }

    public void setIdx(int idx) {
        this.idx = idx;
    }

    public void setUnits(long units) {
        this.units = units;
    }

    public void setTag(boolean tag) {
        this.tag = tag ? 1 : 0;
    }

    public void setTag(int tag) {
        this.tag = tag;
    }

    public void setNone(boolean none) {
        this.none = none;
    }

    public void setOrder(int order) {
        this.order = order;
    }

    public void setGroup(int group) {
        this.group = group;
    }

    public void setLayer(int layer) {
        this.layer = layer;
    }

    public void setAttachments(List<IAttachment> attachments) {
        this.attachments = attachments;
    }

    @Override
    public int getIndex() {
        return this.idx;
    }

    @Override
    public long getUnits() {
        return this.units;
    }

    @Override
    public DomainValue getPosition() {
        return this.samples != null && this.samples.getDomainBase() != null ? new DomainValue(this.samples.getDomainBase(), this.units) : null;
    }

    @Override
    public int getOrder() {
        return this.order;
    }

    @Override
    public int getGroup() {
        return this.group;
    }

    public int getLayer() {
        return this.layer;
    }

    @Override
    public Object val() {
        return this.value;
    }

    @Override
    public CompoundValue compound() {
        return this;
    }

    @Override
    public CompoundValue compound(int atchments) {
        return this;
    }

    @Override
    public CompoundPack packed() {
        return null;
    }

    @Override
    public List<IAttachment> attachments(int type) {
        return this.attachments;
    }

    @Override
    public boolean isNone() {
        return this.none;
    }

    @Override
    @Deprecated
    public boolean isConflict() {
        return this.isTagged();
    }

    @Override
    public boolean isTagged() {
        return this.tag > 0;
    }

    @Override
    public int getTag() {
        return this.tag;
    }

    @Override
    public ISamples.TagDomain getTagDomain() {
        return this.samples != null ? this.samples.getTagDomain() : ISamples.TagDomain.Unknown;
    }

    @Override
    public int noOfMembers() {
        if (this.value instanceof IReadableMembers) {
            return ((IReadableMembers)this.value).noOfMembers();
        }
        if (this.value instanceof IArrayValue) {
            return ((IArrayValue)this.value).length();
        }
        if (this.value instanceof byte[]) {
            return 0;
        }
        if (this.value != null && this.value.getClass().isArray()) {
            return Array.getLength(this.value);
        }
        return 0;
    }

    @Override
    public String nameOf(int memberIndex) {
        IMemberDescriptor m;
        if (this.value instanceof IReadableMembers) {
            return ((IReadableMembers)this.value).nameOf(memberIndex);
        }
        if (this.value != null && this.samples != null && (m = this.samples.getMemberDescriptor(memberIndex)) != null) {
            return m.getName();
        }
        return String.valueOf(memberIndex);
    }

    @Override
    public String pathOf(int memberIndex) {
        if (this.value instanceof IReadableMembers) {
            return ((IReadableMembers)this.value).pathOf(memberIndex);
        }
        return this.nameOf(memberIndex);
    }

    @Override
    public int idOf(int memberIndex) {
        if (this.value instanceof IReadableMembers) {
            return ((IReadableMembers)this.value).idOf(memberIndex);
        }
        if (this.value != null && this.samples != null) {
            return memberIndex;
        }
        return -1;
    }

    @Override
    public String textOf(int memberIndex) {
        if (this.value instanceof IReadableMembers) {
            return ((IReadableMembers)this.value).textOf(memberIndex);
        }
        return null;
    }

    @Override
    public int indexOf(Object memberIdentifier) {
        if (this.value instanceof IReadableMembers) {
            return ((IReadableMembers)this.value).indexOf(memberIdentifier);
        }
        if (this.value != null && this.samples != null) {
            if (memberIdentifier instanceof Integer) {
                return (Integer)memberIdentifier;
            }
            IMemberDescriptor m = this.samples.getMemberDescriptor(memberIdentifier);
            return m != null ? m.getId() : -1;
        }
        return -1;
    }

    @Override
    public List<Object> membersWithContent(String content) {
        if (this.value instanceof IReadableMembers) {
            return ((IReadableMembers)this.value).membersWithContent(content);
        }
        if (this.value != null && this.samples != null) {
            return this.samples.membersWithContent(content);
        }
        return Collections.EMPTY_LIST;
    }

    @Override
    public IMemberDescriptor descriptorOf(Object memberIdentifier) {
        if (this.value instanceof IReadableMembers) {
            return ((IReadableMembers)this.value).descriptorOf(memberIdentifier);
        }
        if (this.value != null && this.samples != null) {
            return this.samples.getMemberDescriptor(memberIdentifier);
        }
        return null;
    }

    @Override
    public boolean hasMember(Object memberIdentifier) {
        if (this.value instanceof IReadableMembers) {
            return ((IReadableMembers)this.value).hasMember(memberIdentifier);
        }
        if (this.value != null) {
            if (this.value instanceof byte[]) {
                return false;
            }
            int memberIdx = this.indexOf(memberIdentifier);
            if (memberIdx < 0) {
                return false;
            }
            if (this.value instanceof IArrayValue) {
                return memberIdx < ((IArrayValue)this.value).length();
            }
            if (this.value.getClass().isArray()) {
                return memberIdx < Array.getLength(this.value);
            }
        }
        return false;
    }

    @Override
    public Object valueOf(Object memberIdentifier) {
        if (this.value instanceof IReadableMembers) {
            return ((IReadableMembers)this.value).valueOf(memberIdentifier);
        }
        if (this.value != null) {
            if (this.value instanceof byte[]) {
                return null;
            }
            int memberIdx = this.indexOf(memberIdentifier);
            if (memberIdx < 0) {
                return false;
            }
            if (this.value instanceof IArrayValue) {
                this.value = ((IArrayValue)this.value).getArray();
            } else if (this.value.getClass().isArray()) {
                try {
                    return Array.get(this.value, memberIdx);
                }
                catch (Throwable throwable) {}
            }
        }
        return null;
    }

    @Override
    public String format(int format) {
        if ((format & 0xFFFF) == 65535) {
            format = format & 0xFFFF0000 | this.defaultFormat() & 0xFFFF;
        }
        if ((format & 0xFFFF0000) == -65536) {
            format = format & 0xFFFF | this.defaultFormat() & 0xFFFF0000;
        }
        if ((format & 0xFFFF) == 16) {
            return String.valueOf(this.idx);
        }
        if ((format & 0xFFFF) == 19) {
            return this.group >= 0 ? String.valueOf(this.group) : null;
        }
        if ((format & 0xFFFF) == 20) {
            return this.group >= 0 ? ISample.GROUP_ORDER_LABELS[this.order & 7] : null;
        }
        if ((format & 0xFFFF) >= 16 && (format & 0xFFFF) <= 20) {
            return this.samples != null ? this.samples.formatAt(this.idx, format) : null;
        }
        return super.format(format);
    }

    @Override
    public int defaultFormat() {
        return SampleConverter.getDefaultFormat(this.samples.getSignalType(), this.samples.getSignalDescriptor());
    }

    @Override
    public String formatOf(Object memberIdentifer, int format) {
        if ((format & 0xFFFF) == 65535) {
            format = this.defaultFormatOf(memberIdentifer);
        }
        if ((format & 0xFFFF) == 16) {
            return String.valueOf(this.idx);
        }
        if ((format & 0xFFFF) == 19) {
            return this.group >= 0 ? String.valueOf(this.group) : null;
        }
        if ((format & 0xFFFF) == 20) {
            return this.group >= 0 ? ISample.GROUP_ORDER_LABELS[this.order & 7] : null;
        }
        if ((format & 0xFFFF) >= 16 && (format & 0xFFFF) <= 16) {
            return this.samples != null ? this.samples.formatAt(this.idx, format) : null;
        }
        return super.formatOf(memberIdentifer, format);
    }

    @Override
    public int defaultFormatOf(Object memberIdentifier) {
        if (this.value instanceof IReadableMembers) {
            return ((IReadableMembers)this.value).defaultFormatOf(memberIdentifier);
        }
        if (this.value != null && this.samples != null) {
            if (this.value instanceof byte[]) {
                return 0;
            }
            IMemberDescriptor m = this.samples.getMemberDescriptor(memberIdentifier);
            if (m != null) {
                if (m.getFormat() != -1) {
                    return m.getFormat();
                }
                return m.defaultFormat();
            }
        }
        if (this.value instanceof int[]) {
            return 5;
        }
        if (this.value instanceof long[]) {
            return 5;
        }
        if (this.value instanceof float[]) {
            return 5;
        }
        if (this.value instanceof double[]) {
            return 5;
        }
        if (this.value instanceof Enumeration[]) {
            return 7;
        }
        if (this.value instanceof Logic) {
            return 1;
        }
        if (this.value instanceof String[]) {
            return 6;
        }
        return 0;
    }

    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }
}

