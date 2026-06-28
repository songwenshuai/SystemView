/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.ILogicSamplesWriter;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.samples.producer.AbstractUpdatableSamplesProducer;
import de.toem.impulse.values.Logic;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.source.PropertySource;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import java.util.List;

public class LogicCombine
extends AbstractUpdatableSamplesProducer {
    private boolean swap;
    private boolean invert;
    private boolean keepTags;
    private boolean ignoreNone;
    private boolean hideIdentical;
    private int[] inputDimension;
    private int[] dimensionOffset;
    private int[] sourceIndex;
    int dimension;
    private byte[] value;
    private boolean[] tags;
    private int tagged;
    private boolean inital;

    public LogicCombine() {
    }

    public LogicCombine(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public static IPropertyModel getPropertyModel() {
        return new PropertyModel(){

            @Override
            public IControlProvider getControls() {
                return new AbstractControlProvider(){

                    @Override
                    protected boolean fillThis() {
                        this.tlk().addButton(this.container(), new CheckController(this.editor(), new PropertySource("swap")), this.cols(), 2048, I18n.Producer_LogicExtract_SwapBits, null);
                        this.tlk().addButton(this.container(), new CheckController(this.editor(), new PropertySource("invert")), this.cols(), 2048, I18n.Producer_LogicExtract_InvertBits, null);
                        return true;
                    }
                };
            }
        }.add("swap", false, I18n.Producer_LogicExtract_Swap, null).add("invert", false, I18n.Producer_LogicExtract_InvertBits, null).add("keepTags", true, I18n.Producer_KeepTags, I18n.Producer_KeepTagsComment).add("ignoreNone", true, I18n.Producer_IgnoreNone, I18n.Producer_IgnoreNoneComment).add("hideIdentical", true, I18n.Producer_HideIdentical, I18n.Producer_HideIdenticalComment);
    }

    @Override
    public void init(String sstart, String send, String srate, IDomainBaseProvider readerBaseProvider) {
        this.swap = this.parameters.getTyped("swap", Boolean.class);
        this.invert = this.parameters.getTyped("invert", Boolean.class);
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
        int bitWidth = 0;
        for (IReadableSamples readable : this.sources) {
            if (readable == null || ISamples.SignalType.Logic != readable.getSignalType() || readable.getSignalDescriptor() == null) continue;
            bitWidth += readable.getSignalDescriptor().getScale();
        }
        if ((this.flags & 4) == 0) {
            this.signalDescriptor = ISamples.SignalDescriptor.LogicWidth(bitWidth);
        }
        super.modifyInit();
    }

    @Override
    protected boolean instatiate(IProgress p) {
        this.targetWriter = PackedSamples.createWriter(this.processType, ISamples.SignalType.Logic, this.signalDescriptor, this.productionBase);
        if (this.targetWriter == null) {
            return false;
        }
        this.targetWriter.open(this.start, (this.flags & 0x20) != 0 ? this.end : this.start, this.rate, 0, 0, null);
        int bitWidth = 0;
        for (IReadableSamples readable : this.sources) {
            if (readable == null || readable.getSignalType() != ISamples.SignalType.Logic || readable.getSignalDescriptor() == null) continue;
            bitWidth += readable.getSignalDescriptor().getScale();
        }
        this.dimension = bitWidth;
        ISamplePointer[] input = new ISamplePointer[this.sources.size()];
        this.inputDimension = new int[this.sources.size()];
        this.dimensionOffset = new int[this.sources.size()];
        this.sourceIndex = new int[this.sources.size()];
        bitWidth = 0;
        int n = 0;
        while (n < this.sources.size()) {
            IReadableSamples source = (IReadableSamples)this.sources.get(n);
            this.sourceIndex[n] = -1;
            this.dimensionOffset[n] = bitWidth;
            if (source != null) {
                input[n] = new SamplePointer(source);
                if (source.getSignalType() == ISamples.SignalType.Logic && source.getSignalDescriptor() != null) {
                    int bits;
                    this.inputDimension[n] = bits = source.getSignalDescriptor().getScale();
                    bitWidth += bits;
                }
            }
            ++n;
        }
        this.value = new byte[this.dimension];
        this.tags = new boolean[this.sources.size()];
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
            boolean changed = false;
            int n = 0;
            while (n < input.length) {
                ISamplePointer pointer = input[n];
                if (pointer != null && pointer.getIndex() > this.sourceIndex[n]) {
                    this.sourceIndex[n] = pointer.getIndex();
                    boolean isNone = pointer.isNone();
                    if (!this.ignoreNone || !isNone) {
                        boolean tag;
                        Logic lv = pointer.logicValue();
                        if (this.invert) {
                            lv = lv.invert();
                        }
                        if (this.swap) {
                            lv = lv.swap();
                        }
                        int dim = this.inputDimension[n];
                        int offset = this.dimensionOffset[n];
                        int b = 0;
                        while (b < dim) {
                            byte v = lv.getState(b);
                            changed |= this.hideIdentical && v != this.value[this.dimension - 1 - (offset + b)];
                            this.value[this.dimension - 1 - (offset + b)] = v;
                            ++b;
                        }
                        boolean bl = tag = this.keepTags && pointer.isTagged();
                        if (this.keepTags) {
                            boolean total;
                            boolean bl2 = total = this.tagged > 0;
                            if (tag != this.tags[n]) {
                                this.tags[n] = tag;
                                int n2 = this.tagged = tag ? this.tagged + 1 : this.tagged - 1;
                            }
                            changed |= this.hideIdentical && total != this.tagged > 0;
                        }
                    }
                }
                ++n;
            }
            if (this.hideIdentical && !changed && this.inital) continue;
            ((ILogicSamplesWriter)this.targetWriter).write(current, this.tagged > 0, (byte)0, this.value, 0, this.dimension);
            this.inital = false;
        }
        return true;
    }

    @Override
    protected void destroy() {
        super.destroy();
        this.sourceIndex = null;
    }
}

