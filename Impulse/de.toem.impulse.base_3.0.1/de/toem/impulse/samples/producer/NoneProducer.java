/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.producer.AbstractUpdatableSamplesProducer;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.List;

public class NoneProducer
extends AbstractUpdatableSamplesProducer {
    public static final int UNKNOWN_PRODUCER = 1;
    public static final int LICENSE_LOCK = 2;
    private int type;

    public NoneProducer() {
    }

    public NoneProducer(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider, int type) {
        super(id, name, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
        this.type = type;
    }

    @Override
    public void init(String sstart, String send, String srate, IDomainBaseProvider readerBaseProvider) {
        this.setError(I18n.General_NoLicense);
        super.init(sstart, send, srate, readerBaseProvider);
    }

    @Override
    protected boolean instatiate(IProgress p) {
        return this.reference != null;
    }

    @Override
    protected boolean execute(IProgress p) {
        return true;
    }

    public int getType() {
        return this.type;
    }
}

