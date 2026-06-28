/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.writer;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesLegend;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.legend.SamplesLegend;
import de.toem.impulse.values.AttachedLabel;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.pattern.element.exploits.Marker;
import de.toem.toolkits.pattern.element.exploits.Markers;
import de.toem.toolkits.pattern.pageable.BytesPageable;
import de.toem.toolkits.pattern.pageable.Pageable;
import java.util.LinkedList;
import java.util.List;

public class SamplesWriter
extends PackedSamples
implements ISamplesWriter {
    protected static volatile long usedBuffer = 0L;
    protected static volatile long existingWriters = 0L;
    protected static volatile long adjustedFor = 0L;
    static byte[] NONE_BUFFER = new byte[2092];
    protected static int startBuffer;
    protected static int maxBuffer;
    protected static long totalBuffer;
    protected byte[] buffer = NONE_BUFFER;
    protected int buffered;
    private int bufferFlushed;
    private boolean newFragment;
    private byte[] header = new byte[6];
    static final int DEFAULT_SAMPLES_PER_FRAGMENT = 16;
    static final int MAX_SAMPLES_PER_FRAGMENT = 256;
    private int samples256PerFragment;
    private int maxFragments;
    protected int count;
    protected int groups;
    private long written;
    private long writtenInCurrentFragment;
    private long last;
    private Long discreteRate;
    private List<Long> fragmentStarts;
    private long maxUnits;
    private int writerRelease;

    public SamplesWriter(String id, String name, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        super(id, name, processType, signalType, signalDescriptor, domainBase);
        ++existingWriters;
    }

    protected void finalize() {
        --existingWriters;
    }

    @Override
    public boolean isOpen() {
        return this.buffer != NONE_BUFFER;
    }

    @Override
    public final boolean open(long units) {
        return this.open(units, null);
    }

    @Override
    public final boolean open(long units, Pageable<byte[]> samples) {
        return this.open(units, 0, 0, samples);
    }

    @Override
    public final boolean open(long units, int samples256PerFragment, int maxFragments, Pageable<byte[]> samples) {
        return this.open(units, 0L, samples256PerFragment, maxFragments, samples);
    }

    @Override
    public boolean open(long units, long rate, int samples256PerFragment, int maxFragments, Pageable<byte[]> samples) {
        return this.open(units, units, rate, samples256PerFragment, maxFragments, samples);
    }

    @Override
    public boolean open(long units, long maxUnits, long rate, int samples256PerFragment, int maxFragments, Pageable<byte[]> samples) {
        if (this.isOpen()) {
            return true;
        }
        this.end = this.start = units;
        this.maxUnits = maxUnits;
        this.rate = rate;
        this.buffered = 0;
        this.bufferFlushed = 0;
        this.written = 0L;
        this.writtenInCurrentFragment = 0L;
        this.count = 0;
        this.groups = 0;
        this.samples256PerFragment = samples256PerFragment <= 0 ? 16 : (samples256PerFragment > 256 ? 256 : samples256PerFragment);
        this.maxFragments = maxFragments < 0 ? 0 : maxFragments;
        this.last = this.start;
        this.newFragment = true;
        this.discreteRate = null;
        if (this.maxFragments > 0) {
            this.fragmentStarts = new LinkedList<Long>();
        }
        if (this.processType == ISamples.ProcessType.Discrete) {
            this.rate = 0L;
        } else if (this.processType == ISamples.ProcessType.Continuous && this.rate <= 0L) {
            this.rate = 1L;
        } else if (this.processType == ISamples.ProcessType.Unknown && this.rate <= 0L) {
            this.processType = ISamples.ProcessType.Discrete;
            this.rate = 0L;
        } else if (this.processType == ISamples.ProcessType.Unknown && this.rate > 0L) {
            this.processType = ISamples.ProcessType.Continuous;
        }
        if (this.processType == ISamples.ProcessType.Continuous) {
            this.last -= rate;
        }
        if (adjustedFor != existingWriters) {
            adjustedFor = existingWriters;
            SamplesWriter.adjustBufferGeometry();
        }
        try {
            this.createBuffer();
        }
        catch (Throwable throwable) {
            return false;
        }
        this.header[0] = 21;
        this.header[1] = 8;
        this.header[2] = 6;
        this.header[3] = 0;
        this.header[4] = (byte)(this.samples256PerFragment - 1);
        try {
            this.samples = samples == null ? new BytesPageable() : samples;
            this.samples.open();
            return true;
        }
        catch (Throwable throwable) {
            this.buffer = NONE_BUFFER;
            samples = null;
            return false;
        }
    }

    @Override
    public void close() {
        this.close(this.maxUnits > this.start ? this.maxUnits : this.end);
    }

    @Override
    public void close(long units) {
        int noOfSamples;
        if (!this.isOpen()) {
            return;
        }
        if (this.processType == ISamples.ProcessType.Continuous && this.last < units && (noOfSamples = (int)((units - this.last) / this.rate)) > 0) {
            this.writeMultiplier(noOfSamples);
        }
        this.flush(units);
        if (this.samples != null) {
            this.samples.close();
        }
        if (this.buffer != NONE_BUFFER) {
            usedBuffer -= (long)this.buffer.length;
        }
        this.buffer = NONE_BUFFER;
    }

    @Override
    public void flush() {
        this.flush(this.end);
    }

    @Override
    public void flush(long units) {
        if (!this.isOpen()) {
            return;
        }
        if (this.processType == ISamples.ProcessType.Discrete && units >= this.end) {
            this.end = units;
        }
        if (this.buffered > 0 && this.samples != null) {
            if (this.newFragment) {
                if (this.samples.getFragmentCount() == this.maxFragments && this.maxFragments > 0) {
                    this.start = this.fragmentStarts.remove(0);
                    this.samples.reduce();
                }
                this.samples.extend(true, this.header, 0, this.header.length);
                this.newFragment = false;
            }
            this.samples.extend(false, this.buffer, 0, this.buffered);
            this.samples.a = this.count;
            this.samples.b = this.groups;
            this.buffered = 0;
        }
        ++this.writerRelease;
    }

    @Override
    public long getMaxUnits() {
        return this.maxUnits;
    }

    @Override
    public boolean isVolatile() {
        return this.isOpen();
    }

    @Override
    public final int getRelease() {
        return this.writerRelease;
    }

    @Override
    public final boolean isEmpty() {
        return this.count <= 0;
    }

    @Override
    public final int getCount() {
        return this.count;
    }

    @Override
    public final int getGroups() {
        return this.groups;
    }

    @Override
    public final int getPackVersion() {
        return 6;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    @Override
    public boolean apply(Signal signal) {
        Signal signal2 = signal;
        synchronized (signal2) {
            block8: {
                Pageable<byte[]> bcontext;
                if (this.end == signal.end && this.samples == signal.samples && this.samples == signal.samples && ISamples.SignalType.equals(this.signalType, signal) && ISamples.ProcessType.equals(this.getProcessType(), signal) && ISamples.SignalDescriptor.equals(this.signalDescriptor, signal) && this.tagged == signal.tag && ISamples.TagDomain.equals(this.tagDomain, signal) && (this.samples == null || this.samples.a == this.count && this.samples.b == this.groups) && this.domainBase.equals(DomainBase.parse(signal.domainBase)) && this.start == signal.start && this.rate == signal.rate && (this.legend == null || !this.legend.isModified())) break block8;
                signal.processType = this.getProcessType().toString();
                signal.signalType = this.signalType != null ? this.signalType.toString() : null;
                signal.signalDescriptor = this.signalDescriptor.toString();
                signal.tag = this.tagged;
                signal.diff = this.tagDomain == ISamples.TagDomain.Diff && this.tagged ? 1 : 0;
                signal.tagDomain = this.tagDomain != null ? this.tagDomain.toString() : null;
                signal.domainBase = this.domainBase.toString();
                signal.start = this.start;
                signal.end = this.end;
                signal.rate = this.rate;
                if (signal.samples != this.samples) {
                    signal.samples = this.samples;
                    if (signal.samples != null) {
                        signal.samples.a = this.count;
                        signal.samples.b = this.groups;
                    }
                }
                if (signal.samples != null) {
                    signal.samples.a = this.count;
                    signal.samples.b = this.groups;
                }
                Pageable<byte[]> pageable = bcontext = this.legend != null ? this.legend.toBytes() : null;
                if (bcontext != signal.legend) {
                    signal.legend = bcontext;
                }
                signal.markers = this.markers != null ? this.markers.value() : null;
                return true;
            }
            return false;
        }
    }

    @Override
    public void setDomainBase(IDomainBase domainBase) {
        this.domainBase = domainBase;
    }

    @Override
    public boolean addMarker(Marker marker) {
        if (this.markers == null) {
            this.markers = new Markers();
        }
        this.markers.add(marker);
        return true;
    }

    @Override
    public void setTagDomain(ISamples.TagDomain tagDomain) {
        this.tagDomain = tagDomain;
    }

    public static void adjustBufferGeometry() {
        Runtime runtime = Runtime.getRuntime();
        long freeMemory = runtime.freeMemory() + (runtime.maxMemory() - runtime.totalMemory());
        totalBuffer = (long)((double)freeMemory * 0.2);
        startBuffer = (int)Math.min(Math.max(totalBuffer / Math.max(existingWriters, 1L) / 4L, 6L), 65536L);
        if (startBuffer < 1024) {
            runtime.gc();
            freeMemory = runtime.freeMemory() + (runtime.maxMemory() - runtime.totalMemory());
            totalBuffer = (long)((double)freeMemory * 0.2);
            startBuffer = (int)Math.min(Math.max(totalBuffer / existingWriters / 4L, 6L), 65536L);
        }
    }

    protected void createBuffer() {
        this.buffer = new byte[startBuffer];
        usedBuffer += (long)this.buffer.length;
    }

    protected void adjustBuffer(int request, int flushed) {
        block10: {
            int newBufferSize = 0;
            boolean force = false;
            if (this.buffer.length < 4 * request) {
                newBufferSize = 4 * request;
                force = true;
            } else if (flushed > 5) {
                newBufferSize = this.buffer.length * 10;
            } else if (flushed > 50) {
                newBufferSize = this.buffer.length * 10;
            } else if (flushed > 500) {
                newBufferSize = this.buffer.length * 10;
            }
            if (newBufferSize > 0 && (usedBuffer < totalBuffer || force)) {
                try {
                    byte[] newBuffer = new byte[newBufferSize];
                    usedBuffer -= (long)this.buffer.length;
                    this.buffer = newBuffer;
                    usedBuffer += (long)this.buffer.length;
                }
                catch (Throwable throwable) {
                    if (!force) break block10;
                    this.buffer = NONE_BUFFER;
                }
            }
        }
    }

    protected final int beginWrite(long units, int request) {
        return this.begin(units, false, request + 24);
    }

    protected final int beginAttach(int request) {
        return this.begin(this.last, true, request + 4);
    }

    private final int begin(long units, boolean attachment, int request) {
        boolean samplesPerFragmentReached;
        if (!attachment) {
            if (this.processType == ISamples.ProcessType.Discrete) {
                if (units < this.last || this.maxUnits > this.start && units > this.maxUnits) {
                    return -1;
                }
            } else if (units != Long.MIN_VALUE) {
                if (units <= this.last || this.maxUnits > this.start && units > this.maxUnits) {
                    return -1;
                }
                int noOfSamples = (int)((units - this.last) / this.rate);
                if (this.last + (long)noOfSamples * this.rate != units) {
                    return -1;
                }
                if (noOfSamples > 1) {
                    this.writeMultiplier(noOfSamples - 1);
                }
            } else if (this.maxUnits > this.start && this.last + this.rate > this.maxUnits) {
                return -1;
            }
        }
        boolean bufferInsufficient = this.buffer.length - this.buffered < request;
        boolean bl = samplesPerFragmentReached = !attachment && this.writtenInCurrentFragment >= (long)(256 * this.samples256PerFragment);
        if (bufferInsufficient || samplesPerFragmentReached) {
            if (this.buffered > 0) {
                if (this.samples != null) {
                    if (this.newFragment) {
                        if (this.samples.getFragmentCount() == this.maxFragments && this.maxFragments > 0) {
                            this.start = this.fragmentStarts.remove(0);
                            this.samples.reduce();
                        }
                        this.samples.extend(true, this.header, 0, this.header.length);
                        this.newFragment = false;
                    }
                    if (!this.samples.extend(false, this.buffer, 0, this.buffered)) {
                        return -1;
                    }
                }
                this.buffered = 0;
            }
            if (bufferInsufficient) {
                ++this.bufferFlushed;
                this.adjustBuffer(request, this.bufferFlushed);
            }
            if (samplesPerFragmentReached) {
                this.newFragment = true;
                this.writtenInCurrentFragment = 0L;
                if (this.maxFragments > 0) {
                    this.fragmentStarts.add(this.last);
                    this.discreteRate = null;
                }
            }
        }
        return this.buffered;
    }

    protected void writeMultiplier(int noOfSamples) {
        if (this.written == 0L) {
            this.writeNone(this.start, false);
        }
        int begin = this.begin(this.last, true, 8);
        this.buffer[this.buffered++] = 32;
        this.buffer[this.buffered++] = 0;
        this.buffered += SamplesWriter.plusWrite(this.buffer, this.buffered, noOfSamples);
        int n = begin + 1;
        this.buffer[n] = (byte)(this.buffer[n] | (byte)(this.buffered - begin - 2));
        this.writtenInCurrentFragment += (long)(noOfSamples - 1);
        this.count += noOfSamples;
        this.written += (long)noOfSamples;
        this.last += this.rate * (long)noOfSamples;
        this.end = this.last;
    }

    protected final boolean endWrite(long units, int begin) {
        if (this.processType == ISamples.ProcessType.Discrete) {
            long delta = units - this.last;
            if (delta == 0L) {
                int n = begin + 1;
                this.buffer[n] = (byte)(this.buffer[n] | 0xE0);
                this.discreteRate = null;
            } else if (this.discreteRate != null && this.discreteRate == delta) {
                int n = begin + 1;
                this.buffer[n] = (byte)(this.buffer[n] | 0x80);
            } else {
                int timeFormat = 0;
                this.buffer[this.buffered++] = (byte)(delta & 0xFFL);
                if ((delta & 0xFFFFFFFFFFFFFF00L) != 0L) {
                    timeFormat = 1;
                    this.buffer[this.buffered++] = (byte)((delta >>>= 8) & 0xFFL);
                    if ((delta & 0xFFFFFFFFFFFFFF00L) != 0L) {
                        timeFormat = 2;
                        this.buffer[this.buffered++] = (byte)((delta >>>= 8) & 0xFFL);
                        this.buffer[this.buffered++] = (byte)((delta >>>= 8) & 0xFFL);
                        if ((delta & 0xFFFFFFFFFFFFFF00L) != 0L) {
                            timeFormat = 3;
                            this.buffer[this.buffered++] = (byte)((delta >>>= 8) & 0xFFL);
                            this.buffer[this.buffered++] = (byte)((delta >>>= 8) & 0xFFL);
                            this.buffer[this.buffered++] = (byte)((delta >>>= 8) & 0xFFL);
                            this.buffer[this.buffered++] = (byte)((delta >>>= 8) & 0xFFL);
                        }
                    }
                }
                int n = begin + 1;
                this.buffer[n] = (byte)(this.buffer[n] | timeFormat << 5);
                this.discreteRate = units - this.last;
            }
            this.end = this.last = units;
        } else if (this.processType == ISamples.ProcessType.Continuous) {
            this.last += this.rate;
            this.end = this.last;
        }
        this.tagged |= (this.buffer[begin] & 1) != 0;
        ++this.count;
        ++this.written;
        ++this.writtenInCurrentFragment;
        return true;
    }

    @Override
    public boolean write(long units, boolean tag) {
        return this.writeSample(units, (byte)(tag ? 1 : 0));
    }

    @Override
    public boolean write(long units, int tag) {
        if (tag > 1) {
            return false;
        }
        return this.writeSample(units, (byte)(tag == 1 ? 1 : 0));
    }

    @Override
    public boolean writeNone(long units, boolean tag) {
        return this.writeSample(units, (byte)(tag ? 1 : 0));
    }

    @Override
    public boolean writeNone(long units, int tag) {
        if (tag > 1) {
            return false;
        }
        return this.writeSample(units, (byte)(tag == 1 ? 1 : 0));
    }

    @Override
    public boolean writeSample(long units, boolean tag, Object value) {
        return this.writeSample(units, tag ? 1 : 0, value);
    }

    @Override
    public boolean writeSample(long units, int tag, Object value) {
        return false;
    }

    @Override
    public boolean writeSample(long units, boolean tag, int group, int order, int layer, Object value) {
        return this.writeSample(units, tag ? 1 : 0, group, order, layer, value);
    }

    @Override
    public boolean writeSample(long units, int tag, int group, int order, int layer, Object value) {
        return false;
    }

    @Override
    public final boolean writeSample(long units, byte format0) {
        if ((format0 & 6) != 0 || (format0 & 0x21) == 33) {
            return false;
        }
        int begin = this.beginWrite(units, 0);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = format0;
        this.buffer[this.buffered++] = 0;
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean writeSample(long units, byte format0, byte data0) {
        if ((format0 & 6) != 0 || (format0 & 0x21) == 33) {
            return false;
        }
        int begin = this.beginWrite(units, 1);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = format0;
        this.buffer[this.buffered++] = 1;
        this.buffer[this.buffered++] = data0;
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean writeSample(long units, byte format0, byte[] data, int start, int dlength) {
        if ((format0 & 6) != 0 || (format0 & 0x21) == 33) {
            return false;
        }
        int begin = this.beginWrite(units, dlength);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = format0;
        if (dlength <= 15) {
            this.buffer[this.buffered++] = (byte)dlength;
        } else {
            int s = dlength;
            this.buffer[this.buffered++] = (byte)(s & 0xF | 0x10);
            s >>>= 4;
            while (true) {
                if (s <= 127) {
                    this.buffer[this.buffered++] = (byte)(s & 0x7F);
                    break;
                }
                this.buffer[this.buffered++] = (byte)(s & 0x7F | 0x80);
                s >>>= 7;
            }
        }
        int i = start;
        while (i < start + dlength) {
            this.buffer[this.buffered++] = data[i];
            ++i;
        }
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean writeSample(long units, byte format0, int group, int layer, byte[] data, int start, int dlength) {
        if ((format0 & 0x21) == 33) {
            return false;
        }
        int order = (format0 & 6) >>> 1;
        if (order == 1 && group != this.groups || group > this.groups) {
            return false;
        }
        int begin = this.beginWrite(units, dlength);
        if (begin < 0) {
            return false;
        }
        this.groups = Math.max(this.groups, group + 1);
        this.buffer[this.buffered++] = format0;
        if (dlength <= 15) {
            this.buffer[this.buffered++] = (byte)dlength;
        } else {
            int s = dlength;
            this.buffer[this.buffered++] = (byte)(s & 0xF | 0x10);
            s >>>= 4;
            while (true) {
                if (s <= 127) {
                    this.buffer[this.buffered++] = (byte)(s & 0x7F);
                    break;
                }
                this.buffer[this.buffered++] = (byte)(s & 0x7F | 0x80);
                s >>>= 7;
            }
        }
        if ((format0 & 6) != 0) {
            if (order == 1) {
                this.buffer[this.buffered++] = (byte)(layer & 0x7F);
                ++this.groups;
            }
            this.buffered += SamplesWriter.plusWrite(this.buffer, this.buffered, group);
        }
        int i = start;
        while (i < start + dlength) {
            this.buffer[this.buffered++] = data[i];
            ++i;
        }
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean writeSample(CompoundPack packed) {
        int dlength = packed.getLength();
        byte format0 = (byte)packed.getFormat0();
        int begin = this.beginWrite(packed.getUnits(), dlength);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = format0;
        if (dlength <= 15) {
            this.buffer[this.buffered++] = (byte)dlength;
            if ((format0 & 0x21) == 33) {
                this.buffer[this.buffered++] = (byte)packed.getTag();
            }
        } else {
            int s = dlength;
            this.buffer[this.buffered++] = (byte)(s & 0xF | 0x10);
            if ((format0 & 0x21) == 33) {
                this.buffer[this.buffered++] = (byte)packed.getTag();
            }
            s >>>= 4;
            while (true) {
                if (s <= 127) {
                    this.buffer[this.buffered++] = (byte)(s & 0x7F);
                    break;
                }
                this.buffer[this.buffered++] = (byte)(s & 0x7F | 0x80);
                s >>>= 7;
            }
        }
        if ((format0 & 6) != 0) {
            int order = packed.getOrder();
            if (order == 1 || order == 5) {
                byte layer = (byte)packed.getLayer();
                layer = order == 5 ? (byte)(layer | 0x80) : (byte)(layer & 0x7F);
                this.buffer[this.buffered++] = layer;
            }
            this.groups = Math.max(this.groups, packed.getGroup() + 1);
            this.buffered += SamplesWriter.plusWrite(this.buffer, this.buffered, packed.getGroup());
        }
        byte[] bytes = packed.getBytes();
        int i = 0;
        while (i < dlength) {
            this.buffer[this.buffered++] = bytes[i];
            ++i;
        }
        return this.endWrite(packed.getUnits(), begin);
    }

    @Override
    public boolean writeSample(CompoundValue value) {
        if (value == null) {
            return false;
        }
        boolean result = false;
        result = value.isNone() ? this.writeNone(value.getUnits(), value.getTag()) : (value.getGroup() >= 0 && value.getOrder() > 0 ? this.writeSample(value.getUnits(), value.getTag(), value.getGroup(), value.getOrder(), value.getLayer(), value.val()) : this.writeSample(value.getUnits(), value.getTag(), value.val()));
        if (result) {
            value.attachments(30);
        }
        return result;
    }

    @Override
    public void setLegend(ISamplesLegend legend) {
        if (legend instanceof SamplesLegend) {
            this.legend = (SamplesLegend)legend;
        }
    }

    @Override
    public int addMember(String name, String content, int format) {
        if (this.legend != null) {
            return this.legend.addMember(name, 0, content, format);
        }
        return -1;
    }

    @Override
    public boolean setMember(int id, String name, String content, int format) {
        return this.setMember(id, name, 0, content, format);
    }

    @Override
    public boolean setMember(int id, String name, int type, String content, int format) {
        if (this.legend != null) {
            return this.legend.setMember(id, name, type, content, format);
        }
        return false;
    }

    @Override
    public boolean setMember(int id, int parentId, String name, int type, String content, int format) {
        if (this.legend != null) {
            return this.legend.setMember(id, parentId, name, type, content, format);
        }
        return false;
    }

    @Override
    public boolean setEnum(int enumerationGroup, String label, int value) {
        if (this.legend != null) {
            return this.legend.setEnum(enumerationGroup, label, value);
        }
        return false;
    }

    public boolean attach(IAttachment attachment) {
        if (attachment instanceof IAttachment.IAttachedRelation) {
            this.attachRelation(((IAttachment.IAttachedRelation)attachment).getType(), ((IAttachment.IAttachedRelation)attachment).getTargetId(), ((IAttachment.IAttachedRelation)attachment).getStyle(), ((IAttachment.IAttachedRelation)attachment).getTargetPosition(), ((IAttachment.IAttachedRelation)attachment).getTargetBase(), ((IAttachment.IAttachedRelation)attachment).getTargetIdx(), ((IAttachment.IAttachedRelation)attachment).getTargetLayer());
        } else if (attachment instanceof AttachedLabel) {
            this.attachLabel(((AttachedLabel)attachment).getStyle());
        }
        return true;
    }

    @Override
    public boolean attachRelation(String target, String style, long deltaPositionInUnits) {
        return this.attachRelation(0, target, style, deltaPositionInUnits);
    }

    @Override
    public boolean attachRelation(int type, String target, String style, long targetPosition) {
        if (this.legend != null) {
            int styleId;
            int targetId;
            int n = targetId = target != null ? this.legend.valOfEnum(1, target) : -1;
            if (targetId == -1 && target != null) {
                targetId = this.legend.addEnum(1, target);
            }
            int n2 = styleId = style != null ? this.legend.valOfEnum(2, style) : -1;
            if (styleId == -1 && style != null) {
                styleId = this.legend.addEnum(2, style);
            }
            return this.attachRelation(type, targetId, styleId, targetPosition);
        }
        return false;
    }

    @Override
    public boolean attachRelation(int type, String target, String style, long targetPosition, IDomainBase targetBase) {
        return this.attachRelation(type, target, style, targetPosition, targetBase, 0, 0);
    }

    @Override
    public boolean attachRelation(int type, String target, String style, long targetPosition, IDomainBase targetBase, int targetIdx, int targetLayer) {
        if (this.legend != null) {
            int targetBaseId;
            int styleId;
            int targetId;
            int n = targetId = target != null ? this.legend.valOfEnum(1, target) : 0;
            if (targetId == -1 && target != null) {
                targetId = this.legend.addEnum(1, target);
            }
            int n2 = styleId = style != null ? this.legend.valOfEnum(2, style) : 0;
            if (styleId == -1 && style != null) {
                styleId = this.legend.addEnum(2, style);
            }
            int n3 = targetBaseId = targetBase != null ? this.legend.valOfEnum(4, targetBase.toString()) : 0;
            if (styleId == -1 && targetBase != null) {
                styleId = this.legend.addEnum(2, style);
            }
            return this.attachRelation(type, targetId, styleId, targetPosition, targetBaseId, targetIdx, targetLayer);
        }
        return false;
    }

    @Override
    public boolean attachRelation(int targetId, int styleId, long deltaPositionInUnits) {
        return this.attachRelation(0, targetId, styleId, deltaPositionInUnits);
    }

    @Override
    public boolean attachRelation(int type, int targetId, int styleId, long targetPosition) {
        return this.attachRelation(type, targetId, styleId, targetPosition, 0);
    }

    @Override
    public boolean attachRelation(int type, int targetId, int styleId, long targetPosition, int targetBaseId) {
        return this.attachRelation(type, targetId, styleId, targetPosition, targetBaseId, 0, 0);
    }

    /*
     * Unable to fully structure code
     */
    @Override
    public boolean attachRelation(int type, int targetId, int styleId, long targetPosition, int targetBaseId, int targetIdx, int targetLayer) {
        block10: {
            begin = this.beginAttach(16);
            if (begin < 0) {
                return false;
            }
            this.buffer[this.buffered++] = 36;
            this.buffer[this.buffered++] = 0;
            this.buffer[this.buffered++] = (byte)type;
            this.buffered += SamplesWriter.plusWrite(this.buffer, this.buffered, targetId);
            this.buffered += SamplesWriter.plusWrite(this.buffer, this.buffered, styleId);
            if ((type & 2) != 0) {
                this.buffered += SamplesWriter.plusWrite(this.buffer, this.buffered, targetBaseId);
            }
            if ((type & 4) != 0) {
                this.buffered += SamplesWriter.plusWrite(this.buffer, this.buffered, targetIdx);
            }
            if ((type & 8) != 0) {
                this.buffer[this.buffered++] = (byte)(targetLayer % 31);
            }
            if (targetPosition == 0L) break block10;
            if (targetPosition <= 0L) ** GOTO lbl26
            while (targetPosition != 0L) {
                this.buffer[this.buffered++] = (byte)(targetPosition & 255L);
                targetPosition >>>= 8;
            }
            if ((this.buffer[this.buffered - 1] & 128) == 0) break block10;
            this.buffer[this.buffered++] = 0;
            break block10;
lbl-1000:
            // 1 sources

            {
                this.buffer[this.buffered++] = (byte)(targetPosition & 255L);
                targetPosition >>= 8;
lbl26:
                // 2 sources

                ** while (targetPosition != -1L)
            }
lbl27:
            // 1 sources

            if ((this.buffer[this.buffered - 1] & 128) == 0) {
                this.buffer[this.buffered++] = -1;
            }
        }
        v0 = begin + 1;
        this.buffer[v0] = (byte)(this.buffer[v0] | (byte)(this.buffered - begin - 2));
        return true;
    }

    @Override
    public boolean attachLabel(String style) {
        if (this.legend != null) {
            int styleId = this.legend.valOfEnum(3, style);
            if (styleId == -1) {
                styleId = this.legend.addEnum(3, style);
            }
            return this.attachLabel(styleId);
        }
        return false;
    }

    @Override
    public boolean attachLabel(int styleId) {
        int begin = this.beginAttach(16);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = 40;
        this.buffer[this.buffered++] = 0;
        this.buffered += SamplesWriter.plusWrite(this.buffer, this.buffered, styleId);
        this.buffered += SamplesWriter.plusWrite(this.buffer, this.buffered, 0);
        this.buffered += SamplesWriter.plusWrite(this.buffer, this.buffered, 0);
        int n = begin + 1;
        this.buffer[n] = (byte)(this.buffer[n] | (byte)(this.buffered - begin - 2));
        return true;
    }
}

