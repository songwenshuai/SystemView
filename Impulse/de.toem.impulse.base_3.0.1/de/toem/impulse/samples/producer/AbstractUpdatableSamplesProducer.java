/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.samples.producer.AbstractSamplesProducer;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.List;

public abstract class AbstractUpdatableSamplesProducer
extends AbstractSamplesProducer {
    protected boolean instantiated;
    protected SamplesIterator iter;
    protected ISamplesWriter targetWriter;

    public AbstractUpdatableSamplesProducer() {
    }

    public AbstractUpdatableSamplesProducer(String id, String name, int mode, Object sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, mode, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public AbstractUpdatableSamplesProducer(String id, String name, Object sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, 0, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    @Override
    public void produce(IProgress p, List<IReadableSamples> sourceList) {
        if (!this.isInstantiated()) {
            if (!this.instatiate(p) || this.iter == null || this.reference == null) {
                this.setError(String.valueOf(I18n.Producer_CouldNotBeInstatiated) + (this.getError() != null ? "->" + this.getError() : ""));
                return;
            }
            this.instantiated = true;
        } else {
            if (!this.isProducing()) {
                this.setError(I18n.Producer_CouldNotBeExecuted);
                return;
            }
            if (!this.reInstatiate(p)) {
                this.setError(I18n.Producer_CouldNotBeExecuted);
                return;
            }
        }
        if (!this.execute(p)) {
            this.setError(String.valueOf(I18n.Producer_CouldNotBeExecuted) + (this.getError() != null ? "->" + this.getError() : ""));
        }
        this.flushOrClose(this.continueProducing());
        if (this.updateReader()) {
            ++this.productionRelease;
        }
        if (!this.continueProducing()) {
            this.destroy();
        }
    }

    protected boolean isInstantiated() {
        return this.instantiated;
    }

    protected boolean isProducing() {
        return this.iter != null;
    }

    protected boolean continueProducing() {
        return this.volatileSources;
    }

    protected boolean continueExecution() {
        return true;
    }

    protected void flushOrClose(boolean continueProducing) {
        if (this.targetWriter != null) {
            if (continueProducing) {
                this.targetWriter.flush(this.iter.current(this.targetWriter));
            } else {
                this.targetWriter.close(this.end);
            }
        }
    }

    protected boolean updateReader() {
        if (this.targetWriter != null && ((ISamplesReader)this.reference).update(this.targetWriter, this.readerBase) < 0) {
            this.setReference(PackedSamples.createReader(this.targetWriter, this.readerBase));
            if (this.reference == null) {
                this.setError(I18n.Producer_CouldNotBeRead);
            }
            return true;
        }
        return false;
    }

    protected void destroy() {
        ++this.productionRelease;
        this.targetWriter = null;
        this.iter = null;
    }

    protected abstract boolean instatiate(IProgress var1);

    protected boolean reInstatiate(IProgress p) {
        this.iter.update();
        return true;
    }

    protected abstract boolean execute(IProgress var1);

    @Override
    public final boolean isVolatile() {
        return !this.hasReference() || this.isProducing();
    }

    @Override
    public boolean isReleased() {
        return this.hasReference();
    }
}

