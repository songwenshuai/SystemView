/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.writer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IEventSamplesWriter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.writer.SamplesWriter;
import de.toem.impulse.values.Enumeration;
import de.toem.toolkits.core.Utils;

public class EventSamplesWriter
extends SamplesWriter
implements IEventSamplesWriter {
    public EventSamplesWriter(String id, String name, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        super(id, name, processType, signalType, signalDescriptor, domainBase);
    }

    @Override
    public final boolean write(long units, boolean tag) {
        if (this.signalType != ISamples.SignalType.Event && this.signalType != ISamples.SignalType.EventArray) {
            return false;
        }
        int begin = this.beginWrite(units, 0);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)(0x40 | (tag ? 1 : 0));
        this.buffer[this.buffered++] = 0;
        return this.endWrite(units, begin);
    }

    /*
     * Unable to fully structure code
     */
    @Override
    public final boolean write(long units, boolean tag, int value) {
        block8: {
            if (this.signalType != ISamples.SignalType.Event) {
                return false;
            }
            begin = this.beginWrite(units, 4);
            if (begin < 0) {
                return false;
            }
            this.buffer[this.buffered++] = (byte)(128 | (tag != false ? 1 : 0));
            dlength = 0;
            lenPos = this.buffered++;
            if (value == 0) break block8;
            if (value <= 0) ** GOTO lbl23
            while (value != 0) {
                this.buffer[this.buffered++] = (byte)(value & 255);
                ++dlength;
                value >>>= 8;
            }
            if ((this.buffer[this.buffered - 1] & 128) == 0) break block8;
            this.buffer[this.buffered++] = 0;
            ++dlength;
            break block8;
lbl-1000:
            // 1 sources

            {
                this.buffer[this.buffered++] = (byte)(value & 255);
                ++dlength;
                value >>= 8;
lbl23:
                // 2 sources

                ** while (value != -1)
            }
lbl24:
            // 1 sources

            if (dlength == 0 || (this.buffer[this.buffered - 1] & 128) == 0) {
                this.buffer[this.buffered++] = -1;
                ++dlength;
            }
        }
        this.buffer[lenPos] = (byte)dlength;
        return this.endWrite(units, begin);
    }

    /*
     * Unable to fully structure code
     */
    @Override
    public final boolean write(long units, boolean tag, int[] value) {
        if (this.signalType != ISamples.SignalType.EventArray || value == null) {
            return false;
        }
        count = 0;
        addCount = false;
        if (this.signalDescriptor.hasScale()) {
            count = this.signalDescriptor.getScale();
            if (value.length != count) {
                return false;
            }
        } else {
            addCount = true;
            count = value.length;
        }
        if ((begin = this.beginWrite(units, dlength = 5 * count + (addCount != false ? EventSamplesWriter.plusLength(count) : 0))) < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)((addCount != false ? 192 : 128) | (tag != false ? 1 : 0));
        lenPos = this.buffered;
        addLenBytes = dlength >= 15 ? EventSamplesWriter.plusLength(dlength >> 4) : 0;
        this.buffered += 1 + addLenBytes;
        dlength = 0;
        if (addCount) {
            dlength = EventSamplesWriter.plusWrite(this.buffer, this.buffered, count);
            this.buffered += dlength;
        }
        n = 0;
        while (n < count) {
            block18: {
                avalue = n >= value.length ? 0 : value[n];
                ++this.buffered;
                alength = 0;
                if (avalue == 0) break block18;
                if (avalue <= 0) ** GOTO lbl41
                while (avalue != 0) {
                    this.buffer[this.buffered++] = (byte)(avalue & 255);
                    ++alength;
                    avalue >>>= 8;
                }
                if ((this.buffer[this.buffered - 1] & 128) == 0) break block18;
                this.buffer[this.buffered++] = 0;
                ++alength;
                break block18;
lbl-1000:
                // 1 sources

                {
                    this.buffer[this.buffered++] = (byte)(avalue & 255);
                    ++alength;
                    avalue >>= 8;
lbl41:
                    // 2 sources

                    ** while (avalue != -1)
                }
lbl42:
                // 1 sources

                if (alength == 0 || (this.buffer[this.buffered - 1] & 128) == 0) {
                    this.buffer[this.buffered++] = -1;
                    ++alength;
                }
            }
            this.buffer[alenPos] = (byte)alength;
            dlength += 1 + alength;
            ++n;
        }
        if (dlength <= 15 && addLenBytes == 0) {
            this.buffer[lenPos] = (byte)dlength;
        } else {
            usedAddLenBytes = 1;
            s = dlength;
            this.buffer[lenPos++] = (byte)(s & 15 | 16);
            s >>>= 4;
            while (true) {
                if (s <= 127 && usedAddLenBytes == addLenBytes) {
                    this.buffer[lenPos++] = (byte)(s & 127);
                    break;
                }
                this.buffer[lenPos++] = (byte)(s & 127 | 128);
                s >>>= 7;
                ++usedAddLenBytes;
            }
        }
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean write(long units, boolean tag, String value) {
        if (this.signalType != ISamples.SignalType.Event) {
            return false;
        }
        if (value == null) {
            return this.write(units, tag);
        }
        if (this.legend.containsEnum(0, value)) {
            return this.write(units, tag, this.legend.valOfEnum(0, value));
        }
        return this.write(units, tag, this.legend.addEnum(0, value));
    }

    @Override
    public final boolean write(long units, boolean tag, String[] value) {
        if (this.signalType != ISamples.SignalType.EventArray || value == null) {
            return false;
        }
        int[] intArray = new int[value.length];
        int n = 0;
        while (n < intArray.length) {
            intArray[n] = value[n] == null ? 0 : (this.legend.containsEnum(8 + n, value[n]) ? this.legend.valOfEnum(8 + n, value[n]) : this.legend.addEnum(8 + n, value[n]));
            ++n;
        }
        return this.write(units, tag, intArray);
    }

    @Override
    public final boolean write(long units, boolean tag, Object[] value) {
        if (this.signalType != ISamples.SignalType.EventArray || value == null) {
            return false;
        }
        int[] intArray = new int[value.length];
        int n = 0;
        while (n < intArray.length) {
            if (value[n] == null) {
                intArray[n] = 0;
            } else if (value[n] instanceof Number) {
                intArray[n] = ((Number)value[n]).intValue();
            } else if (value[n] instanceof String) {
                intArray[n] = this.legend.containsEnum(8 + n, (String)value[n]) ? this.legend.valOfEnum(8 + n, (String)value[n]) : this.legend.addEnum(8 + n, (String)value[n]);
            } else if (value[n] instanceof Enumeration) {
                if (Utils.isEmpty(((Enumeration)value[n]).label)) {
                    intArray[n] = ((Enumeration)value[n]).value;
                } else if (this.legend.containsEnum(8 + n, ((Enumeration)value[n]).value)) {
                    intArray[n] = ((Enumeration)value[n]).value;
                } else {
                    this.legend.setEnum(8 + n, ((Enumeration)value[n]).label, ((Enumeration)value[n]).value);
                    intArray[n] = ((Enumeration)value[n]).value;
                }
            }
            ++n;
        }
        return this.write(units, tag, intArray);
    }

    @Override
    public boolean write(long units, boolean tag, Enumeration value) {
        if (this.signalType != ISamples.SignalType.Event) {
            return false;
        }
        if (value == null) {
            return this.write(units, tag);
        }
        if (Utils.isEmpty(value.label)) {
            return this.write(units, tag, value.value);
        }
        if (this.legend.containsEnum(0, value.value)) {
            return this.write(units, tag, value.value);
        }
        this.legend.setEnum(0, value.label, value.value);
        return this.write(units, tag, value.value);
    }

    @Override
    public boolean write(long units, boolean tag, Enumeration[] value) {
        if (this.signalType != ISamples.SignalType.EventArray || value == null) {
            return false;
        }
        int[] intArray = new int[value.length];
        int n = 0;
        while (n < intArray.length) {
            if (value[n] == null) {
                intArray[n] = 0;
            } else if (Utils.isEmpty(value[n].label)) {
                intArray[n] = value[n].value;
            } else if (this.legend.containsEnum(8 + n, value[n].value)) {
                intArray[n] = value[n].value;
            } else {
                this.legend.setEnum(8 + n, value[n].label, value[n].value);
                intArray[n] = value[n].value;
            }
            ++n;
        }
        return this.write(units, tag, intArray);
    }

    @Override
    public boolean writeSample(long units, boolean tag, Object value) {
        if (value == null) {
            return this.write(units, tag);
        }
        if (value instanceof Enumeration) {
            return this.write(units, tag, (Enumeration)value);
        }
        if (value instanceof Enumeration[]) {
            return this.write(units, tag, (Enumeration[])value);
        }
        if (value instanceof String) {
            return this.write(units, tag, (String)value);
        }
        if (value instanceof String[]) {
            return this.write(units, tag, (String[])value);
        }
        if (value instanceof Integer) {
            return this.write(units, tag, (Integer)value);
        }
        if (value instanceof int[]) {
            return this.write(units, tag, (int[])value);
        }
        if (value instanceof Object[]) {
            return this.write(units, tag, (Object[])value);
        }
        return false;
    }

    @Override
    public boolean writeInt(long units, boolean tag, int value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeString(long units, boolean tag, String value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeEnum(long units, boolean tag, Enumeration value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeIntArray(long units, boolean tag, int[] value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeIntArgs(long units, boolean tag, int ... value) {
        return this.writeIntArray(units, tag, value);
    }

    @Override
    public boolean writeStringArray(long units, boolean tag, String[] value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeStringArgs(long units, boolean tag, String ... value) {
        return this.writeStringArray(units, tag, value);
    }

    @Override
    public boolean writeEnumArray(long units, boolean tag, Enumeration[] value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeEnumArgs(long units, boolean tag, Enumeration ... value) {
        return this.writeEnumArray(units, tag, value);
    }
}

