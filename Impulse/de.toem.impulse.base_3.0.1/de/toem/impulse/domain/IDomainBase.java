/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.domain;

import de.toem.impulse.domain.IDomainBaseProvider;
import java.math.BigDecimal;

public interface IDomainBase
extends IDomainBaseProvider {
    public static final int STYLE_ALLOW_0_WITHOUT_UNITS = 1;
    public static final int STYLE_AUTO_UNIT = 2;
    public static final int STYLE_PREFFERED_UNIT = 4;
    public static final int STYLE_REMOVE_TRAILING_ZEROS = 8;
    public static final int STYLE_DELTA = 16;
    public static final int STYLE_DEFAULT = 14;
    public static final int UL0 = 0;
    public static final int UL1 = 1;
    public static final int UL2 = 2;

    public int ordinal();

    public String getClazz();

    public String toString(long var1);

    public String toString(long var1, int var3);

    public long parseUnits(String var1);

    public boolean isCompatible(IDomainBase var1);

    public long getConversionFactor(IDomainBase var1);

    public long convertTo(IDomainBase var1, long var2);

    public String getDomainLabel();

    public BigDecimal getQuantum();

    public double toCommonBase();

    public double toCommonBase(long var1);

    public boolean isFinerThan(IDomainBase var1);

    public int userLevel();

    public void setPreferred(boolean var1);

    public boolean isPreferred();
}

