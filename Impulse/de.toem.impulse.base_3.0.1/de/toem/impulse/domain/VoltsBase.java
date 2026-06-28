/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.domain;

import de.toem.impulse.domain.IDomainBase;
import java.math.BigDecimal;

public enum VoltsBase implements IDomainBase
{
    yV(0),
    yV10(0),
    yV100(0),
    zV(0),
    zV10(0),
    zV100(0),
    aV(0),
    aV10(0),
    aV100(0),
    fV(0),
    fV10(0),
    fV100(0),
    pV(2),
    pV10(1),
    pV100(1),
    nV(2),
    nV10(1),
    nV100(1),
    uV(2),
    uV10(1),
    uV100(1),
    mV(2),
    mV10(1),
    mV100(1),
    V(2),
    V10(1),
    V100(1),
    kV(2),
    kV10(1),
    kV100(1),
    MV(2),
    MV10(1),
    MV100(1),
    GV(2),
    GV10(0),
    GV100(0),
    TV(0),
    TV10(0),
    TV100(0),
    PV(0),
    PV10(0),
    PV100(0),
    EV(0),
    EV10(0),
    EV100(0),
    ZV(0),
    ZV10(0),
    ZV100(0),
    YV(0);

    public static final String CLAZZ = "Volts";
    private int userLevel;
    private boolean isPreferred;
    private VoltsBase myPreferred;
    public static final String DOMAIN_LABEL = "Volts";

    private VoltsBase(int userLevel) {
        this.userLevel = userLevel;
    }

    @Override
    public String getClazz() {
        return "Volts";
    }

    @Override
    public int userLevel() {
        return this.userLevel;
    }

    public static void resetPreferred() {
        VoltsBase[] voltsBaseArray = VoltsBase.values();
        int n = voltsBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            VoltsBase b = voltsBaseArray[n2];
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
            VoltsBase[] voltsBaseArray = VoltsBase.values();
            int n2 = voltsBaseArray.length;
            int n3 = 0;
            while (n3 < n2) {
                VoltsBase b = voltsBaseArray[n3];
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
            while (azeros >= 3 && base < VoltsBase.values().length - 3) {
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
        builder.append(VoltsBase.values()[base].toString());
        return builder.toString();
    }

    @Override
    public BigDecimal getQuantum() {
        return BigDecimal.ONE.scaleByPowerOfTen(this.ordinal() - V.ordinal());
    }

    public double toVolts() {
        return this.toCommonBase();
    }

    @Override
    public double toCommonBase() {
        return Math.pow(10.0, this.ordinal() - V.ordinal());
    }

    @Override
    public double toCommonBase(long units) {
        return Math.pow(10.0, this.ordinal() - V.ordinal()) * (double)units;
    }

    public static VoltsBase parse(String text) {
        if (text == null) {
            return null;
        }
        VoltsBase[] voltsBaseArray = VoltsBase.values();
        int n = voltsBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            VoltsBase t = voltsBaseArray[n2];
            if (text.equals(t.toString())) {
                return t;
            }
            ++n2;
        }
        return null;
    }

    public static VoltsBase bestFit(double diff) {
        VoltsBase[] voltsBaseArray = VoltsBase.values();
        int n = voltsBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            VoltsBase b = voltsBaseArray[n2];
            if (b.getQuantum().doubleValue() > diff) {
                if (b.ordinal() < 2) break;
                return VoltsBase.valueOf(b.ordinal() - 2);
            }
            ++n2;
        }
        return VoltsBase.valueOf(0);
    }

    @Override
    public boolean isCompatible(IDomainBase base) {
        return base instanceof VoltsBase;
    }

    @Override
    public long getConversionFactor(IDomainBase base) {
        if (base == this) {
            return 1L;
        }
        if (base instanceof VoltsBase) {
            VoltsBase that = (VoltsBase)base;
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
        if (base instanceof VoltsBase) {
            if (unit == 0L) {
                return 0L;
            }
            VoltsBase that = (VoltsBase)base;
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
        return "Volts";
    }

    @Override
    public long parseUnits(String value) throws NumberFormatException {
        if (Character.isDigit((value = value.toLowerCase().trim()).charAt(value.length() - 1))) {
            return Long.parseLong(value);
        }
        VoltsBase[] voltsBaseArray = VoltsBase.values();
        int n = voltsBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            VoltsBase t = voltsBaseArray[n2];
            if (value.endsWith(t.toString().toLowerCase())) {
                int comma = (value = value.substring(0, value.length() - t.toString().length()).trim()).lastIndexOf(46);
                comma = comma >= 0 ? value.length() - comma - 1 : 0;
                value = value.replaceAll("\\.", "");
                long units = Long.parseLong(value);
                if (t.ordinal() < comma) {
                    throw new NumberFormatException("Doesn't fit");
                }
                t = VoltsBase.values()[t.ordinal() - comma];
                return t.convertTo(this, units);
            }
            ++n2;
        }
        throw new NumberFormatException("Unknown unit");
    }

    @Override
    public boolean isFinerThan(IDomainBase base) {
        return base instanceof VoltsBase && ((VoltsBase)base).ordinal() < this.ordinal();
    }

    @Override
    public IDomainBase getDomainBase(IDomainBase samplesBase) {
        return this;
    }

    public static VoltsBase valueOf(String string) {
        return Enum.valueOf(VoltsBase.class, string);
    }
}

