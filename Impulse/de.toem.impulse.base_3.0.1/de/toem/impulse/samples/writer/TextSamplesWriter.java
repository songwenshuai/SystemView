/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.writer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ITextSamplesWriter;
import de.toem.impulse.samples.writer.SamplesWriter;
import java.io.UnsupportedEncodingException;

public class TextSamplesWriter
extends SamplesWriter
implements ITextSamplesWriter {
    public TextSamplesWriter(String id, String name, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        super(id, name, processType, signalType, signalDescriptor, domainBase);
    }

    @Override
    public boolean write(long units, boolean tag, String value) {
        int dlength;
        int begin;
        if (this.signalType != ISamples.SignalType.Text || value == null) {
            return false;
        }
        byte[] data = null;
        try {
            if (value != null) {
                data = value.getBytes("UTF-8");
            }
        }
        catch (UnsupportedEncodingException unsupportedEncodingException) {}
        if ((begin = this.beginWrite(units, dlength = data != null ? data.length : 0)) < 0) {
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
        if (data != null) {
            int n = 0;
            while (n < data.length) {
                this.buffer[this.buffered++] = data[n];
                ++n;
            }
        }
        return this.endWrite(units, begin);
    }

    @Override
    public boolean write(long units, boolean tag, String[] value) {
        if (this.signalType != ISamples.SignalType.TextArray || value == null) {
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
        byte[][] data = new byte[count][];
        int dlength = addCount ? TextSamplesWriter.plusLength(count) : 0;
        try {
            int n = 0;
            while (n < count) {
                if (value != null) {
                    data[n] = value[n] != null ? value[n].getBytes("UTF-8") : null;
                }
                dlength += data[n] != null ? data[n].length + TextSamplesWriter.plusLength(data[n].length) : 1;
                ++n;
            }
        }
        catch (UnsupportedEncodingException unsupportedEncodingException) {}
        int begin = this.beginWrite(units, dlength);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)((addCount ? 192 : 64) | 8 | (tag ? 1 : 0));
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
            this.buffered += TextSamplesWriter.plusWrite(this.buffer, this.buffered, count);
        }
        int n = 0;
        while (n < count) {
            this.buffered += TextSamplesWriter.plusWrite(this.buffer, this.buffered, data[n] != null ? data[n].length : 0);
            if (data[n] != null) {
                System.arraycopy(data[n], 0, this.buffer, this.buffered, data[n].length);
                this.buffered += data[n].length;
            }
            ++n;
        }
        return this.endWrite(units, begin);
    }

    @Override
    public boolean writeSample(long units, boolean tag, Object value) {
        if (value == null) {
            return this.write(units, tag);
        }
        if (value instanceof String) {
            return this.write(units, tag, (String)value);
        }
        if (value instanceof String[]) {
            return this.write(units, tag, (String[])value);
        }
        return false;
    }

    @Override
    public boolean writeString(long units, boolean tag, String value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeStringArray(long units, boolean tag, String[] value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeStringArgs(long units, boolean tag, String ... value) {
        return this.writeStringArray(units, tag, value);
    }
}

