/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer.base;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.serializer.FluxReaderConfiguration;
import de.toem.impulse.flux.FluxConfigurationHandler;
import de.toem.impulse.flux.FluxParser;
import de.toem.impulse.flux.IFluxHandler;
import de.toem.impulse.serializer.AbstractParsingRecordReader;
import de.toem.impulse.serializer.BinaryParseBuffer;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.serializer.IInputRequest;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.InputStream;

public class FluxReader
extends AbstractParsingRecordReader {
    private static final String HEAD = "\u0000\u0001flux";
    private int changed;
    private int version;
    protected BinaryParseBuffer buffer;
    protected FluxParser parser;
    protected IFluxHandler handler;
    private boolean synced;

    public FluxReader() {
    }

    public FluxReader(String id, InputStream in) {
        super(id, in);
    }

    public FluxReader(String id, InputStream in, IFluxHandler handler) {
        super(id, in);
        this.handler = handler;
    }

    @Override
    protected int isApplicable(String name, String contentType, IInputRequest inputRequest) {
        String text = inputRequest.text(HEAD.length());
        if (text != null && text.equals(HEAD)) {
            return 1;
        }
        return -1;
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        return HEAD.length();
    }

    @Override
    protected int isApplicable(byte[] buffer) {
        if (new String(buffer, 0, buffer.length).equals(HEAD)) {
            return 1;
        }
        return -1;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    @Override
    protected void parse(IProgress p, InputStream in) throws ParseException {
        block19: {
            BinaryParseBuffer b;
            FluxParser parser;
            block17: {
                if (this.configuration instanceof FluxReaderConfiguration && this.handler == null) {
                    this.handler = new FluxConfigurationHandler((FluxReaderConfiguration)this.configuration);
                }
                this.parser = parser = this.parser == null ? new FluxParser(this, this.handler) : this.parser;
                this.buffer = b = this.buffer == null ? new BinaryParseBuffer(524288) : this.buffer;
                b.clear();
                this.changed = 4;
                while (!(this.synced || !b.fill(in) || b.isError() || p != null && p.isCanceled())) {
                    b = b.begin();
                    this.synced = parser.synchronize(b);
                    b = b.end();
                    b.clean();
                }
                if (this.synced) break block17;
                this.doFlush();
                if (b.isError()) {
                    ParseException e = new ParseException(b.getErrorText());
                    e.position = b.used();
                    throw e;
                }
                return;
            }
            try {
                try {
                    boolean first = true;
                    while (!(parser.isFinished() || b.isError() || p != null && p.isCanceled() || !first && !b.fill(in))) {
                        first = false;
                        this.waitStreaming(p);
                        FluxReader fluxReader = this;
                        synchronized (fluxReader) {
                            do {
                                b = b.begin();
                                this.changed = parser.parseEntry(p, b, this.changed);
                            } while ((b = b.end()).isOk());
                        }
                        if (p != null) {
                            double d = 1.0 * (double)b.used() / (double)in.available();
                            p.done(d);
                        }
                        b.clean();
                    }
                }
                catch (Throwable e) {
                    SystemLog.log(e);
                    this.doFlush();
                    if (b.isError()) {
                        ParseException e2 = new ParseException(b.getErrorText());
                        e2.position = b.used();
                        throw e2;
                    }
                    break block19;
                }
            }
            catch (Throwable throwable) {
                this.doFlush();
                if (b.isError()) {
                    ParseException e = new ParseException(b.getErrorText());
                    e.position = b.used();
                    throw e;
                }
                throw throwable;
            }
            this.doFlush();
            if (b.isError()) {
                ParseException e = new ParseException(b.getErrorText());
                e.position = b.used();
                throw e;
            }
        }
    }

    @Override
    public boolean supportsStreaming() {
        return true;
    }

    @Override
    public synchronized ICover flush() {
        this.changed = 0;
        return this.doFlush();
    }

    @Override
    protected ICover doFlush() {
        for (ICell cell : this.base.getTribe(false, Signal.class)) {
            if (cell.getData("SERIALIZER") != this) continue;
            Signal signal = (Signal)cell;
            if (this.parser == null) continue;
            this.parser.flush(signal);
        }
        return this.cover;
    }

    @Override
    public int hasChanged() {
        return this.changed;
    }
}

