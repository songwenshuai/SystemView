/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.domain;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.AmpsBase;
import de.toem.impulse.domain.DateBase;
import de.toem.impulse.domain.FloatBase;
import de.toem.impulse.domain.FrequencyBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IndexBase;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.domain.UnknownBase;
import de.toem.impulse.domain.VoltsBase;
import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.Arrays;

public class DomainBase {
    public static final IDomainBase Unknown = UnknownBase.Unknown;
    public static IDomainBase[] ALL;
    public static String[] ALL_OPTIONS;
    public static String[] ALL_LABELS;
    public static String[] CLASSES;
    public static String[] CLASS_LABELS;

    static {
        ArrayList<Enum> all = new ArrayList<Enum>();
        all.addAll(Arrays.asList(TimeBase.values()));
        all.addAll(Arrays.asList(FrequencyBase.values()));
        all.addAll(Arrays.asList(VoltsBase.values()));
        all.addAll(Arrays.asList(AmpsBase.values()));
        all.addAll(Arrays.asList(FloatBase.values()));
        all.addAll(Arrays.asList(IndexBase.values()));
        all.addAll(Arrays.asList(DateBase.values()));
        ALL = all.toArray(new IDomainBase[all.size()]);
        ALL_OPTIONS = new String[all.size()];
        ALL_LABELS = new String[all.size()];
        int n = 0;
        while (n < all.size()) {
            DomainBase.ALL_OPTIONS[n] = ALL[n].toString();
            DomainBase.ALL_LABELS[n] = ALL[n].toString(1L, 3);
            ++n;
        }
        CLASSES = new String[]{"Time", "Frequency", "Volts", "Amps", "Float", "Index", "Date"};
        CLASS_LABELS = new String[]{"Time", "Frequency", "Volts", "Amps", "Value", "Index", "Date"};
    }

    public static IDomainBase parse(String value) {
        Enum base = TimeBase.parse(value);
        if (base != null) {
            return base;
        }
        base = FrequencyBase.parse(value);
        if (base != null) {
            return base;
        }
        base = VoltsBase.parse(value);
        if (base != null) {
            return base;
        }
        base = AmpsBase.parse(value);
        if (base != null) {
            return base;
        }
        base = FloatBase.parse(value);
        if (base != null) {
            return base;
        }
        base = IndexBase.parse(value);
        if (base != null) {
            return base;
        }
        base = DateBase.parse(value);
        if (base != null) {
            return base;
        }
        return UnknownBase.Unknown;
    }

    public static IDomainBase parse(String value, String domainClass) {
        if ("Time".equals(domainClass)) {
            return TimeBase.parse(value);
        }
        if ("Frequency".equals(domainClass)) {
            return FrequencyBase.parse(value);
        }
        if ("Volts".equals(domainClass)) {
            return VoltsBase.parse(value);
        }
        if ("Amps".equals(domainClass)) {
            return AmpsBase.parse(value);
        }
        if ("Float".equals(domainClass)) {
            return FloatBase.parse(value);
        }
        if ("Index".equals(domainClass)) {
            return IndexBase.parse(value);
        }
        if ("Date".equals(domainClass)) {
            return DateBase.parse(value);
        }
        return UnknownBase.Unknown;
    }

    public static IDomainBase valueOf(Signal signal) {
        return DomainBase.parse(signal != null ? signal.domainBase : null);
    }

    public static IDomainBase bestFit(Class<? extends IDomainBase> c, double diff) {
        Enum base = FloatBase.fX;
        if (c == TimeBase.class) {
            base = TimeBase.bestFit(diff);
        } else if (c == FrequencyBase.class) {
            base = FrequencyBase.bestFit(diff);
        } else if (c == FloatBase.class) {
            base = FloatBase.bestFit(diff);
        } else if (c == VoltsBase.class) {
            base = VoltsBase.bestFit(diff);
        } else if (c == AmpsBase.class) {
            base = AmpsBase.bestFit(diff);
        }
        return base;
    }

    public static long toUnits(double value, BigDecimal quantum) {
        return new BigDecimal(value).divide(quantum).setScale(0, 6).longValue();
    }
}

