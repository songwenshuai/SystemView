/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.domain;

import de.toem.impulse.domain.IDomainBase;
import de.toem.toolkits.core.Utils;
import java.math.BigDecimal;

public enum PixelBase implements IDomainBase
{
    px;

    public static final String CLAZZ = "Pixel";
    public static final String DOMAIN_LABEL = "Pixels";

    @Override
    public String getClazz() {
        return CLAZZ;
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
        return "";
    }

    @Override
    public String toString(long units) {
        return this.toString(units, 14);
    }

    @Override
    public String toString(long units, int style) {
        return "";
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

    public static PixelBase parse(String text) {
        if (text == null) {
            return null;
        }
        PixelBase[] pixelBaseArray = PixelBase.values();
        int n = pixelBaseArray.length;
        int n2 = 0;
        while (n2 < n) {
            PixelBase t = pixelBaseArray[n2];
            if (text.equalsIgnoreCase(t.toString())) {
                return t;
            }
            ++n2;
        }
        return null;
    }

    public static PixelBase bestFit(double diff) {
        return px;
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
        return DOMAIN_LABEL;
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

    public static PixelBase valueOf(String string) {
        return Enum.valueOf(PixelBase.class, string);
    }
}

