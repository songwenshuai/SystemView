/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.convert;

import de.toem.impulse.values.Logic;

public class SampleConverterConfiguration {
    public static final int LINEARIZE = 1;
    public static final int LOGIC_MASK = 6;
    public static final int LOGIC_SIGNED = 2;
    public static final int LOGIC_754 = 4;
    protected short flags;
    public float factor;
    public float base;
    public int decDigits;

    static Number logicToNumber(Logic logic, SampleConverterConfiguration config) {
        if (config == null) {
            return logic.toNumber(false);
        }
        if ((config.flags & 6) == 0) {
            return SampleConverterConfiguration.linearize(logic.toNumber(false), config);
        }
        if ((config.flags & 6) == 2) {
            return SampleConverterConfiguration.linearize(logic.toNumber(true), config);
        }
        if ((config.flags & 6) == 4 && logic.getBitWidth() > 32) {
            return SampleConverterConfiguration.linearize(Double.longBitsToDouble(logic.longValue()), config);
        }
        if ((config.flags & 6) == 4) {
            return SampleConverterConfiguration.linearize(Float.valueOf(Float.intBitsToFloat(logic.intValue())), config);
        }
        return null;
    }

    static Number linearize(Number value, SampleConverterConfiguration config) {
        if (value != null && config != null && (config.flags & 1) != 0) {
            return value.doubleValue() * (double)config.factor + (double)config.base;
        }
        return value;
    }
}

