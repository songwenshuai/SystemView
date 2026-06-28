/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer.base;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.samples.IBinarySamplesWriter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.IOException;
import java.io.InputStream;

public class ByteBlockReader
extends AbstractSingleDomainRecordReader {
    private int bytesProcessed;
    private long current;
    private Signal signal;
    private long started;
    private int changed;

    public ByteBlockReader() {
    }

    public ByteBlockReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        return -1;
    }

    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        block14: {
            try {
                int read;
                this.initRecord("Bytes", TimeBase.ms);
                this.signal = this.addSignal(this.base, "Bytes", null, ISamples.ProcessType.Discrete, ISamples.SignalType.Binary, ISamples.SignalDescriptor.DEFAULT);
                this.changed = 4;
                this.waitStreaming(progress);
                this.bytesProcessed = 1;
                this.started = Utils.millies();
                this.current = 0L;
                this.open(this.current);
                byte[] bytes = new byte[16];
                while (!((read = in.read(bytes)) < 0 || progress != null && progress.isCanceled())) {
                    this.parse(bytes, read);
                }
                this.close(this.current + 1L);
            }
            catch (ParseException e) {
                throw e;
            }
            catch (IOException iOException) {
                try {
                    in.close();
                }
                catch (IOException iOException2) {}
                break block14;
            }
            catch (Throwable e) {
                try {
                    throw new ParseException(this.bytesProcessed, e.getMessage(), e);
                }
                catch (Throwable throwable) {
                    try {
                        in.close();
                    }
                    catch (IOException iOException) {}
                    throw throwable;
                }
            }
            try {
                in.close();
            }
            catch (IOException iOException) {}
        }
    }

    @Override
    public boolean supportsStreaming() {
        return true;
    }

    @Override
    public synchronized ICover flush() {
        this.changed = 0;
        return super.doFlush(this.current);
    }

    @Override
    public int hasChanged() {
        return this.changed;
    }

    private synchronized void parse(byte[] bytes, int size) throws ParseException {
        if (size <= 0) {
            return;
        }
        if (size < bytes.length) {
            byte[] buffer = new byte[size];
            System.arraycopy(bytes, 0, buffer, 0, size);
            bytes = buffer;
        }
        this.current = Utils.millies() - this.started;
        ((IBinarySamplesWriter)this.getWriter(this.signal)).write(Utils.millies() - this.started, false, bytes);
        this.bytesProcessed += size;
        this.changed = 3;
    }
}

