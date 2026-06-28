/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IEventSamplesWriter;
import de.toem.impulse.samples.IFloatSamplesWriter;
import de.toem.impulse.samples.IIntegerSamplesWriter;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ITextSamplesWriter;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.samples.producer.AbstractUpdatableSamplesProducer;
import de.toem.impulse.values.Enumeration;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.List;

public class ArrayCombine
extends AbstractUpdatableSamplesProducer {
    private boolean keepTags;
    private boolean ignoreNone;
    private boolean hideIdentical;
    private int[] sourceIndex;
    int dimension;
    private Object value;
    private boolean[] tags;
    private int tagged;
    private boolean initial;

    public ArrayCombine() {
    }

    public ArrayCombine(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public static IPropertyModel getPropertyModel() {
        return new PropertyModel(){

            @Override
            public IControlProvider getControls() {
                return null;
            }
        }.add("keepTags", true, I18n.Producer_KeepTags, I18n.Producer_KeepTagsComment).add("ignoreNone", true, I18n.Producer_IgnoreNone, I18n.Producer_IgnoreNoneComment).add("hideIdentical", true, I18n.Producer_HideIdentical, I18n.Producer_HideIdenticalComment);
    }

    @Override
    public void init(String sstart, String send, String srate, IDomainBaseProvider readerBaseProvider) {
        this.keepTags = this.parameters.getTyped("keepTags", Boolean.class);
        this.ignoreNone = this.parameters.getTyped("ignoreNone", Boolean.class);
        this.hideIdentical = this.parameters.getTyped("hideIdentical", Boolean.class);
        int noOfInputs = 0;
        int n = 0;
        while (n < this.sources.size()) {
            IReadableSamples source = (IReadableSamples)this.sources.get(n);
            if (source != null) {
                ++noOfInputs;
            }
            ++n;
        }
        if (noOfInputs == 0) {
            this.setError(I18n.General_NoInput);
        }
        super.init(sstart, send, srate, readerBaseProvider);
    }

    @Override
    public void modifyInit() {
        int dim = 0;
        for (IReadableSamples readable : this.sources) {
            if (readable == null) continue;
            ++dim;
        }
        if ((this.flags & 2) == 0) {
            switch (this.signalType) {
                case Integer: {
                    this.signalType = ISamples.SignalType.IntegerArray;
                    break;
                }
                case Float: {
                    this.signalType = ISamples.SignalType.FloatArray;
                    break;
                }
                case Text: {
                    this.signalType = ISamples.SignalType.TextArray;
                    break;
                }
                case Event: {
                    this.signalType = ISamples.SignalType.EventArray;
                    break;
                }
                default: {
                    this.setError(I18n.Producer_ArrayCombiner_InvalidType);
                }
            }
        }
        switch (this.signalType) {
            case EventArray: 
            case IntegerArray: 
            case FloatArray: 
            case TextArray: {
                break;
            }
            default: {
                this.setError(I18n.Producer_ArrayCombiner_InvalidType);
            }
        }
        if ((this.flags & 4) == 0) {
            this.signalDescriptor = new ISamples.SignalDescriptor("default", dim, -1, -1);
        }
        super.modifyInit();
    }

    @Override
    protected boolean instatiate(IProgress p) {
        this.targetWriter = PackedSamples.createWriter(this.processType, this.signalType, this.signalDescriptor, this.productionBase);
        if (this.targetWriter == null) {
            return false;
        }
        this.targetWriter.open(this.start, (this.flags & 0x20) != 0 ? this.end : this.start, this.rate, 0, 0, null);
        int idx = 0;
        for (IReadableSamples readable : this.sources) {
            if (readable == null) continue;
            if (!Utils.isEmpty(readable.getName())) {
                this.targetWriter.setMember(idx, readable.getName(), null, -1);
            }
            ++idx;
        }
        this.dimension = idx;
        ISamplePointer[] input = new ISamplePointer[this.sources.size()];
        this.sourceIndex = new int[this.sources.size()];
        int n = 0;
        while (n < this.sources.size()) {
            IReadableSamples source = (IReadableSamples)this.sources.get(n);
            if (source != null) {
                input[n] = new SamplePointer(source);
            }
            this.sourceIndex[n] = -1;
            ++n;
        }
        switch (this.signalType) {
            case IntegerArray: {
                this.value = new long[this.dimension];
                break;
            }
            case FloatArray: {
                this.value = new double[this.dimension];
                break;
            }
            case TextArray: {
                this.value = new String[this.dimension];
                break;
            }
            case EventArray: {
                this.value = new Enumeration[this.dimension];
            }
        }
        this.tags = new boolean[this.dimension];
        this.tagged = 0;
        this.iter = new SamplesIterator(this.targetWriter, input);
        this.setReference(PackedSamples.createReader(this.targetWriter, this.readerBase));
        return this.reference != null;
    }

    @Override
    protected boolean execute(IProgress p) {
        ISamplePointer[] input = this.iter.pointers();
        while (this.iter.hasNext() && !p.isCanceled()) {
            Long current = this.iter.next(this.targetWriter);
            int idx = 0;
            boolean changed = false;
            int n = 0;
            while (n < input.length) {
                block21: {
                    block22: {
                        boolean tag;
                        ISamplePointer pointer = input[n];
                        if (pointer == null) break block21;
                        if (pointer.getIndex() <= this.sourceIndex[n]) break block22;
                        this.sourceIndex[n] = pointer.getIndex();
                        boolean isNone = pointer.isNone();
                        if (this.ignoreNone && isNone) break block21;
                        if (this.value instanceof long[]) {
                            long v = pointer.longValue();
                            changed |= this.hideIdentical && v != ((long[])this.value)[idx];
                            ((long[])this.value)[idx] = pointer.longValue();
                        } else if (this.value instanceof Enumeration[]) {
                            Enumeration v = pointer.enumValue();
                            changed |= this.hideIdentical && !Utils.equals(v, ((Enumeration[])this.value)[idx]);
                            ((Enumeration[])this.value)[idx] = pointer.enumValue();
                        } else if (this.value instanceof double[]) {
                            double v = pointer.doubleValue();
                            changed |= this.hideIdentical && v != ((double[])this.value)[idx];
                            ((double[])this.value)[idx] = pointer.doubleValue();
                        } else if (this.value instanceof String[]) {
                            String v = pointer.stringValue();
                            changed |= this.hideIdentical && !Utils.equals(v, ((String[])this.value)[idx]);
                            ((String[])this.value)[idx] = pointer.stringValue();
                        }
                        boolean bl = tag = this.keepTags && pointer.isTagged();
                        if (this.keepTags) {
                            boolean total;
                            boolean bl2 = total = this.tagged > 0;
                            if (tag != this.tags[idx]) {
                                this.tags[idx] = tag;
                                int n2 = this.tagged = tag ? this.tagged + 1 : this.tagged - 1;
                            }
                            changed |= this.hideIdentical && total != this.tagged > 0;
                        }
                    }
                    ++idx;
                }
                ++n;
            }
            if (this.hideIdentical && !changed && this.initial) continue;
            if (this.targetWriter instanceof IIntegerSamplesWriter && this.value instanceof long[]) {
                ((IIntegerSamplesWriter)this.targetWriter).write((long)current, this.tagged > 0, (long[])this.value);
            } else if (this.targetWriter instanceof IFloatSamplesWriter && this.value instanceof double[]) {
                ((IFloatSamplesWriter)this.targetWriter).write((long)current, this.tagged > 0, (double[])this.value);
            } else if (this.targetWriter instanceof IEventSamplesWriter && this.value instanceof Enumeration[]) {
                ((IEventSamplesWriter)this.targetWriter).write((long)current, this.tagged > 0, (Enumeration[])this.value);
            } else if (this.targetWriter instanceof ITextSamplesWriter && this.value instanceof String[]) {
                ((ITextSamplesWriter)this.targetWriter).write((long)current, this.tagged > 0, (String[])this.value);
            }
            this.initial = false;
        }
        return true;
    }

    @Override
    protected void destroy() {
        super.destroy();
        this.sourceIndex = null;
    }
}

