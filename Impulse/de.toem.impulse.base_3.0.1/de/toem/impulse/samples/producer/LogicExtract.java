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
import de.toem.impulse.samples.producer.AbstractUpdatableMasterSamplesProducer;
import de.toem.impulse.values.Logic;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.scan.TextScanResult;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.controller.source.PropertySource;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import java.util.ArrayList;
import java.util.List;

public class LogicExtract
extends AbstractUpdatableMasterSamplesProducer {
    private boolean extract;
    private int bit;
    private int count;
    private boolean swap;
    private boolean invert;
    private boolean keepTags;
    private boolean ignoreNone;
    private boolean hideIdentical;
    private IReadableSamples source;
    private int bits;
    private boolean previousTag;
    private Logic previousValue;

    public LogicExtract() {
    }

    public LogicExtract(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, 0, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public LogicExtract(IReadableSamples source) {
        super(source.getId(), source.getName(), 2, source, ISamples.ProcessType.Unknown, ISamples.SignalType.Unknown, ISamples.SignalDescriptor.DEFAULT, source.getDomainBase(), null, null, null, null, null, null, source.getDomainBase());
    }

    public static IPropertyModel getPropertyModel() {
        return new PropertyModel(){

            @Override
            public IControlProvider getControls() {
                return new AbstractControlProvider(){

                    @Override
                    protected boolean fillThis() {
                        this.tlk().addButton(this.container(), new CheckController(this.editor(), new PropertySource("extract")), 1, 2048, I18n.Producer_Extract, null);
                        ITlkComposite sub = this.tlk().addComposite(this.container(), null, 4, this.cols() - 1, 0, null, null);
                        this.tlk().addText(sub, new TextController(this.editor(), new PropertySource("bit")){

                            @Override
                            protected TextScanResult doCheck(String formatted, int options) {
                                int bit = Utils.parseInt(formatted, -1);
                                if (bit >= 0) {
                                    return TextScanResult.SCAN_OK;
                                }
                                return TextScanResult.SCAN_ERROR;
                            }
                        }, this.tlk().ld(1, 4, 40), 0x100001, I18n.Producer_LogicExtract_From_);
                        this.tlk().addText(sub, new TextController(this.editor(), new PropertySource("count")){

                            @Override
                            protected TextScanResult doCheck(String formatted, int options) {
                                int count = Utils.parseInt(formatted, -1);
                                if (count > 0) {
                                    return TextScanResult.SCAN_OK;
                                }
                                return TextScanResult.SCAN_ERROR;
                            }
                        }, this.tlk().ld(1, 4, 40), 0x100001, I18n.Producer_LogicExtract_Count_);
                        this.tlk().addButton(this.container(), new CheckController(this.editor(), new PropertySource("swap")), this.cols(), 2048, I18n.Producer_LogicExtract_SwapBits, null);
                        this.tlk().addButton(this.container(), new CheckController(this.editor(), new PropertySource("invert")), this.cols(), 2048, I18n.Producer_LogicExtract_InvertBits, null);
                        return true;
                    }
                };
            }
        }.add("extract", true, I18n.Producer_Extract, null).add("bit", 0, I18n.Producer_LogicExtract_BitPos, null).add("count", 1, I18n.Producer_LogicExtract_Count, null).add("swap", false, I18n.Producer_LogicExtract_Swap, null).add("invert", false, I18n.Producer_LogicExtract_InvertBits, null).add("keepTags", true, I18n.Producer_KeepTags, I18n.Producer_KeepTagsComment).add("ignoreNone", true, I18n.Producer_IgnoreNone, I18n.Producer_IgnoreNoneComment).add("hideIdentical", true, I18n.Producer_HideIdentical, I18n.Producer_HideIdenticalComment);
    }

    @Override
    protected IReadableSamples loopThroughSource() {
        return this.source;
    }

    @Override
    protected void init(String sstart, String send, String srate, IDomainBaseProvider readerBaseProvider) {
        int noOfSources = 0;
        int n = 0;
        while (n < this.sources.size()) {
            IReadableSamples source = (IReadableSamples)this.sources.get(n);
            if (source != null) {
                ++noOfSources;
                this.source = source;
            }
            ++n;
        }
        if (noOfSources == 0) {
            this.setError(I18n.General_NoInput);
        }
        if (noOfSources != 1) {
            this.setError(I18n.General_InvalidNoOfInput);
        }
        if (this.parameters != null) {
            this.extract = this.parameters.getTyped("extract", Boolean.class);
            this.bit = this.parameters.getTyped("bit", Integer.class);
            this.count = this.parameters.getTyped("count", Integer.class);
            this.swap = this.parameters.getTyped("swap", Boolean.class);
            this.invert = this.parameters.getTyped("invert", Boolean.class);
            this.keepTags = this.parameters.getTyped("keepTags", Boolean.class);
            this.ignoreNone = this.parameters.getTyped("ignoreNone", Boolean.class);
            this.hideIdentical = this.parameters.getTyped("hideIdentical", Boolean.class);
        } else {
            this.extract = true;
            this.bit = 0;
            this.count = 1;
            this.swap = false;
            this.invert = false;
            this.keepTags = false;
            this.ignoreNone = false;
            this.hideIdentical = false;
        }
        if (this.mode == 2 && this.source != null) {
            this.bits = this.source.getScale();
            n = 0;
            while (n < this.bits) {
                new LogicExtractSlaveProduction(n);
                ++n;
            }
        }
        super.init(sstart, send, srate, readerBaseProvider);
    }

    @Override
    public void modifyInit() {
        super.modifyInit();
        if (this.mode == 0 && (this.flags & 4) == 0 && this.extract) {
            this.signalDescriptor = ISamples.SignalDescriptor.LogicWidth(this.count);
        }
    }

    @Override
    protected void updateInit() {
        this.bits = this.source.getScale();
        int n = 0;
        while (n < this.bits) {
            if (this.slaveProductions.size() <= n) {
                new LogicExtractSlaveProduction(n);
            }
            ++n;
        }
    }

    @Override
    public List<String> getChildSlaveIds(String slaveId) {
        ArrayList<String> list = new ArrayList<String>();
        if (this.mode != 0 && slaveId == null && this.slaveProductions != null && this.idMap != null) {
            for (AbstractUpdatableMasterSamplesProducer.AbstractSlaveProduction c : this.slaveProductions) {
                list.add(c.slaveId);
            }
        }
        return list;
    }

    @Override
    protected boolean instatiate(IProgress p) {
        ISamplePointer[] pointer = new ISamplePointer[]{new SamplePointer(this.source)};
        if (this.mode == 0) {
            this.targetWriter = PackedSamples.createWriter(this.processType, this.signalType, this.signalDescriptor, this.productionBase);
            if (this.targetWriter == null) {
                return false;
            }
            this.targetWriter.open(this.start, (this.flags & 0x20) != 0 ? this.end : this.start, this.rate, 0, 0, null);
            this.iter = new SamplesIterator(this.targetWriter, pointer);
            this.setReference(PackedSamples.createReader(this.targetWriter, this.readerBase));
            return this.reference != null;
        }
        if (this.mode == 2) {
            for (AbstractUpdatableMasterSamplesProducer.AbstractSlaveProduction slave : this.slaveProductions) {
                slave.instantiate();
            }
            this.iter = new SamplesIterator(pointer);
            return true;
        }
        return false;
    }

    @Override
    protected boolean reInstatiate(IProgress p) {
        boolean result = super.reInstatiate(p);
        for (AbstractUpdatableMasterSamplesProducer.AbstractSlaveProduction slave : this.slaveProductions) {
            if (slave.isInstantiated()) continue;
            slave.instantiate();
        }
        return result;
    }

    @Override
    protected boolean execute(IProgress p) {
        block11: {
            ISamplePointer[] input;
            block10: {
                input = this.iter.pointers();
                if (this.mode != 0) break block10;
                ILogicSamplesWriter targetWriter = (ILogicSamplesWriter)this.targetWriter;
                while (this.iter.hasNext() && !p.isCanceled()) {
                    boolean tag;
                    long current = this.iter.next(targetWriter);
                    Logic value = input[0].logicValue();
                    boolean isNone = input[0].isNone();
                    if (this.ignoreNone && isNone || !isNone && value == null) continue;
                    boolean bl = tag = this.keepTags && input[0].isTagged();
                    if (value != null) {
                        if (this.extract) {
                            value = value.extract(this.bit, this.count);
                        }
                        if (this.invert) {
                            value = value.invert();
                        }
                        if (this.swap) {
                            value = value.swap();
                        }
                    }
                    if (this.hideIdentical && Utils.equals(value, this.previousValue) && this.previousTag == tag) continue;
                    if (isNone) {
                        targetWriter.writeNone(current, tag);
                    } else {
                        targetWriter.writeSample(current, tag, (Object)value);
                    }
                    this.previousTag = tag;
                    this.previousValue = value;
                }
                break block11;
            }
            if (this.mode != 2) break block11;
            while (this.iter.hasNext() && !p.isCanceled()) {
                boolean tag;
                long current = this.iter.next();
                Logic value = input[0].logicValue();
                boolean isNone = input[0].isNone();
                if (this.ignoreNone && isNone || !isNone && value == null) continue;
                boolean bl = tag = this.keepTags && input[0].isTagged();
                if (this.hideIdentical && tag == this.previousTag && Utils.equals(value, this.previousValue)) continue;
                for (AbstractUpdatableMasterSamplesProducer.AbstractSlaveProduction slave : this.slaveProductions) {
                    ILogicSamplesWriter targetWriter = (ILogicSamplesWriter)((AbstractUpdatableMasterSamplesProducer.AbstractWritableSlaveProduction)slave).targetWriter;
                    if (targetWriter == null) continue;
                    int bit = ((LogicExtractSlaveProduction)slave).bit;
                    if (isNone) {
                        targetWriter.write(current, tag);
                        continue;
                    }
                    byte nextState = value.getState(bit);
                    if (this.hideIdentical && this.previousValue != null && this.previousValue.getState(bit) == nextState && this.previousTag == tag) continue;
                    targetWriter.write(current, tag, nextState);
                }
                this.previousTag = tag;
                this.previousValue = value;
            }
        }
        return true;
    }

    class LogicExtractSlaveProduction
    extends AbstractUpdatableMasterSamplesProducer.AbstractWritableSlaveProduction {
        int bit;

        public LogicExtractSlaveProduction(int bit) {
            super(String.valueOf(bit), null, "." + String.valueOf(bit), ISamples.SignalType.Logic, ISamples.SignalDescriptor.DEFAULT);
            this.bit = bit;
        }
    }
}

