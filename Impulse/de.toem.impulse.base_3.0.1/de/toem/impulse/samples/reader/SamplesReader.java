/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.reader;

import de.toem.impulse.cells.record.PortScope;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IGroupService;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IPackedSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.IStatService;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.base.SamplesStat;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.impulse.samples.convert.SampleConverterConfiguration;
import de.toem.impulse.samples.legend.DefaultSamplesLegend;
import de.toem.impulse.values.AttachedLabel;
import de.toem.impulse.values.AttachedRelation;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.GroupedValue;
import de.toem.impulse.values.IAttachment;
import de.toem.impulse.values.Logic;
import de.toem.impulse.values.Struct;
import de.toem.impulse.values.Transaction;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.pattern.threading.IProgressStatus;
import java.lang.ref.WeakReference;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class SamplesReader
extends PackedSamples
implements ISamplesReader {
    private int sourceRelease;
    private int readerRelease;
    private boolean extendable;
    private SamplesFragmentReader[] fragments;
    private boolean simpleGeometry;
    private boolean settled;
    protected boolean updated;
    private int samplesPerFragment;
    private int count;
    private int groups;
    private int version;
    IStatService stat;
    private static volatile /* synthetic */ int[] $SWITCH_TABLE$de$toem$impulse$samples$ISamples$SignalType;

    public SamplesReader(Signal signal, IDomainBase domainBase) {
        super(signal, domainBase);
        if (this.samples != null) {
            this.extendable = this.sourceRelease != 0;
            this.readerRelease = 0;
        } else {
            this.extendable = false;
            this.sourceRelease = -1;
            this.readerRelease = -1;
        }
    }

    public SamplesReader(IPackedSamples packed, IDomainBase domainBase) {
        super(packed, domainBase);
        if (this.samples != null) {
            this.sourceRelease = this.samples.getRelease();
            this.extendable = packed.isVolatile();
            this.readerRelease = 0;
        } else {
            this.extendable = false;
            this.sourceRelease = -1;
            this.readerRelease = -1;
        }
    }

    private synchronized boolean updateFragments() {
        if (this.samples == null || this.fragments == null) {
            return false;
        }
        int[] fragmentIds = this.samples.getFragmentIds();
        if (this.fragments.length > fragmentIds.length || this.fragments.length == 0) {
            return false;
        }
        int n = 0;
        while (n < this.fragments.length) {
            if (this.fragments[n].fragmentId != fragmentIds[n]) {
                return false;
            }
            if (this.fragments[n].fragmentRelease != this.samples.getFragmentRelease(fragmentIds[n]) && n < this.fragments.length - 1) {
                return false;
            }
            ++n;
        }
        SamplesFragmentReader[] newFragments = new SamplesFragmentReader[fragmentIds.length];
        int n2 = 0;
        while (n2 < this.fragments.length) {
            newFragments[n2] = this.fragments[n2];
            ++n2;
        }
        n2 = this.fragments.length;
        while (n2 < fragmentIds.length) {
            newFragments[n2] = new SamplesFragmentReader(fragmentIds[n2]);
            ++n2;
        }
        this.sourceRelease = this.samples != null ? this.samples.getRelease() : 0;
        this.fragments = newFragments;
        this.updated = true;
        return true;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    @Override
    public int update(Signal signal, IDomainBase domainBase) {
        Signal signal2 = signal;
        synchronized (signal2) {
            block9: {
                long send;
                block11: {
                    block10: {
                        block8: {
                            IDomainBase sdb = DomainBase.parse(signal.domainBase);
                            IDomainBase rdb = domainBase != null ? domainBase : sdb;
                            long sstart = sdb.convertTo(rdb, signal.start);
                            send = sdb.convertTo(rdb, signal.end);
                            PortScope pscope = (PortScope)signal.getParent(PortScope.class);
                            if (pscope != null) {
                                long offset = pscope.getSamplesOffset(domainBase);
                                sstart += offset;
                                send += offset;
                            }
                            if (this.samples != null && this.samples == signal.samples && this.samples.getRelease() >= this.sourceRelease && Utils.equals(this.legend, signal.legend) && ISamples.ProcessType.equals(this.processType, signal) && ISamples.SignalType.equals(this.signalType, signal) && ISamples.TagDomain.equals(this.tagDomain, signal) && ISamples.SignalDescriptor.equals(this.signalDescriptor, signal) && this.sampleDomainBase.equals(sdb) && this.domainBase == domainBase && this.start == sstart && this.end <= send && this.rate == signal.rate) break block8;
                            return -1;
                        }
                        if (this.end >= send && this.samples.getRelease() == this.sourceRelease && this.tagged == signal.tag) break block9;
                        if (this.isVolatile()) break block10;
                        return -1;
                    }
                    if (this.updateFragments()) break block11;
                    return -1;
                }
                this.end = send;
                ++this.readerRelease;
                return 1;
            }
            return 0;
        }
    }

    @Override
    public int update(IPackedSamples packed, IDomainBase domainBase) {
        if (!this.isVolatile()) {
            return -1;
        }
        if (!(this.samples == packed.getSamples() && packed.getRelease() >= this.sourceRelease && Utils.equals(this.legend, packed.getLegend()) && Utils.equals((Object)this.processType, (Object)packed.getProcessType()) && Utils.equals((Object)this.signalType, (Object)packed.getSignalType()) && Utils.equals((Object)this.tagDomain, (Object)packed.getTagDomain()) && Utils.equals(this.signalDescriptor, packed.getSignalDescriptor()) && this.sampleDomainBase.equals(packed.getSamplesDomainBase()) && Utils.equals(this.domainBase, packed.getDomainBase()) && this.start == packed.getStartUnits() && this.end <= packed.getEndUnits() && this.rate == packed.getRateUnits())) {
            return -1;
        }
        if (this.end > packed.getEndUnits() || packed != null && packed.getRelease() != this.sourceRelease || this.tagged != packed.hasTag()) {
            if (!this.isVolatile()) {
                return -1;
            }
            if (!this.updateFragments()) {
                return -1;
            }
            this.end = packed.getEndUnits();
            ++this.readerRelease;
            return 1;
        }
        return 0;
    }

    @Override
    public boolean isVolatile() {
        return this.extendable;
    }

    @Override
    public boolean isMonotonous() {
        return true;
    }

    @Override
    public final int getRelease() {
        return this.readerRelease;
    }

    @Override
    public final boolean isReleased() {
        return this.readerRelease >= 0;
    }

    @Override
    public final int getCount() {
        if (!this.isSettled()) {
            return 0;
        }
        return this.count;
    }

    @Override
    public final int getGroups() {
        if (!this.isSettled()) {
            return 0;
        }
        return this.groups;
    }

    @Override
    public final boolean isEmpty() {
        if (!this.isSettled()) {
            return true;
        }
        return this.count <= 0;
    }

    @Override
    public final int getPackVersion() {
        if (!this.isSettled()) {
            return 0;
        }
        return this.version;
    }

    @Override
    public final boolean isSettled() {
        return this.settled;
    }

    @Override
    public boolean isSettling() {
        return false;
    }

    public final boolean isUpdated() {
        return this.updated;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     * Enabled aggressive block sorting
     * Enabled unnecessary exception pruning
     * Enabled aggressive exception aggregation
     */
    @Override
    public final boolean ensureSettled(IProgress p) {
        if (this.settled && !this.isUpdated()) {
            return true;
        }
        SamplesReader samplesReader = this;
        synchronized (samplesReader) {
            if (this.settled && !this.isUpdated()) {
                return true;
            }
            try {
                if (!this.settled) {
                    this.sourceRelease = this.samples != null ? this.samples.getRelease() : 0;
                    int[] fragmentIds = this.samples != null ? this.samples.getFragmentIds() : new int[]{};
                    this.fragments = new SamplesFragmentReader[fragmentIds.length];
                    int n = 0;
                    while (n < this.fragments.length) {
                        this.fragments[n] = new SamplesFragmentReader(fragmentIds[n]);
                        ++n;
                    }
                }
                if (this.fragments.length == 0) {
                    this.count = 0;
                    this.groups = 0;
                    this.simpleGeometry = true;
                    this.settled = true;
                    this.updated = false;
                    return true;
                }
                if (!this.settled) {
                    byte[] bytes = (byte[])this.samples.get(this.fragments[0].fragmentId);
                    if (bytes == null || this.fragments == null) {
                        return false;
                    }
                    if (bytes.length < 6) {
                        return false;
                    }
                    if (bytes[0] != 21 || bytes[1] != 8) {
                        return false;
                    }
                    this.version = bytes[2];
                    this.flags = bytes[3];
                    if (this.version > 6) {
                        return false;
                    }
                    this.samplesPerFragment = this.version >= 3 ? ((bytes[4] & 0xFF) + 1) * 256 : this.samples.a;
                }
                long fstart = this.start;
                long fdelta = 0L;
                int count = 0;
                int groups = 0;
                boolean simpleGeometry = true;
                GroupIndexBuilder groupIndexBuilder = new GroupIndexBuilder(this.samplesPerFragment);
                int n = 0;
                while (!(n >= this.fragments.length || p != null && p.isCanceled())) {
                    SamplesFragmentReader f = this.fragments[n];
                    if (f.needsSettlement()) {
                        groupIndexBuilder.init(groups, f.settled ? f : null);
                        if (!f.ensureSettled(p, count, this.samplesPerFragment, fstart, fdelta, groupIndexBuilder)) {
                            this.setError(f.error);
                            return false;
                        }
                        int m = 0;
                        while (m < groupIndexBuilder.getPreviousGroupEnds()) {
                            int group = groupIndexBuilder.getPreviousGroup(m);
                            int index = groupIndexBuilder.getPreviousGroupEndIndex(m) + f.first;
                            int lf = this.fragmentAtGroup(group);
                            if (lf < n && lf != -1) {
                                this.fragments[lf].adaptLaterGroupEnd(group, index);
                            }
                            ++m;
                        }
                        groupIndexBuilder.apply(f, this.sourceRelease == 0);
                        if (n < this.fragments.length - 1 && f.count != this.samplesPerFragment || f.count > this.samplesPerFragment) {
                            simpleGeometry = false;
                        }
                    }
                    count += f.count;
                    groups += f.groups;
                    fstart = this.processType == ISamples.ProcessType.Discrete ? f.end : (fstart += (long)f.count * this.rate);
                    fdelta = f.delta;
                    ++n;
                }
                this.count = count;
                this.groups = groups;
                this.simpleGeometry = simpleGeometry;
                this.settled = true;
                this.updated = false;
                if (p != null && p.isCanceled()) {
                    if (p instanceof IProgressStatus && ((IProgressStatus)p).hadTimeout()) {
                        this.setError(I18n.General_Timeout);
                    } else {
                        this.setError(I18n.General_Cancelled);
                    }
                }
                return true;
            }
            catch (Throwable e) {
                this.setError(e.getMessage());
                return false;
            }
        }
    }

    @Override
    public Object getService(Class<?> cs) {
        if (IGroupService.class.equals(cs)) {
            return new GroupService();
        }
        if (IStatService.class.equals(cs)) {
            return new StatService();
        }
        return null;
    }

    @Override
    public final SamplesStat getStat(int idx0, int idxN, int content) {
        if (!this.isSettled()) {
            return null;
        }
        if (this.stat == null || this.stat.getContent() != content) {
            this.stat = (IStatService)this.getService(IStatService.class);
            if (this.stat != null) {
                this.stat.init(null, content);
            }
        }
        if (this.stat != null) {
            return this.stat.getStat(idx0, idxN);
        }
        return null;
    }

    @Override
    public final int indexAt(long units) {
        if (this.processType == ISamples.ProcessType.Continuous) {
            if (this.rate > 0L && units >= this.start) {
                long index = (units - this.start) / this.rate;
                if (index < 0L) {
                    index = -1L;
                }
                if (index >= (long)this.count) {
                    index = this.count - 1;
                }
                return (int)index;
            }
            return -1;
        }
        if (!this.isSettled()) {
            return -1;
        }
        if (this.fragments.length == 0) {
            return -1;
        }
        int index = 0;
        while (index < this.fragments.length) {
            long end;
            long start = this.fragments[index].start;
            long l = end = index < this.fragments.length - 1 ? this.fragments[index + 1].start - 1L : Long.MAX_VALUE;
            if (units >= start && units <= end) {
                return this.fragments[index].indexAt(units);
            }
            ++index;
        }
        return -1;
    }

    @Override
    public final long unitsAt(int idx) {
        if (this.processType == ISamples.ProcessType.Continuous) {
            if (idx < 0) {
                return Long.MIN_VALUE;
            }
            if (idx >= this.count) {
                return Long.MAX_VALUE;
            }
            return this.start + (long)idx * this.rate;
        }
        if (!this.isSettled()) {
            return 0L;
        }
        if (idx < 0) {
            return Long.MIN_VALUE;
        }
        if (idx >= this.count) {
            return Long.MAX_VALUE;
        }
        int fragmentIdx = this.fragmentAtIdx(idx);
        if (fragmentIdx != -1) {
            return this.fragments[fragmentIdx].unitsAt(idx);
        }
        return 0L;
    }

    @Override
    public final DomainValue positionAt(int idx) {
        return new DomainValue(this.domainBase, this.unitsAt(idx));
    }

    @Override
    public int indexAt(DomainValue position) {
        if (position == null) {
            return -1;
        }
        if ((position = position.convertTo(this.domainBase)) != null) {
            return this.indexAt(position.units);
        }
        return -1;
    }

    private final int fragmentAtIdx(int idx) {
        if (this.simpleGeometry && this.samplesPerFragment > 0) {
            int fragmentIdx = idx / this.samplesPerFragment;
            if (fragmentIdx < 0 || fragmentIdx >= this.fragments.length) {
                return -1;
            }
            return fragmentIdx;
        }
        if (this.fragments.length == 0) {
            return -1;
        }
        int fragmentIdx = this.fragments.length / 2;
        int min = 0;
        int max = this.fragments.length - 1;
        while (true) {
            int last;
            int first = this.fragments[fragmentIdx].first;
            int n = last = fragmentIdx < this.fragments.length - 1 ? this.fragments[fragmentIdx + 1].first - 1 : this.count - 1;
            if (idx >= first && idx <= last) {
                return fragmentIdx;
            }
            if (idx > last) {
                if (fragmentIdx == max) {
                    return -1;
                }
                min = fragmentIdx + 1;
            } else {
                if (fragmentIdx == min) {
                    return -1;
                }
                max = fragmentIdx - 1;
            }
            fragmentIdx = (max + min) / 2;
        }
    }

    private final int fragmentAtGroup(int group) {
        if (this.fragments.length == 0) {
            return -1;
        }
        int fragmentIdx = this.fragments.length / 2;
        int min = 0;
        int max = this.fragments.length - 1;
        while (true) {
            int last;
            int first = this.fragments[fragmentIdx].firstGroup;
            int n = last = fragmentIdx < this.fragments.length - 1 ? this.fragments[fragmentIdx + 1].firstGroup - 1 : this.groups - 1;
            if (group >= first && group <= last) {
                return fragmentIdx;
            }
            if (group > last) {
                if (fragmentIdx == max) {
                    return -1;
                }
                min = fragmentIdx + 1;
            } else {
                if (fragmentIdx == min) {
                    return -1;
                }
                max = fragmentIdx - 1;
            }
            fragmentIdx = (max + min) / 2;
        }
    }

    @Override
    public ISamplesReader getReader() {
        return this;
    }

    @Override
    public ISamplesProducer getProducer() {
        return null;
    }

    @Override
    public final boolean isNoneAt(int idx) {
        if (!this.isSettled()) {
            return true;
        }
        int fragmentIdx = this.fragmentAtIdx(idx);
        if (fragmentIdx != -1) {
            return this.fragments[fragmentIdx].isNoneAt(idx);
        }
        return true;
    }

    @Override
    @Deprecated
    public final boolean isConflictAt(int idx) {
        return this.isTaggedAt(idx);
    }

    @Override
    public final boolean isTaggedAt(int idx) {
        if (!this.isSettled()) {
            return false;
        }
        int fragmentIdx = this.fragmentAtIdx(idx);
        if (fragmentIdx != -1) {
            return this.fragments[fragmentIdx].isTaggedAt(idx);
        }
        return false;
    }

    @Override
    public int getTagAt(int idx) {
        if (!this.isSettled()) {
            return 0;
        }
        int fragmentIdx = this.fragmentAtIdx(idx);
        if (fragmentIdx != -1) {
            return this.fragments[fragmentIdx].getTagAt(idx);
        }
        return 0;
    }

    @Override
    public final Object valueAt(int idx) {
        if (!this.isSettled()) {
            return null;
        }
        int fragmentIdx = this.fragmentAtIdx(idx);
        if (fragmentIdx != -1) {
            return this.fragments[fragmentIdx].valueAt(idx);
        }
        return null;
    }

    @Override
    public final CompoundValue compoundAt(int idx) {
        if (!this.isSettled()) {
            return null;
        }
        int fragmentIdx = this.fragmentAtIdx(idx);
        if (fragmentIdx != -1) {
            return this.fragments[fragmentIdx].compoundAt(idx);
        }
        return null;
    }

    @Override
    public final CompoundValue compoundAt(int idx, int flags) {
        if (!this.isSettled()) {
            return null;
        }
        int fragmentIdx = this.fragmentAtIdx(idx);
        if (fragmentIdx != -1) {
            return this.fragments[fragmentIdx].compoundAt(idx, flags);
        }
        return null;
    }

    @Override
    public final CompoundPack packedAt(int idx) {
        if (!this.isSettled()) {
            return null;
        }
        int fragmentIdx = this.fragmentAtIdx(idx);
        if (fragmentIdx != -1) {
            return this.fragments[fragmentIdx].packedAt(idx);
        }
        return null;
    }

    @Override
    public final List<IAttachment> attachmentsAt(int idx, int type) {
        if (!this.isSettled()) {
            return null;
        }
        int fragmentIdx = this.fragmentAtIdx(idx);
        if (fragmentIdx != -1) {
            return this.fragments[fragmentIdx].attachmentsAt(idx, type);
        }
        return null;
    }

    @Override
    public final int groupAt(int idx) {
        if (!this.isSettled()) {
            return -1;
        }
        int fragmentIdx = this.fragmentAtIdx(idx);
        if (fragmentIdx != -1) {
            return this.fragments[fragmentIdx].groupAt(idx);
        }
        return -1;
    }

    @Override
    public int orderAt(int idx) {
        if (!this.isSettled()) {
            return 0;
        }
        int fragmentIdx = this.fragmentAtIdx(idx);
        if (fragmentIdx != -1) {
            return this.fragments[fragmentIdx].orderAt(idx);
        }
        return 0;
    }

    @Override
    public GroupedValue valuesAtGroup(int group) {
        return this.valuesAtGroup(group, 0);
    }

    @Override
    public GroupedValue valuesAtGroup(int group, int flags) {
        int endFragment;
        int[] range;
        if (!this.isSettled() || this.getSignalType() != ISamples.SignalType.Struct) {
            return null;
        }
        int fragmentIdx = this.fragmentAtGroup(group);
        if (fragmentIdx != -1 && (range = this.fragments[fragmentIdx].indexRangeOfGroup(group)) != null && (endFragment = this.fragmentAtIdx(range[2])) != -1) {
            ArrayList<CompoundValue> list = new ArrayList<CompoundValue>();
            int n = fragmentIdx;
            while (n <= endFragment) {
                this.fragments[n].collectGroupValuesOf(group, range[0], range[2], list, flags);
                ++n;
            }
            if (!list.isEmpty()) {
                return new Transaction((List<CompoundValue>)list);
            }
        }
        return null;
    }

    @Override
    public List<IAttachment> attachmentsAtGroup(int group, int types) {
        int endFragment;
        int[] range;
        if (!this.isSettled()) {
            return null;
        }
        int fragmentIdx = this.fragmentAtGroup(group);
        if (fragmentIdx != -1 && (range = this.fragments[fragmentIdx].indexRangeOfGroup(group)) != null && (endFragment = this.fragmentAtIdx(range[2])) != -1) {
            ArrayList<IAttachment> list = new ArrayList<IAttachment>();
            int n = fragmentIdx;
            while (n <= endFragment) {
                this.fragments[n].collectGroupAttachmentsOf(group, types, range[0], range[2], list);
                ++n;
            }
            if (!list.isEmpty()) {
                return list;
            }
        }
        return null;
    }

    @Override
    public int indexAtGroup(int group) {
        if (!this.isSettled()) {
            return -1;
        }
        int fragmentIdx = this.fragmentAtGroup(group);
        if (fragmentIdx != -1) {
            int[] range = this.fragments[fragmentIdx].indexRangeOfGroup(group);
            return range != null ? range[0] : -1;
        }
        return -1;
    }

    /*
     * Unable to fully structure code
     * Could not resolve type clashes
     */
    protected final Object packed2Value(byte[] bytes, int pos, int format0, int dataLength, int flags) {
        block83: {
            valObj /* !! */  = null;
            try {
                if ((format0 & 192) == 0) {
                    valObj /* !! */  = null;
                    break block83;
                }
                block1 : switch (SamplesReader.$SWITCH_TABLE$de$toem$impulse$samples$ISamples$SignalType()[this.getSignalType().ordinal()]) {
                    case 4: {
                        valObj /* !! */  = Logic.expand(this.getScale(), format0, bytes, pos, dataLength);
                        break;
                    }
                    case 5: {
                        if ((format0 & 192) == 64) {
                            xdf = format0 & 24;
                            if (xdf == 24 && dataLength > 2) {
                                bdscale = bytes[pos++] & 255 | bytes[pos++] << 8;
                                buffer = new byte[dataLength - 2];
                                index = 0;
                                i = pos + dataLength - 1 - 2;
                                while (i >= pos) {
                                    buffer[index++] = bytes[i];
                                    --i;
                                }
                                big = new BigInteger(buffer);
                                valObj /* !! */  = new BigDecimal(big, bdscale);
                                break;
                            }
                            if (dataLength == 4) {
                                intdata = 0;
                                i = pos + 3;
                                while (i >= pos) {
                                    intdata = intdata << 8 | 255 & bytes[i];
                                    --i;
                                }
                                valObj /* !! */  = Float.valueOf(Float.intBitsToFloat(intdata));
                                break;
                            }
                            if (dataLength == 8) {
                                longdata = 0L;
                                i = pos + 7;
                                while (i >= pos) {
                                    longdata = longdata << 8 | (long)(255 & bytes[i]);
                                    --i;
                                }
                                valObj /* !! */  = Double.longBitsToDouble(longdata);
                            }
                        }
                        break;
                    }
                    case 11: {
                        if (dataLength > 0) {
                            scale = this.getScale();
                            if ((format0 & 192) == 192) {
                                nscale = SamplesReader.plusRead(bytes, pos);
                                pos += nscale[1];
                                dataLength -= nscale[1];
                                scale = nscale[0];
                            }
                            endPos = pos + dataLength;
                            xdf = format0 & 24;
                            if (xdf == 8 && dataLength % 4 == 0) {
                                array = new float[scale];
                                n = 0;
                                while (n < scale && pos < endPos) {
                                    intdata = 0;
                                    i = pos + 3;
                                    while (i >= pos) {
                                        intdata = intdata << 8 | 255 & bytes[i];
                                        --i;
                                    }
                                    array[n] = Float.intBitsToFloat(intdata);
                                    pos += 4;
                                    ++n;
                                }
                                valObj /* !! */  = (String[])array;
                                break;
                            }
                            if (xdf == 16 && dataLength % 8 == 0) {
                                array = new double[scale];
                                n = 0;
                                while (n < scale && pos < endPos) {
                                    longdata = 0L;
                                    i = pos + 7;
                                    while (i >= pos) {
                                        longdata = longdata << 8 | (long)(255 & bytes[i]);
                                        --i;
                                    }
                                    array[n] = Double.longBitsToDouble(longdata);
                                    pos += 8;
                                    ++n;
                                }
                                valObj /* !! */  = (String[])array;
                                break;
                            }
                            if (xdf == 24) {
                                array = new Object[scale];
                                n = 0;
                                while (n < scale && pos < endPos) {
                                    if ((pos += (length = SamplesReader.plusRead(bytes, pos))[1]) + length[0] > endPos) break;
                                    bdscale = bytes[pos++] & 255 | bytes[pos++] << 8;
                                    buffer = new byte[length[0] - 2];
                                    j = 0;
                                    i = pos + length[0] - 1;
                                    while (i >= pos) {
                                        buffer[j++] = bytes[i];
                                        --i;
                                    }
                                    pos += length[0];
                                    bi = new BigInteger(buffer);
                                    array[n] = new BigDecimal(bi, bdscale);
                                    ++n;
                                }
                                valObj /* !! */  = array;
                            }
                        }
                        break;
                    }
                    case 3: {
                        if ((format0 & 192) == 64) {
                            if (dataLength == 0) {
                                valObj /* !! */  = 0;
                                break;
                            }
                            if (dataLength <= 4) {
                                value = (bytes[pos + dataLength - 1] & 128) != 0 ? -1 : 0;
                                i = pos + dataLength - 1;
                                while (i >= pos) {
                                    value = value << 8 | 255 & bytes[i];
                                    --i;
                                }
                                valObj /* !! */  = value;
                                break;
                            }
                            if (dataLength <= 8) {
                                value = (bytes[pos + dataLength - 1] & 128) != 0 ? -1L : 0L;
                                i = pos + dataLength - 1;
                                while (i >= pos) {
                                    value = value << 8 | (long)(255 & bytes[i]);
                                    --i;
                                }
                                valObj /* !! */  = value;
                                break;
                            }
                            buffer = new byte[dataLength];
                            index = 0;
                            i = pos + dataLength - 1;
                            while (i >= pos) {
                                buffer[index++] = bytes[i];
                                --i;
                            }
                            valObj /* !! */  = new BigInteger(buffer);
                        }
                        break;
                    }
                    case 10: {
                        if (dataLength > 0) {
                            scale = this.getScale();
                            if ((format0 & 192) == 192) {
                                nscale = SamplesReader.plusRead(bytes, pos);
                                pos += nscale[1];
                                dataLength -= nscale[1];
                                scale = nscale[0];
                            }
                            endPos = pos + dataLength;
                            xdf = format0 & 24;
                            if (xdf == 8) {
                                array = new int[scale];
                                n = 0;
                                while (n < scale && pos < endPos) {
                                    if ((pos += (length = SamplesReader.plusRead(bytes, pos))[1]) + length[0] > endPos) break;
                                    val = 0;
                                    if (length[0] != 0 && length[0] <= 4) {
                                        val = (bytes[pos + length[0] - 1] & 128) != 0 ? -1 : 0;
                                        i = pos + length[0] - 1;
                                        while (i >= pos) {
                                            val = val << 8 | 255 & bytes[i];
                                            --i;
                                        }
                                    }
                                    pos += length[0];
                                    array[n] = val;
                                    ++n;
                                }
                                valObj /* !! */  = (String[])array;
                                break;
                            }
                            if (xdf == 16) {
                                array = new long[scale];
                                n = 0;
                                while (n < scale && pos < endPos) {
                                    if ((pos += (length = SamplesReader.plusRead(bytes, pos))[1]) + length[0] > endPos) break;
                                    val = 0L;
                                    if (length[0] != 0 && length[0] <= 8) {
                                        val = (bytes[pos + length[0] - 1] & 128) != 0 ? -1 : 0;
                                        i = pos + length[0] - 1;
                                        while (i >= pos) {
                                            val = val << 8 | (long)(255 & bytes[i]);
                                            --i;
                                        }
                                    }
                                    pos += length[0];
                                    array[n] = val;
                                    ++n;
                                }
                                valObj /* !! */  = (String[])array;
                                break;
                            }
                            if (xdf == 24) {
                                array = new Object[scale];
                                n = 0;
                                while (n < scale && pos < endPos) {
                                    if ((pos += (length = SamplesReader.plusRead(bytes, pos))[1]) + length[0] > endPos) break;
                                    val /* !! */  = null;
                                    if (length[0] == 0) {
                                        val /* !! */  = 0;
                                    } else if (length[0] <= 4) {
                                        value = (bytes[pos + length[0] - 1] & 128) != 0 ? -1 : 0;
                                        i = pos + length[0] - 1;
                                        while (i >= pos) {
                                            value = value << 8 | 255 & bytes[i];
                                            --i;
                                        }
                                        val /* !! */  = value;
                                    } else if (length[0] <= 8) {
                                        value = (bytes[pos + length[0] - 1] & 128) != 0 ? -1L : 0L;
                                        i = pos + length[0] - 1;
                                        while (i >= pos) {
                                            value = value << 8 | (long)(255 & bytes[i]);
                                            --i;
                                        }
                                        val /* !! */  = value;
                                    } else {
                                        buffer = new byte[length[0]];
                                        index = 0;
                                        i = pos + length[0] - 1;
                                        while (i >= pos) {
                                            buffer[index++] = bytes[i];
                                            --i;
                                        }
                                        val /* !! */  = new BigInteger(buffer);
                                    }
                                    pos += length[0];
                                    array[n] = val /* !! */ ;
                                    ++n;
                                }
                                valObj /* !! */  = array;
                            }
                        }
                        break;
                    }
                    case 6: {
                        if ((format0 & 192) == 64) {
                            valObj /* !! */  = new String(bytes, pos, dataLength, "UTF-8");
                        }
                        break;
                    }
                    case 12: {
                        if (dataLength > 0) {
                            scale = this.getScale();
                            if ((format0 & 192) == 192) {
                                nscale = SamplesReader.plusRead(bytes, pos);
                                pos += nscale[1];
                                dataLength -= nscale[1];
                                scale = nscale[0];
                            }
                            endPos = pos + dataLength;
                            array = new String[scale];
                            n = 0;
                            while (n < scale && pos < endPos) {
                                if ((pos += (length = SamplesReader.plusRead(bytes, pos))[1]) + length[0] > endPos) break;
                                val = new String(bytes, pos, length[0], "UTF-8");
                                pos += length[0];
                                array[n] = val;
                                ++n;
                            }
                            valObj /* !! */  = array;
                        }
                        break;
                    }
                    case 8: {
                        valObj /* !! */  = new Struct(bytes, pos, dataLength, (DefaultSamplesLegend)this.legend, flags);
                        break;
                    }
                    case 7: {
                        value = new byte[dataLength];
                        System.arraycopy(bytes, pos, value, 0, dataLength);
                        valObj /* !! */  = (String[])value;
                        break;
                    }
                    case 2: {
                        if ((format0 & 192) == 64) {
                            valObj /* !! */  = null;
                            break;
                        }
                        if ((format0 & 192) == 128) {
                            value = 0;
                            if (dataLength != 0 && dataLength <= 4) {
                                value = (bytes[pos + dataLength - 1] & 128) != 0 ? -1 : 0;
                                i = pos + dataLength - 1;
                                while (i >= pos) {
                                    value = value << 8 | 255 & bytes[i];
                                    --i;
                                }
                            }
                            if ((flags & 1) != 0) {
                                valObj /* !! */  = value;
                                break;
                            }
                            e = null;
                            if (this.legend != null) {
                                e = this.legend.getEnum(0, value);
                            }
                            if (e == null) {
                                e = new Enumeration(0, value);
                            }
                            valObj /* !! */  = e;
                        }
                        break;
                    }
                    case 9: {
                        if (dataLength <= 0) break;
                        scale = this.getScale();
                        if ((format0 & 192) == 192) {
                            nscale = SamplesReader.plusRead(bytes, pos);
                            pos += nscale[1];
                            dataLength -= nscale[1];
                            scale = nscale[0];
                        }
                        if ((flags & 1) == 0) ** GOTO lbl318
                        array /* !! */  = new int[scale];
                        endPos = pos + dataLength;
                        n = 0;
                        ** GOTO lbl313
                        while ((pos += (length = SamplesReader.plusRead(bytes, pos))[1]) + length[0] <= endPos) {
                            value = 0;
                            if (length[0] != 0 && length[0] <= 4) {
                                value = (bytes[pos + length[0] - 1] & 128) != 0 ? -1 : 0;
                                i = pos + length[0] - 1;
                                while (i >= pos) {
                                    value = value << 8 | 255 & bytes[i];
                                    --i;
                                }
                            }
                            pos += length[0];
                            array /* !! */ [n] = value;
                            ++n;
lbl313:
                            // 2 sources

                            if (n < scale) {
                                if (pos < endPos) continue;
                                break block1;
                            }
                            break block83;
                        }
                        break;
lbl318:
                        // 1 sources

                        array /* !! */  = (int[])new Enumeration[scale];
                        endPos = pos + dataLength;
                        n = 0;
                        while (n < scale && pos < endPos) {
                            if ((pos += (length = SamplesReader.plusRead(bytes, pos))[1]) + length[0] > endPos) break;
                            value = 0;
                            if (length[0] != 0 && length[0] <= 4) {
                                value = (bytes[pos + length[0] - 1] & 128) != 0 ? -1 : 0;
                                i = pos + length[0] - 1;
                                while (i >= pos) {
                                    value = value << 8 | 255 & bytes[i];
                                    --i;
                                }
                            }
                            pos += length[0];
                            e = null;
                            if (this.legend != null) {
                                e = this.legend.getEnum(8 + n, value);
                            }
                            if (e == null) {
                                e = new Enumeration(0, value);
                            }
                            array /* !! */ [n] = (int)e;
                            ++n;
                        }
                        valObj /* !! */  = (String[])array /* !! */ ;
                    }
                }
            }
            catch (Throwable v0) {}
        }
        return valObj /* !! */ ;
    }

    @Override
    public List<IMemberDescriptor> getMemberDescriptors() {
        return this.legend != null ? this.legend.getMembers() : Collections.EMPTY_LIST;
    }

    @Override
    public List<Enumeration> getEnums(int enumerationType) {
        return this.legend != null ? this.legend.getEnums(enumerationType) : Collections.EMPTY_LIST;
    }

    @Override
    public IMemberDescriptor getMemberDescriptor(Object memberIdentifier) {
        if (memberIdentifier instanceof IMemberDescriptor) {
            return (IMemberDescriptor)memberIdentifier;
        }
        return this.legend != null ? this.legend.getMember(memberIdentifier) : null;
    }

    @Override
    public List<Enumeration> getMemberEnums(Object memberIdentifier) {
        if (memberIdentifier instanceof IMemberDescriptor) {
            memberIdentifier = ((IMemberDescriptor)memberIdentifier).getId();
        }
        return this.legend != null ? this.legend.getEnums(memberIdentifier) : Collections.EMPTY_LIST;
    }

    @Override
    public Enumeration getMemberEnum(Object memberIdentifier, String label) {
        if (memberIdentifier instanceof IMemberDescriptor) {
            memberIdentifier = ((IMemberDescriptor)memberIdentifier).getId();
        }
        return this.legend != null ? this.legend.getEnum(memberIdentifier, label) : null;
    }

    @Override
    public Enumeration getMemberEnum(Object memberIdentifier, int value) {
        if (memberIdentifier instanceof IMemberDescriptor) {
            memberIdentifier = ((IMemberDescriptor)memberIdentifier).getId();
        }
        return this.legend != null ? this.legend.getEnum(memberIdentifier, value) : null;
    }

    @Override
    public List<Object> membersWithContent(String content) {
        return this.legend != null ? this.legend.getMemberIdentifier(content) : Collections.EMPTY_LIST;
    }

    static /* synthetic */ ISamples.ProcessType access$2(SamplesReader samplesReader) {
        return samplesReader.processType;
    }

    static /* synthetic */ int[] $SWITCH_TABLE$de$toem$impulse$samples$ISamples$SignalType() {
        if ($SWITCH_TABLE$de$toem$impulse$samples$ISamples$SignalType != null) {
            return $SWITCH_TABLE$de$toem$impulse$samples$ISamples$SignalType;
        }
        int[] nArray = new int[ISamples.SignalType.values().length];
        try {
            nArray[ISamples.SignalType.Binary.ordinal()] = 7;
        }
        catch (NoSuchFieldError noSuchFieldError) {}
        try {
            nArray[ISamples.SignalType.Event.ordinal()] = 2;
        }
        catch (NoSuchFieldError noSuchFieldError) {}
        try {
            nArray[ISamples.SignalType.EventArray.ordinal()] = 9;
        }
        catch (NoSuchFieldError noSuchFieldError) {}
        try {
            nArray[ISamples.SignalType.Float.ordinal()] = 5;
        }
        catch (NoSuchFieldError noSuchFieldError) {}
        try {
            nArray[ISamples.SignalType.FloatArray.ordinal()] = 11;
        }
        catch (NoSuchFieldError noSuchFieldError) {}
        try {
            nArray[ISamples.SignalType.Integer.ordinal()] = 3;
        }
        catch (NoSuchFieldError noSuchFieldError) {}
        try {
            nArray[ISamples.SignalType.IntegerArray.ordinal()] = 10;
        }
        catch (NoSuchFieldError noSuchFieldError) {}
        try {
            nArray[ISamples.SignalType.Logic.ordinal()] = 4;
        }
        catch (NoSuchFieldError noSuchFieldError) {}
        try {
            nArray[ISamples.SignalType.Struct.ordinal()] = 8;
        }
        catch (NoSuchFieldError noSuchFieldError) {}
        try {
            nArray[ISamples.SignalType.Text.ordinal()] = 6;
        }
        catch (NoSuchFieldError noSuchFieldError) {}
        try {
            nArray[ISamples.SignalType.TextArray.ordinal()] = 12;
        }
        catch (NoSuchFieldError noSuchFieldError) {}
        try {
            nArray[ISamples.SignalType.Unknown.ordinal()] = 1;
        }
        catch (NoSuchFieldError noSuchFieldError) {}
        $SWITCH_TABLE$de$toem$impulse$samples$ISamples$SignalType = nArray;
        return nArray;
    }

    public class DomainIndex {
        private final int length;
        private final ByteBuffer index;

        DomainIndex(int length) {
            this.index = ByteBuffer.allocateDirect(length * 8);
            this.length = length;
        }

        DomainIndex(int length, ByteBuffer index) {
            this.index = index;
            this.length = length;
        }

        public DomainIndex reduce(int length) {
            return new DomainIndex(length, this.index);
        }

        public void convert(long factor) {
            block3: {
                block2: {
                    if (factor <= 1L) break block2;
                    int n = 0;
                    while (n < this.length) {
                        this.index.putLong(n * 8, this.index.getLong(n * 8) * factor);
                        ++n;
                    }
                    break block3;
                }
                if (factor >= 1L) break block3;
                long devider = -factor;
                int n = 0;
                while (n < this.length) {
                    this.index.putLong(n * 8, this.index.getLong(n * 8) / devider);
                    ++n;
                }
            }
        }

        final long get(int idx) {
            int bidx = (idx >= this.length ? this.length - 1 : idx) * 8;
            return this.index.getLong(bidx);
        }

        final void set(int idx, long pos) {
            int bidx = (idx >= this.length ? this.length - 1 : idx) * 8;
            this.index.putLong(bidx, pos);
        }
    }

    public class GroupIndexBuilder {
        private int samplesPerFragment;
        private int firstGroup;
        private int groups;
        private int previousEnds;
        private int firstPreviousGroup;
        private int[][] index;
        private int distance;
        private int layers;

        public GroupIndexBuilder(int samplesPerFragment) {
            this.samplesPerFragment = samplesPerFragment;
        }

        public void init(int firstGroup, SamplesFragmentReader reader) {
            this.firstGroup = this.firstPreviousGroup = firstGroup;
            if (reader != null) {
                this.groups = reader.groups;
                this.layers = reader.layers;
                this.index = reader.groupIndex;
            } else {
                this.groups = 0;
                this.layers = 0;
                this.index = null;
            }
            this.previousEnds = 0;
        }

        public boolean add(int group, int order, int layer, int idx) {
            if (this.index == null && !this.allocateIndex()) {
                return false;
            }
            if (group < this.firstGroup && order == 3) {
                if (this.groups + this.previousEnds >= this.index.length && !this.allocateIndex()) {
                    return false;
                }
                this.index[this.index.length - this.previousEnds - 1][0] = idx;
                this.index[this.index.length - this.previousEnds - 1][1] = group;
                ++this.previousEnds;
                if (this.firstPreviousGroup > group) {
                    this.firstPreviousGroup = group;
                }
                return true;
            }
            if (order == 1 || order == 5) {
                if (group != this.firstGroup + this.groups || this.groups + this.previousEnds >= this.index.length && !this.allocateIndex()) {
                    return false;
                }
                ++this.groups;
                this.index[group - this.firstGroup][0] = idx;
                this.index[group - this.firstGroup][1] = layer;
                if (order == 5) {
                    this.index[group - this.firstGroup][2] = idx;
                }
                if (layer >= this.layers) {
                    this.layers = layer + 1;
                }
            } else if (order == 3) {
                if (group < this.firstGroup || group > this.firstGroup + this.groups) {
                    return false;
                }
                this.index[group - this.firstGroup][2] = idx;
                int d = idx - this.index[group - this.firstGroup][0];
                if (this.distance < d) {
                    this.distance = d;
                }
            }
            return true;
        }

        private boolean allocateIndex() {
            try {
                if (this.index == null) {
                    this.index = new int[this.samplesPerFragment / 2 + 64][3];
                } else {
                    int[][] index = new int[this.index.length + this.samplesPerFragment / 4][3];
                    System.arraycopy(this.index, 0, index, 0, this.groups);
                    System.arraycopy(this.index, this.index.length - this.previousEnds - 1, index, index.length - this.previousEnds - 1, this.previousEnds);
                    this.index = index;
                }
                return true;
            }
            catch (Throwable throwable) {
                return false;
            }
        }

        public void apply(SamplesFragmentReader samplesSegmentReader, boolean close) {
            int[][] index = null;
            if (this.groups > 0) {
                if (close) {
                    index = new int[this.groups][3];
                    int n = 0;
                    while (n < this.groups) {
                        System.arraycopy(this.index[n], 0, index[n], 0, 3);
                        ++n;
                    }
                } else {
                    index = this.index;
                    this.index = null;
                }
            }
            samplesSegmentReader.groupIndex = index;
            samplesSegmentReader.firstGroup = this.firstGroup;
            samplesSegmentReader.groups = this.groups;
            samplesSegmentReader.layers = this.layers;
        }

        public int getPreviousGroupEnds() {
            return this.previousEnds;
        }

        public int getPreviousGroup(int n) {
            return this.index[this.index.length - this.previousEnds + n][1];
        }

        public int getPreviousGroupEndIndex(int n) {
            return this.index[this.index.length - this.previousEnds + n][0];
        }

        public int getMaxDistance() {
            return this.distance;
        }

        public int size() {
            return this.index.length;
        }
    }

    public class GroupService
    implements IGroupService {
        @Override
        public int[] availableLayers() {
            if (!SamplesReader.this.isSettled()) {
                return null;
            }
            int layers = 0;
            SamplesFragmentReader[] samplesFragmentReaderArray = SamplesReader.this.fragments;
            int n = samplesFragmentReaderArray.length;
            int n2 = 0;
            while (n2 < n) {
                SamplesFragmentReader f = samplesFragmentReaderArray[n2];
                if (f.layers > layers) {
                    layers = f.layers;
                }
                ++n2;
            }
            int[] nArray = new int[2];
            nArray[1] = layers - 1;
            return nArray;
        }

        @Override
        public IGroupService.IGroupIterator activeGroupIterator(int idx1, int idx2, final int layer) {
            return new IGroupService.IGroupIterator(idx1, idx2){
                boolean found;
                int filterIdx1;
                int filterIdx2;
                int filterlayer;
                private SamplesFragmentReader[] fragments;
                int currentFragment;
                int firstIndexInCurrentFragment;
                int lastIndexInCurrentFragment;
                int currentGroup;
                int firstGroupInCurrentFragment;
                int lastGroupInCurrentFragment;
                int[][] currentIndex;
                int[] currentGroupRange;
                {
                    this.filterIdx1 = n;
                    this.filterIdx2 = n2;
                    this.filterlayer = n3;
                    this.fragments = SamplesReader.this.fragments;
                    this.currentFragment = -1;
                    this.firstIndexInCurrentFragment = 0;
                    this.lastIndexInCurrentFragment = 0;
                    this.currentGroup = -1;
                    this.firstGroupInCurrentFragment = -1;
                    this.lastGroupInCurrentFragment = -1;
                }

                /*
                 * Unable to fully structure code
                 */
                private boolean findNext() {
                    this.found = false;
                    block0: while (true) {
                        if (this.currentFragment < 0 || this.currentGroup >= this.lastGroupInCurrentFragment || this.filterIdx1 > this.lastIndexInCurrentFragment) {
                            if (this.fragments.length <= this.currentFragment + 1 || this.filterIdx2 < this.firstIndexInCurrentFragment) {
                                return this.found;
                            }
                            ++this.currentFragment;
                            this.firstIndexInCurrentFragment = this.fragments[this.currentFragment].first;
                            this.lastIndexInCurrentFragment = this.firstIndexInCurrentFragment + this.fragments[this.currentFragment].count - 1;
                            this.firstGroupInCurrentFragment = this.fragments[this.currentFragment].firstGroup;
                            this.currentGroup = this.firstGroupInCurrentFragment - 1;
                            this.currentIndex = this.fragments[this.currentFragment].groupIndex;
                            this.lastGroupInCurrentFragment = this.firstGroupInCurrentFragment + this.fragments[this.currentFragment].groups - 1;
                            continue;
                        }
                        do {
                            if (++this.currentGroup <= this.lastGroupInCurrentFragment) ** break;
                            continue block0;
                            this.currentGroupRange = this.currentIndex[this.currentGroup - this.firstGroupInCurrentFragment];
                        } while (layer != -1 && this.currentGroupRange[1] != this.filterlayer || this.currentGroupRange[0] + this.firstIndexInCurrentFragment > this.filterIdx2 || this.currentGroupRange[2] + this.firstIndexInCurrentFragment < this.filterIdx1);
                        break;
                    }
                    this.found = true;
                    return true;
                }

                /*
                 * Unable to fully structure code
                 */
                private boolean findNextBefore(int idx) {
                    this.found = false;
                    currentFragment = this.currentFragment;
                    firstIndexInCurrentFragment = this.firstIndexInCurrentFragment;
                    lastIndexInCurrentFragment = this.lastIndexInCurrentFragment;
                    currentGroup = this.currentGroup;
                    firstGroupInCurrentFragment = this.firstGroupInCurrentFragment;
                    lastGroupInCurrentFragment = this.lastGroupInCurrentFragment;
                    currentIndex = this.currentIndex;
                    currentGroupRange = this.currentGroupRange;
                    _currentFragment = -1;
                    _firstIndexInCurrentFragment = 0;
                    _lastIndexInCurrentFragment = 0;
                    _currentGroup = -1;
                    _firstGroupInCurrentFragment = -1;
                    _lastGroupInCurrentFragment = -1;
                    _currentIndex = null;
                    _currentGroupRange = null;
                    block0: while (true) {
                        if (currentFragment < 0 || currentGroup >= lastGroupInCurrentFragment || this.filterIdx1 > lastIndexInCurrentFragment) {
                            if (this.fragments.length <= currentFragment + 1 || this.filterIdx2 < firstIndexInCurrentFragment) {
                                return this.found;
                            }
                            firstIndexInCurrentFragment = this.fragments[++currentFragment].first;
                            lastIndexInCurrentFragment = firstIndexInCurrentFragment + this.fragments[currentFragment].count - 1;
                            firstGroupInCurrentFragment = this.fragments[currentFragment].firstGroup;
                            currentGroup = firstGroupInCurrentFragment - 1;
                            currentIndex = this.fragments[currentFragment].groupIndex;
                            lastGroupInCurrentFragment = firstGroupInCurrentFragment + this.fragments[currentFragment].groups - 1;
                            continue;
                        }
                        while (true) {
                            if (++currentGroup <= lastGroupInCurrentFragment) ** break;
                            continue block0;
                            currentGroupRange = currentIndex[currentGroup - firstGroupInCurrentFragment];
                            if (currentGroupRange[0] + firstIndexInCurrentFragment >= idx) {
                                if (this.found) {
                                    this.currentFragment = _currentFragment;
                                    this.firstIndexInCurrentFragment = _firstIndexInCurrentFragment;
                                    this.lastIndexInCurrentFragment = _lastIndexInCurrentFragment;
                                    this.currentGroup = _currentGroup;
                                    this.firstGroupInCurrentFragment = _firstGroupInCurrentFragment;
                                    this.lastGroupInCurrentFragment = _lastGroupInCurrentFragment;
                                    this.currentIndex = _currentIndex;
                                    this.currentGroupRange = _currentGroupRange;
                                }
                                return this.found;
                            }
                            if (layer != -1 && currentGroupRange[1] != this.filterlayer || currentGroupRange[0] + firstIndexInCurrentFragment > this.filterIdx2 || currentGroupRange[2] + firstIndexInCurrentFragment < this.filterIdx1) continue;
                            this.found = true;
                            _currentFragment = currentFragment;
                            _firstIndexInCurrentFragment = firstIndexInCurrentFragment;
                            _lastIndexInCurrentFragment = lastIndexInCurrentFragment;
                            _currentGroup = currentGroup;
                            _firstGroupInCurrentFragment = firstGroupInCurrentFragment;
                            _lastGroupInCurrentFragment = lastGroupInCurrentFragment;
                            _currentIndex = currentIndex;
                            _currentGroupRange = currentGroupRange;
                        }
                        break;
                    }
                }

                @Override
                public boolean hasNext() {
                    return SamplesReader.this.settled && (this.found || this.findNext());
                }

                @Override
                public Integer next() {
                    if (this.hasNext()) {
                        this.found = false;
                        return this.currentGroup;
                    }
                    return null;
                }

                @Override
                public boolean hasNextBefore(int idx) {
                    return SamplesReader.this.settled && (this.found || this.findNextBefore(idx));
                }

                @Override
                public int currentGroup() {
                    return this.currentGroup;
                }

                @Override
                public int currentGroupFirstIdx() {
                    return this.currentGroupRange != null ? this.currentGroupRange[0] + this.firstIndexInCurrentFragment : -1;
                }

                @Override
                public int currentGroupLayer() {
                    return this.currentGroupRange != null ? this.currentGroupRange[1] : 0;
                }

                @Override
                public int currentGroupLastIdx() {
                    return this.currentGroupRange != null ? this.currentGroupRange[2] + this.firstIndexInCurrentFragment : -1;
                }
            };
        }

        @Override
        public int size() {
            return SamplesReader.this.groups;
        }

        @Override
        public int[] rangeOf(int group) {
            if (!SamplesReader.this.isSettled()) {
                return null;
            }
            int fragment = SamplesReader.this.fragmentAtGroup(group);
            if (fragment != -1) {
                int[] range = SamplesReader.this.fragments[fragment].indexRangeOfGroup(group);
                return range;
            }
            return null;
        }
    }

    public class PosIndex {
        private final int length;
        private final ByteBuffer index;

        PosIndex(int length) {
            this.index = ByteBuffer.allocateDirect(length * 4);
            this.length = length;
        }

        PosIndex(int length, ByteBuffer index) {
            this.index = index;
            this.length = length;
        }

        public PosIndex reduce(int length) {
            return new PosIndex(length, this.index);
        }

        final int get(int idx) {
            int bidx = (idx >= this.length ? this.length - 1 : idx) * 4;
            return this.index.getInt(bidx);
        }

        final void set(int idx, int pos) {
            int bidx = (idx >= this.length ? this.length - 1 : idx) * 4;
            this.index.putInt(bidx, pos);
        }

        static /* synthetic */ int access$0(PosIndex posIndex) {
            return posIndex.length;
        }
    }

    private final class SamplesFragmentReader {
        private int fragmentId;
        private int fragmentRelease;
        boolean settled;
        protected int first;
        protected int count;
        protected PosIndex posIndex;
        protected long start;
        protected long end;
        protected long delta = 0L;
        protected DomainIndex unitsIndex;
        protected int firstGroup;
        protected int groups;
        protected int[][] groupIndex;
        protected int layers;
        protected long tryAllocate;
        protected String error;
        protected boolean tagged;
        protected WeakReference<byte[]> weakBytes;

        public SamplesFragmentReader(int fragmentId) {
            this.fragmentId = fragmentId;
            this.fragmentRelease = SamplesReader.this.samples.getFragmentRelease(fragmentId);
        }

        public final boolean needsSettlement() {
            return !this.settled || SamplesReader.this.samples.getFragmentRelease(this.fragmentId) != this.fragmentRelease;
        }

        /*
         * WARNING - Removed back jump from a try to a catch block - possible behaviour change.
         * Unable to fully structure code
         * Enabled aggressive block sorting
         * Enabled unnecessary exception pruning
         * Enabled aggressive exception aggregation
         */
        public final boolean ensureSettled(IProgress p, int first, int maxCount, long start, long delta, GroupIndexBuilder groupIndexBuilder) {
            v0 = doExtend = this.settled != false && this.fragmentRelease != 0;
            if (this.settled) {
                if (SamplesReader.access$0(SamplesReader.this).getFragmentRelease(this.fragmentId) == this.fragmentRelease) {
                    return true;
                }
                this.settled = false;
            }
            if (this.error != null && this.tryAllocate == 0L) {
                return false;
            }
            bytes = (byte[])SamplesReader.access$0(SamplesReader.this).get(this.fragmentId);
            if (bytes == null) {
                return false;
            }
            this.weakBytes = new WeakReference<byte[]>(bytes);
            current = start;
            idx = 0;
            pos = SamplesReader.access$1(SamplesReader.this) < 3 ? 4 : 6;
            posIndex = this.posIndex;
            unitsIndex = this.unitsIndex;
            if (!doExtend || posIndex == null || PosIndex.access$0(posIndex) < maxCount + 1) {
                if (this.tryAllocate != 0L && Utils.millies() < this.tryAllocate) {
                    return false;
                }
                try {
                    posIndex = new PosIndex(maxCount + 1);
                    unitsIndex = SamplesReader.access$2(SamplesReader.this) == ISamples.ProcessType.Discrete ? new DomainIndex(maxCount + 1) : null;
                }
                catch (Throwable v1) {
                    this.error = I18n.Samples_CouldNotAllocate;
                    posIndex = null;
                    unitsIndex = null;
                    this.tryAllocate = Utils.millies() + 1000L;
                    return false;
                }
                this.tryAllocate = 0L;
            } else {
                idx = this.count;
                if (unitsIndex != null) {
                    current = unitsIndex.get(idx - 1);
                    delta = this.delta;
                }
                pos = posIndex.get(idx);
            }
            try {
                if (Utils.equals(SamplesReader.access$3(SamplesReader.this), SamplesReader.access$4(SamplesReader.this)) || SamplesReader.access$4(SamplesReader.this) == null) ** GOTO lbl174
                current = SamplesReader.access$4(SamplesReader.this).convertTo(SamplesReader.access$3(SamplesReader.this), start);
                if (true) ** GOTO lbl174
            }
            catch (Throwable e) {
                block67: {
                    try {
                        this.error = String.valueOf(I18n.Samples_ExceptionAt_) + " " + first + idx + " -> " + e.getLocalizedMessage();
                        SystemLog.log(e);
                        this.count = 0;
                        this.posIndex = null;
                        this.unitsIndex = null;
                        this.groupIndex = null;
                        this.tryAllocate = Utils.millies() + 5000L;
                    }
                    catch (Throwable var25_24) {
                        if (SamplesReader.access$3(SamplesReader.this) != null && SamplesReader.access$3(SamplesReader.this) != SamplesReader.access$4(SamplesReader.this)) {
                            if (unitsIndex != null) {
                                unitsIndex.convert(SamplesReader.access$3(SamplesReader.this).getConversionFactor(SamplesReader.access$4(SamplesReader.this)));
                            }
                            current = SamplesReader.access$3(SamplesReader.this).convertTo(SamplesReader.access$4(SamplesReader.this), current);
                        }
                        if (idx < maxCount && SamplesReader.access$5(SamplesReader.this) == 0) {
                            try {
                                posIndex = posIndex.reduce(idx);
                                if (SamplesReader.access$2(SamplesReader.this) == ISamples.ProcessType.Discrete) {
                                    unitsIndex.reduce(idx + 1);
                                }
                            }
                            catch (Throwable v2) {}
                        }
                        this.posIndex = posIndex;
                        this.unitsIndex = unitsIndex;
                        this.count = idx;
                        this.first = first;
                        this.start = start;
                        this.end = current;
                        this.delta = delta;
                        this.error = null;
                        this.settled = true;
                        throw var25_24;
                    }
lbl74:
                    // 1 sources

                    while (true) {
                        if (SamplesReader.access$3(SamplesReader.this) != null && SamplesReader.access$3(SamplesReader.this) != SamplesReader.access$4(SamplesReader.this)) {
                            if (unitsIndex != null) {
                                unitsIndex.convert(SamplesReader.access$3(SamplesReader.this).getConversionFactor(SamplesReader.access$4(SamplesReader.this)));
                            }
                            current = SamplesReader.access$3(SamplesReader.this).convertTo(SamplesReader.access$4(SamplesReader.this), current);
                        }
                        if (idx < maxCount && SamplesReader.access$5(SamplesReader.this) == 0) {
                            try {
                                posIndex = posIndex.reduce(idx);
                                if (SamplesReader.access$2(SamplesReader.this) == ISamples.ProcessType.Discrete) {
                                    unitsIndex.reduce(idx + 1);
                                }
                            }
                            catch (Throwable v3) {}
                        }
                        this.posIndex = posIndex;
                        this.unitsIndex = unitsIndex;
                        this.count = idx;
                        this.first = first;
                        this.start = start;
                        this.end = current;
                        this.delta = delta;
                        this.error = null;
                        this.settled = true;
                        return false;
                    }
                    {
                        block66: {
                            block65: {
                                do {
                                    if ((format0 & 30) == 0 && SamplesReader.access$2(SamplesReader.this) == ISamples.ProcessType.Continuous) {
                                        multiply = SamplesReader.plusRead(bytes, pos);
                                        n = 0;
                                        while (true) {
                                            if (n >= multiply[0] || n + idx >= maxCount) {
                                                idx += multiply[0];
                                                break;
                                            }
                                            if (idx > 0) {
                                                posIndex.set(idx + n, posIndex.get(idx - 1));
                                            } else {
                                                posIndex.set(idx + n, -1);
                                            }
                                            ++n;
                                        }
                                    }
                                    pos += len;
                                    if (true) ** GOTO lbl174
                                    do {
                                        if ((format0 & 6) != 0) {
                                            order = (format0 & 6) >>> 1;
                                            group = 0;
                                            layer = 0;
                                            shift = 0;
                                            if (order == 1 && ((layer = bytes[pos++]) & 128) != 0) {
                                                order = 5;
                                                layer &= 127;
                                            }
                                            do {
                                                sn = bytes[pos++];
                                                group |= (sn & 127) << shift;
                                                shift += 7;
                                            } while ((sn & 128) != 0);
                                            if (groupIndexBuilder != null) {
                                                groupIndexBuilder.add(group, order, layer, idx);
                                            }
                                        }
                                        pos += len;
                                        if (SamplesReader.access$2(SamplesReader.this) == ISamples.ProcessType.Discrete) {
                                            switch ((format1 & 224) >> 5) {
                                                case 3: {
                                                    delta = 255L & (long)bytes[pos++];
                                                    delta |= (255L & (long)bytes[pos++]) << 8;
                                                    delta |= (255L & (long)bytes[pos++]) << 16;
                                                    delta |= (255L & (long)bytes[pos++]) << 24;
                                                    delta |= (255L & (long)bytes[pos++]) << 32;
                                                    delta |= (255L & (long)bytes[pos++]) << 40;
                                                    delta |= (255L & (long)bytes[pos++]) << 48;
                                                    current += (delta |= (255L & (long)bytes[pos++]) << 56);
                                                    break;
                                                }
                                                case 2: {
                                                    delta = 255L & (long)bytes[pos++];
                                                    delta |= (255L & (long)bytes[pos++]) << 8;
                                                    delta |= (255L & (long)bytes[pos++]) << 16;
                                                    current += (delta |= (255L & (long)bytes[pos++]) << 24);
                                                    break;
                                                }
                                                case 1: {
                                                    delta = 255L & (long)bytes[pos++];
                                                    current += (delta |= (255L & (long)bytes[pos++]) << 8);
                                                    break;
                                                }
                                                case 0: {
                                                    delta = 255L & (long)bytes[pos++];
                                                    current += delta;
                                                    break;
                                                }
                                                case 7: {
                                                    break;
                                                }
                                                case 4: {
                                                    current += delta;
                                                    break;
                                                }
                                                case 5: {
                                                    delta = 255L & (long)bytes[pos++];
                                                    delta |= (255L & (long)bytes[pos++]) << 8;
                                                    delta |= (255L & (long)bytes[pos++]) << 16;
                                                    delta |= (255L & (long)bytes[pos++]) << 24;
                                                    delta |= (255L & (long)bytes[pos++]) << 32;
                                                    delta |= (255L & (long)bytes[pos++]) << 40;
                                                    delta |= (255L & (long)bytes[pos++]) << 48;
                                                    current = delta |= (255L & (long)bytes[pos++]) << 56;
                                                    break;
                                                }
                                            }
                                            unitsIndex.set(idx, current);
                                        }
                                        ++idx;
lbl174:
                                        // 4 sources

                                        if (pos >= bytes.length || idx >= maxCount || p != null && p.isCanceled()) {
                                            if (idx >= PosIndex.access$0(posIndex) - 1) break block65;
                                            posIndex.set(idx, pos);
                                            break block66;
                                        }
                                        posIndex.set(idx, pos);
                                        format0 = bytes[pos++];
                                        format1 = bytes[pos++];
                                        this.tagged |= (format0 & 1) != 0;
                                        len = 0;
                                        if ((format1 & 16) != 0) {
                                            len = format1 & 15;
                                            if ((format0 & 33) == 33) {
                                                // empty if block
                                            }
                                            shift = 4;
                                            do {
                                                v4 = ++pos;
                                                ++pos;
                                                sn = bytes[v4];
                                                len |= (sn & 127) << shift;
                                                shift += 7;
                                            } while ((sn & 128) != 0);
                                            continue;
                                        }
                                        len = format1 & 15;
                                        if ((format0 & 33) != 33) continue;
                                        ++pos;
                                    } while ((format0 & 33) != 32);
                                } while (idx != 0);
                                this.error = I18n.Samples_AttachmentAtStart;
                                ** continue;
                            }
                            posIndex.set(PosIndex.access$0(posIndex) - 1, posIndex.get(PosIndex.access$0(posIndex) - 2));
                        }
                        if (unitsIndex == null) break block67;
                        unitsIndex.set(idx, 0x7FFFFFFFFFFFFFFFL);
                    }
                }
                if (SamplesReader.access$3(SamplesReader.this) != null && SamplesReader.access$3(SamplesReader.this) != SamplesReader.access$4(SamplesReader.this)) {
                    if (unitsIndex != null) {
                        unitsIndex.convert(SamplesReader.access$3(SamplesReader.this).getConversionFactor(SamplesReader.access$4(SamplesReader.this)));
                    }
                    current = SamplesReader.access$3(SamplesReader.this).convertTo(SamplesReader.access$4(SamplesReader.this), current);
                }
                if (idx < maxCount && SamplesReader.access$5(SamplesReader.this) == 0) {
                    try {
                        posIndex = posIndex.reduce(idx);
                        if (SamplesReader.access$2(SamplesReader.this) == ISamples.ProcessType.Discrete) {
                            unitsIndex.reduce(idx + 1);
                        }
                    }
                    catch (Throwable v5) {}
                }
                this.posIndex = posIndex;
                this.unitsIndex = unitsIndex;
                this.count = idx;
                this.first = first;
                this.start = start;
                this.end = current;
                this.delta = delta;
                this.error = null;
                this.settled = true;
                return true;
                if (SamplesReader.access$3(SamplesReader.this) != null && SamplesReader.access$3(SamplesReader.this) != SamplesReader.access$4(SamplesReader.this)) {
                    if (unitsIndex != null) {
                        unitsIndex.convert(SamplesReader.access$3(SamplesReader.this).getConversionFactor(SamplesReader.access$4(SamplesReader.this)));
                    }
                    current = SamplesReader.access$3(SamplesReader.this).convertTo(SamplesReader.access$4(SamplesReader.this), current);
                }
                if (idx < maxCount && SamplesReader.access$5(SamplesReader.this) == 0) {
                    try {
                        posIndex = posIndex.reduce(idx);
                        if (SamplesReader.access$2(SamplesReader.this) == ISamples.ProcessType.Discrete) {
                            unitsIndex.reduce(idx + 1);
                        }
                    }
                    catch (Throwable v6) {}
                }
                this.posIndex = posIndex;
                this.unitsIndex = unitsIndex;
                this.count = idx;
                this.first = first;
                this.start = start;
                this.end = current;
                this.delta = delta;
                this.error = null;
                this.settled = true;
                return false;
            }
        }

        final byte[] getBytes() {
            byte[] bytes;
            WeakReference<byte[]> weakBytes = this.weakBytes;
            byte[] byArray = bytes = weakBytes != null ? (byte[])weakBytes.get() : null;
            if (bytes == null && (bytes = (byte[])SamplesReader.this.samples.get(this.fragmentId)) != null) {
                this.weakBytes = new WeakReference<byte[]>(bytes);
            }
            return bytes;
        }

        public final int indexAt(long units) {
            if (this.count == 0 || this.unitsIndex == null || this.unitsIndex.get(0) > units) {
                return this.first - 1;
            }
            int index = this.count / 2;
            int min = 0;
            int max = this.count - 1;
            while (this.unitsIndex.get(index) >= units || index < this.count - 1 && this.unitsIndex.get(index + 1) <= units) {
                if (this.unitsIndex.get(index) == units && (index == 0 || this.unitsIndex.get(index - 1) < units)) {
                    return index + this.first;
                }
                if (units > this.unitsIndex.get(index)) {
                    min = index;
                    if (max - min > 1) {
                        index = (min + max) / 2;
                        continue;
                    }
                    if (index == max) {
                        return index + this.first;
                    }
                    index = max;
                    continue;
                }
                max = index;
                if (max - min > 1) {
                    index = (min + max) / 2;
                    continue;
                }
                if (index == min) {
                    return index + this.first;
                }
                index = min;
            }
            return index + this.first;
        }

        public final long unitsAt(int idx) {
            if ((idx -= this.first) < 0 || this.unitsIndex == null) {
                return Long.MIN_VALUE;
            }
            if (idx >= this.count) {
                return Long.MAX_VALUE;
            }
            return this.unitsIndex.get(idx);
        }

        public final boolean isNoneAt(int idx) {
            byte[] bytes = this.getBytes();
            if (bytes == null) {
                return false;
            }
            if ((idx -= this.first) < 0 || idx >= this.count) {
                return true;
            }
            return (bytes[this.posIndex.get(idx)] & 0xC0) == 0;
        }

        @Deprecated
        public final boolean isConflictAt(int idx) {
            return this.isTaggedAt(idx);
        }

        public final boolean isTaggedAt(int idx) {
            if (!this.tagged) {
                return false;
            }
            byte[] bytes = this.getBytes();
            if (bytes == null || !this.tagged) {
                return false;
            }
            if ((idx -= this.first) < 0 || idx >= this.count) {
                return false;
            }
            int pos = this.posIndex.get(idx);
            return (bytes[pos] & 1) != 0;
        }

        public final int getTagAt(int idx) {
            if (!this.tagged) {
                return 0;
            }
            byte[] bytes = this.getBytes();
            if (bytes == null) {
                return 0;
            }
            if ((idx -= this.first) < 0 || idx >= this.count) {
                return 0;
            }
            int pos = this.posIndex.get(idx);
            byte format0 = bytes[pos];
            if ((format0 & 1) != 0) {
                if ((format0 & 0x21) == 33) {
                    return bytes[pos + 2];
                }
                return 1;
            }
            return 0;
        }

        public final Object valueAt(int idx) {
            byte[] bytes = this.getBytes();
            if (bytes == null) {
                return null;
            }
            int fidx = idx - this.first;
            if (fidx < 0 || fidx >= this.count) {
                return null;
            }
            int pos = this.posIndex.get(fidx);
            return this.getValue(idx, bytes, pos, false, 0);
        }

        public final CompoundValue compoundAt(int idx) {
            byte[] bytes = this.getBytes();
            if (bytes == null) {
                return null;
            }
            int fidx = idx - this.first;
            if (fidx < 0 || fidx >= this.count) {
                return null;
            }
            int pos = this.posIndex.get(fidx);
            return (CompoundValue)this.getValue(idx, bytes, pos, true, 0);
        }

        public final CompoundValue compoundAt(int idx, int flags) {
            byte[] bytes = this.getBytes();
            if (bytes == null) {
                return null;
            }
            int fidx = idx - this.first;
            if (fidx < 0 || fidx >= this.count) {
                return null;
            }
            int pos = this.posIndex.get(fidx);
            return (CompoundValue)this.getValue(idx, bytes, pos, true, flags);
        }

        private final Object getValue(int idx, byte[] bytes, int pos, boolean compound, int flags) {
            byte format0 = bytes[pos++];
            byte format1 = bytes[pos++];
            byte tag = (format0 & 1) != 0 ? (byte)1 : 0;
            int dataLength = 0;
            if ((format1 & 0x10) != 0) {
                byte sn;
                dataLength = format1 & 0xF;
                if ((format0 & 0x21) == 33) {
                    tag = bytes[pos++];
                }
                int shift = 4;
                do {
                    sn = bytes[pos++];
                    dataLength |= (sn & 0x7F) << shift;
                    shift += 7;
                } while ((sn & 0x80) != 0);
            } else {
                dataLength = format1 & 0xF;
                if ((format0 & 0x21) == 33) {
                    tag = bytes[pos++];
                }
            }
            int group = 0;
            int order = 0;
            int layer = 0;
            if ((format0 & 6) != 0) {
                byte sn;
                order = (format0 & 6) >>> 1;
                if (order == 1 && ((layer = bytes[pos++]) & 0x80) != 0) {
                    order = 5;
                    layer &= 0x7F;
                }
                int shift = 0;
                do {
                    sn = bytes[pos++];
                    group |= (sn & 0x7F) << shift;
                    shift += 7;
                } while ((sn & 0x80) != 0);
            }
            Object valObj = SamplesReader.this.packed2Value(bytes, pos, format0, dataLength, flags);
            if (compound) {
                List<IAttachment> attachments = null;
                if (flags != 0) {
                    pos += dataLength;
                    switch ((format1 & 0xE0) >> 5) {
                        case 3: 
                        case 5: {
                            pos += 8;
                            break;
                        }
                        case 2: {
                            pos += 4;
                            break;
                        }
                        case 1: {
                            pos += 2;
                            break;
                        }
                        case 0: {
                            ++pos;
                        }
                    }
                    attachments = this.getAttachments(idx, group, layer, bytes, pos, flags & 0x1E);
                }
                valObj = new CompoundValue(SamplesReader.this, idx, SamplesReader.this.unitsAt(idx), (format0 & 0xC0) == 0, tag, group, order, layer, valObj, attachments);
            }
            return valObj;
        }

        public final CompoundPack packedAt(int idx) {
            byte[] bytes = this.getBytes();
            if (bytes == null) {
                return null;
            }
            int fidx = idx - this.first;
            if (fidx < 0 || fidx >= this.count) {
                return null;
            }
            int pos = this.posIndex.get(fidx);
            return this.getPacked(idx, bytes, pos);
        }

        private CompoundPack getPacked(int idx, byte[] bytes, int pos) {
            byte format0 = bytes[pos++];
            byte format1 = bytes[pos++];
            byte tag = (format0 & 1) != 0 ? (byte)1 : 0;
            int dataLength = 0;
            if ((format1 & 0x10) != 0) {
                byte sn;
                dataLength = format1 & 0xF;
                if ((format0 & 0x21) == 33) {
                    tag = bytes[pos++];
                }
                int shift = 4;
                do {
                    sn = bytes[pos++];
                    dataLength |= (sn & 0x7F) << shift;
                    shift += 7;
                } while ((sn & 0x80) != 0);
            } else {
                dataLength = format1 & 0xF;
                if ((format0 & 0x21) == 33) {
                    tag = bytes[pos++];
                }
            }
            int group = 0;
            int order = 0;
            int layer = 0;
            if ((format0 & 6) != 0) {
                byte sn;
                order = (format0 & 6) >>> 1;
                if (order == 1 && ((layer = bytes[pos++]) & 0x80) != 0) {
                    order = 5;
                    layer &= 0x7F;
                }
                int shift = 0;
                do {
                    sn = bytes[pos++];
                    group |= (sn & 0x7F) << shift;
                    shift += 7;
                } while ((sn & 0x80) != 0);
            }
            return new CompoundPack(SamplesReader.this, idx, this.unitsAt(idx), format0, tag, group, order, layer, bytes, pos, dataLength);
        }

        protected final void getStatInfo(int fidx, byte[] bytes, StatInfo info) {
            byte sn;
            int pos = this.posIndex.get(fidx);
            byte format0 = bytes[pos++];
            byte format1 = bytes[pos++];
            byte tag = (format0 & 1) != 0 ? (byte)1 : 0;
            int dataLength = 0;
            if ((format1 & 0x10) != 0) {
                dataLength = format1 & 0xF;
                if ((format0 & 0x21) == 33) {
                    tag = bytes[pos++];
                }
                int shift = 4;
                do {
                    sn = bytes[pos++];
                    dataLength |= (sn & 0x7F) << shift;
                    shift += 7;
                } while ((sn & 0x80) != 0);
            } else {
                dataLength = format1 & 0xF;
                if ((format0 & 0x21) == 33) {
                    tag = bytes[pos++];
                }
            }
            info.hasTag = info.hasTag | (format0 & 1) != 0;
            if (tag > 0 && (info.tag == 0 || info.tag > tag)) {
                info.tag = tag;
            }
            info.hasNonTag = info.hasNonTag | (format0 & 1) == 0;
            info.hasNone = info.hasNone | (format0 & 0xC0) == 0;
            info.hasValues = info.hasValues | (format0 & 0xC0) != 0;
            if ((format0 & 6) != 0) {
                int order = (format0 & 6) >>> 1;
                if (order == 1 && (bytes[pos++] & 0x80) != 0) {
                    order = 5;
                }
                while (((sn = bytes[pos++]) & 0x80) != 0) {
                }
            }
            if (info.readValues) {
                info.val = SamplesReader.this.packed2Value(bytes, pos, format0, dataLength, 0);
            }
            if (!info.hasChange && info.detectChange && info.refPacked != null) {
                info.hasChange = info.hasChange | info.refPacked[0] != format0;
                info.hasChange = info.hasChange | dataLength != info.refPacked.length - 1;
                if (!info.hasChange) {
                    int n = 1;
                    while (n <= dataLength) {
                        if (bytes[pos++] != info.refPacked[n]) {
                            info.hasChange = true;
                            info.detectChange = false;
                            break;
                        }
                        ++n;
                    }
                }
            }
            if (info.readReference) {
                info.readReference = false;
                info.refPacked = new byte[dataLength + 1];
                info.refPacked[0] = format0;
                System.arraycopy(bytes, pos, info.refPacked, 1, dataLength);
            }
        }

        public final List<IAttachment> attachmentsAt(int idx, int types) {
            byte[] bytes = this.getBytes();
            if (bytes == null) {
                return null;
            }
            int fidx = idx - this.first;
            if (fidx < 0 || fidx >= this.count) {
                return null;
            }
            int pos = this.posIndex.get(fidx);
            byte format0 = bytes[pos++];
            byte format1 = bytes[pos++];
            int dataLength = 0;
            if ((format1 & 0x10) != 0) {
                byte sn;
                dataLength = format1 & 0xF;
                if ((format0 & 0x21) == 33) {
                    // empty if block
                }
                int shift = 4;
                do {
                    int n = ++pos;
                    ++pos;
                    sn = bytes[n];
                    dataLength |= (sn & 0x7F) << shift;
                    shift += 7;
                } while ((sn & 0x80) != 0);
            } else {
                dataLength = format1 & 0xF;
                if ((format0 & 0x21) == 33) {
                    ++pos;
                }
            }
            int group = 0;
            int order = 0;
            int layer = 0;
            if ((format0 & 6) != 0) {
                byte sn;
                order = (format0 & 6) >>> 1;
                if (order == 1 && ((layer = bytes[pos++]) & 0x80) != 0) {
                    order = 5;
                    layer &= 0x7F;
                }
                int shift = 0;
                do {
                    sn = bytes[pos++];
                    group |= (sn & 0x7F) << shift;
                    shift += 7;
                } while ((sn & 0x80) != 0);
            }
            pos += dataLength;
            switch ((format1 & 0xE0) >> 5) {
                case 3: 
                case 5: {
                    pos += 8;
                    break;
                }
                case 2: {
                    pos += 4;
                    break;
                }
                case 1: {
                    pos += 2;
                    break;
                }
                case 0: {
                    ++pos;
                }
            }
            return this.getAttachments(idx, group, layer, bytes, pos, types);
        }

        private final List<IAttachment> getAttachments(int idx, int group, int layer, byte[] bytes, int pos, int types) {
            ArrayList<IAttachment> attachments = new ArrayList<IAttachment>();
            byte format0 = 0;
            byte format1 = 0;
            int dataLength = 0;
            while (pos < bytes.length) {
                if (((format0 = bytes[pos++]) & 0x21) != 32) break;
                if (((format1 = bytes[pos++]) & 0x10) != 0) {
                    byte sn;
                    dataLength = format1 & 0xF;
                    if ((format0 & 0x21) == 33) {
                        // empty if block
                    }
                    int shift = 4;
                    do {
                        int n = ++pos;
                        ++pos;
                        sn = bytes[n];
                        dataLength |= (sn & 0x7F) << shift;
                        shift += 7;
                    } while ((sn & 0x80) != 0);
                } else {
                    dataLength = format1 & 0xF;
                    if ((format0 & 0x21) == 33) {
                        ++pos;
                    }
                }
                int next = pos + dataLength;
                if ((format0 & 0x1E & types) != 0) {
                    int[] m = null;
                    switch (format0 & 0x1E) {
                        case 4: {
                            byte relationType = 0;
                            if (SamplesReader.this.version >= 4) {
                                relationType = bytes[pos++];
                            }
                            String target = null;
                            m = SamplesReader.plusRead(bytes, pos);
                            pos += m[1];
                            if (m[0] > 0 && SamplesReader.this.legend != null) {
                                target = SamplesReader.this.legend.labelOfEnum(1, m[0]);
                            }
                            if (target == null) {
                                target = SamplesReader.this.getId();
                            }
                            String style = null;
                            m = SamplesReader.plusRead(bytes, pos);
                            pos += m[1];
                            if (m[0] > 0 && SamplesReader.this.legend != null) {
                                style = SamplesReader.this.legend.labelOfEnum(2, m[0]);
                            }
                            if (style == null) {
                                style = "";
                            }
                            IDomainBase targetBase = SamplesReader.this.getSamplesDomainBase();
                            if ((relationType & 2) != 0) {
                                String base;
                                m = SamplesReader.plusRead(bytes, pos);
                                pos += m[1];
                                if (SamplesReader.this.legend != null && (base = SamplesReader.this.legend.labelOfEnum(4, m[0])) != null) {
                                    targetBase = DomainBase.parse(base);
                                }
                            }
                            int targetIdx = -1;
                            if ((relationType & 4) != 0) {
                                m = SamplesReader.plusRead(bytes, pos);
                                pos += m[1];
                                targetIdx = m[0];
                            }
                            byte targetLayer = 0;
                            if ((relationType & 8) != 0) {
                                targetLayer = bytes[pos++];
                            }
                            long targetPosition = 0L;
                            if (next > pos) {
                                targetPosition = (bytes[next - 1] & 0x80) != 0 ? -1L : 0L;
                                int i = next - 1;
                                while (i >= pos) {
                                    targetPosition = targetPosition << 8 | (long)(0xFF & bytes[i]);
                                    --i;
                                }
                            }
                            if (SamplesReader.this.sampleDomainBase != null && SamplesReader.this.sampleDomainBase != SamplesReader.this.domainBase) {
                                targetPosition = SamplesReader.this.sampleDomainBase.convertTo(SamplesReader.this.domainBase, targetPosition);
                            }
                            long sourcePosition = this.unitsAt(idx);
                            if (target == null || style == null) break;
                            attachments.add(new AttachedRelation(relationType, style, target, targetPosition, targetBase, targetIdx, targetLayer, sourcePosition, idx, group, layer));
                            break;
                        }
                        case 8: {
                            String style = null;
                            m = SamplesReader.plusRead(bytes, pos);
                            pos += m[1];
                            if (SamplesReader.this.legend != null) {
                                style = SamplesReader.this.legend.labelOfEnum(3, m[0]);
                            }
                            m = SamplesReader.plusRead(bytes, pos);
                            pos += m[1];
                            int cfr_ignored_0 = m[0];
                            m = SamplesReader.plusRead(bytes, pos);
                            pos += m[1];
                            int cfr_ignored_1 = m[0];
                            if (style == null) break;
                            attachments.add(new AttachedLabel(style, this.unitsAt(idx), idx, group, 0));
                            break;
                        }
                        default: {
                            byte[] data = new byte[dataLength + 2];
                            System.arraycopy(bytes, pos - 2, data, 0, dataLength + 2);
                        }
                    }
                }
                pos = next;
            }
            return attachments;
        }

        protected final int groupAt(int idx) {
            byte[] bytes = this.getBytes();
            if (bytes == null) {
                return -1;
            }
            if ((idx -= this.first) < 0 || idx >= this.count) {
                return -1;
            }
            int pos = this.posIndex.get(idx);
            byte format0 = bytes[pos++];
            byte format1 = bytes[pos++];
            if ((format0 & 0x21) == 33) {
                ++pos;
            }
            if ((format1 & 0x10) != 0) {
                byte sn;
                while (((sn = bytes[pos++]) & 0x80) != 0) {
                }
            }
            int group = 0;
            if ((format0 & 6) != 0) {
                byte sn;
                int order = (format0 & 6) >>> 1;
                if (order == 1 && (bytes[pos++] & 0x80) != 0) {
                    order = 5;
                }
                int shift = 0;
                do {
                    sn = bytes[pos++];
                    group |= (sn & 0x7F) << shift;
                    shift += 7;
                } while ((sn & 0x80) != 0);
                return group;
            }
            return -1;
        }

        protected final int orderAt(int idx) {
            byte[] bytes = this.getBytes();
            if (bytes == null) {
                return -1;
            }
            if ((idx -= this.first) < 0 || idx >= this.count) {
                return -1;
            }
            int pos = this.posIndex.get(idx);
            byte format0 = bytes[pos++];
            byte format1 = bytes[pos++];
            if ((format0 & 0x21) == 33) {
                ++pos;
            }
            if ((format1 & 0x10) != 0) {
                byte sn;
                while (((sn = bytes[pos++]) & 0x80) != 0) {
                }
            }
            if ((format0 & 6) != 0) {
                int order = (format0 & 6) >>> 1;
                if (order == 1 && (bytes[pos++] & 0x80) != 0) {
                    order = 5;
                }
                return order;
            }
            return 0;
        }

        protected final int[] indexRangeOfGroup(int group) {
            if (this.groupIndex == null || this.groups <= (group -= this.firstGroup) || this.groupIndex.length <= group || group < 0) {
                return null;
            }
            int[] range = new int[]{this.groupIndex[group][0] + this.first, this.groupIndex[group][1], this.groupIndex[group][2] + this.first};
            return range;
        }

        protected final boolean collectGroupValuesOf(int group, int idx0, int idxN, List<CompoundValue> values, int flags) {
            boolean found = false;
            int n = Math.max(idx0, this.first);
            while (n <= idxN && n < this.first + this.count) {
                if (this.groupAt(n) == group) {
                    values.add(this.compoundAt(n, flags));
                    found = true;
                }
                ++n;
            }
            return found;
        }

        protected final boolean collectGroupAttachmentsOf(int group, int types, int idx0, int idxN, List<IAttachment> values) {
            boolean found = false;
            int n = Math.max(idx0, this.first);
            while (n <= idxN && n < this.first + this.count) {
                if (this.groupAt(n) == group) {
                    List<IAttachment> atts = this.attachmentsAt(n, types);
                    if (!Utils.isEmpty(atts)) {
                        values.addAll(atts);
                    }
                    found = true;
                }
                ++n;
            }
            return found;
        }

        protected final boolean adaptLaterGroupEnd(int group, int idx) {
            if (this.groupIndex == null || this.groups <= (group -= this.firstGroup) || this.groupIndex.length <= group || group < 0) {
                return false;
            }
            this.groupIndex[group][2] = idx - this.first;
            return true;
        }
    }

    class StatInfo {
        boolean readReference;
        boolean readValues;
        boolean detectChange;
        boolean hasTag;
        byte tag;
        boolean hasNonTag;
        boolean hasNone;
        boolean hasValues;
        boolean hasChange;
        Object val;
        byte[] refPacked;

        StatInfo() {
        }

        void clear() {
            this.hasTag = false;
            this.tag = 0;
            this.hasNonTag = false;
            this.hasChange = false;
            this.hasNone = false;
            this.hasValues = false;
        }
    }

    public class StatService
    extends SampleConverter
    implements IStatService {
        private int content;
        private int samplesRelease = 0;
        private StatFragment[] statFragments;
        private SampleConverterConfiguration converter;

        StatService() {
        }

        @Override
        public boolean init(SampleConverterConfiguration converter, int content) {
            this.content = content;
            this.converter = converter;
            this.update();
            return true;
        }

        @Override
        public int getContent() {
            return this.content;
        }

        public int update() {
            if (this.statFragments == null || this.samplesRelease != SamplesReader.this.sourceRelease) {
                this.samplesRelease = SamplesReader.this.sourceRelease;
                SamplesFragmentReader[] fragments = SamplesReader.this.fragments;
                if (fragments == null) {
                    this.statFragments = null;
                    return -1;
                }
                if (this.statFragments == null || this.statFragments.length != fragments.length) {
                    int n;
                    StatFragment[] newStat = new StatFragment[fragments.length];
                    if (this.statFragments != null) {
                        n = 0;
                        while (n < this.statFragments.length) {
                            newStat[n] = this.statFragments[n];
                            ++n;
                        }
                    }
                    n = 0;
                    while (n < newStat.length) {
                        if (newStat[n] == null || newStat[n].fragment != fragments[n]) {
                            newStat[n] = new StatFragment(fragments[n]);
                        }
                        ++n;
                    }
                    this.statFragments = newStat;
                }
                int n = 0;
                while (n < this.statFragments.length) {
                    if (this.statFragments[n].update() < 0) {
                        return -1;
                    }
                    ++n;
                }
                return 1;
            }
            return 0;
        }

        @Override
        public final SamplesStat getStat(int idx0, int idxN) {
            if (this.update() >= 0) {
                SamplesStat stat = new SamplesStat();
                int n = SamplesReader.this.fragmentAtIdx(idx0);
                while (n <= SamplesReader.this.fragmentAtIdx(idxN)) {
                    if (n != -1) {
                        this.statFragments[n].fetchStat(idx0, idxN, stat);
                    }
                    ++n;
                }
                stat.first = this.floatValue(SamplesReader.this.valueAt(idx0));
                stat.last = this.floatValue(SamplesReader.this.valueAt(idxN));
                float f = stat.med = stat.count > 0 ? stat.med / (float)stat.count : 0.0f;
                if (stat.value != null && (stat.flags & 4) == 0) {
                    stat.value = SamplesReader.this.packed2Value((byte[])stat.value, 1, ((byte[])stat.value)[0], ((byte[])stat.value).length - 1, 0);
                }
                return stat;
            }
            return null;
        }

        @Override
        public SampleConverterConfiguration getConverterConfiguration() {
            return this.converter;
        }

        class StatFragment
        extends SampleConverter {
            int count;
            int fragmentRelease;
            SamplesFragmentReader fragment;
            protected byte[] flags;
            protected byte[] tag;
            protected float[] min;
            protected float[] med;
            protected float[] max;
            protected byte[][] packed;

            public StatFragment(SamplesFragmentReader fragment) {
                this.fragment = fragment;
                int blocks = SamplesReader.this.samplesPerFragment / 256;
                if (SamplesReader.this.samplesPerFragment % 256 != 0) {
                    ++blocks;
                }
                this.flags = new byte[blocks];
                this.tag = new byte[blocks];
                if ((StatService.this.content & 8) != 0) {
                    this.min = new float[blocks];
                    this.max = new float[blocks];
                }
                if ((StatService.this.content & 0x10) != 0) {
                    this.med = new float[blocks];
                }
                if ((StatService.this.content & 2) != 0) {
                    this.packed = new byte[blocks][];
                }
            }

            public int update() {
                if (this.count < this.fragment.count) {
                    StatInfo info = new StatInfo();
                    byte[] bytes = (byte[])SamplesReader.this.samples.get(this.fragment.fragmentId);
                    if (bytes == null) {
                        return -1;
                    }
                    float _min = Float.MAX_VALUE;
                    float _med = 0.0f;
                    float _max = -3.4028235E38f;
                    int _count = 0;
                    int fidx = this.count;
                    while (fidx < this.fragment.count) {
                        if (fidx % 256 == 0) {
                            if (_count > 0) {
                                this.apply(fidx - _count, _count, info, _min, _med, _max);
                            }
                            _count = 0;
                            _min = Float.MAX_VALUE;
                            _med = 0.0f;
                            _max = -3.4028235E38f;
                            info.clear();
                            info.readValues = (StatService.this.content & 0x18) != 0;
                            info.readReference = (StatService.this.content & 2) != 0;
                        }
                        this.fragment.getStatInfo(fidx, bytes, info);
                        if ((StatService.this.content & 0x18) != 0) {
                            float val = this.floatValue(info.val);
                            if (_min > val) {
                                _min = val;
                            }
                            if (_max < val) {
                                _max = val;
                            }
                            _med += val;
                        }
                        ++_count;
                        ++fidx;
                    }
                    if (_count > 0) {
                        this.apply(fidx - _count, _count, info, _min, _med, _max);
                    }
                    return 1;
                }
                return 0;
            }

            private void apply(int idx, int _count, StatInfo info, float _min, float _med, float _max) {
                int block = idx / 256;
                int included = idx % 256;
                if (block >= 0 && block < this.flags.length) {
                    int n = block;
                    this.flags[n] = (byte)(this.flags[n] | (info.hasTag ? (byte)1 : 0));
                    int n2 = block;
                    this.flags[n2] = (byte)(this.flags[n2] | (info.hasNonTag ? 2 : 0));
                    int n3 = block;
                    this.flags[n3] = (byte)(this.flags[n3] | (info.hasNone ? 8 : 0));
                    int n4 = block;
                    this.flags[n4] = (byte)(this.flags[n4] | (info.hasValues ? 16 : 0));
                    int n5 = block;
                    this.flags[n5] = (byte)(this.flags[n5] | (info.hasChange ? 4 : 0));
                    if (info.tag > 0 && (this.tag[block] == 0 || info.tag < this.tag[block])) {
                        this.tag[block] = info.tag;
                    }
                    if (this.min != null) {
                        if (included > 0) {
                            this.min[block] = _min < this.min[block] ? _min : this.min[block];
                            this.max[block] = _max > this.max[block] ? _max : this.max[block];
                        } else {
                            this.min[block] = _min;
                            this.max[block] = _max;
                        }
                    }
                    if (this.med != null) {
                        this.med[block] = (this.med[block] * (float)included + _med * (float)_count) / (float)(included + _count);
                    }
                    if (this.packed != null && this.packed[block] == null && !info.hasChange) {
                        this.packed[block] = info.refPacked;
                    }
                    this.count += _count;
                }
            }

            /*
             * Unable to fully structure code
             */
            public void fetchStat(int idx0, int idxN, SamplesStat stat) {
                lazy = (StatService.access$0(StatService.this) & 2) == 0 && (StatService.access$0(StatService.this) & 24) == 0 && (idxN - idx0) / 256 > 2;
                idxN -= this.fragment.first;
                if ((idx0 -= this.fragment.first) < 0) {
                    idx0 = 0;
                }
                if (idxN > this.fragment.count - 1) {
                    idxN = this.fragment.count - 1;
                }
                bytes = null;
                info = null;
                if (!lazy) {
                    bytes = (byte[])SamplesReader.access$0(StatService.access$2(StatService.this)).get(SamplesFragmentReader.access$0(this.fragment));
                    if (bytes == null) {
                        return;
                    }
                    info = StatService.access$2(StatService.this).new StatInfo();
                    info.readValues = (StatService.access$0(StatService.this) & 24) != 0;
                    v0 = info.detectChange = (stat.flags & 4) == 0 && (StatService.access$0(StatService.this) & 2) != 0;
                    if (info.detectChange) {
                        if (stat.value == null) {
                            info.readReference = (StatService.access$0(StatService.this) & 2) != 0;
                        } else {
                            info.refPacked = (byte[])stat.value;
                        }
                    }
                }
                _min = 3.4028235E38f;
                _med = 0.0f;
                _max = -3.4028235E38f;
                _flags = 0;
                _tag = 0;
                _count = 0;
                fidx = idx0;
                while (fidx <= idxN) {
                    if (fidx % 256 != 0) ** GOTO lbl-1000
                    block = fidx / 256;
                    if (idxN - fidx >= 256 && block < this.flags.length) {
                        if (this.min != null && this.min[block] < _min) {
                            _min = this.min[block];
                        }
                        if (this.max != null && this.max[block] > _max) {
                            _max = this.max[block];
                        }
                        if (this.med != null) {
                            _med += this.med[block];
                        }
                        _flags = (byte)(_flags | this.flags[block]);
                        if (this.tag[block] > 0 && (_tag == 0 || this.tag[block] < _tag)) {
                            _tag = this.tag[block];
                        }
                        _count += 256;
                        if (info != null && info.detectChange) {
                            if (stat.value == null) {
                                this.fragment.getStatInfo(fidx, bytes, info);
                            } else if (stat.value != null && !Arrays.equals((byte[])stat.value, this.packed[block])) {
                                info.hasChange = true;
                                info.detectChange = false;
                            }
                        }
                        fidx += 255;
                    } else lbl-1000:
                    // 2 sources

                    {
                        if (!lazy) {
                            this.fragment.getStatInfo(fidx, bytes, info);
                            if ((StatService.access$0(StatService.this) & 24) != 0 && info.val != null) {
                                val = this.floatValue(info.val);
                                if (_min > val) {
                                    _min = val;
                                }
                                if (_max < val) {
                                    _max = val;
                                }
                                _med += val;
                            }
                        }
                        ++_count;
                    }
                    ++fidx;
                }
                if (!lazy) {
                    _flags = (byte)(_flags | (info.hasTag != false ? 1 : 0));
                    _flags = (byte)(_flags | (info.hasNonTag != false ? 2 : 0));
                    _flags = (byte)(_flags | (info.hasNone != false ? 8 : 0));
                    _flags = (byte)(_flags | (info.hasValues != false ? 16 : 0));
                    _flags = (byte)(_flags | (info.hasChange != false ? 4 : 0));
                    if (info.tag > 0 && (_tag == 0 || info.tag < _tag)) {
                        _tag = info.tag;
                    }
                }
                stat.flags = (short)(stat.flags | _flags);
                if (_tag > 0 && (stat.tag == 0 || stat.tag < _tag)) {
                    stat.tag = _tag;
                }
                if (stat.min > _min) {
                    stat.min = _min;
                }
                if (stat.max < _max) {
                    stat.max = _max;
                }
                stat.med += _med;
                stat.count += _count;
                if (stat.value == null && info != null) {
                    stat.value = info.refPacked;
                }
            }

            @Override
            public SampleConverterConfiguration getConverterConfiguration() {
                return StatService.this.converter;
            }
        }
    }
}

