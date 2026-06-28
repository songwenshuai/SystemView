/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.domain;

import de.toem.impulse.domain.IDomainBase;
import java.math.BigDecimal;

public enum AmpsBase implements IDomainBase
{
    yA(0),
    yA10(0),
    yA100(0),
    zA(0),
    zA10(0),
    zA100(0),
    aA(0),
    aA10(0),
    aA100(0),
    fA(0),
    fA10(0),
    fA100(0),
    pA(0),
    pA10(0),
    pA100(0),
    nA(0),
    nA10(0),
    nA100(0),
    uA(2),
    uA10(1),
    uA100(1),
    mA(2),
    mA10(1),
    mA100(1),
    A(2),
    A10(1),
    A100(1),
    kA(2),
    kA10(0),
    kA100(0),
    MA(0),
    MA10(0),
    MA100(0),
    GA(0),
    GA10(0),
    GA100(0),
    TA(0),
    TA10(0),
    TA100(0),
    PA(0),
    PA10(0),
    PA100(0),
    EA(0),
    EA10(0),
    EA100(0),
    ZA(0),
    ZA10(0),
    ZA100(0),
    YA(0);

    public static final String CLAZZ = "Amps";
    private int userLevel;
    private boolean isPreferred;
    private AmpsBase myPreferred;
    public static final String DOMAIN_LABEL = "Amps";

    private AmpsBase(int userLevel) {
        this.userLevel = userLevel;
    }

    @Override
    public String getClazz() {
        return "Amps";
    }

    @Override
    public int userLevel() {
        return this.userLevel;
    }

    public static void resetPreferred() {
        AmpsBase[] ampsBaseArray = AmpsBase.values();
        int n = ampsBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            AmpsBase b = ampsBaseArray[n2];
            b.myPreferred = null;
            b.isPreferred = false;
            ++n2;
        }
    }

    @Override
    public void setPreferred(boolean preferred) {
        this.isPreferred = preferred;
    }

    @Override
    public boolean isPreferred() {
        return this.isPreferred;
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
        int n;
        if ((style & 1) != 0 && units == 0L) {
            return "0";
        }
        StringBuilder builder = new StringBuilder();
        builder.append(units);
        if ((style & 4) != 0 && this.myPreferred == null) {
            int dist = Integer.MAX_VALUE;
            AmpsBase[] ampsBaseArray = AmpsBase.values();
            int n2 = ampsBaseArray.length;
            int n3 = 0;
            while (n3 < n2) {
                AmpsBase b = ampsBaseArray[n3];
                if (b.isPreferred() && Math.abs(b.ordinal() - this.ordinal()) < dist) {
                    dist = Math.abs(b.ordinal() - this.ordinal());
                    this.myPreferred = b;
                }
                ++n3;
            }
            if (this.myPreferred == null) {
                this.myPreferred = this;
            }
        }
        if ((style & 4) != 0 && this.myPreferred != this) {
            int delta = this.myPreferred.ordinal() - this.ordinal();
            if (delta > 0) {
                int ulen = builder.length();
                if (delta < ulen) {
                    builder.insert(ulen - delta, '.');
                } else {
                    n = 0;
                    while (n < delta - ulen) {
                        builder.insert(0, '0');
                        ++n;
                    }
                    builder.insert(0, '.');
                    builder.insert(0, '0');
                }
            } else if (delta < 0) {
                int n4 = 0;
                while (n4 < -delta) {
                    builder.append('0');
                    ++n4;
                }
            }
            builder.append(this.myPreferred.toString());
            return builder.toString();
        }
        int base = this.ordinal() / 3 * 3;
        int zeros = this.ordinal() % 3;
        if ((style & 2) != 0) {
            int azeros = zeros;
            long aval = units;
            while (aval != 0L && aval / 10L * 10L == aval) {
                aval /= 10L;
                ++azeros;
            }
            while (azeros >= 3 && base < AmpsBase.values().length - 3) {
                azeros -= 3;
                zeros -= 3;
                base += 3;
            }
        }
        if (zeros > 0) {
            n = 0;
            while (n < zeros) {
                builder.append('0');
                ++n;
            }
        } else if (zeros < 0) {
            builder.setLength(builder.length() + zeros);
        }
        builder.append(AmpsBase.values()[base].toString());
        return builder.toString();
    }

    @Override
    public BigDecimal getQuantum() {
        return BigDecimal.ONE.scaleByPowerOfTen(this.ordinal() - A.ordinal());
    }

    public double toAmps() {
        return this.toCommonBase();
    }

    @Override
    public double toCommonBase() {
        return Math.pow(10.0, this.ordinal() - A.ordinal());
    }

    @Override
    public double toCommonBase(long units) {
        return Math.pow(10.0, this.ordinal() - A.ordinal()) * (double)units;
    }

    public static AmpsBase parse(String text) {
        if (text == null) {
            return null;
        }
        AmpsBase[] ampsBaseArray = AmpsBase.values();
        int n = ampsBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            AmpsBase t = ampsBaseArray[n2];
            if (text.equals(t.toString())) {
                return t;
            }
            ++n2;
        }
        return null;
    }

    public static AmpsBase bestFit(double diff) {
        AmpsBase[] ampsBaseArray = AmpsBase.values();
        int n = ampsBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            AmpsBase b = ampsBaseArray[n2];
            if (b.getQuantum().doubleValue() > diff) {
                if (b.ordinal() < 2) break;
                return AmpsBase.valueOf(b.ordinal() - 2);
            }
            ++n2;
        }
        return AmpsBase.valueOf(0);
    }

    @Override
    public boolean isCompatible(IDomainBase base) {
        return base instanceof AmpsBase;
    }

    @Override
    public long getConversionFactor(IDomainBase base) {
        if (base == this) {
            return 1L;
        }
        if (base instanceof AmpsBase) {
            AmpsBase that = (AmpsBase)base;
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
        if (base instanceof AmpsBase) {
            if (unit == 0L) {
                return 0L;
            }
            AmpsBase that = (AmpsBase)base;
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
        return "Amps";
    }

    @Override
    public long parseUnits(String value) throws NumberFormatException {
        if (Character.isDigit((value = value.toLowerCase().trim()).charAt(value.length() - 1))) {
            return Long.parseLong(value);
        }
        AmpsBase[] ampsBaseArray = AmpsBase.values();
        int n = ampsBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            AmpsBase t = ampsBaseArray[n2];
            if (value.endsWith(t.toString().toLowerCase())) {
                int comma = (value = value.substring(0, value.length() - t.toString().length()).trim()).lastIndexOf(46);
                comma = comma >= 0 ? value.length() - comma - 1 : 0;
                value = value.replaceAll("\\.", "");
                long units = Long.parseLong(value);
                if (t.ordinal() < comma) {
                    throw new NumberFormatException("Doesn't fit");
                }
                t = AmpsBase.values()[t.ordinal() - comma];
                return t.convertTo(this, units);
            }
            ++n2;
        }
        throw new NumberFormatException("Unknown unit");
    }

    @Override
    public boolean isFinerThan(IDomainBase base) {
        return base instanceof AmpsBase && ((AmpsBase)base).ordinal() < this.ordinal();
    }

    @Override
    public IDomainBase getDomainBase(IDomainBase samplesBase) {
        return this;
    }

    public static AmpsBase valueOf(String string) {
        return Enum.valueOf(AmpsBase.class, string);
    }
}

