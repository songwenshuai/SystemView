/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.convert.SampleConverter;

public class Enumeration
implements ISample {
    public int enumeration;
    public int value;
    public String label;

    public Enumeration(int enumeration, String label, int value) {
        this.enumeration = enumeration;
        this.value = value;
        this.label = label;
    }

    public Enumeration(int enumeration, int value) {
        this.enumeration = enumeration;
        this.value = value;
    }

    public String toString(int format) {
        switch (format & 0xFFFF) {
            case 6: {
                return this.label != null ? this.label : null;
            }
            case -1: 
            case 7: {
                return String.valueOf(this.label != null ? this.label : "") + "{" + String.valueOf(this.value) + "}";
            }
        }
        return SampleConverter.instance.format(this.value, format, false);
    }

    public String toString() {
        return this.toString(7);
    }

    public double doubleValue() {
        return this.value;
    }

    public float floatValue() {
        return this.value;
    }

    public int intValue() {
        return this.value;
    }

    public long longValue() {
        return this.value;
    }
}

