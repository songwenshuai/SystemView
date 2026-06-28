/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.swv;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.extension.embedded.swv.SwvConfiguration;
import de.toem.impulse.samples.IBinarySamplesWriter;
import de.toem.impulse.samples.IEventSamplesWriter;
import de.toem.impulse.samples.IFloatSamplesWriter;
import de.toem.impulse.samples.IIntegerSamplesWriter;
import de.toem.impulse.samples.ILogicSamplesWriter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.IStructSamplesWriter;
import de.toem.impulse.samples.ITextSamplesWriter;
import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.impulse.values.StructMember;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.InputStream;

public class SwvReader
extends AbstractSingleDomainRecordReader {
    public static final int ITM_SIGNAL_COUNT = 32;
    public static final int STAT_SIGNAL_COUNT = 3;
    public static final int STAT_SIGNAL_OVERFLOW = 32;
    public static final int STAT_SIGNAL_SYSTEM = 33;
    public static final int STAT_SIGNAL_FREQ = 34;
    int changed;
    long current;
    boolean waitForSync;
    boolean synced;
    long t;
    long tsNsFactor;
    int[] itmModes;
    Object[] data;
    StructMember[][] logs;
    Signal[] signals;
    StructMember[] statMembers;
    long lastMilli;
    long tstotal;
    int noOfProbes;
    double frequency;
    int lastPort = -1;

    public SwvReader() {
    }

    public SwvReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        return -1;
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

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     * Unable to fully structure code
     */
    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        selected = null;
        if (this.configuration instanceof SwvConfiguration) {
            selected = (SwvConfiguration)this.configuration;
        }
        if (selected == null) {
            throw new ParseException(-1, "Configuration required");
        }
        this.waitForSync = selected.waitForSync;
        this.tsNsFactor = 1000000000000L / (long)selected.tsFrequency / (long)selected.tsDevider;
        this.synced = this.waitForSync == false;
        buffer = new byte[65536];
        read = 0;
        readTotal = false;
        available = 0;
        used = 0;
        wrapped = 0;
        if (in == null) {
            return;
        }
        this.initRecord("SWV Trace", TimeBase.ps);
        this.signals = new Signal[35];
        this.data = new Object[32];
        this.logs = new StructMember[32][];
        this.itmModes = new int[32];
        n = 0;
        while (n < 32) {
            name = selected.getValue("itmLabel_" + n, String.class);
            if (Utils.isEmpty(name)) {
                name = String.valueOf(n);
            }
            this.itmModes[n] = mode = selected.getValueAsInt("itmMode_" + n);
            if (mode != 0) {
                type = ISamples.SignalType.Integer;
                descriptor = ISamples.SignalDescriptor.DEFAULT;
                if (mode == 1) {
                    type = ISamples.SignalType.Text;
                } else if (mode == 11) {
                    type = ISamples.SignalType.Float;
                    descriptor = ISamples.SignalDescriptor.Float32;
                } else if (mode == 21) {
                    type = ISamples.SignalType.Float;
                    descriptor = ISamples.SignalDescriptor.Float64;
                } else if (mode == 13) {
                    type = ISamples.SignalType.Logic;
                    descriptor = ISamples.SignalDescriptor.LogicWidth(1);
                } else if (mode == 14) {
                    type = ISamples.SignalType.Struct;
                    descriptor = new ISamples.SignalDescriptor("default", 458751);
                } else if (mode == 15) {
                    type = ISamples.SignalType.Binary;
                    descriptor = ISamples.SignalDescriptor.DEFAULT;
                }
                this.signals[n] = this.addSignal(null, name, null, ISamples.ProcessType.Discrete, type, descriptor);
            }
            ++n;
        }
        this.signals[32] = this.addSignal(null, "Overflow", null, ISamples.ProcessType.Discrete, ISamples.SignalType.Event, ISamples.SignalDescriptor.DEFAULT);
        this.signals[33] = this.addSignal(null, "System", null, ISamples.ProcessType.Discrete, ISamples.SignalType.Struct, ISamples.SignalDescriptor.DEFAULT);
        this.signals[34] = this.addSignal(null, "Frequency", null, ISamples.ProcessType.Discrete, ISamples.SignalType.Float, ISamples.SignalDescriptor.Float32);
        this.statMembers = new StructMember[2];
        this.statMembers[0] = new StructMember("Message", 2, null, 6);
        this.statMembers[1] = new StructMember("Argument", 1, null, -1);
        this.statMembers[0].setValid(true);
        this.changed = 4;
        this.waitStreaming(progress);
        this.t = 0L;
        Utils.millies();
        n = this;
        synchronized (n) {
            this.open(this.t);
            if (this.signals[33] != null) {
                this.statMembers[0].setValue("Start");
                this.statMembers[1].setValue("Wait for sync " + this.waitForSync);
                this.statMembers[1].setValid(true);
                ((IStructSamplesWriter)this.getWriter(this.signals[33])).write(this.t, false, this.statMembers);
            }
            this.changed = 3 > this.changed ? 3 : this.changed;
            // MONITOREXIT @DISABLED, blocks:[0, 2] lbl79 : MonitorExitStatement: MONITOREXIT : n
            if (true) ** GOTO lbl92
        }
        {
            do {
                toRead = in.available();
                read = (toRead = Math.min(toRead, buffer.length - wrapped)) > 1 ? in.read(buffer, wrapped, buffer.length - wrapped) : in.read(buffer, wrapped, 1);
                available = wrapped + (read >= 0 ? read : 0);
                used = this.parse(buffer, available);
                if (used > 0) {
                    v1 = this.changed = 3 > this.changed ? 3 : this.changed;
                }
                if ((wrapped = available - used) <= 0) continue;
                System.arraycopy(buffer, used, buffer, 0, wrapped);
lbl92:
                // 3 sources

            } while (read != -1 && (progress == null || !progress.isCanceled()));
            this.close(this.t);
        }
    }

    private synchronized int parse(byte[] parseBuffer, int length) {
        int pos = 0;
        if (!this.synced) {
            int zeros = 0;
            while (pos < length) {
                byte b;
                if ((b = parseBuffer[pos++]) == 0) {
                    ++zeros;
                    continue;
                }
                if (b == -128 && zeros >= 5) {
                    this.synced = true;
                    if (this.signals[33] == null) break;
                    this.statMembers[0].setValue("Synced");
                    this.statMembers[1].setValid(false);
                    ((IStructSamplesWriter)this.getWriter(this.signals[33])).write(this.t, false, this.statMembers);
                    break;
                }
                zeros = 0;
            }
        }
        if (this.synced) {
            block1: while (pos < length) {
                byte header = parseBuffer[pos];
                if (header == 0) {
                    if (length - pos < 6) {
                        return pos;
                    }
                    if (parseBuffer[pos + 1] == 0 && parseBuffer[pos + 2] == 0 && parseBuffer[pos + 3] == 0 && parseBuffer[pos + 4] == 0 && parseBuffer[pos + 5] == -128) {
                        pos += 6;
                        if (this.signals[33] == null) continue;
                        this.statMembers[0].setValue("Found Sync");
                        this.statMembers[1].setValid(false);
                        ((IStructSamplesWriter)this.getWriter(this.signals[33])).write(this.t, false, this.statMembers);
                        continue;
                    }
                } else {
                    if (header == 112) {
                        this.handleOverflow();
                        ++pos;
                        continue;
                    }
                    if ((header & 3) == 0) {
                        int c = 0;
                        while (c < 5 && pos + c < length) {
                            if ((parseBuffer[pos + c] & 0x80) == 0) {
                                if ((header & 0xC) == 0) {
                                    this.handleTimeStamp(parseBuffer, pos, c + 1);
                                } else if ((header & 8) != 0) {
                                    this.handleExtension(parseBuffer, pos, c + 1);
                                } else if ((header & 0xC) == 4) {
                                    this.handleReserved(parseBuffer, pos, c + 1);
                                }
                                pos += c + 1;
                                continue block1;
                            }
                            ++c;
                        }
                        if (pos + 5 >= length) {
                            return pos;
                        }
                    } else if ((header & 3) != 0) {
                        int bytes;
                        int n = bytes = (header & 3) == 3 ? 4 : header & 3;
                        if (pos + bytes < length) {
                            if ((header & 4) == 0) {
                                this.handleInstrumentation(parseBuffer, pos, bytes + 1);
                            } else if ((header & 4) != 0) {
                                this.handleHardware(parseBuffer, pos, bytes + 1);
                            }
                            pos += bytes + 1;
                            continue;
                        }
                        return pos;
                    }
                }
                if (this.signals[33] != null) {
                    this.statMembers[0].setValue("ERROR");
                    this.statMembers[1].setValue("Invalid byte: " + parseBuffer[pos]);
                    this.statMembers[1].setValid(true);
                    ((IStructSamplesWriter)this.getWriter(this.signals[33])).write(this.t, true, this.statMembers);
                }
                ++pos;
                if (!this.waitForSync) continue;
                this.synced = false;
                return pos;
            }
        }
        return pos;
    }

    private void handleOverflow() {
        if (this.signals[32] != null) {
            ((IEventSamplesWriter)this.getWriter(this.signals[32])).write(this.t, false);
        }
    }

    private void handleTimeStamp(byte[] buffer, int pos, int count) {
        long ts = 0L;
        int shift = 0;
        if ((buffer[pos + 0] & 0x80) != 0) {
            int cfr_ignored_0 = (buffer[0] & 0x30) >> 4;
            ts = 0L;
            shift = 0;
        } else {
            ts = (buffer[pos + 0] & 0x70) >> 4;
            shift = 3;
        }
        int n = 1;
        while (n < count) {
            ts |= (long)((buffer[pos + n] & 0x7F) << shift);
            shift += 7;
            ++n;
        }
        this.tstotal += ts;
        this.t += this.tsNsFactor * ts;
        long milli = Utils.millies();
        if (this.lastMilli == 0L) {
            this.lastMilli = milli;
        } else if (milli - this.lastMilli > 100L) {
            double diff = (double)(milli - this.lastMilli) / 1000.0;
            double freq = (double)this.tstotal / diff;
            this.frequency = (freq + this.frequency * (double)this.noOfProbes) / (double)(++this.noOfProbes);
            ((IFloatSamplesWriter)this.getWriter(this.signals[34])).write(this.t, false, this.frequency);
            this.lastMilli = milli;
            this.tstotal = 0L;
        }
    }

    private void handleReserved(byte[] buffer, int pos, int i) {
    }

    private void handleExtension(byte[] buffer, int pos, int i) {
    }

    private void handleInstrumentation(byte[] buffer, int pos, int length) {
        int port = (buffer[pos] & 0xF8) >> 3;
        if (this.itmModes[port] == 0) {
            return;
        }
        if ((this.itmModes[port] == 1 || this.itmModes[port] == 2 || this.itmModes[port] == 14) && length >= 2 && this.signals[port] != null) {
            int n = 1;
            while (n < length) {
                char c = (char)buffer[pos + 1];
                if (this.data[port] != null && (c == '\u0000' || (c == '\n' || c == '\r') && this.itmModes[port] == 2)) {
                    if (this.itmModes[port] == 14) {
                        String[] splitted;
                        if (this.logs[port] == null) {
                            this.logs[port] = new StructMember[3];
                            this.logs[port][0] = new StructMember("Severity", 2, null, 6);
                            this.logs[port][1] = new StructMember("Source", 2, null, 6);
                            this.logs[port][2] = new StructMember("Message", 1, null, 6);
                        }
                        if ((splitted = ((StringBuilder)this.data[port]).toString().split("\\n")) != null && splitted.length == 3) {
                            boolean tag = false;
                            if (splitted[0].equals("w")) {
                                this.logs[port][0].setValue("Warning");
                            } else if (splitted[0].equals("e")) {
                                this.logs[port][0].setValue("Error");
                                tag = true;
                            } else {
                                this.logs[port][0].setValue("Info");
                            }
                            this.logs[port][1].setValue(splitted[1]);
                            this.logs[port][2].setValue(splitted[2]);
                            ((IStructSamplesWriter)this.getWriter(this.signals[port])).write(this.t, tag, this.logs[port]);
                        }
                    } else {
                        ((ITextSamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, ((StringBuilder)this.data[port]).toString());
                    }
                    this.data[port] = null;
                } else if (c != '\u0000' && (c != '\n' && c != '\r' || this.itmModes[port] != 2)) {
                    if (this.data[port] == null) {
                        this.data[port] = new StringBuilder();
                    }
                    ((StringBuilder)this.data[port]).append(c);
                }
                ++n;
            }
        } else if (this.itmModes[port] == 3 && length == 2 && this.signals[port] != null) {
            ((IIntegerSamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, buffer[pos + 1]);
        } else if (this.itmModes[port] == 4 && length == 2 && this.signals[port] != null) {
            ((IIntegerSamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, buffer[pos + 1] & 0xFF);
        } else if (this.itmModes[port] == 5 && length == 3 && this.signals[port] != null) {
            ((IIntegerSamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, buffer[pos + 1] & 0xFF | buffer[pos + 2] << 8);
        } else if (this.itmModes[port] == 6 && length == 3 && this.signals[port] != null) {
            ((IIntegerSamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, buffer[pos + 1] & 0xFF | (buffer[pos + 2] & 0xFF) << 8);
        } else if (this.itmModes[port] == 7 && length == 5 && this.signals[port] != null) {
            int i = buffer[pos + 1] & 0xFF | (buffer[pos + 2] & 0xFF) << 8 | (buffer[pos + 3] & 0xFF) << 16 | (buffer[pos + 4] & 0xFF) << 24;
            ((IIntegerSamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, i);
        } else if (this.itmModes[port] == 8 && length == 5 && this.signals[port] != null) {
            long l = (long)(buffer[pos + 1] & 0xFF) | (long)((buffer[pos + 2] & 0xFF) << 8) | (long)((buffer[pos + 3] & 0xFF) << 16) | (long)((buffer[pos + 3] & 0xFF) << 24);
            ((IIntegerSamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, l);
        } else if ((this.itmModes[port] == 9 || this.itmModes[port] == 10) && length == 5 && this.signals[port] != null) {
            int i = buffer[pos + 1] & 0xFF | (buffer[pos + 2] & 0xFF) << 8 | (buffer[pos + 3] & 0xFF) << 16 | (buffer[pos + 4] & 0xFF) << 24;
            if (this.data[port] instanceof Integer && this.lastPort == port) {
                long l = (long)((Integer)this.data[port]).intValue() & 0xFFFFFFFFL | (long)i << 32;
                ((IIntegerSamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, l);
                this.data[port] = null;
            } else {
                this.data[port] = i;
            }
        } else if (this.itmModes[port] == 11 && length == 5 && this.signals[port] != null) {
            int i = buffer[pos + 1] & 0xFF | (buffer[pos + 2] & 0xFF) << 8 | (buffer[pos + 3] & 0xFF) << 16 | (buffer[pos + 4] & 0xFF) << 24;
            float f = Float.intBitsToFloat(i);
            ((IFloatSamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, f);
        } else if (this.itmModes[port] == 21 && length == 5 && this.signals[port] != null) {
            int i = buffer[pos + 1] & 0xFF | (buffer[pos + 2] & 0xFF) << 8 | (buffer[pos + 3] & 0xFF) << 16 | (buffer[pos + 4] & 0xFF) << 24;
            if (this.data[port] instanceof Integer && this.lastPort == port) {
                long l = (long)((Integer)this.data[port]).intValue() & 0xFFFFFFFFL | (long)i << 32;
                double d = Double.longBitsToDouble(l);
                ((IFloatSamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, d);
                this.data[port] = null;
            } else {
                this.data[port] = i;
            }
        } else if (this.itmModes[port] == 13 && length == 2 && this.signals[port] != null) {
            ((ILogicSamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, buffer[pos + 1]);
        } else if (this.itmModes[port] == 14 && length == 2 && this.signals[port] != null) {
            ((ILogicSamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, buffer[pos + 1]);
        } else if (this.itmModes[port] == 15 && this.signals[port] != null) {
            ((IBinarySamplesWriter)this.getWriter(this.signals[port])).write(this.t, false, buffer, pos + 1, length - 1);
        }
        this.lastPort = port;
    }

    private void handleHardware(byte[] buffer, int pos, int i) {
    }
}

