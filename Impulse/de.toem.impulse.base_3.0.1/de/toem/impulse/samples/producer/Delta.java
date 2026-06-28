/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.INumberSamplesWriter;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.samples.producer.AbstractUpdatableSamplesProducer;
import de.toem.impulse.values.CompoundValue;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.List;

public class Delta
extends AbstractUpdatableSamplesProducer {
    public Delta() {
    }

    public Delta(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public static IPropertyModel getPropertyModel() {
        return new PropertyModel();
    }

    @Override
    public void init(String sstart, String send, String srate, IDomainBaseProvider readerBaseProvider) {
        int modified = 0;
        int noOfInputs = 0;
        this.signalType = ISamples.SignalType.Integer;
        for (IReadableSamples readable : this.sources) {
            if (readable == null) continue;
            ++noOfInputs;
            if (readable.getSignalType() == ISamples.SignalType.Integer) continue;
            this.signalType = ISamples.SignalType.Float;
            modified = 2;
        }
        if (noOfInputs != 2) {
            this.setError(I18n.General_NoInput);
        }
        super.init(sstart, send, srate, readerBaseProvider);
        this.flags &= ~modified;
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
        while (this.iter.hasNext() && !p.isCanceled()) {
            boolean differentPos;
            long current = this.iter.next(this.targetWriter);
            CompoundValue packa = input[0].compound();
            CompoundValue packb = input[1].compound();
            if (packa == null && packb == null) continue;
            boolean differentValue = packa == null && packb != null || packa != null && packb == null || !packa.equalValues(packb);
            boolean bl = differentPos = packa != null && packb != null && packa.getUnits() != packb.getUnits();
            if (!differentValue && differentPos) {
                ((INumberSamplesWriter)this.targetWriter).write(current, false, 0);
                continue;
            }
            if (differentValue) {
                Double delta = (packb != null ? packb.numberValue().doubleValue() : 0.0) - (packa != null ? packa.numberValue().doubleValue() : 0.0);
                ((INumberSamplesWriter)this.targetWriter).write(current, false, delta);
                continue;
            }
            ((INumberSamplesWriter)this.targetWriter).write(current, false, 0);
        }
        return true;
    }
}

