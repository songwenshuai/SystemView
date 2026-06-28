/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.domain;

import de.toem.impulse.domain.IDomainBase;
import java.math.BigDecimal;

public enum FrequencyBase implements IDomainBase
{
    yHz(0),
    yHz10(0),
    yHz100(0),
    zHz(0),
    zHz10(0),
    zHz100(0),
    aHz(0),
    aHz10(0),
    aHz100(0),
    fHz(0),
    fHz10(0),
    fHz100(0),
    pHz(0),
    pHz10(0),
    pHz100(0),
    nHz(0),
    nHz10(0),
    nHz100(0),
    uHz(0),
    uHz10(0),
    uHz100(0),
    mHz(2),
    mHz10(1),
    mHz100(1),
    Hz(2),
    Hz10(1),
    Hz100(1),
    kHz(2),
    kHz10(1),
    kHz100(1),
    MHz(2),
    MHz10(1),
    MHz100(1),
    GHz(2),
    GHz10(1),
    GHz100(1),
    THz(2),
    THz10(0),
    THz100(0),
    PHz(0),
    PHz10(0),
    PHz100(0),
    EHz(0),
    EHz10(0),
    EHz100(0),
    ZHz(0),
    ZHz10(0),
    ZHz100(0),
    YHz(0);

    public static final String CLAZZ = "Frequency";
    private int userLevel;
    private boolean isPreferred;
    private FrequencyBase myPreferred;
    public static final String DOMAIN_LABEL = "Frequency";

    private FrequencyBase(int userLevel) {
        this.userLevel = userLevel;
    }

    @Override
    public String getClazz() {
        return "Frequency";
    }

    @Override
    public int userLevel() {
        return this.userLevel;
    }

    public static void resetPreferred() {
        FrequencyBase[] frequencyBaseArray = FrequencyBase.values();
        int n = frequencyBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            FrequencyBase b = frequencyBaseArray[n2];
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
            FrequencyBase[] frequencyBaseArray = FrequencyBase.values();
            int n2 = frequencyBaseArray.length;
            int n3 = 0;
            while (n3 < n2) {
                FrequencyBase b = frequencyBaseArray[n3];
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
            while (azeros >= 3 && base < FrequencyBase.values().length - 3) {
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
        builder.append(FrequencyBase.values()[base].toString());
        return builder.toString();
    }

    @Override
    public BigDecimal getQuantum() {
        return BigDecimal.ONE.scaleByPowerOfTen(this.ordinal() - Hz.ordinal());
    }

    public double toHertz() {
        return this.toCommonBase();
    }

    @Override
    public double toCommonBase() {
        return Math.pow(10.0, this.ordinal() - Hz.ordinal());
    }

    @Override
    public double toCommonBase(long units) {
        return Math.pow(10.0, this.ordinal() - Hz.ordinal()) * (double)units;
    }

    public static FrequencyBase parse(String text) {
        if (text == null) {
            return null;
        }
        FrequencyBase[] frequencyBaseArray = FrequencyBase.values();
        int n = frequencyBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            FrequencyBase t = frequencyBaseArray[n2];
            if (text.equals(t.toString())) {
                return t;
            }
            ++n2;
        }
        return null;
    }

    public static FrequencyBase bestFit(double diff) {
        FrequencyBase[] frequencyBaseArray = FrequencyBase.values();
        int n = frequencyBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            FrequencyBase b = frequencyBaseArray[n2];
            if (b.getQuantum().doubleValue() > diff) {
                if (b.ordinal() < 2) break;
                return FrequencyBase.valueOf(b.ordinal() - 2);
            }
            ++n2;
        }
        return FrequencyBase.valueOf(0);
    }

    @Override
    public boolean isCompatible(IDomainBase base) {
        return base instanceof FrequencyBase;
    }

    @Override
    public long getConversionFactor(IDomainBase base) {
        if (base == this) {
            return 1L;
        }
        if (base instanceof FrequencyBase) {
            FrequencyBase that = (FrequencyBase)base;
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
        if (base instanceof FrequencyBase) {
            FrequencyBase that = (FrequencyBase)base;
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
        return "Frequency";
    }

    @Override
    public long parseUnits(String value) throws NumberFormatException {
        if (Character.isDigit((value = value.toLowerCase().trim()).charAt(value.length() - 1))) {
            return Long.parseLong(value);
        }
        FrequencyBase[] frequencyBaseArray = FrequencyBase.values();
        int n = frequencyBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            FrequencyBase t = frequencyBaseArray[n2];
            if (value.endsWith(t.toString().toLowerCase())) {
                int comma = (value = value.substring(0, value.length() - t.toString().length()).trim()).lastIndexOf(46);
                comma = comma >= 0 ? value.length() - comma - 1 : 0;
                value = value.replaceAll("\\.", "");
                long units = Long.parseLong(value);
                if (t.ordinal() < comma) {
                    throw new NumberFormatException("Doesn't fit");
                }
                t = FrequencyBase.values()[t.ordinal() - comma];
                return t.convertTo(this, units);
            }
            ++n2;
        }
        throw new NumberFormatException("Unknown unit");
    }

    @Override
    public boolean isFinerThan(IDomainBase base) {
        return base instanceof FrequencyBase && ((FrequencyBase)base).ordinal() < this.ordinal();
    }

    @Override
    public IDomainBase getDomainBase(IDomainBase samplesBase) {
        return this;
    }

    public static FrequencyBase valueOf(String string) {
        return Enum.valueOf(FrequencyBase.class, string);
    }
}

