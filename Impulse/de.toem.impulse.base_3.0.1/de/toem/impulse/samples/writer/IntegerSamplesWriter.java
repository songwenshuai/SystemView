/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.writer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IIntegerSamplesWriter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.writer.SamplesWriter;
import de.toem.impulse.values.Logic;
import java.math.BigInteger;

public class IntegerSamplesWriter
extends SamplesWriter
implements IIntegerSamplesWriter {
    public IntegerSamplesWriter(String id, String name, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        super(id, name, processType, signalType, signalDescriptor, domainBase);
    }

    /*
     * Unable to fully structure code
     */
    @Override
    public boolean write(long units, boolean tag, int value) {
        block8: {
            if (this.signalType != ISamples.SignalType.Integer) {
                return false;
            }
            begin = this.beginWrite(units, 4);
            if (begin < 0) {
                return false;
            }
            this.buffer[this.buffered++] = (byte)(64 | (tag != false ? 1 : 0));
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
    public boolean write(long units, boolean tag, long value) {
        block8: {
            if (this.signalType != ISamples.SignalType.Integer) {
                return false;
            }
            begin = this.beginWrite(units, 8);
            if (begin < 0) {
                return false;
            }
            this.buffer[this.buffered++] = (byte)(64 | (tag != false ? 1 : 0));
            dlength = 0;
            lenPos = this.buffered++;
            if (value == 0L) break block8;
            if (value <= 0L) ** GOTO lbl23
            while (value != 0L) {
                this.buffer[this.buffered++] = (byte)(value & 255L);
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
                this.buffer[this.buffered++] = (byte)(value & 255L);
                ++dlength;
                value >>= 8;
lbl23:
                // 2 sources

                ** while (value != -1L)
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

    @Override
    public boolean write(long units, boolean tag, BigInteger value) {
        if (this.signalType != ISamples.SignalType.Integer || value == null) {
            return false;
        }
        byte[] data = value.toByteArray();
        int dlength = data.length;
        int begin = this.beginWrite(units, dlength);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)(0x40 | (tag ? 1 : 0));
        if (dlength <= 15) {
            this.buffer[this.buffered++] = (byte)dlength;
        } else {
            int s = dlength;
            this.buffer[this.buffered++] = (byte)(s & 0xF | 0x10);
            s >>>= 4;
            while (true) {
                if (s <= 127) {
                    this.buffer[this.buffered++] = (byte)(s & 0x7F);
                    break;
                }
                this.buffer[this.buffered++] = (byte)(s & 0x7F | 0x80);
                s >>>= 7;
            }
        }
        int n = data.length - 1;
        while (n >= 0) {
            this.buffer[this.buffered++] = data[n];
            --n;
        }
        return this.endWrite(units, begin);
    }

    /*
     * Unable to fully structure code
     */
    @Override
    public final boolean write(long units, boolean tag, int[] value) {
        if (this.signalType != ISamples.SignalType.IntegerArray || value == null) {
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
        if ((begin = this.beginWrite(units, dlength = 5 * count + (addCount != false ? IntegerSamplesWriter.plusLength(count) : 0))) < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)((addCount != false ? 192 : 64) | 8 | (tag != false ? 1 : 0));
        lenPos = this.buffered;
        addLenBytes = dlength >= 15 ? IntegerSamplesWriter.plusLength(dlength >> 4) : 0;
        this.buffered += 1 + addLenBytes;
        dlength = 0;
        if (addCount) {
            dlength = IntegerSamplesWriter.plusWrite(this.buffer, this.buffered, count);
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

    /*
     * Unable to fully structure code
     */
    @Override
    public final boolean write(long units, boolean tag, long[] value) {
        if (this.signalType != ISamples.SignalType.IntegerArray || value == null) {
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
        if ((begin = this.beginWrite(units, dlength = 9 * count + (addCount != false ? IntegerSamplesWriter.plusLength(count) : 0))) < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)((addCount != false ? 192 : 64) | 16 | (tag != false ? 1 : 0));
        lenPos = this.buffered;
        addLenBytes = dlength >= 15 ? IntegerSamplesWriter.plusLength(dlength >> 4) : 0;
        this.buffered += 1 + addLenBytes;
        dlength = 0;
        if (addCount) {
            dlength = IntegerSamplesWriter.plusWrite(this.buffer, this.buffered, count);
            this.buffered += dlength;
        }
        n = 0;
        while (n < count) {
            block18: {
                avalue = n >= value.length ? 0L : value[n];
                ++this.buffered;
                alength = 0;
                if (avalue == 0L) break block18;
                if (avalue <= 0L) ** GOTO lbl41
                while (avalue != 0L) {
                    this.buffer[this.buffered++] = (byte)(avalue & 255L);
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
                    this.buffer[this.buffered++] = (byte)(avalue & 255L);
                    ++alength;
                    avalue >>= 8;
lbl41:
                    // 2 sources

                    ** while (avalue != -1L)
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
        this.buffer[lenPos] = (byte)dlength;
        if (dlength <= 15 && addLenBytes == 0) {
            this.buffer[this.buffered++] = (byte)dlength;
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
    public final boolean write(long units, boolean tag, BigInteger[] value) {
        if (this.signalType != ISamples.SignalType.IntegerArray || value == null) {
            return false;
        }
        int count = 0;
        boolean addCount = false;
        if (this.signalDescriptor.hasScale()) {
            count = this.signalDescriptor.getScale();
            if (value.length != count) {
                return false;
            }
        } else {
            addCount = true;
            count = value.length;
        }
        int dlength = addCount ? IntegerSamplesWriter.plusLength(count) : 0;
        byte[][] adata = new byte[count][];
        int n = 0;
        while (n < count) {
            if (value[n] != null) {
                adata[n] = value[n].toByteArray();
                dlength += adata[n] != null ? adata[n].length + IntegerSamplesWriter.plusLength(adata[n].length) : 1;
            } else {
                ++dlength;
            }
            ++n;
        }
        int begin = this.beginWrite(units, dlength);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)((addCount ? 192 : 64) | 0x18 | (tag ? 1 : 0));
        if (dlength <= 15) {
            this.buffer[this.buffered++] = (byte)dlength;
        } else {
            int s = dlength;
            this.buffer[this.buffered++] = (byte)(s & 0xF | 0x10);
            s >>>= 4;
            while (true) {
                if (s <= 127) {
                    this.buffer[this.buffered++] = (byte)(s & 0x7F);
                    break;
                }
                this.buffer[this.buffered++] = (byte)(s & 0x7F | 0x80);
                s >>>= 7;
            }
        }
        if (addCount) {
            this.buffered += IntegerSamplesWriter.plusWrite(this.buffer, this.buffered, count);
        }
        int n2 = 0;
        while (n2 < count) {
            byte[] data = adata[n2];
            this.buffered += IntegerSamplesWriter.plusWrite(this.buffer, this.buffered, data != null ? data.length : 0);
            if (data != null) {
                int m = data.length - 1;
                while (m >= 0) {
                    this.buffer[this.buffered++] = data[m];
                    --m;
                }
            }
            ++n2;
        }
        return this.endWrite(units, begin);
    }

    @Override
    public boolean write(long units, boolean tag, Number value) {
        if (this.signalType != ISamples.SignalType.Integer || value == null) {
            return false;
        }
        if (value instanceof Long) {
            return this.write(units, tag, value.longValue());
        }
        if (value instanceof BigInteger) {
            return this.write(units, tag, (BigInteger)value);
        }
        if (value instanceof Logic) {
            return this.write(units, tag, ((Logic)value).toNumber(false));
        }
        return this.write(units, tag, value.intValue());
    }

    @Override
    public boolean writeSample(long units, boolean tag, Object value) {
        if (value == null) {
            return this.write(units, tag);
        }
        if (value instanceof Number) {
            return this.write(units, tag, (Number)value);
        }
        if (value instanceof int[]) {
            return this.write(units, tag, (int[])value);
        }
        if (value instanceof long[]) {
            return this.write(units, tag, (long[])value);
        }
        if (value instanceof BigInteger[]) {
            return this.write(units, tag, (BigInteger[])value);
        }
        return false;
    }

    @Override
    public boolean writeInt(long units, boolean tag, int value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeLong(long units, boolean tag, long value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeBig(long units, boolean tag, BigInteger value) {
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
    public boolean writeLongArray(long units, boolean tag, long[] value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeLongArgs(long units, boolean tag, long ... value) {
        return this.writeLongArray(units, tag, value);
    }

    @Override
    public boolean writeBigArray(long units, boolean tag, BigInteger[] value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeBigArgs(long units, boolean tag, BigInteger ... value) {
        return this.writeBigArray(units, tag, value);
    }
}

