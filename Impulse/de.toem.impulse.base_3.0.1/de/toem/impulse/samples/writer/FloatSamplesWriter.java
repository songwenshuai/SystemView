/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.writer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IFloatSamplesWriter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.writer.SamplesWriter;
import java.math.BigDecimal;

public class FloatSamplesWriter
extends SamplesWriter
implements IFloatSamplesWriter {
    public FloatSamplesWriter(String id, String name, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        super(id, name, processType, signalType, signalDescriptor, domainBase);
    }

    @Override
    public final boolean write(long units, boolean tag, float value) {
        if (this.signalType != ISamples.SignalType.Float || Float.isNaN(value)) {
            return false;
        }
        int begin = this.beginWrite(units, 4);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)(0x48 | (tag ? 1 : 0));
        this.buffer[this.buffered++] = 4;
        int intdata = Float.floatToIntBits(value);
        this.buffer[this.buffered++] = (byte)(intdata & 0xFF);
        this.buffer[this.buffered++] = (byte)((intdata >>>= 8) & 0xFF);
        this.buffer[this.buffered++] = (byte)((intdata >>>= 8) & 0xFF);
        this.buffer[this.buffered++] = (byte)((intdata >>>= 8) & 0xFF);
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean write(long units, double value) {
        return this.write(units, false, value);
    }

    @Override
    public final boolean write(long units, boolean tag, double value) {
        if (this.signalDescriptor.getAccuracy() == 1) {
            return this.write(units, tag, (float)value);
        }
        if (this.signalType != ISamples.SignalType.Float || Double.isNaN(value)) {
            return false;
        }
        int begin = this.beginWrite(units, 8);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)(0x50 | (tag ? 1 : 0));
        this.buffer[this.buffered++] = 8;
        long longdata = Double.doubleToLongBits(value);
        this.buffer[this.buffered++] = (byte)(longdata & 0xFFL);
        this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
        this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
        this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
        this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
        this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
        this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
        this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean write(long units, boolean tag, BigDecimal value) {
        if (this.signalType != ISamples.SignalType.Float || value == null) {
            return false;
        }
        if (this.signalDescriptor.getAccuracy() == 1) {
            return this.write(units, tag, value.floatValue());
        }
        if (this.signalDescriptor.getAccuracy() == 2) {
            return this.write(units, tag, value.doubleValue());
        }
        if (this.signalType != ISamples.SignalType.Float) {
            return false;
        }
        byte[] data = value.unscaledValue().toByteArray();
        int bdscale = value.scale();
        if (bdscale > Short.MAX_VALUE || bdscale < Short.MIN_VALUE) {
            return false;
        }
        int dlength = data.length + 2;
        int begin = this.beginWrite(units, dlength);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)(0x58 | (tag ? 1 : 0));
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
        this.buffer[this.buffered++] = (byte)(bdscale & 0xFF);
        this.buffer[this.buffered++] = (byte)((bdscale >>= 8) & 0xFF);
        int n = data.length - 1;
        while (n >= 0) {
            this.buffer[this.buffered++] = data[n];
            --n;
        }
        return this.endWrite(units, begin);
    }

    @Override
    public boolean write(long units, boolean tag, Number value) {
        if (this.signalType != ISamples.SignalType.Float || value == null) {
            return false;
        }
        if (value instanceof Double) {
            return this.write(units, tag, value.doubleValue());
        }
        if (value instanceof BigDecimal) {
            return this.write(units, tag, (BigDecimal)value);
        }
        return this.write(units, tag, value.floatValue());
    }

    @Override
    public final boolean write(long units, boolean tag, float[] value) {
        int dlength;
        int begin;
        if (this.signalType != ISamples.SignalType.FloatArray || value == null) {
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
        if ((begin = this.beginWrite(units, dlength = 4 * count + (addCount ? FloatSamplesWriter.plusLength(count) : 0))) < 0) {
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
            this.buffered += FloatSamplesWriter.plusWrite(this.buffer, this.buffered, count);
        }
        int n = 0;
        while (n < count) {
            int intdata = Float.floatToIntBits(value[n]);
            this.buffer[this.buffered++] = (byte)(intdata & 0xFF);
            this.buffer[this.buffered++] = (byte)((intdata >>>= 8) & 0xFF);
            this.buffer[this.buffered++] = (byte)((intdata >>>= 8) & 0xFF);
            this.buffer[this.buffered++] = (byte)((intdata >>>= 8) & 0xFF);
            ++n;
        }
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean write(long units, boolean tag, double[] value) {
        int dlength;
        int begin;
        if (this.signalType != ISamples.SignalType.FloatArray || value == null) {
            return false;
        }
        if (this.signalDescriptor.getAccuracy() == 1) {
            float[] fvalue = new float[value.length];
            int n = 0;
            while (n < value.length) {
                fvalue[n] = (float)value[n];
                ++n;
            }
            return this.write(units, tag, fvalue);
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
        if ((begin = this.beginWrite(units, dlength = 8 * count + (addCount ? FloatSamplesWriter.plusLength(count) : 0))) < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)((addCount ? 192 : 64) | 0x10 | (tag ? 1 : 0));
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
            this.buffered += FloatSamplesWriter.plusWrite(this.buffer, this.buffered, count);
        }
        int n = 0;
        while (n < count) {
            long longdata = Double.doubleToLongBits(value[n]);
            this.buffer[this.buffered++] = (byte)(longdata & 0xFFL);
            this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
            this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
            this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
            this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
            this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
            this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
            this.buffer[this.buffered++] = (byte)((longdata >>>= 8) & 0xFFL);
            ++n;
        }
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean write(long units, boolean tag, BigDecimal[] value) {
        if (this.signalType != ISamples.SignalType.FloatArray || value == null) {
            return false;
        }
        if (this.signalDescriptor.getAccuracy() == 1) {
            float[] fvalue = new float[value.length];
            int n = 0;
            while (n < value.length) {
                fvalue[n] = value != null ? value[n].floatValue() : 0.0f;
                ++n;
            }
            return this.write(units, tag, fvalue);
        }
        if (this.signalDescriptor.getAccuracy() == 2) {
            double[] dvalue = new double[value.length];
            int n = 0;
            while (n < value.length) {
                dvalue[n] = value != null ? value[n].doubleValue() : 0.0;
                ++n;
            }
            return this.write(units, tag, dvalue);
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
        int dlength = addCount ? FloatSamplesWriter.plusLength(count) : 0;
        byte[][] adata = new byte[count][];
        int[] abdscale = new int[count];
        int n = 0;
        while (n < count) {
            if (value[n] != null) {
                adata[n] = value[n].unscaledValue().toByteArray();
                abdscale[n] = value[n].scale();
                if (abdscale[n] > Short.MAX_VALUE || abdscale[n] < Short.MIN_VALUE) {
                    return false;
                }
                dlength += adata[n] != null ? adata[n].length + 2 + FloatSamplesWriter.plusLength(adata[n].length + 2) : 1;
            } else {
                ++dlength;
            }
            ++n;
        }
        int begin = this.beginWrite(units, dlength);
        if (begin < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)((addCount ? 192 : 64) | 0x10 | (tag ? 1 : 0));
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
            this.buffered += FloatSamplesWriter.plusWrite(this.buffer, this.buffered, count);
        }
        int n2 = 0;
        while (n2 < count) {
            byte[] data = adata[n2];
            int bdscale = abdscale[n2];
            this.buffered += FloatSamplesWriter.plusWrite(this.buffer, this.buffered, data != null ? data.length + 2 : 0);
            if (data != null) {
                this.buffer[this.buffered++] = (byte)(bdscale & 0xFF);
                this.buffer[this.buffered++] = (byte)((bdscale >>= 8) & 0xFF);
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
    public boolean writeSample(long units, boolean tag, Object value) {
        if (value == null) {
            return this.write(units, tag);
        }
        if (value instanceof Number) {
            return this.write(units, tag, (Number)value);
        }
        if (value instanceof float[]) {
            return this.write(units, tag, (float[])value);
        }
        if (value instanceof double[]) {
            return this.write(units, tag, (double[])value);
        }
        if (value instanceof BigDecimal[]) {
            return this.write(units, tag, (BigDecimal[])value);
        }
        return false;
    }

    @Override
    public boolean writeFloat(long units, boolean tag, float value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeDouble(long units, boolean tag, double value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeBig(long units, boolean tag, BigDecimal value) {
        return this.write(units, tag, value);
    }

    @Override
    public final boolean writeFloatArray(long units, boolean tag, float[] value) {
        return this.write(units, tag, value);
    }

    @Override
    public final boolean writeDoubleArray(long units, boolean tag, double[] value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeFloatArgs(long units, boolean tag, float ... value) {
        return this.writeFloatArray(units, tag, value);
    }

    @Override
    public boolean writeDoubleArgs(long units, boolean tag, double ... value) {
        return this.writeDoubleArray(units, tag, value);
    }

    @Override
    public boolean writeBigArgs(long units, boolean tag, BigDecimal ... value) {
        return this.writeBigArray(units, tag, value);
    }

    @Override
    public final boolean writeBigArray(long units, boolean tag, BigDecimal[] value) {
        return this.write(units, tag, value);
    }
}

