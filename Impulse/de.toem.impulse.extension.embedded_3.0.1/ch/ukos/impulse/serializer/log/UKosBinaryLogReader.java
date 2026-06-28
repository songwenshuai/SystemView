/*
 * Decompiled with CFR 0.152.
 */
package ch.ukos.impulse.serializer.log;

import de.toem.impulse.cells.ports.IPortProgress;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.IStructSamplesWriter;
import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.impulse.values.StructMember;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;

public class UKosBinaryLogReader
extends AbstractSingleDomainRecordReader {
    private long current;
    private int changed;

    public UKosBinaryLogReader() {
    }

    public UKosBinaryLogReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        if (name != null && !name.endsWith("imp")) {
            return -1;
        }
        return 8;
    }

    @Override
    protected int isApplicable(byte[] buffer) {
        if (buffer[0] == 117 && buffer[1] == 75 && buffer[2] == 79 && buffer[3] == 83) {
            return 1;
        }
        return -1;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     * Unable to fully structure code
     * Enabled aggressive block sorting
     * Enabled unnecessary exception pruning
     * Enabled aggressive exception aggregation
     */
    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        block21: {
            this.initRecord("uKos Log", TimeBase.us);
            if (progress instanceof IPortProgress) {
                var3_3 = progress;
                synchronized (var3_3) {
                    while (true) {
                        if (((IPortProgress)progress).isStreaming() || progress.isCanceled()) {
                            break;
                        }
                        try {
                            progress.wait(250L);
                        }
                        catch (InterruptedException v0) {}
                    }
                }
            }
            tasks = new HashMap<Integer, Task>();
            lastTask = null;
            this.current = 0L;
            opened = false;
            sLength = 40;
            bytes = new byte[sLength];
            input = new BufferedInputStream(in);
            try {
                while (true) lbl-1000:
                // 2 sources

                {
                    b = 0;
                    filled = 0;
                    while (true) {
                        if (filled >= sLength || b == -1) {
                            if (b != -1) break;
                            break block21;
                        }
                        b = input.read();
                        bytes[filled] = (byte)b;
                        ++filled;
                    }
                    taskId = this.readByte(bytes, 4);
                    this.current = this.readLong(bytes, 16);
                    if (!opened) {
                        this.open(this.current);
                    }
                    opened = true;
                    var12_13 = this;
                    synchronized (var12_13) {
                        if (lastTask != null) {
                            lastTask.writer.write(this.current, false, lastTask.tid, 3, 0, null);
                            ++lastTask.tid;
                        }
                        if (!tasks.containsKey(taskId)) {
                            task = new Task();
                            task.id = taskId;
                            task.signal = this.addSignal(null, "Process " + taskId, "Process", ISamples.ProcessType.Discrete, ISamples.SignalType.Struct, ISamples.SignalDescriptor.StructTransaction);
                            task.writer = (IStructSamplesWriter)this.getWriter(task.signal);
                            task.struct = new StructMember[5];
                            task.struct[0] = new StructMember("State", 1, null, -1);
                            task.struct[1] = new StructMember("Executions", 3, null, -1);
                            task.struct[2] = new StructMember("Process Time", 3, null, -1);
                            task.struct[3] = new StructMember("Priority", 3, null, -1);
                            task.struct[4] = new StructMember("Stack", 3, null, 3);
                            tasks.put(taskId, task);
                            this.changed = 4;
                        }
                        task = lastTask = (Task)tasks.get(taskId);
                        task.struct[0].setValue(this.bytesToString(bytes, 12));
                        task.struct[1].setValue(this.readLong(bytes, 24));
                        task.struct[2].setValue(this.readLong(bytes, 32));
                        task.struct[3].setValue(this.readByte(bytes, 5));
                        task.struct[4].setValue(this.readInteger(bytes, 8));
                        task.writer.write(this.current, false, task.tid, 1, 0, task.struct);
                        this.changed = this.changed > 3 ? this.changed : 3;
                        continue;
                    }
                    break;
                }
            }
            catch (IOException e) {
                throw new ParseException(0, (Throwable)e);
            }
            {
                ** while (true)
            }
        }
        if (opened) {
            this.close(this.current + 10L);
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

    private int readByte(byte[] struct, int pos) {
        byte value = struct[pos + 0];
        return value;
    }

    private int readShort(byte[] struct, int pos) {
        short value = (short)(struct[pos + 1] & 0xFF);
        value = (short)(value << 8 | struct[pos + 0] & 0xFF);
        return value;
    }

    private int readInteger(byte[] struct, int pos) {
        int value = struct[pos + 3] & 0xFF;
        value = value << 8 | struct[pos + 2] & 0xFF;
        value = value << 8 | struct[pos + 1] & 0xFF;
        value = value << 8 | struct[pos + 0] & 0xFF;
        return value;
    }

    private long readLong(byte[] struct, int pos) {
        long value = struct[pos + 7] & 0xFF;
        value = value << 8 | (long)(struct[pos + 6] & 0xFF);
        value = value << 8 | (long)(struct[pos + 5] & 0xFF);
        value = value << 8 | (long)(struct[pos + 4] & 0xFF);
        value = value << 8 | (long)(struct[pos + 3] & 0xFF);
        value = value << 8 | (long)(struct[pos + 2] & 0xFF);
        value = value << 8 | (long)(struct[pos + 1] & 0xFF);
        value = value << 8 | (long)(struct[pos + 0] & 0xFF);
        return value;
    }

    private String bytesToString(byte[] struct, int pos) {
        char[] result = new char[4];
        result[3] = (char)struct[pos + 3];
        result[2] = (char)struct[pos + 2];
        result[1] = (char)struct[pos + 1];
        result[0] = (char)struct[pos + 0];
        return new String(result);
    }
}

