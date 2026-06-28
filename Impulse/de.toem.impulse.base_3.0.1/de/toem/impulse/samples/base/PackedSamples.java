/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.base;

import de.toem.impulse.cells.record.PortScope;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IBinarySamplesWriter;
import de.toem.impulse.samples.IEventSamplesWriter;
import de.toem.impulse.samples.IFloatSamplesWriter;
import de.toem.impulse.samples.IIntegerSamplesWriter;
import de.toem.impulse.samples.ILogicSamplesWriter;
import de.toem.impulse.samples.IPackedSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesLegend;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.IStructSamplesWriter;
import de.toem.impulse.samples.ITextSamplesWriter;
import de.toem.impulse.samples.base.Samples;
import de.toem.impulse.samples.legend.SamplesLegend;
import de.toem.impulse.samples.reader.SamplesReader;
import de.toem.impulse.samples.writer.BinarySamplesWriter;
import de.toem.impulse.samples.writer.ConvergingLogicSamplesWriter;
import de.toem.impulse.samples.writer.EventSamplesWriter;
import de.toem.impulse.samples.writer.FloatSamplesWriter;
import de.toem.impulse.samples.writer.IntegerSamplesWriter;
import de.toem.impulse.samples.writer.LogicSamplesWriter;
import de.toem.impulse.samples.writer.SamplesWriter;
import de.toem.impulse.samples.writer.StructSamplesWriter;
import de.toem.impulse.samples.writer.TextSamplesWriter;
import de.toem.toolkits.pattern.element.exploits.Markers;
import de.toem.toolkits.pattern.pageable.Pageable;

