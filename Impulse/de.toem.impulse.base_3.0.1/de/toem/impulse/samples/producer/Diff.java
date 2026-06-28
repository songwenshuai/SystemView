/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.samples.producer.AbstractUpdatableSamplesProducer;
import de.toem.impulse.values.CompoundPack;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.source.PropertySource;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import java.util.List;

public class Diff
extends AbstractUpdatableSamplesProducer {
    private boolean ignoreMore;
    private boolean ignoreLess;
    boolean different;

    public Diff() {
    }

    public Diff(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public static IPropertyModel getPropertyModel() {
        return new PropertyModel(){

            @Override
            public IControlProvider getControls() {
                return new AbstractControlProvider(){

                    @Override
                    protected boolean fillThis() {
                        this.tlk().addButton(this.container(), new CheckController(this.editor(), new PropertySource("ignoreMore")), this.cols(), 2048, I18n.Producer_Diff_IgnoreLonger, null);
                        this.tlk().addButton(this.container(), new CheckController(this.editor(), new PropertySource("ignoreLess")), this.cols(), 2048, I18n.Producer_Diff_IgnoreShorter, null);
                        return true;
                    }
                };
            }
        }.add("ignoreMore", false, I18n.Producer_Diff_IgnoreLonger, null).add("ignoreLess", false, I18n.Producer_Diff_IgnoreShorter, null);
    }

    @Override
    public void init(String sstart, String send, String srate, IDomainBaseProvider readerBaseProvider) {
        this.ignoreMore = Boolean.TRUE.toString().equals(this.parameters.get("ignoreMore"));
        this.ignoreLess = Boolean.TRUE.toString().equals(this.parameters.get("ignoreLess"));
        int noOfInputs = 0;
        for (IReadableSamples readable : this.sources) {
            if (readable == null) continue;
            ++noOfInputs;
        }
        if (noOfInputs != 2) {
            this.setError(I18n.General_NoInput);
        }
        super.init(sstart, send, srate, readerBaseProvider);
    }

    @Override
    protected boolean instatiate(IProgress p) {
        this.targetWriter = PackedSamples.createWriter(this.processType, this.signalType, this.signalDescriptor, this.productionBase);
        if (this.targetWriter == null) {
            return false;
        }
        this.targetWriter.open(this.start, (this.flags & 0x20) != 0 ? this.end : this.start, this.rate, 0, 0, null);
        ISamplePointer[] input = new ISamplePointer[2];
        if (this.sources == null) {
            return false;
        }
        int noOfInputs = 0;
        int n = 0;
        while (n < this.sources.size()) {
            IReadableSamples source = (IReadableSamples)this.sources.get(n);
            if (source != null) {
                input[noOfInputs++] = new SamplePointer(source);
            }
            ++n;
        }
        if (input[0] == null || input[1] == null) {
            return false;
        }
        this.iter = new SamplesIterator(input);
        this.setReference(PackedSamples.createReader(this.targetWriter, this.readerBase));
        return this.reference != null;
    }

    @Override
    protected boolean execute(IProgress p) {
        ISamplePointer[] input = this.iter.pointers();
        this.targetWriter.setLegend(input[0].getLegend());
        this.targetWriter.setTagDomain(ISamples.TagDomain.Diff);
        while (this.iter.hasNext() && !p.isCanceled()) {
            boolean differentPos;
            long current = this.iter.next();
            if (current >= input[0].getEndUnits() && this.ignoreMore || current >= input[1].getEndUnits() && this.ignoreLess) break;
            CompoundPack packa = input[0].packed();
            CompoundPack packb = input[1].packed();
            if (packa == null && packb == null) continue;
            boolean differentValue = packa == null && packb != null || packa != null && packb == null || packa != null && packb != null && !packa.equalValues(packb);
            boolean bl = differentPos = packa != null && packb != null && packa.getUnits() != packb.getUnits();
            if (!this.different && !differentValue && differentPos) {
                packa.setUnits(current);
                packa.setTag(true);
                this.targetWriter.writeSample(packa);
                continue;
            }
            if (differentValue) {
                if (!this.different) {
                    this.targetWriter.writeSample(current, (byte)1);
                }
                this.different = true;
                continue;
            }
            packa.setUnits(current);
            packa.setTag(false);
            this.targetWriter.writeSample(packa);
            this.different = false;
        }
        return true;
    }
}

