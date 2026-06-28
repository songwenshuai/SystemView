/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.IReadableValue;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.samples.producer.AbstractUpdatableSamplesProducer;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.GroupedValue;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public abstract class AbstractSamplesFilter
extends AbstractUpdatableSamplesProducer {
    private ArrayList<int[]> sampleIdx = new ArrayList();
    private int count;
    private ArrayList<int[]> groupIdx = new ArrayList();
    private boolean filterGroups;
    private int groups;
    private final int FRAGMENT = 4096;
    private Map<Integer, Boolean> activeDecissions = new HashMap<Integer, Boolean>();

    public AbstractSamplesFilter() {
    }

    public AbstractSamplesFilter(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public AbstractSamplesFilter(String id, String name, Object sources, boolean filterGroups) {
        super(id, name, sources, ISamples.ProcessType.Discrete, ISamples.SignalType.Unknown, null, DomainBase.Unknown, null, null, null, null, null, null, null);
        this.filterGroups = filterGroups;
    }

    @Override
    protected boolean instatiate(IProgress p) {
        this.targetWriter = null;
        if (this.sources == null) {
            return false;
        }
        ArrayList<IReadableSamples> sources = new ArrayList<IReadableSamples>();
        ArrayList<ISamplePointer> pointers = new ArrayList<ISamplePointer>();
        for (IReadableSamples source : this.sources) {
            if (source == null) continue;
            pointers.add(new SamplePointer(source));
            sources.add(source);
            this.setReference(source);
            break;
        }
        if (sources.size() == 0) {
            return false;
        }
        this.iter = new SamplesIterator(pointers);
        return this.reference != null;
    }

    @Override
    protected boolean execute(IProgress p) {
        ISamplePointer[] input = this.iter.pointers();
        int executions = 0;
        while (this.iter.hasNext() && !p.isCanceled()) {
            CompoundValue value;
            long current = this.iter.next();
            int index = input[0].getIndex();
            int group = this.filterGroups ? input[0].getGroup() : -1;
            boolean filter = false;
            if (index < 0) continue;
            if (this.filterGroups && group >= 0) {
                value = input[0].compound();
                int order = value.getOrder();
                if (order == 1 || order == 5) {
                    GroupedValue gvalue = input[0].valuesAtGroup(group);
                    filter = this.filter(current, index, group, gvalue);
                    if (!filter) {
                        this.addSample(index);
                        this.addGroup(group);
                    }
                    this.activeDecissions.put(group, filter);
                } else {
                    boolean bl = filter = this.activeDecissions.containsKey(group) ? this.activeDecissions.get(group) : true;
                    if (!filter) {
                        this.addSample(index);
                    }
                    if (order == 3) {
                        this.activeDecissions.remove(group);
                    }
                }
            } else {
                value = input[0].compound();
                filter = this.filter(current, index, -1, value);
                if (!filter) {
                    this.addSample(index);
                }
            }
            if (++executions <= 0 || executions % 1000 != 0) continue;
            if (!this.continueExecution()) {
                return false;
            }
            int sourceCount = 0;
            ISamplePointer[] iSamplePointerArray = input;
            int n = input.length;
            int n2 = 0;
            while (n2 < n) {
                ISamplePointer i = iSamplePointerArray[n2];
                sourceCount += i != null ? i.getCount() : 0;
                ++n2;
            }
            if (p == null) continue;
            p.done(1.0 * (double)index / (double)sourceCount);
        }
        return true;
    }

    protected abstract boolean filter(long var1, int var3, int var4, IReadableValue var5);

    protected void addSample(int idx) {
        if (this.count % 4096 == 0) {
            int[] f = new int[4096];
            this.sampleIdx.add(f);
        }
        if (this.sampleIdx.size() * 4096 > this.count) {
            this.sampleIdx.get((int)(this.sampleIdx.size() - 1))[this.count % 4096] = idx;
            ++this.count;
        }
    }

    protected void addGroup(int group) {
        if (this.groups % 4096 == 0) {
            int[] f = new int[4096];
            this.groupIdx.add(f);
        }
        if (this.groupIdx.size() * 4096 > this.groups) {
            this.groupIdx.get((int)(this.groupIdx.size() - 1))[this.groups % 4096] = group;
            ++this.groups;
        }
    }

    protected void clear() {
        this.sampleIdx.clear();
        this.count = 0;
        this.groupIdx.clear();
        this.groups = 0;
    }

    public int fil2SrcIdx(int idx) {
        if (idx >= 0 && idx < this.count) {
            return this.sampleIdx.get(idx / 4096)[idx % 4096];
        }
        return -1;
    }

    public int src2FilIdx(int ridx) {
        int n = 0;
        for (int[] f : this.sampleIdx) {
            int m = 0;
            while (m < 4096 && n < this.count) {
                if (f[m] > ridx && n > 0) {
                    return n - 1;
                }
                ++m;
                ++n;
            }
        }
        return this.count - 1;
    }

    public int fil2SrcGroup(int group) {
        if (group >= 0 && group < this.groups) {
            return this.groupIdx.get(group / 4096)[group % 4096];
        }
        return -1;
    }

    public int src2FilGroup(int rgroup) {
        int n = 0;
        for (int[] f : this.groupIdx) {
            int m = 0;
            while (m < 4096 && n < this.groups) {
                if (f[m] > rgroup && n > 0) {
                    return n - 1;
                }
                ++m;
                ++n;
            }
        }
        return this.groups - 1;
    }

    @Override
    public int getCount() {
        return this.count;
    }

    @Override
    public int getGroups() {
        return this.groups;
    }

    @Override
    public boolean isEmpty() {
        return this.getCount() == 0;
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
    public boolean isNoneAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).isNoneAt(this.fil2SrcIdx(idx)) : true;
    }

    @Override
    public int indexAt(long units) {
        return this.src2FilIdx(this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).indexAt(units) : -1);
    }

    @Override
    public int indexAt(DomainValue position) {
        return this.src2FilIdx(this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).indexAt(position) : -1);
    }

    @Override
    public long unitsAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).unitsAt(this.fil2SrcIdx(idx)) : 0L;
    }

    @Override
    public DomainValue positionAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).positionAt(this.fil2SrcIdx(idx)) : DomainValue.NONE;
    }

    @Override
    public int groupAt(int idx) {
        return this.src2FilGroup(this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).groupAt(this.fil2SrcIdx(idx)) : -1);
    }

    @Override
    public int orderAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).orderAt(this.fil2SrcIdx(idx)) : -1;
    }

    @Override
    public GroupedValue valuesAtGroup(int group) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).valuesAtGroup(this.fil2SrcGroup(group)) : null;
    }

    @Override
    public GroupedValue valuesAtGroup(int group, int flags) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).valuesAtGroup(this.fil2SrcGroup(group), flags) : null;
    }

    @Override
    public List<IAttachment> attachmentsAtGroup(int group, int types) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).attachmentsAtGroup(this.fil2SrcGroup(group), types) : null;
    }

    @Override
    public int indexAtGroup(int group) {
        return this.src2FilIdx(this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).indexAtGroup(this.fil2SrcGroup(group)) : -1);
    }

    @Override
    public Object valueAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).valueAt(this.fil2SrcIdx(idx)) : null;
    }

    @Override
    public CompoundValue compoundAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).compoundAt(this.fil2SrcIdx(idx)) : null;
    }

    @Override
    public CompoundValue compoundAt(int idx, int flags) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).compoundAt(this.fil2SrcIdx(idx), flags) : null;
    }

    @Override
    public CompoundPack packedAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).packedAt(this.fil2SrcIdx(idx)) : null;
    }

    @Override
    public List<IAttachment> attachmentsAt(int idx, int types) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).attachmentsAt(this.fil2SrcIdx(idx), types) : null;
    }

    @Override
    @Deprecated
    public boolean isConflictAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).isTaggedAt(this.fil2SrcIdx(idx)) : false;
    }

    @Override
    public boolean isTaggedAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).isTaggedAt(this.fil2SrcIdx(idx)) : false;
    }

    @Override
    public int getTagAt(int idx) {
        return this.reference instanceof IReadableSamples ? ((IReadableSamples)this.reference).getTagAt(this.fil2SrcIdx(idx)) : 0;
    }

    @Override
    public Object getService(Class<?> cs) {
        return null;
    }
}

