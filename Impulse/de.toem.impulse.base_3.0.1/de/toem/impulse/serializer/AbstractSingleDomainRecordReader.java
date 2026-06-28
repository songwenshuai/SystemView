/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.ISingleDomainRecordGenerator;
import de.toem.impulse.serializer.AbstractParsingRecordReader;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.InputStream;

public abstract class AbstractSingleDomainRecordReader
extends AbstractParsingRecordReader
implements ISingleDomainRecordGenerator {
    private IDomainBase domainBase = DomainBase.Unknown;
    private boolean singleDomain;
    private long start;
    private boolean opened;
    private boolean closed;

    public AbstractSingleDomainRecordReader() {
    }

    public AbstractSingleDomainRecordReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected abstract void parse(IProgress var1, InputStream var2) throws ParseException;

    @Override
    public void initRecord(String name, IDomainBase domainBase) {
        this.initRecord(name);
        this.domainBase = domainBase;
        this.singleDomain = true;
    }

    @Override
    public Signal addSignal(ICell container, String name, String description, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor) {
        Signal signal = this.addSignal(container, name, description, processType, signalType, signalDescriptor, this.domainBase);
        if (this.singleDomain && this.opened && !this.closed) {
            this.getWriter(signal).open(this.start);
        }
        return signal;
    }

    @Override
    public Signal addSignal(ICell container, String name, String description, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, boolean createWriter) {
        Signal signal = super.addSignal(container, name, description, processType, signalType, signalDescriptor, this.singleDomain ? this.domainBase : domainBase, createWriter);
        if (this.singleDomain && this.opened && !this.closed) {
            this.getWriter(signal).open(this.start);
        }
        return signal;
    }

    @Override
    public Signal addSignal(ICell container, String name, String description, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        Signal signal = super.addSignal(container, name, description, processType, signalType, signalDescriptor, this.singleDomain ? this.domainBase : domainBase);
        if (this.singleDomain && this.opened && !this.closed) {
            this.getWriter(signal).open(this.start);
        }
        return signal;
    }

    @Override
    public void open(long units) {
        if (this.singleDomain) {
            this.start = units;
            this.opened = true;
            for (ISamplesWriter writer : this.writers) {
                writer.open(units);
            }
        }
    }

    @Override
    public void open(long units, long rate) {
        if (this.singleDomain) {
            this.start = units;
            this.opened = true;
            for (ISamplesWriter writer : this.writers) {
                writer.open(units, rate, 0, 0, null);
            }
        }
    }

    @Override
    public void open(long units, long rate, int samples256PerFragment, int maxFragments) {
        if (this.singleDomain) {
            this.start = units;
            this.opened = true;
            for (ISamplesWriter writer : this.writers) {
                writer.open(units, rate, samples256PerFragment, maxFragments, null);
            }
        }
    }

    @Override
    public void close(long time) {
        if (this.singleDomain) {
            this.closed = true;
            for (ISamplesWriter writer : this.writers) {
                writer.close(time);
            }
        }
    }
}

