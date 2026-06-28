/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer.base;

import de.toem.impulse.cells.ports.IPortProgress;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IBinarySamplesWriter;
import de.toem.impulse.samples.IEventSamplesWriter;
import de.toem.impulse.samples.IFloatSamplesWriter;
import de.toem.impulse.samples.IIntegerSamplesWriter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.ITextSamplesWriter;
import de.toem.impulse.serializer.AbstractParsingRecordReader;
import de.toem.impulse.values.Logic;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.math.BigInteger;
import java.util.Vector;

public class TraceReader
extends AbstractParsingRecordReader {
    static final int VERSION = 1;
    private static final String SYNC = "de.toem.impulse.trace";
    private Vector<Signal> signals = new Vector();
    int changed;
    int version = 0;

    public TraceReader() {
    }

    public TraceReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        return SYNC.length();
    }

    @Override
    protected int isApplicable(byte[] buffer) {
        if (new String(buffer, 0, 21).toLowerCase().equals(SYNC)) {
            return 1;
        }
        return -1;
    }

    /*
     * Unable to fully structure code
     */
    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        block23: {
            block20: {
                block22: {
                    block21: {
                        b = new BinaryParseBuffer();
                        synced = false;
                        try {
                            try {
                                this.changed = 4;
                                while (!(!b.read(in) || b.isError() || progress != null && progress.isCanceled())) {
                                    do {
                                        b.begin();
                                        if (!synced) {
                                            synced = this.parseSync(b);
                                            if (this.version > 1) {
                                                b.setError(I18n.Serializer_InvalidVersion);
                                            } else {
                                                b.setOk();
                                            }
                                        } else {
                                            this.waitStreaming(progress);
                                            this.parseEvent(progress, b);
                                        }
                                        b.end();
                                    } while (b.isOk());
                                    b.emptyBin();
                                }
                                break block20;
                            }
                            catch (Throwable e) {
                                SystemLog.log(e);
                                if (this.signals == null) break block21;
                                ** for (signal : this.signals)
                            }
                        }
                        catch (Throwable var6_15) {
                            if (this.signals == null) break block22;
                            ** for (signal : this.signals)
                        }
lbl-1000:
                        // 1 sources

                        {
                            if (signal == null) continue;
                            current = (Long)signal.getData("CURRENT");
                            if (this.getWriter(signal).isOpen() && current != null) {
                                this.getWriter(signal).close(current + 1L);
                            }
                            signal.setData("CURRENT", null);
                            continue;
                        }
                    }
                    if (b.isError()) {
                        throw new ParseException(b.comment);
                    }
                    break block23;
lbl-1000:
                    // 1 sources

                    {
                        if (signal == null) continue;
                        current = (Long)signal.getData("CURRENT");
                        if (this.getWriter(signal).isOpen() && current != null) {
                            this.getWriter(signal).close(current + 1L);
                        }
                        signal.setData("CURRENT", null);
                        continue;
                    }
                }
                if (b.isError()) {
                    throw new ParseException(b.comment);
                }
                throw var6_15;
            }
            if (this.signals != null) {
                for (Signal signal : this.signals) {
                    if (signal == null) continue;
                    current = (Long)signal.getData("CURRENT");
                    if (this.getWriter(signal).isOpen() && current != null) {
                        this.getWriter(signal).close(current + 1L);
                    }
                    signal.setData("CURRENT", null);
                }
            }
            if (b.isError()) {
                throw new ParseException(b.comment);
            }
        }
    }

    private synchronized void parseEvent(IProgress progress, BinaryParseBuffer b) {
        int first = (int)this.parsePlus(b);
        if (!b.isOk()) {
            return;
        }
        if (first == 0) {
            int command = (int)this.parsePlus(b);
            if (!b.isOk()) {
                return;
            }
            if (command == Command.Initialize.ordinal()) {
                ICell child;
                int id = (int)this.parsePlus(b) - 1;
                if (!b.isOk()) {
                    return;
                }
                if (id < 0 || id > 4095) {
                    b.setError(I18n.Serializer_InvalidId);
                    return;
                }
                String path = this.parseString(b);
                if (!b.isOk()) {
                    return;
                }
                int pt = (int)this.parsePlus(b);
                if (!b.isOk()) {
                    return;
                }
                ISamples.ProcessType processType = null;
                if (pt < 0 || pt >= ISamples.ProcessType.values().length) {
                    b.setError(I18n.Serializer_InvalidProcessType);
                    return;
                }
                processType = ISamples.ProcessType.values()[pt];
                int st = (int)this.parsePlus(b);
                if (!b.isOk()) {
                    return;
                }
                ISamples.SignalType signalType = null;
                if (st < 0 || st >= ISamples.SignalType.values().length) {
                    b.setError(I18n.Serializer_InvalidSignalType);
                    return;
                }
                signalType = ISamples.SignalType.values()[st];
                String descr = this.parseString(b);
                if (!b.isOk()) {
                    return;
                }
                ISamples.SignalDescriptor signalDescriptor = ISamples.SignalDescriptor.parseUser(signalType, descr);
                String bs = this.parseString(b);
                if (!b.isOk()) {
                    return;
                }
                IDomainBase domainBase = DomainBase.parse(bs);
                this.parseString(b);
                if (id + 1 > this.signals.size()) {
                    this.signals.setSize(id + 1);
                }
                String[] splitted = path.split("\\/");
                ICell scope = this.base;
                int n = 0;
                while (n < splitted.length - 1) {
                    String p = splitted[n];
                    child = scope.getChildByName(p = p.replaceAll("[\\\\]+", "_"));
                    if (child == null) {
                        child = this.addScope(scope, p);
                    }
                    scope = child;
                    ++n;
                }
                String name = splitted.length > 0 ? splitted[splitted.length - 1] : "unamed";
                name = name.replaceAll("[\\\\]+", "_");
                child = scope.getChildByName(name);
                this.signals.set(id, this.addSignal(scope, name, null, processType, signalType, signalDescriptor, domainBase));
                this.changed = 4;
            } else if (command == Command.Open.ordinal()) {
                int id = (int)this.parsePlus(b) - 1;
                if (!b.isOk()) {
                    return;
                }
                if (id < 0 || id >= this.signals.size()) {
                    b.setError(I18n.Serializer_InvalidId);
                    return;
                }
                Signal signal = this.signals.get(id);
                long start = this.parseLong(b);
                if (!b.isOk()) {
                    return;
                }
                long rate = this.parseLong(b);
                if (!b.isOk()) {
                    return;
                }
                int mode = (int)this.parsePlus(b);
                if (!b.isOk()) {
                    return;
                }
                int limitation = (int)this.parsePlus(b);
                if (!b.isOk()) {
                    return;
                }
                ISamplesWriter writer = this.getWriter(signal);
                writer.open(start, rate, mode, limitation, null);
                signal.setData("CURRENT", start);
            } else if (command == Command.Close.ordinal()) {
                int id = (int)this.parsePlus(b) - 1;
                if (!b.isOk()) {
                    return;
                }
                if (id < 0 || id >= this.signals.size()) {
                    b.setError(I18n.Serializer_InvalidId);
                    return;
                }
                Signal signal = this.signals.get(id);
                long start = this.parseLong(b);
                if (!b.isOk()) {
                    return;
                }
                ISamplesWriter writer = this.getWriter(signal);
                writer.close(start);
                signal.setData("CURRENT", start);
            } else {
                b.setError(I18n.Serializer_InvalidEntry);
            }
        } else {
            int id = (first >>> 1) - 1;
            boolean tag = false;
            if ((first & 1) != 0) {
                tag = (first & 2) != 0;
                id = (first >>> 4) - 1;
            }
            if (id < 0 || id > this.signals.size()) {
                b.setError(I18n.Serializer_InvalidId);
                return;
            }
            long delta = this.parsePlus(b);
            if (!b.isOk()) {
                return;
            }
            int length = (int)this.parsePlus(b);
            this.dataAvalable(b, length);
            if (!b.isOk()) {
                return;
            }
            Signal signal = this.signals.get(id);
            ISamplesWriter writer = this.getWriter(signal);
            if (!writer.isOpen()) {
                return;
            }
            if (delta < 0L) {
                return;
            }
            Long current = (Long)signal.getData("CURRENT");
            signal.setData("CURRENT", delta += current.longValue());
            Object value = TraceReader.parseValue(writer.getSignalType(), writer.getSignalDescriptor(), (byte)0, b.buffer, b.pos, length);
            if (!(progress instanceof IPortProgress) || ((IPortProgress)progress).isStreaming()) {
                if (writer instanceof IIntegerSamplesWriter) {
                    if (value instanceof Integer) {
                        ((IIntegerSamplesWriter)writer).write(delta, tag, (Integer)value);
                    } else if (value instanceof Long) {
                        ((IIntegerSamplesWriter)writer).write(delta, tag, (Long)value);
                    } else if (value instanceof BigInteger) {
                        ((IIntegerSamplesWriter)writer).write(delta, tag, (BigInteger)value);
                    }
                } else if (writer instanceof IFloatSamplesWriter) {
                    if (value instanceof Float) {
                        ((IFloatSamplesWriter)writer).write(delta, tag, (Float)value);
                    } else if (value instanceof Double) {
                        ((IFloatSamplesWriter)writer).write(delta, tag, (Double)value);
                    }
                } else if (writer instanceof ITextSamplesWriter && value instanceof String) {
                    ((ITextSamplesWriter)writer).write(delta, tag, (String)value);
                } else if (writer instanceof IEventSamplesWriter) {
                    if (value == null) {
                        ((IEventSamplesWriter)writer).write(delta, tag);
                    } else if (value instanceof Integer) {
                        ((IEventSamplesWriter)writer).write(delta, tag, (Integer)value);
                    }
                } else if (writer instanceof IBinarySamplesWriter && value instanceof byte[]) {
                    ((IBinarySamplesWriter)writer).write(delta, tag, (byte[])value);
                }
            }
            b.pos += length;
            this.changed = 3 > this.changed ? 3 : this.changed;
        }
    }

    private boolean dataAvalable(BinaryParseBuffer b, int length) {
        if (b.pos + length <= b.available) {
            return true;
        }
        b.setNotEnoughData();
        return false;
    }

    private long parsePlus(BinaryParseBuffer b) {
        int shift = 0;
        long value = 0L;
        while (b.pos < b.available) {
            byte sn = b.buffer[b.pos++];
            value |= (long)((sn & 0x7F) << shift);
            shift += 7;
            if ((sn & 0x80) != 0) continue;
            b.setOk();
            return value;
        }
        b.setNotEnoughData();
        return 0L;
    }

    private long parseLong(BinaryParseBuffer b) {
        int size = (int)this.parsePlus(b);
        if (b.isOk() && b.available - b.pos >= size) {
            byte[] bytes = b.buffer;
            int pos = b.pos;
            b.pos += size;
            if (size == 0) {
                return 0L;
            }
            if (size <= 4) {
                int value = (b.buffer[pos + size - 1] & 0x80) != 0 ? -1 : 0;
                int i = pos + size - 1;
                while (i >= pos) {
                    value = value << 8 | 0xFF & bytes[i];
                    --i;
                }
                return value;
            }
            if (size <= 8) {
                long value = (bytes[pos + size - 1] & 0x80) != 0 ? -1L : 0L;
                int i = pos + size - 1;
                while (i >= pos) {
                    value = value << 8 | (long)(0xFF & bytes[i]);
                    --i;
                }
                return value;
            }
            return 0L;
        }
        b.setNotEnoughData();
        return 0L;
    }

    private String parseString(BinaryParseBuffer b) {
        int size = (int)this.parsePlus(b);
        if (b.isOk() && b.available - b.pos >= size) {
            String value = new String(b.buffer, b.pos, size);
            b.pos += size;
            b.setOk();
            return value;
        }
        b.setNotEnoughData();
        return null;
    }

    private synchronized boolean parseSync(BinaryParseBuffer b) {
        int n = b.pos;
        while (n <= b.pos + b.available - (SYNC.length() + 1)) {
            if (b.buffer[n] == 100 && new String(b.buffer, n, SYNC.length()).toLowerCase().equals(SYNC)) {
                this.version = b.buffer[n + SYNC.length()];
                b.pos = n + SYNC.length() + 1;
                return true;
            }
            ++n;
        }
        return false;
    }

    public static Object parseValue(ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, byte format0, byte[] bytes, int pos, int length) {
        if (signalType == ISamples.SignalType.Logic) {
            return Logic.expand(signalDescriptor.getScale(), format0, bytes, pos, length);
        }
        if (signalType == ISamples.SignalType.Float) {
            if (length == 4) {
                int intdata = 0;
                int i = pos + 3;
                while (i >= pos) {
                    intdata = intdata << 8 | 0xFF & bytes[i];
                    --i;
                }
                return Float.valueOf(Float.intBitsToFloat(intdata));
            }
            if (length == 8) {
                long longdata = 0L;
                int i = pos + 7;
                while (i >= pos) {
                    longdata = longdata << 8 | (long)(0xFF & bytes[i]);
                    --i;
                }
                return Double.longBitsToDouble(longdata);
            }
        } else {
            if (signalType == ISamples.SignalType.Integer) {
                if (length == 0) {
                    return 0;
                }
                if (length <= 4) {
                    int value = (bytes[pos + length - 1] & 0x80) != 0 ? -1 : 0;
                    int i = pos + length - 1;
                    while (i >= pos) {
                        value = value << 8 | 0xFF & bytes[i];
                        --i;
                    }
                    return value;
                }
                if (length <= 8) {
                    long value = (bytes[pos + length - 1] & 0x80) != 0 ? -1L : 0L;
                    int i = pos + length - 1;
                    while (i >= pos) {
                        value = value << 8 | (long)(0xFF & bytes[i]);
                        --i;
                    }
                    return value;
                }
                byte[] buffer = new byte[length];
                int index = 0;
                int i = pos + length - 1;
                while (i >= pos) {
                    buffer[index++] = bytes[i];
                    --i;
                }
                return new BigInteger(buffer);
            }
            if (signalType == ISamples.SignalType.Text) {
                try {
                    return new String(bytes, pos, length, "UTF-8");
                }
                catch (UnsupportedEncodingException unsupportedEncodingException) {
                }
            } else {
                if (signalType == ISamples.SignalType.Event) {
                    int value = 0;
                    if (length != 0 && length <= 4) {
                        value = (bytes[pos + length - 1] & 0x80) != 0 ? -1 : 0;
                        int i = pos + length - 1;
                        while (i >= pos) {
                            value = value << 8 | 0xFF & bytes[i];
                            --i;
                        }
                    }
                    return value;
                }
                if (signalType == ISamples.SignalType.Binary) {
                    byte[] value = new byte[length];
                    System.arraycopy(bytes, pos, value, 0, length);
                    return value;
                }
            }
        }
        return null;
    }

    @Override
    public boolean supportsStreaming() {
        return true;
    }

    @Override
    public synchronized ICover flush() {
        this.changed = 0;
        return super.doFlush();
    }

    @Override
    public int hasChanged() {
        return this.changed;
    }

    class BinaryParseBuffer {
        final int length = 65536;
        byte[] buffer = new byte[65536];
        int available = 0;
        int used = 0;
        int pos = 0;
        public static final int NONE = 0;
        public static final int OK = 1;
        public static final int NOT_ENOUGH_DATA = 2;
        public static final int ERROR = 3;
        int result;
        String comment;

        BinaryParseBuffer() {
        }

        boolean read(InputStream in) throws IOException {
            int read = in.read(this.buffer, this.available, 65536 - this.available);
            if (read > 0) {
                this.available += read;
            }
            return read != -1;
        }

        void emptyBin() {
            this.available -= this.used <= this.available ? this.used : this.available;
            if (this.available > 0) {
                System.arraycopy(this.buffer, this.used, this.buffer, 0, this.available);
            }
            this.used = 0;
        }

        void begin() {
            this.result = 0;
            this.pos = this.used;
        }

        public void end() {
            if (this.result == 1) {
                this.used = this.pos;
            }
        }

        public void setOk() {
            this.result = 1;
        }

        public void setNotEnoughData() {
            this.result = 2;
        }

        public void setError(String text) {
            this.result = 3;
            this.comment = text;
        }

        public boolean isOk() {
            return this.result == 1;
        }

        public boolean isError() {
            return this.result == 3;
        }
    }

    public static enum Command {
        Initialize,
        Open,
        Close;

    }
}

