/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.domain;

import de.toem.impulse.domain.IDomainBase;
import java.math.BigDecimal;

public enum FloatBase implements IDomainBase
{
    yX(0),
    yX10(0),
    yX100(0),
    zX(0),
    zX10(0),
    zX100(0),
    aX(0),
    aX10(0),
    aX100(0),
    fX(0),
    fX10(0),
    fX100(0),
    pX(2),
    pX10(1),
    pX100(1),
    nX(2),
    nX10(1),
    nX100(1),
    uX(2),
    uX10(1),
    uX100(1),
    mX(2),
    mX10(1),
    mX100(1),
    X(2),
    X10(1),
    X100(1),
    kX(2),
    kX10(1),
    kX100(1),
    MX(2),
    MX10(1),
    MX100(1),
    GX(2),
    GX10(1),
    GX100(1),
    TX(2),
    TX10(0),
    TX100(0),
    PX(0),
    PX10(0),
    PX100(0),
    EX(0),
    EX10(0),
    EX100(0),
    ZX(0),
    ZX10(0),
    ZX100(0),
    YX(0);

    public static final String CLAZZ = "Float";
    private int userLevel;
    public static final String DOMAIN_LABEL = "Value";

    private FloatBase(int userLevel) {
        this.userLevel = userLevel;
    }

    @Override
    public String getClazz() {
        return CLAZZ;
    }

    @Override
    public int userLevel() {
        return this.userLevel;
    }

    @Override
    public void setPreferred(boolean preferred) {
    }

    @Override
    public boolean isPreferred() {
        return false;
    }

    public String toString() {
        return super.toString();
    }

    @Override
    public String toString(long units) {
        return this.toString(units, 14);
    }

    @Override
    public String toString(long units, int style) {
        return String.valueOf(String.valueOf(this.toValue() * (double)units)) + X.toString();
    }

    @Override
    public BigDecimal getQuantum() {
        return BigDecimal.ONE.scaleByPowerOfTen(this.ordinal() - X.ordinal());
    }

    @Override
    public double toCommonBase() {
        return Math.pow(10.0, this.ordinal() - X.ordinal());
    }

    @Override
    public double toCommonBase(long units) {
        return Math.pow(10.0, this.ordinal() - X.ordinal()) * (double)units;
    }

    public double toValue() {
        return Math.pow(10.0, this.ordinal() - X.ordinal());
    }

    public static FloatBase parse(String text) {
        if (text == null) {
            return null;
        }
        FloatBase[] floatBaseArray = FloatBase.values();
        int n = floatBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            FloatBase t = floatBaseArray[n2];
            if (text.equals(t.toString())) {
                return t;
            }
            ++n2;
        }
        return null;
    }

    public static FloatBase bestFit(double diff) {
        FloatBase[] floatBaseArray = FloatBase.values();
        int n = floatBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            FloatBase b = floatBaseArray[n2];
            if (b.getQuantum().doubleValue() > diff) {
                if (b.ordinal() < 2) break;
                return FloatBase.valueOf(b.ordinal() - 2);
            }
            ++n2;
        }
        return FloatBase.valueOf(0);
    }

    @Override
    public boolean isCompatible(IDomainBase base) {
        return base instanceof FloatBase;
    }

    @Override
    public long getConversionFactor(IDomainBase base) {
        if (base == this) {
            return 1L;
        }
        if (base instanceof FloatBase) {
            FloatBase that = (FloatBase)base;
            if (that.ordinal() < this.ordinal()) {
                long factor = 1L;
                int diff = this.ordinal() - that.ordinal();
                while (diff > 0) {
                    factor *= 10L;
                    --diff;
                }
                return factor;
            }
            long divider = 1L;
            int diff = that.ordinal() - this.ordinal();
            while (diff > 0) {
                divider *= 10L;
                --diff;
            }
            return -divider;
        }
        return 0L;
    }

    @Override
    public long convertTo(IDomainBase base, long unit) {
        if (base == this) {
            return unit;
        }
        if (base instanceof FloatBase) {
            if (unit == 0L) {
                return 0L;
            }
            FloatBase that = (FloatBase)base;
            if (that.ordinal() < this.ordinal()) {
                long factor = 1L;
                int diff = this.ordinal() - that.ordinal();
                while (diff > 0) {
                    factor *= 10L;
                    --diff;
                }
                unit *= factor;
            } else {
                long divider = 1L;
                int diff = that.ordinal() - this.ordinal();
                while (diff > 0) {
                    divider *= 10L;
                    --diff;
                }
                unit /= divider;
            }
            return unit;
        }
        return unit;
    }

    @Override
    public String getDomainLabel() {
        return DOMAIN_LABEL;
    }

    @Override
    public long parseUnits(String value) throws NumberFormatException {
        FloatBase t = X;
        int comma = (value = value.trim()).lastIndexOf(46);
        comma = comma >= 0 ? value.length() - comma - 1 : 0;
        value = value.replaceAll("\\.", "");
        long units = Long.parseLong(value);
        if (t.ordinal() < comma) {
            throw new NumberFormatException("Doesn't fit");
        }
        t = FloatBase.values()[t.ordinal() - comma];
        return t.convertTo(this, units);
    }

    @Override
    public boolean isFinerThan(IDomainBase base) {
        return base instanceof FloatBase && ((FloatBase)base).ordinal() < this.ordinal();
    }

    @Override
    public IDomainBase getDomainBase(IDomainBase samplesBase) {
        return this;
    }

    public static FloatBase valueOf(String string) {
        return Enum.valueOf(FloatBase.class, string);
    }
}

