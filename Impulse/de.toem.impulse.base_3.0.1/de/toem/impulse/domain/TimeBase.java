/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.domain;

import de.toem.impulse.domain.IDomainBase;
import de.toem.toolkits.core.Utils;
import java.math.BigDecimal;

public enum TimeBase implements IDomainBase
{
    ys(0),
    ys10(0),
    ys100(0),
    zs(0),
    zs10(0),
    zs100(0),
    as(0),
    as10(0),
    as100(0),
    fs(2),
    fs10(1),
    fs100(1),
    ps(2),
    ps10(1),
    ps100(1),
    ns(2),
    ns10(1),
    ns100(1),
    us(2),
    us10(1),
    us100(1),
    ms(2),
    ms10(1),
    ms100(1),
    s(2),
    s10(0),
    s100(0),
    ks(0),
    ks10(0),
    ks100(0),
    Ms(0),
    Ms10(0),
    Ms100(0),
    Gs(0),
    Gs10(0),
    Gs100(0),
    Ts(0),
    Ts10(0),
    Ts100(0),
    Ps(0),
    Ps10(0),
    Ps100(0),
    Es(0),
    Es10(0),
    Es100(0),
    Zs(0),
    Zs10(0),
    Zs100(0),
    Ys(0);

    public static final String[] USUAL_OPTIONS;
    public static final String CLAZZ = "Time";
    private int userLevel;
    private boolean isPreferred;
    private TimeBase myPreferred;
    public static final String DOMAIN_LABEL = "Time";

    static {
        USUAL_OPTIONS = new String[]{fs.toString(), ps.toString(), ns.toString(), us.toString(), ms.toString(), s.toString(), s10.toString(), s100.toString()};
    }

    private TimeBase(int userLevel) {
        this.userLevel = userLevel;
    }

    @Override
    public String getClazz() {
        return "Time";
    }

    @Override
    public int userLevel() {
        return this.userLevel;
    }

    public static void resetPreferred() {
        TimeBase[] timeBaseArray = TimeBase.values();
        int n = timeBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            TimeBase b = timeBaseArray[n2];
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
        builder.append(units >= 0L ? units : -units);
        if ((style & 4) != 0 && this.myPreferred == null) {
            int dist = Integer.MAX_VALUE;
            TimeBase[] timeBaseArray = TimeBase.values();
            int n2 = timeBaseArray.length;
            int n3 = 0;
            while (n3 < n2) {
                TimeBase b = timeBaseArray[n3];
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
                if ((style & 8) != 0) {
                    int p = builder.length() - 1;
                    while (p > 0 && builder.charAt(p) == '0') {
                        --p;
                    }
                    if (builder.charAt(p) == '.') {
                        --p;
                    }
                    builder.setLength(p + 1);
                }
            } else if (delta < 0) {
                int n4 = 0;
                while (n4 < -delta) {
                    builder.append('0');
                    ++n4;
                }
            }
            builder.append(this.myPreferred.toString());
            if (units < 0L) {
                builder.insert(0, '-');
            }
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
            while (azeros >= 3 && base < TimeBase.values().length - 3) {
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
        builder.append(TimeBase.values()[base].toString());
        if (units < 0L) {
            builder.insert(0, '-');
        }
        return builder.toString();
    }

    @Override
    public BigDecimal getQuantum() {
        return BigDecimal.ONE.scaleByPowerOfTen(this.ordinal() - s.ordinal());
    }

    public double toSeconds() {
        return this.toCommonBase();
    }

    @Override
    public double toCommonBase() {
        return Math.pow(10.0, this.ordinal() - s.ordinal());
    }

    @Override
    public double toCommonBase(long units) {
        return Math.pow(10.0, this.ordinal() - s.ordinal()) * (double)units;
    }

    public static TimeBase parse(String text) {
        if (text == null) {
            return null;
        }
        TimeBase[] timeBaseArray = TimeBase.values();
        int n = timeBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            TimeBase t = timeBaseArray[n2];
            if (text.equals(t.toString())) {
                return t;
            }
            ++n2;
        }
        return null;
    }

    public static TimeBase bestFit(double diff) {
        TimeBase[] timeBaseArray = TimeBase.values();
        int n = timeBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            TimeBase b = timeBaseArray[n2];
            if (b.getQuantum().doubleValue() > diff) {
                if (b.ordinal() < 2) break;
                return TimeBase.valueOf(b.ordinal() - 2);
            }
            ++n2;
        }
        return TimeBase.valueOf(0);
    }

    @Override
    public boolean isCompatible(IDomainBase base) {
        return base instanceof TimeBase;
    }

    @Override
    public long getConversionFactor(IDomainBase base) {
        if (base == this) {
            return 1L;
        }
        if (base instanceof TimeBase) {
            TimeBase that = (TimeBase)base;
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
        if (base instanceof TimeBase) {
            if (unit == 0L) {
                return 0L;
            }
            TimeBase that = (TimeBase)base;
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
        return "Time";
    }

    @Override
    public long parseUnits(String value) throws NumberFormatException {
        if (!Utils.isEmpty(value)) {
            if (Character.isDigit((value = value.toLowerCase().trim()).charAt(value.length() - 1))) {
                return Long.parseLong(value);
            }
            TimeBase[] timeBaseArray = TimeBase.values();
            int n = timeBaseArray.length;
            int n2 = 0;
            while (n2 < n) {
                TimeBase t = timeBaseArray[n2];
                if (value.endsWith(t.toString().toLowerCase())) {
                    int comma = (value = value.substring(0, value.length() - t.toString().length()).trim()).lastIndexOf(46);
                    comma = comma >= 0 ? value.length() - comma - 1 : 0;
                    value = value.replaceAll("\\.", "");
                    long units = Long.parseLong(value);
                    if (t.ordinal() < comma) {
                        throw new NumberFormatException("Doesn't fit");
                    }
                    t = TimeBase.values()[t.ordinal() - comma];
                    return t.convertTo(this, units);
                }
                ++n2;
            }
        }
        throw new NumberFormatException("Unknown unit");
    }

    @Override
    public boolean isFinerThan(IDomainBase base) {
        return base instanceof TimeBase && ((TimeBase)base).ordinal() > this.ordinal();
    }

    @Override
    public IDomainBase getDomainBase(IDomainBase samplesBase) {
        return this;
    }

    public static TimeBase valueOf(String string) {
        return Enum.valueOf(TimeBase.class, string);
    }
}

