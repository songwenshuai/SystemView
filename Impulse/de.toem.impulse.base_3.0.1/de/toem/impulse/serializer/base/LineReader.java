/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer.base;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ITextSamplesWriter;
import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

public class LineReader
extends AbstractSingleDomainRecordReader {
    private int linesProcessed;
    private long current;
    private Signal signal;
    private long started;
    private int changed;

    public LineReader() {
    }

    public LineReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        return -1;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     * Unable to fully structure code
     */
    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        block21: {
            reader = new BufferedReader(new InputStreamReader(in));
            try {
                this.initRecord("Lines", TimeBase.ms);
                this.signal = this.addSignal(this.base, "Lines", null, ISamples.ProcessType.Discrete, ISamples.SignalType.Text, ISamples.SignalDescriptor.DEFAULT);
                this.changed = 4;
                this.waitStreaming(progress);
                var4_4 = this;
                synchronized (var4_4) {
                    this.linesProcessed = 1;
                    this.started = Utils.millies();
                    this.current = 0L;
                    this.open(this.current);
                    this.changed = this.changed > 2 ? this.changed : 2;
                    // MONITOREXIT @DISABLED, blocks:[0, 1, 2, 8] lbl16 : MonitorExitStatement: MONITOREXIT : var4_4
                    if (true) ** GOTO lbl22
                }
                do {
                    this.parse(line);
lbl22:
                    // 2 sources

                } while ((line = reader.readLine()) != null && (progress == null || !progress.isCanceled()));
                var5_7 = this;
                synchronized (var5_7) {
                    this.close(this.current + 1L);
                    this.changed = 0;
                }
            }
            catch (ParseException e) {
                throw e;
            }
            catch (IOException v2) {
                try {
                    reader.close();
                }
                catch (IOException v3) {}
                break block21;
            }
            catch (Throwable e) {
                try {
                    throw new ParseException(this.linesProcessed, e.getMessage(), e);
                }
                catch (Throwable var6_8) {
                    try {
                        reader.close();
                    }
                    catch (IOException v4) {}
                    throw var6_8;
                }
            }
            try {
                reader.close();
            }
            catch (IOException v5) {}
        }
    }

    @Override
    public boolean supportsStreaming() {
        return true;
    }

    @Override
    public synchronized ICover flush() {
        if (this.changed != 0) {
            this.changed = 0;
            return super.doFlush(this.current);
        }
        return null;
    }

    @Override
    public int hasChanged() {
        return this.changed;
    }

    private synchronized void parse(String line) throws ParseException {
        if (line.trim().isEmpty()) {
            return;
        }
        this.current = Utils.millies() - this.started;
        ((ITextSamplesWriter)this.getWriter(this.signal)).write(this.current, false, line);
        ++this.linesProcessed;
        this.changed = this.changed > 3 ? this.changed : 3;
    }
}

