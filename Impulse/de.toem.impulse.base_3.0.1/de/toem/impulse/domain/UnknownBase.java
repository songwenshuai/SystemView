/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.domain;

import de.toem.impulse.domain.IDomainBase;
import de.toem.toolkits.core.Utils;
import java.math.BigDecimal;

public enum UnknownBase implements IDomainBase
{
    Unknown;

    public static final String CLAZZ = "Unknown";
    public static final String DOMAIN_LABEL = "Unknown";

    @Override
    public String getClazz() {
        return "Unknown";
    }

    @Override
    public int userLevel() {
        return 0;
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
        return String.valueOf(units);
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

    public static UnknownBase parse(String text) {
        if (text == null) {
            return null;
        }
        UnknownBase[] unknownBaseArray = UnknownBase.values();
        int n = unknownBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            UnknownBase t = unknownBaseArray[n2];
            if (text.equalsIgnoreCase(t.toString())) {
                return t;
            }
            ++n2;
        }
        return null;
    }

    public static UnknownBase bestFit(double diff) {
        return Unknown;
    }

    @Override
    public boolean isCompatible(IDomainBase base) {
        return base == this;
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
        return "Unknown";
    }

    @Override
    public long parseUnits(String value) {
        return Utils.parseLong(value, 0L);
    }

    @Override
    public boolean isFinerThan(IDomainBase base) {
        return false;
    }

    @Override
    public IDomainBase getDomainBase(IDomainBase samplesBase) {
        return this;
    }

    public static UnknownBase valueOf(String string) {
        return Enum.valueOf(UnknownBase.class, string);
    }
}

