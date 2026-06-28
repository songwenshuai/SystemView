/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.base;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.IStatService;
import de.toem.impulse.samples.base.ReferencedSamples;
import de.toem.impulse.samples.base.SamplesStat;
import de.toem.impulse.samples.convert.SampleConverterConfiguration;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.GroupedValue;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.Collections;
import java.util.List;

public class ReferencedReadableSamples
extends ReferencedSamples
implements IReadableSamples {
    SampleConverterConfiguration converterConfiguration;
    IStatService stat;

    public ReferencedReadableSamples() {
    }

    public ReferencedReadableSamples(ISamples reference) {
        super(reference);
    }

    public ReferencedReadableSamples(ISamples reference, SampleConverterConfiguration formatConfiguration) {
        super(reference);
        this.converterConfiguration = formatConfiguration;
    }

    @Override
    public void setReference(ISamples reference) {
        super.setReference(reference);
        this.stat = null;
    }

    @Override
    public ISamplesReader getReader() {
        return this.reference instanceof ISamplesReader ? (ISamplesReader)this.reference : (this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getReader() : null);
    }

    @Override
    public ISamplesProducer getProducer() {
        return this.reference instanceof ISamplesProducer ? (ISamplesProducer)this.reference : (this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getProducer() : null);
    }

    @Override
    public SampleConverterConfiguration getConverterConfiguration() {
        return this.converterConfiguration == null && this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getConverterConfiguration() : this.converterConfiguration;
    }

    @Override
    public boolean isSettled() {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).isSettled() : false;
    }

    @Override
    public boolean ensureSettled(IProgress p) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).ensureSettled(p) : false;
    }

    @Override
    public boolean isSettling() {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).isSettling() : false;
    }

    @Override
    public int getCount() {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getCount() : 0;
    }

    @Override
    public int getGroups() {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getGroups() : 0;
    }

    @Override
    public boolean isEmpty() {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).isEmpty() : true;
    }

    @Override
    public boolean isNoneAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).isNoneAt(idx) : true;
    }

    @Override
    public int indexAt(long units) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).indexAt(units) : -1;
    }

    @Override
    public int indexAt(DomainValue position) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).indexAt(position) : -1;
    }

    @Override
    public long unitsAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).unitsAt(idx) : 0L;
    }

    @Override
    public DomainValue positionAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).positionAt(idx) : DomainValue.NONE;
    }

    @Override
    public int groupAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).groupAt(idx) : -1;
    }

    @Override
    public int orderAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).orderAt(idx) : 0;
    }

    @Override
    public GroupedValue valuesAtGroup(int group) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).valuesAtGroup(group) : null;
    }

    @Override
    public GroupedValue valuesAtGroup(int group, int flags) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).valuesAtGroup(group, flags) : null;
    }

    @Override
    public List<IAttachment> attachmentsAtGroup(int group, int type) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).attachmentsAtGroup(group, type) : null;
    }

    @Override
    public int indexAtGroup(int group) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).indexAtGroup(group) : -1;
    }

    @Override
    public Object valueAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).valueAt(idx) : null;
    }

    @Override
    public CompoundValue compoundAt(int idx) {
        CompoundValue value;
        CompoundValue compoundValue = value = this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).compoundAt(idx) : null;
        if (value != null && this.converterConfiguration != null) {
            value.setConverterConfiguration(this.converterConfiguration);
        }
        return value;
    }

    @Override
    public CompoundValue compoundAt(int idx, int flags) {
        CompoundValue value;
        CompoundValue compoundValue = value = this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).compoundAt(idx, flags) : null;
        if (value != null && this.converterConfiguration != null) {
            value.setConverterConfiguration(this.converterConfiguration);
        }
        return value;
    }

    @Override
    public CompoundPack packedAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).packedAt(idx) : null;
    }

    @Override
    public List<IAttachment> attachmentsAt(int idx, int types) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).attachmentsAt(idx, types) : null;
    }

    @Override
    @Deprecated
    public boolean isConflictAt(int idx) {
        return this.isTaggedAt(idx);
    }

    @Override
    public boolean isTaggedAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).isTaggedAt(idx) : false;
    }

    @Override
    public int getTagAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getTagAt(idx) : 0;
    }

    @Override
    public final SamplesStat getStat(int idx0, int idxN, int content) {
        IStatService stat = this.stat;
        if (stat == null || stat.getContent() != content || stat.getConverterConfiguration() != this.converterConfiguration) {
            this.stat = stat = (IStatService)this.getService(IStatService.class);
            if (stat != null) {
                stat.init(this.converterConfiguration, content);
            }
        }
        if (stat != null) {
            return stat.getStat(idx0, idxN);
        }
        return null;
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof ReferencedReadableSamples)) {
            return false;
        }
        ReferencedReadableSamples that = (ReferencedReadableSamples)obj;
        if (this.converterConfiguration != that.converterConfiguration) {
            return false;
        }
        return super.equals(obj);
    }

    @Override
    public List<IMemberDescriptor> getMemberDescriptors() {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getMemberDescriptors() : Collections.EMPTY_LIST;
    }

    @Override
    public List<Enumeration> getEnums(int enumerationType) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getEnums(enumerationType) : Collections.EMPTY_LIST;
    }

    @Override
    public IMemberDescriptor getMemberDescriptor(Object memberIdentifier) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getMemberDescriptor(memberIdentifier) : null;
    }

    @Override
    public List<Enumeration> getMemberEnums(Object memberIdentifier) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getMemberEnums(memberIdentifier) : Collections.EMPTY_LIST;
    }

    @Override
    public Enumeration getMemberEnum(Object memberIdentifier, String label) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getMemberEnum(memberIdentifier, label) : null;
    }

    @Override
    public Enumeration getMemberEnum(Object memberIdentifier, int value) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getMemberEnum(memberIdentifier, value) : null;
    }

    @Override
    public List<Object> membersWithContent(String content) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).membersWithContent(content) : Collections.EMPTY_LIST;
    }
}

