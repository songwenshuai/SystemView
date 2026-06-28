/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.convert;

import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.convert.ConvertedSample;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.Logic;
import de.toem.impulse.values.Struct;
import java.math.BigDecimal;
import java.math.BigInteger;

public abstract class ConvertedMembers
extends ConvertedSample {
    protected abstract Object valueOf(Object var1);

    protected abstract int defaultFormatOf(Object var1);

    public Logic logicValueOf(Object member) {
        return this.logicValue(this.valueOf(member));
    }

    public int logicStateOf(Object member) {
        return this.logicState(this.valueOf(member));
    }

    public boolean isHighOf(Object member, ILogicDetector detector) {
        return this.isHigh(this.valueOf(member), detector);
    }

    public boolean isLowOf(Object member, ILogicDetector detector) {
        return this.isLow(this.valueOf(member), detector);
    }

    public boolean isHighOf(Object member) {
        return this.isHigh(this.valueOf(member), null);
    }

    public boolean isLowOf(Object member) {
        return this.isLow(this.valueOf(member), null);
    }

    public boolean booleanValueOf(Object member) {
        return this.booleanValue(this.valueOf(member));
    }

    public Number numberValueOf(Object member) {
        return this.numberValue(this.valueOf(member));
    }

    public float floatValueOf(Object member) {
        return this.floatValue(this.valueOf(member));
    }

    public double doubleValueOf(Object member) {
        return this.doubleValue(this.valueOf(member));
    }

    public BigDecimal bigDecimalValueOf(Object member) {
        return this.bigDecimalValue(this.valueOf(member));
    }

    public long longValueOf(Object member) {
        return this.longValue(this.valueOf(member));
    }

    public int intValueOf(Object member) {
        return this.intValue(this.valueOf(member));
    }

    public BigInteger bigIntValueOf(Object member) {
        return this.bigIntValue(this.valueOf(member));
    }

    public String stringValueOf(Object member) {
        return this.stringValue(this.valueOf(member));
    }

    public Enumeration enumValueOf(Object member) {
        return this.enumValue(this.valueOf(member));
    }

    public byte[] bytesValueOf(Object member) {
        return this.bytesValue(this.valueOf(member));
    }

    public Struct structValueOf(Object member) {
        return this.structValue(this.valueOf(member));
    }

    public String formatOf(Object member, int format) {
        if ((format & 0xFFFF) == 65535) {
            format = this.defaultFormatOf(member);
        }
        return this.format(this.valueOf(member), format);
    }

    public String fhexOf(Object member) {
        return this.formatOf(member, 3);
    }

    public String fdecOf(Object member) {
        return this.formatOf(member, 5);
    }

    public String foctOf(Object member) {
        return this.formatOf(member, 2);
    }

    public String fbinOf(Object member) {
        return this.formatOf(member, 1);
    }

    public String fasciiOf(Object member) {
        return this.formatOf(member, 4);
    }
}

