/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.convert;

import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.IReadableGroup;
import de.toem.impulse.samples.IReadableSample;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.Logic;
import de.toem.impulse.values.Struct;
import java.math.BigDecimal;
import java.math.BigInteger;

public abstract class ConvertedSample
extends SampleConverter
implements ISample {
    public final boolean isSample() {
        return this instanceof IReadableSample;
    }

    public final boolean isGroup() {
        return this instanceof IReadableGroup;
    }

    protected abstract Object val();

    protected abstract int defaultFormat();

    public Logic logicValue() {
        return this.logicValue(this.val());
    }

    public int logicState() {
        return this.logicState(this.val());
    }

    public boolean isHigh(ILogicDetector detector) {
        return this.isHigh(this.val(), detector);
    }

    public boolean isLow(ILogicDetector detector) {
        return this.isLow(this.val(), detector);
    }

    public boolean isHigh() {
        return this.isHigh(this.val(), null);
    }

    public boolean isLow() {
        return this.isLow(this.val(), null);
    }

    public Number numberValue() {
        return this.numberValue(this.val());
    }

    public float floatValue() {
        return this.floatValue(this.val());
    }

    public double doubleValue() {
        return this.doubleValue(this.val());
    }

    public BigDecimal bigDecimalValue() {
        return this.bigDecimalValue(this.val());
    }

    public long longValue() {
        return this.longValue(this.val());
    }

    public int intValue() {
        return this.intValue(this.val());
    }

    public BigInteger bigIntValue() {
        return this.bigIntValue(this.val());
    }

    public String stringValue() {
        return this.stringValue(this.val());
    }

    public Enumeration enumValue() {
        return this.enumValue(this.val());
    }

    public byte[] bytesValue() {
        return this.bytesValue(this.val());
    }

    public Struct structValue() {
        return this.structValue(this.val());
    }

    public String format(int format) {
        if ((format & 0xFFFF) == 65535) {
            format = format & 0xFFFF0000 | this.defaultFormat() & 0xFFFF;
        }
        if ((format & 0xFFFF0000) == -65536) {
            format = format & 0xFFFF | this.defaultFormat() & 0xFFFF0000;
        }
        return this.format(this.val(), format);
    }

    @Override
    public String format(Object object, int format) {
        if ((format & 0xFFFF) == 65535) {
            format = format & 0xFFFF0000 | this.defaultFormat() & 0xFFFF;
        }
        if ((format & 0xFFFF0000) == -65536) {
            format = format & 0xFFFF | this.defaultFormat() & 0xFFFF0000;
        }
        return super.format(object, format);
    }

    public String fhex() {
        return this.format(3);
    }

    public String fdec() {
        return this.format(5);
    }

    public String foct() {
        return this.format(2);
    }

    public String fbin() {
        return this.format(1);
    }

    public String fascii() {
        return this.format(4);
    }
}

