/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.domain;

import de.toem.impulse.domain.IDomainBase;
import java.math.BigDecimal;

public enum IndexBase implements IDomainBase
{
    n;

    public static final String CLAZZ = "Index";
    public static final String DOMAIN_LABEL = "Index";

    @Override
    public String getClazz() {
        return "Index";
    }

    @Override
    public int userLevel() {
        return 2;
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
        return String.valueOf(String.valueOf(units)) + n.toString();
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

    public static IndexBase parse(String text) {
        if (text == null) {
            return null;
        }
        IndexBase[] indexBaseArray = IndexBase.values();
        int n = indexBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            IndexBase t = indexBaseArray[n2];
            if (text.equalsIgnoreCase(t.toString())) {
                return t;
            }
            ++n2;
        }
        return null;
    }

    public static IndexBase bestFit(double diff) {
        return n;
    }

    @Override
    public boolean isCompatible(IDomainBase base) {
        return base instanceof IndexBase;
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
        return "Index";
    }

    @Override
    public long parseUnits(String value) {
        if (Character.isDigit((value = value.toLowerCase().trim()).charAt(value.length() - 1))) {
            return Long.parseLong(value);
        }
        if (value.endsWith(n.toString().toLowerCase())) {
            value = value.substring(0, value.length() - n.toString().length()).trim();
            long units = Long.parseLong(value);
            return units;
        }
        throw new NumberFormatException("Unknown unit");
    }

    @Override
    public boolean isFinerThan(IDomainBase base) {
        return false;
    }

    @Override
    public IDomainBase getDomainBase(IDomainBase samplesBase) {
        return this;
    }

    public static IndexBase valueOf(String string) {
        return Enum.valueOf(IndexBase.class, string);
    }
}

