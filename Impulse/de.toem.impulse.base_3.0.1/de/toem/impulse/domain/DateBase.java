/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.domain;

import de.toem.impulse.domain.IDomainBase;
import java.math.BigDecimal;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

public enum DateBase implements IDomainBase
{
    dateTime,
    time,
    timeMs;

    public static final String CLAZZ = "Date";
    public static final String DOMAIN_LABEL = "Date";

    @Override
    public int userLevel() {
        return 2;
    }

    @Override
    public String getClazz() {
        return "Date";
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
        if ((style & 0x10) != 0) {
            units = Math.abs(units) + new Date(0, 0, 0, 0, 0, 0).getTime();
            return new SimpleDateFormat("HH:mm:ss.SSS").format(new Date(units));
        }
        if (this == dateTime) {
            return DateFormat.getDateTimeInstance(3, 2).format(new Date(units));
        }
        if (this == time) {
            return new SimpleDateFormat("HH:mm:ss").format(new Date(units));
        }
        return new SimpleDateFormat("HH:mm:ss.SSS").format(new Date(units));
    }

    @Override
    public BigDecimal getQuantum() {
        return BigDecimal.ONE;
    }

    @Override
    public double toCommonBase() {
        return 1.0;
    }

    @Override
    public double toCommonBase(long units) {
        return units;
    }

    public static DateBase parse(String text) {
        if (text == null) {
            return null;
        }
        DateBase[] dateBaseArray = DateBase.values();
        int n = dateBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            DateBase t = dateBaseArray[n2];
            if (text.equalsIgnoreCase(t.toString())) {
                return t;
            }
            ++n2;
        }
        return null;
    }

    public static DateBase bestFit(double diff) {
        return timeMs;
    }

    @Override
    public boolean isCompatible(IDomainBase base) {
        return base instanceof DateBase;
    }

    @Override
    public long getConversionFactor(IDomainBase base) {
        return 0L;
    }

    @Override
    public long convertTo(IDomainBase base, long unit) {
        return unit;
    }

    @Override
    public String getDomainLabel() {
        return "Date";
    }

    @Override
    public long parseUnits(String value) {
        Date d = null;
        try {
            d = this == dateTime ? DateFormat.getDateTimeInstance(3, 2).parse(value) : (this == time ? new SimpleDateFormat("HH:mm:ss").parse(value) : new SimpleDateFormat("HH:mm:ss.SSS").parse(value));
            return d != null ? d.getTime() : 0L;
        }
        catch (ParseException parseException) {
            throw new NumberFormatException("Invalid format");
        }
    }

    @Override
    public boolean isFinerThan(IDomainBase base) {
        return false;
    }

    @Override
    public IDomainBase getDomainBase(IDomainBase samplesBase) {
        return this;
    }

    public static DateBase valueOf(String string) {
        return Enum.valueOf(DateBase.class, string);
    }
}

