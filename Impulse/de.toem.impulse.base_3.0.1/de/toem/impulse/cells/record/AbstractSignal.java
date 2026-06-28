/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.record;

import de.toem.impulse.cells.record.RecordContent;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.paint.IPaintStyle;
import de.toem.impulse.provider.ISamplesCache;
import de.toem.impulse.provider.ISamplesContext;
import de.toem.impulse.provider.ISamplesProvider;
import de.toem.impulse.provider.ISignalContext;
import de.toem.impulse.provider.ISignalProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesCharacteristic;
import de.toem.impulse.samples.ISamplesDisplayInformation;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.toolkits.pattern.provider.IContext;

public class AbstractSignal
extends RecordContent
implements ISignalProvider,
ISamplesProvider,
ISamplesDisplayInformation,
ISamplesCharacteristic {
    @Override
    public Signal getSignal() {
        return null;
    }

    @Override
    public Signal getSignal(IContext context) {
        return this.getSignal();
    }

    @Override
    public IReadableSamples getSamples() {
        return this.getSamples(null);
    }

    @Override
    public IReadableSamples getSamples(IContext context) {
        IReadableSamples previous;
        ISamplesCache cached = context instanceof ISamplesContext ? ((ISamplesContext)context).getSamplesCache() : null;
        IReadableSamples iReadableSamples = previous = cached != null ? cached.get(this) : null;
        if (previous != null) {
            return previous;
        }
        previous = cached != null ? cached.getInvalidated(this) : null;
        Signal signal = this.getSignal((ISignalContext)context);
        if (signal != null) {
            IDomainBase signalBase;
            IDomainBase readerBase = signalBase = DomainBase.valueOf(signal);
            if (signalBase != null && signalBase != DomainBase.Unknown && cached != null && cached.get(signalBase.getClazz()) != null) {
                readerBase = cached.get(signalBase.getClazz());
            }
            if (previous instanceof ISamplesReader) {
                if (previous.getDomainBase() != readerBase) {
                    previous = null;
                }
            } else {
                previous = null;
            }
            IReadableSamples readable = !(previous instanceof ISamplesReader) || ((ISamplesReader)previous).update(signal, readerBase) < 0 ? PackedSamples.createReader(signal, readerBase) : previous;
            if (cached != null) {
                cached.put(this, readable);
            }
            return readable;
        }
        return null;
    }

    @Override
    public String getId() {
        return this.getPath();
    }

    @Override
    public String getLabel() {
        return this.getName();
    }

    @Override
    public String getDescription() {
        return this.description;
    }

    @Override
    public int getValueColumnFormat() {
        return -1;
    }

    @Override
    public Object getColor() {
        return PlotConfiguration.getColor(this);
    }

    @Override
    public IPaintStyle getPaintStyle() {
        return null;
    }

    @Override
    public ISamples.ProcessType getProcessType() {
        return ISamples.ProcessType.valueOf(this.getSignal());
    }

    @Override
    public ISamples.SignalType getSignalType() {
        return ISamples.SignalType.valueOf(this.getSignal());
    }

    @Override
    public ISamples.SignalDescriptor getSignalDescriptor() {
        return ISamples.SignalDescriptor.valueOf(this.getSignal());
    }

    @Override
    public IDomainBase getDomainBase() {
        return DomainBase.valueOf(this.getSignal());
    }
}

