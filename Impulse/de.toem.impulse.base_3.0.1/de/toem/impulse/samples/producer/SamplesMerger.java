/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.base.SamplesStat;
import de.toem.impulse.samples.convert.ConvertedSamples;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.samples.legend.SamplesLegend;
import de.toem.impulse.samples.producer.AbstractUpdatableSamplesProducer;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.GroupedValue;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.pattern.element.exploits.Markers;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class SamplesMerger
extends AbstractUpdatableSamplesProducer {
    int[] currentIndex;
    int[] currentGroup;

    public SamplesMerger() {
    }

    public SamplesMerger(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public SamplesMerger(String id, String name, List<IReadableSamples> sources) {
        super(id, name, sources, ISamples.ProcessType.Discrete, ISamples.SignalType.Unknown, null, DomainBase.Unknown, null, null, null, null, null, null, null);
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
        }
        if (sources.size() == 0) {
            return false;
        }
        this.iter = new SamplesIterator(pointers);
        this.setReference(new MergedSamples(sources));
        this.currentIndex = new int[sources.size()];
        int n = 0;
        while (n < this.currentIndex.length) {
            this.currentIndex[n] = -1;
            ++n;
        }
        this.currentGroup = new int[sources.size()];
        n = 0;
        while (n < this.currentGroup.length) {
            this.currentGroup[n] = -1;
            ++n;
        }
        return this.reference != null;
    }

    @Override
    protected boolean execute(IProgress p) {
        if (!(this.reference instanceof MergedSamples)) {
            return false;
        }
        ISamplePointer[] input = this.iter.pointers();
        int executions = 0;
        while (this.iter.hasNext() && !p.isCanceled()) {
            this.iter.next();
            int i = 0;
            while (i < input.length) {
                ISamplePointer pointer = input[i];
                if (pointer.getIndex() > this.currentIndex[i]) {
                    this.currentIndex[i] = pointer.getIndex();
                    ((MergedSamples)this.reference).addSample(i, this.currentIndex[i]);
                    int group = pointer.getGroup();
                    if (this.currentGroup[i] < group) {
                        this.currentGroup[i] = group;
                        ((MergedSamples)this.reference).addGroup(i, this.currentGroup[i]);
                    }
                }
                ++i;
            }
            if (++executions <= 0 || executions % 1000 != 0 || this.continueExecution()) continue;
            return false;
        }
        return true;
    }

    public IReadableSamples getSourceAt(int idx) {
        return this.reference instanceof MergedSamples ? ((MergedSamples)this.reference).getSourceAt(idx) : null;
    }

    public int getSourceIdxAt(int idx) {
        return this.reference instanceof MergedSamples ? ((MergedSamples)this.reference).getSourceIdxAt(idx) : -1;
    }

    public int indexAtSource(IReadableSamples samples, int idx) {
        return this.reference instanceof MergedSamples ? Integer.valueOf(((MergedSamples)this.reference).indexAtSource(samples, idx)) : null;
    }

    class MergedSamples
    extends ConvertedSamples
    implements IReadableSamples {
        IReadableSamples[] sources;
        private ArrayList<long[]> sampleIdx = new ArrayList();
        private int count;
        private ArrayList<long[]> groupIdx = new ArrayList();
        private int groups;
        private final int FRAGMENT = 4096;
        boolean tagged;

        public MergedSamples(List<IReadableSamples> sources) {
            for (IReadableSamples source : sources) {
                if (this.tagged || !source.hasTag()) continue;
                this.tagged = true;
            }
            this.sources = sources.toArray(new IReadableSamples[sources.size()]);
        }

        protected void addSample(int source, int idx) {
            if (this.count % 4096 == 0) {
                long[] f = new long[4096];
                this.sampleIdx.add(f);
            }
            if (this.sampleIdx.size() * 4096 > this.count) {
                this.sampleIdx.get((int)(this.sampleIdx.size() - 1))[this.count % 4096] = (long)source << 32 | (long)idx;
                ++this.count;
            }
        }

        protected void addGroup(int source, int group) {
            if (this.groups % 4096 == 0) {
                long[] f = new long[4096];
                this.groupIdx.add(f);
            }
            if (this.groupIdx.size() * 4096 > this.groups) {
                this.groupIdx.get((int)(this.groupIdx.size() - 1))[this.groups % 4096] = (long)source << 32 | (long)group;
                ++this.groups;
            }
        }

        protected void clear() {
            this.sampleIdx.clear();
            this.count = 0;
            this.groupIdx.clear();
            this.groups = 0;
        }

        protected long mrg2SrcIdx(int idx) {
            if (idx >= 0 && idx < this.count) {
                return this.sampleIdx.get(idx / 4096)[idx % 4096];
            }
            return -1L;
        }

        protected int src2MrgIdx(int source, int idx) {
            int n = 0;
            long ridx = (long)source << 32 | (long)idx;
            for (long[] f : this.sampleIdx) {
                int m = 0;
                while (m < 4096 && n < this.count) {
                    if (f[m] == ridx) {
                        return n;
                    }
                    ++m;
                    ++n;
                }
            }
            return this.count - 1;
        }

        protected long mrg2SrcGroup(int group) {
            if (group >= 0 && group < this.groups) {
                return this.groupIdx.get(group / 4096)[group % 4096];
            }
            return -1L;
        }

        protected int src2MrgGroup(int source, int group) {
            int n = 0;
            long ridx = (long)source << 32 | (long)group;
            for (long[] f : this.groupIdx) {
                int m = 0;
                while (m < 4096 && n < this.groups) {
                    if (f[m] == ridx) {
                        return n;
                    }
                    ++m;
                    ++n;
                }
            }
            return this.groups - 1;
        }

        @Override
        public ISamples.ProcessType getProcessType() {
            return ISamples.ProcessType.Discrete;
        }

        @Override
        public ISamples.SignalType getSignalType() {
            return ISamples.SignalType.Unknown;
        }

        @Override
        public ISamples.SignalDescriptor getSignalDescriptor() {
            return ISamples.SignalDescriptor.DEFAULT;
        }

        @Override
        public String getContent() {
            return this.getSignalDescriptor().getContent();
        }

        @Override
        public int getScale() {
            return this.getSignalDescriptor().getScale();
        }

        @Override
        public int getAccuracy() {
            return this.getSignalDescriptor().getAccuracy();
        }

        @Override
        public int getFlags() {
            return this.getSignalDescriptor().getFlags();
        }

        @Override
        public int getFormat() {
            return this.getSignalDescriptor().getFormat();
        }

        @Override
        public boolean isEmpty() {
            return this.count == 0;
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
        public IDomainBase getDomainBase() {
            return SamplesMerger.this.getDomainBase();
        }

        @Override
        public DomainValue getStart() {
            return SamplesMerger.this.getStart();
        }

        @Override
        public DomainValue getEnd() {
            return SamplesMerger.this.getEnd();
        }

        @Override
        public DomainValue getRate() {
            return SamplesMerger.this.getRate();
        }

        @Override
        public long getStartUnits() {
            return SamplesMerger.this.getStartUnits();
        }

        @Override
        public long getEndUnits() {
            return SamplesMerger.this.getEndUnits();
        }

        @Override
        public long getRateUnits() {
            return SamplesMerger.this.getRateUnits();
        }

        @Override
        @Deprecated
        public boolean hasConflict() {
            return this.tagged;
        }

        @Override
        public boolean hasTag() {
            return this.tagged;
        }

        @Override
        public ISamples.TagDomain getTagDomain() {
            return ISamples.TagDomain.Unknown;
        }

        @Override
        public final boolean isSettled() {
            return true;
        }

        @Override
        public boolean ensureSettled(IProgress p) {
            return true;
        }

        @Override
        public boolean isSettling() {
            return false;
        }

        @Override
        public int indexAt(DomainValue position) {
            if (position == null) {
                return -1;
            }
            if ((position = position.convertTo(this.getDomainBase())) != null) {
                return this.indexAt(position.units);
            }
            return -1;
        }

        @Override
        public final int indexAt(long units) {
            int s = 0;
            int idx = -1;
            long found = Long.MIN_VALUE;
            int at = -1;
            IReadableSamples[] iReadableSamplesArray = this.sources;
            int n = this.sources.length;
            int n2 = 0;
            while (n2 < n) {
                IReadableSamples source = iReadableSamplesArray[n2];
                int i = source.indexAt(units);
                long u = source.unitsAt(i);
                if (idx == -1 || u > found) {
                    idx = i;
                    found = u;
                    at = s;
                }
                ++s;
                ++n2;
            }
            return idx != -1 ? this.src2MrgIdx(at, idx) : -1;
        }

        @Override
        public long unitsAt(int idx) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return 0L;
            }
            return this.sources[(int)(ridx >> 32)].unitsAt((int)ridx);
        }

        @Override
        public DomainValue positionAt(int idx) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return null;
            }
            return this.sources[(int)(ridx >> 32)].positionAt((int)ridx);
        }

        @Override
        public boolean isNoneAt(int idx) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return true;
            }
            return this.sources[(int)(ridx >> 32)].isNoneAt((int)ridx);
        }

        @Override
        @Deprecated
        public boolean isConflictAt(int idx) {
            return this.isTaggedAt(idx);
        }

        @Override
        public boolean isTaggedAt(int idx) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return false;
            }
            return this.sources[(int)(ridx >> 32)].isTaggedAt((int)ridx);
        }

        @Override
        public int getTagAt(int idx) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return 0;
            }
            return this.sources[(int)(ridx >> 32)].getTagAt((int)ridx);
        }

        @Override
        public SamplesStat getStat(int idx0, int idxN, int content) {
            return null;
        }

        @Override
        public Object valueAt(int idx) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return null;
            }
            return this.sources[(int)(ridx >> 32)].valueAt((int)ridx);
        }

        @Override
        public CompoundValue compoundAt(int idx) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return null;
            }
            return this.sources[(int)(ridx >> 32)].compoundAt((int)ridx);
        }

        @Override
        public CompoundValue compoundAt(int idx, int flags) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return null;
            }
            return this.sources[(int)(ridx >> 32)].compoundAt((int)ridx, flags);
        }

        @Override
        public CompoundPack packedAt(int idx) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return null;
            }
            return this.sources[(int)(ridx >> 32)].packedAt((int)ridx);
        }

        @Override
        public List<IAttachment> attachmentsAt(int idx, int types) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return null;
            }
            return this.sources[(int)(ridx >> 32)].attachmentsAt((int)ridx, types);
        }

        @Override
        public int defaultFormatAt(int idx) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return -1;
            }
            return this.sources[(int)(ridx >> 32)].defaultFormatAt((int)ridx);
        }

        @Override
        public int groupAt(int idx) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return -1;
            }
            int g = this.sources[(int)(ridx >> 32)].groupAt((int)ridx);
            return this.src2MrgGroup((int)(ridx >> 32), g);
        }

        @Override
        public int orderAt(int idx) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return 0;
            }
            return this.sources[(int)(ridx >> 32)].orderAt((int)ridx);
        }

        @Override
        public GroupedValue valuesAtGroup(int group) {
            long ridx = this.mrg2SrcGroup(group);
            if (ridx < 0L) {
                return null;
            }
            return this.sources[(int)(ridx >> 32)].valuesAtGroup((int)ridx);
        }

        @Override
        public GroupedValue valuesAtGroup(int group, int flags) {
            long ridx = this.mrg2SrcGroup(group);
            if (ridx < 0L) {
                return null;
            }
            return this.sources[(int)(ridx >> 32)].valuesAtGroup((int)ridx, flags);
        }

        @Override
        public List<IAttachment> attachmentsAtGroup(int group, int types) {
            long ridx = this.mrg2SrcGroup(group);
            if (ridx < 0L) {
                return null;
            }
            return this.sources[(int)(ridx >> 32)].attachmentsAtGroup((int)ridx, types);
        }

        @Override
        public int indexAtGroup(int group) {
            long ridx = this.mrg2SrcGroup(group);
            if (ridx < 0L) {
                return -1;
            }
            int i = this.sources[(int)(ridx >> 32)].indexAtGroup((int)ridx);
            return this.src2MrgIdx((int)(ridx >> 32), i);
        }

        @Override
        public SamplesLegend getLegend() {
            return null;
        }

        public IReadableSamples getSourceAt(int idx) {
            long ridx = this.mrg2SrcIdx(idx);
            if (ridx < 0L) {
                return null;
            }
            return this.sources[(int)(ridx >> 32)];
        }

        public int getSourceIdxAt(int idx) {
            return (int)this.mrg2SrcIdx(idx);
        }

        public int indexAtSource(IReadableSamples samples, int idx) {
            if (this.sources != null && idx >= 0) {
                int source = -1;
                int n = 0;
                while (n < this.sources.length) {
                    if (samples == this.sources[n]) {
                        source = n;
                        break;
                    }
                    ++n;
                }
                if (source >= 0) {
                    return this.src2MrgIdx(source, idx);
                }
            }
            return -1;
        }

        @Override
        public ISamplesReader getReader() {
            return null;
        }

        @Override
        public ISamplesProducer getProducer() {
            return null;
        }

        @Override
        public String getName() {
            return null;
        }

        @Override
        public String getMessage() {
            return null;
        }

        @Override
        public String getError() {
            return null;
        }

        @Override
        public String getId() {
            return null;
        }

        @Override
        public Markers getMarkers() {
            return null;
        }

        @Override
        public Object getService(Class<?> cs) {
            return null;
        }

        @Override
        public int getRelease() {
            return 0;
        }

        @Override
        public boolean isVolatile() {
            return false;
        }

        @Override
        public boolean isMonotonous() {
            return true;
        }

        @Override
        public boolean isReleased() {
            return true;
        }

        @Override
        public Object getData() {
            return null;
        }

        @Override
        public Object getData(String key) {
            return null;
        }

        @Override
        public void setData(Object value) {
        }

        @Override
        public void setData(String key, Object value) {
        }

        @Override
        public List<IMemberDescriptor> getMemberDescriptors() {
            return Collections.EMPTY_LIST;
        }

        @Override
        public List<Enumeration> getEnums(int enumerationType) {
            return Collections.EMPTY_LIST;
        }

        @Override
        public IMemberDescriptor getMemberDescriptor(Object memberIdentifier) {
            return null;
        }

        @Override
        public List<Enumeration> getMemberEnums(Object memberIdentifier) {
            return Collections.EMPTY_LIST;
        }

        @Override
        public Enumeration getMemberEnum(Object memberIdentifier, String label) {
            return null;
        }

        @Override
        public Enumeration getMemberEnum(Object memberIdentifier, int value) {
            return null;
        }

        @Override
        public List<Object> membersWithContent(String content) {
            return Collections.EMPTY_LIST;
        }
    }
}