public abstract class PackedSamples
extends Samples
implements IPackedSamples {
    protected SamplesLegend legend;
    protected Markers markers;
    protected Pageable<byte[]> samples;
    protected IDomainBase sampleDomainBase;
    protected int flags;

    public static ISamplesReader createReader(Signal signal) {
        return PackedSamples.createReader(signal, null);
    }

    public static ISamplesReader createReader(Signal signal, IDomainBase domainBase) {
        return new SamplesReader(signal, domainBase);
    }

    public static ISamplesReader createReader(IPackedSamples packed) {
        return PackedSamples.createReader(packed, null);
    }

    public static ISamplesReader createReader(IPackedSamples packed, IDomainBase domainBase) {
        return new SamplesReader(packed, domainBase);
    }

    public static ISamplesWriter createWriter(Signal signal, boolean converging) {
        ISamplesWriter writer = PackedSamples.createWriter(signal.getPath(), signal.getName(), ISamples.ProcessType.valueOf(signal), ISamples.SignalType.valueOf(signal), ISamples.SignalDescriptor.valueOf(signal), DomainBase.valueOf(signal), converging);
        return writer;
    }

    public static ISamplesWriter createWriter(ISamples.ProcessType processType, ISamples.SignalType samplesType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        return PackedSamples.createWriter(processType, samplesType, signalDescriptor, domainBase, false);
    }

    public static ISamplesWriter createWriter(ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, boolean converging) {
        return PackedSamples.createWriter(null, null, processType, signalType, signalDescriptor, domainBase, converging);
    }

    public static ISamplesWriter createWriter(String id, String name, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, boolean converging) {
        switch (signalType) {
            case Logic: {
                if (converging) {
                    return new ConvergingLogicSamplesWriter(id, name, processType, signalDescriptor, domainBase);
                }
                return new LogicSamplesWriter(id, name, processType, signalType, signalDescriptor, domainBase);
            }
            case Float: 
            case FloatArray: {
                return new FloatSamplesWriter(id, name, processType, signalType, signalDescriptor, domainBase);
            }
            case Integer: 
            case IntegerArray: {
                return new IntegerSamplesWriter(id, name, processType, signalType, signalDescriptor, domainBase);
            }
            case Struct: {
                return new StructSamplesWriter(id, name, processType, signalType, signalDescriptor, domainBase);
            }
            case Event: 
            case EventArray: {
                return new EventSamplesWriter(id, name, processType, signalType, signalDescriptor, domainBase);
            }
            case Text: 
            case TextArray: {
                return new TextSamplesWriter(id, name, processType, signalType, signalDescriptor, domainBase);
            }
            case Binary: {
                return new BinarySamplesWriter(id, name, processType, signalType, signalDescriptor, domainBase);
            }
        }
        return new SamplesWriter(id, name, processType, signalType, signalDescriptor, domainBase);
    }

    public static Class<? extends ISamplesWriter> getWriterInterface(ISamples.SignalType signalType) {
        switch (signalType) {
            case Logic: {
                return ILogicSamplesWriter.class;
            }
            case Float: 
            case FloatArray: {
                return IFloatSamplesWriter.class;
            }
            case Integer: 
            case IntegerArray: {
                return IIntegerSamplesWriter.class;
            }
            case Struct: {
                return IStructSamplesWriter.class;
            }
            case Event: 
            case EventArray: {
                return IEventSamplesWriter.class;
            }
            case Text: 
            case TextArray: {
                return ITextSamplesWriter.class;
            }
            case Binary: {
                return IBinarySamplesWriter.class;
            }
        }
        return ISamplesWriter.class;
    }

    public PackedSamples(String id, String name, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        this(id, name, processType, signalType, signalDescriptor, domainBase, 0L, 0L, 0L);
    }

    public PackedSamples(String id, String name, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, long start, long end, long rate) {
        this.id = id;
        this.name = name;
        this.processType = processType != null ? processType : ISamples.ProcessType.Unknown;
        this.signalType = signalType != null ? signalType : ISamples.SignalType.Unknown;
        this.signalDescriptor = signalDescriptor != null ? signalDescriptor : ISamples.SignalDescriptor.DEFAULT;
        this.domainBase = domainBase != null ? domainBase : DomainBase.Unknown;
        this.sampleDomainBase = this.domainBase;
        this.start = start;
        this.end = end;
        this.rate = rate;
        this.legend = SamplesLegend.createLegend(signalType, signalDescriptor);
        this.tagDomain = ISamples.TagDomain.Unknown;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public PackedSamples(Signal signal, IDomainBase domainBase) {
        Signal signal2 = signal;
        synchronized (signal2) {
            this.name = signal.getName();
            this.id = signal.getPath();
            this.processType = ISamples.ProcessType.valueOf(signal);
            this.signalType = ISamples.SignalType.valueOf(signal);
            this.signalDescriptor = ISamples.SignalDescriptor.valueOf(signal);
            this.tagDomain = ISamples.TagDomain.valueOf(signal);
            this.tagged = signal.tag;
            this.samples = signal.samples;
            if (this.samples != null) {
                this.samples.prepareGeneration();
            }
            this.sampleDomainBase = this.domainBase = DomainBase.parse(signal.domainBase);
            this.start = signal.start;
            this.end = signal.end;
            this.rate = signal.rate;
            if (domainBase != null) {
                this.domainBase = domainBase;
                this.start = this.sampleDomainBase.convertTo(this.domainBase, signal.start);
                this.end = this.sampleDomainBase.convertTo(this.domainBase, signal.end);
                this.rate = this.sampleDomainBase.convertTo(this.domainBase, signal.rate);
                PortScope pscope = (PortScope)signal.getParent(PortScope.class);
                if (pscope != null) {
                    long offset = pscope.getSamplesOffset(domainBase);
                    this.start += offset;
                    this.end += offset;
                }
            }
            this.legend = SamplesLegend.createLegend(signal);
            this.markers = signal.markers != null ? new Markers().setValue(signal.markers) : null;
        }
    }

    public PackedSamples(IPackedSamples packed, IDomainBase domainBase) {
        this.name = packed.getName();
        this.id = packed.getId();
        this.error = packed.getError();
        this.processType = packed.getProcessType();
        this.signalType = packed.getSignalType();
        this.signalDescriptor = packed.getSignalDescriptor();
        this.samples = packed.getSamples();
        if (this.samples != null) {
            this.samples.prepareGeneration();
        }
        this.sampleDomainBase = this.domainBase = packed.getDomainBase();
        this.start = packed.getStartUnits();
        this.end = packed.getEndUnits();
        this.rate = packed.getRateUnits();
        if (domainBase != null) {
            this.domainBase = domainBase;
            this.start = this.sampleDomainBase.convertTo(this.domainBase, packed.getStartUnits());
            this.end = this.sampleDomainBase.convertTo(this.domainBase, packed.getEndUnits());
            this.rate = this.sampleDomainBase.convertTo(this.domainBase, packed.getRateUnits());
        }
        this.tagged = packed.hasTag();
        this.tagDomain = packed.getTagDomain();
        this.legend = (SamplesLegend)(packed.getLegend() instanceof SamplesLegend ? packed.getLegend() : null);
        this.markers = packed.getMarkers() != null ? packed.getMarkers().clone() : null;
    }

    public PackedSamples() {
    }

    public static int plusLength(int val) {
        int len = 0;
        while (len == 0 || val != 0) {
            val >>>= 7;
            ++len;
        }
        return len;
    }

    public static int plusWrite(byte[] buffer, int pos, int val) {
        int len = 1;
        while (true) {
            if (val <= 127) {
                buffer[pos++] = (byte)(val & 0x7F);
                return len;
            }
            buffer[pos++] = (byte)(val & 0x7F | 0x80);
            val >>>= 7;
            ++len;
        }
    }

    public static int[] plusRead(byte[] buffer, int pos) {
        int[] nArray = new int[2];
        nArray[1] = 1;
        int[] result = nArray;
        int shift = 0;
        while (true) {
            byte sn = buffer[pos++];
            result[0] = result[0] | (sn & 0x7F) << shift;
            shift += 7;
            if ((sn & 0x80) == 0) break;
            result[1] = result[1] + 1;
        }
        return result;
    }

    public static void plusRead(int[] result, byte[] buffer, int pos) {
        int shift = 0;
        result[0] = 0;
        result[1] = 1;
        while (true) {
            byte sn = buffer[pos++];
            result[0] = result[0] | (sn & 0x7F) << shift;
            shift += 7;
            if ((sn & 0x80) == 0) break;
            result[1] = result[1] + 1;
        }
    }

    @Override
    public final IDomainBase getSamplesDomainBase() {
        return this.sampleDomainBase;
    }

    @Override
    public final Pageable<byte[]> getSamples() {
        return this.samples;
    }

    @Override
    public final ISamplesLegend getLegend() {
        return this.legend;
    }

    @Override
    public Markers getMarkers() {
        return this.markers;
    }
}

