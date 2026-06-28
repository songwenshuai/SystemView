/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.axis;

import de.toem.impulse.axis.IValueAxis;
import de.toem.toolkits.core.Utils;

public class ValueAxis
implements IValueAxis {
    private int type;
    private String unit;
    private double min;
    private double max;
    private double lmin;
    private double lmax;
    protected static final String CLAZZ = "ValueAxis";

    public ValueAxis(int type, String unit, double min, double max) {
        this.type = type;
        this.unit = unit;
        this.min = min;
        this.max = max;
        if (type == 1) {
            this.lmin = Math.log(min);
            this.lmax = Math.log(max);
        }
    }

    protected ValueAxis(String[] value) {
        if (value != null && value.length > 6) {
            this.type = Utils.parseInt(value[1], 0);
            this.unit = value[2];
            this.min = Utils.parseDouble(value[3], 0.0);
            this.max = Utils.parseDouble(value[4], 0.0);
            this.lmin = Utils.parseDouble(value[5], 0.0);
            this.lmax = Utils.parseDouble(value[6], 0.0);
        }
    }

    @Override
    public String getClazz() {
        return CLAZZ;
    }

    public String toString() {
        return String.valueOf(this.getClazz()) + "/" + this.type + "/" + this.unit + "/" + this.min + "/" + this.max + "/" + this.lmin + "/" + this.lmax;
    }

    @Override
    public double getMin() {
        return this.min;
    }

    @Override
    public double getMax() {
        return this.max;
    }

    @Override
    public int getType() {
        return this.type;
    }

    @Override
    public String getUnit() {
        return this.unit;
    }

    public boolean equals(Object obj) {
        if (obj instanceof ValueAxis) {
            ValueAxis that = (ValueAxis)obj;
            return that.min == this.min && that.max == this.max && that.type == this.type && Utils.equals(that.unit, this.unit);
        }
        return false;
    }

    @Override
    public double norm(double value) {
        if (this.type == 0) {
            return (value - this.min) / (this.max - this.min);
        }
        return value > 0.0 ? (Math.log(value) - this.lmin) / (this.lmax - this.lmin) : -1.0;
    }
}

