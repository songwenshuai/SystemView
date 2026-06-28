/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.writer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IBinarySamplesWriter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.writer.SamplesWriter;

public class BinarySamplesWriter
extends SamplesWriter
implements IBinarySamplesWriter {
    public BinarySamplesWriter(String id, String name, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        super(id, name, processType, signalType, signalDescriptor, domainBase);
    }

    @Override
    public boolean write(long units, boolean tag, byte[] value) {
        return this.write(units, tag, value, 0, value != null ? value.length : 0);
    }

    @Override
    public boolean write(long units, boolean tag, byte[] value, int start, int length) {
        if (this.signalType != ISamples.SignalType.Binary || (value != null ? value.length : 0) < start + length || start < 0) {
            return false;
        }
        int dlength = value != null ? length : 0;
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
        if (value != null) {
            int n = start;
            while (n < start + length) {
                this.buffer[this.buffered++] = value[n];
                ++n;
            }
        }
        return this.endWrite(units, begin);
    }

    @Override
    public boolean writeSample(long units, boolean tag, Object value) {
        if (value == null) {
            return this.write(units, tag);
        }
        if (value instanceof byte[]) {
            return this.write(units, tag, (byte[])value);
        }
        return false;
    }
}

