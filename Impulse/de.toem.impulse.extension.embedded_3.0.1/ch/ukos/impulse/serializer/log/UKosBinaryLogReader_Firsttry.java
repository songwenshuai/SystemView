/*
 * Decompiled with CFR 0.152.
 */
package ch.ukos.impulse.serializer.log;

import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.samples.IIntegerSamplesWriter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;

public class UKosBinaryLogReader_Firsttry
extends AbstractSingleDomainRecordReader {
    public UKosBinaryLogReader_Firsttry() {
    }

    public UKosBinaryLogReader_Firsttry(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        if (name != null && !name.endsWith("uLog")) {
            return -1;
        }
        return 8;
    }

    @Override
    protected int isApplicable(byte[] buffer) {
        if (buffer[0] == 83 && buffer[1] == 79 && buffer[2] == 75 && buffer[3] == 117) {
            return 1;
        }
        return -1;
    }

    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        this.initRecord("uKos Log", TimeBase.ms);
        class Task {
            int id;
            IIntegerSamplesWriter oNbExec;
            IIntegerSamplesWriter oNbCalls;
            IIntegerSamplesWriter oState;
            IIntegerSamplesWriter oCPUTime;
            IIntegerSamplesWriter oRes1;
            IIntegerSamplesWriter oRes2;
            IIntegerSamplesWriter oRes3;
            IIntegerSamplesWriter oRes4;
            IIntegerSamplesWriter oRes5;

            Task() {
            }
        }
        HashMap<Integer, Task> tasks = new HashMap<Integer, Task>();
        long t = 0L;
        boolean opened = false;
        int sLength = 60;
        byte[] struct = new byte[sLength];
        BufferedInputStream input = new BufferedInputStream(in);
        try {
            while (input.read(struct, 0, sLength) == sLength) {
                int taskId = this.readInteger(struct, 4);
                t = this.readLong(struct, 8);
                if (!opened) {
                    this.open(t);
                }
                opened = true;
                if (!tasks.containsKey(taskId)) {
                    Scope signals = this.addScope(this.base, "Task " + taskId);
                    Task task = new Task();
                    task.id = taskId;
                    task.oNbExec = (IIntegerSamplesWriter)this.getWriter(this.addSignal(signals, "NbExec", "NbExec", ISamples.ProcessType.Discrete, ISamples.SignalType.Integer, ISamples.SignalDescriptor.DEFAULT));
                    task.oNbCalls = (IIntegerSamplesWriter)this.getWriter(this.addSignal(signals, "NbCalls", "NbCalls", ISamples.ProcessType.Discrete, ISamples.SignalType.Integer, ISamples.SignalDescriptor.DEFAULT));
                    task.oState = (IIntegerSamplesWriter)this.getWriter(this.addSignal(signals, "State", "State", ISamples.ProcessType.Discrete, ISamples.SignalType.Integer, ISamples.SignalDescriptor.DEFAULT));
                    task.oCPUTime = (IIntegerSamplesWriter)this.getWriter(this.addSignal(signals, "CPUTime", "CPUTime", ISamples.ProcessType.Discrete, ISamples.SignalType.Integer, ISamples.SignalDescriptor.DEFAULT));
                    task.oRes1 = (IIntegerSamplesWriter)this.getWriter(this.addSignal(signals, "Res1", "Res1", ISamples.ProcessType.Discrete, ISamples.SignalType.Integer, ISamples.SignalDescriptor.DEFAULT));
                    task.oRes2 = (IIntegerSamplesWriter)this.getWriter(this.addSignal(signals, "Res2", "Res2", ISamples.ProcessType.Discrete, ISamples.SignalType.Integer, ISamples.SignalDescriptor.DEFAULT));
                    task.oRes3 = (IIntegerSamplesWriter)this.getWriter(this.addSignal(signals, "Res3", "Res3", ISamples.ProcessType.Discrete, ISamples.SignalType.Integer, ISamples.SignalDescriptor.DEFAULT));
                    task.oRes4 = (IIntegerSamplesWriter)this.getWriter(this.addSignal(signals, "Res4", "Res4", ISamples.ProcessType.Discrete, ISamples.SignalType.Integer, ISamples.SignalDescriptor.DEFAULT));
                    task.oRes5 = (IIntegerSamplesWriter)this.getWriter(this.addSignal(signals, "Res5", "Res5", ISamples.ProcessType.Discrete, ISamples.SignalType.Integer, ISamples.SignalDescriptor.DEFAULT));
                    tasks.put(taskId, task);
                }
                Task task = (Task)tasks.get(taskId);
                task.oNbExec.write(t, false, this.readLong(struct, 16));
                task.oNbCalls.write(t, false, this.readLong(struct, 24));
                task.oState.write(t, false, this.readLong(struct, 32));
                task.oCPUTime.write(t, false, this.readInteger(struct, 36));
                task.oRes1.write(t, false, this.readInteger(struct, 40));
                task.oRes2.write(t, false, this.readInteger(struct, 44));
                task.oRes3.write(t, false, this.readInteger(struct, 48));
                task.oRes4.write(t, false, this.readInteger(struct, 52));
                task.oRes5.write(t, false, this.readInteger(struct, 56));
            }
        }
        catch (IOException e) {
            throw new ParseException(0, (Throwable)e);
        }
        if (opened) {
            this.close(t + 10L);
        }
    }

    private int readInteger(byte[] struct, int pos) {
        int value = struct[pos + 3];
        value = value << 8 | struct[pos + 2] & 0xFF;
        value = value << 8 | struct[pos + 1] & 0xFF;
        value = value << 8 | struct[pos + 0] & 0xFF;
        return value;
    }

    private long readLong(byte[] struct, int pos) {
        long value = struct[pos + 7];
        value = value << 8 | (long)(struct[pos + 6] & 0xFF);
        value = value << 8 | (long)(struct[pos + 5] & 0xFF);
        value = value << 8 | (long)(struct[pos + 4] & 0xFF);
        value = value << 8 | (long)(struct[pos + 3] & 0xFF);
        value = value << 8 | (long)(struct[pos + 2] & 0xFF);
        value = value << 8 | (long)(struct[pos + 1] & 0xFF);
        value = value << 8 | (long)(struct[pos + 0] & 0xFF);
        return value;
    }
}

