/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.axis;

import de.toem.impulse.axis.ChartDomainAxis;
import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.axis.IValueAxis;
import de.toem.impulse.axis.LinearDomainAxis;
import de.toem.impulse.axis.Log10DomainAxis;
import de.toem.impulse.axis.UnknownDomainAxis;
import de.toem.impulse.axis.ValueAxis;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.TimeBase;
import org.eclipse.swt.graphics.Rectangle;

public class Axes {
    public static final int AXIS_LINEAR = 0;
    public static final int AXIS_LOG10 = 1;
    public static IDomainAxis unknownAxis = new UnknownDomainAxis(0, DomainBase.Unknown);

    public static IDomainAxis create(int type, IDomainBase base) {
        if (1 == type) {
            return new Log10DomainAxis(20, base);
        }
        return new LinearDomainAxis(20, base);
    }

    public static void main(String[] args) {
        Axes.test(new LinearDomainAxis(20, TimeBase.ns), 500000000L);
        Axes.test(new Log10DomainAxis(20, TimeBase.ns), 10000000L);
    }

    static void test(IDomainAxis axis, long max) {
        axis.setUnitFrame(0L, max);
        axis.makeVisible(new Rectangle(0, 0, 1000, 100));
        long millies = System.currentTimeMillis();
        long u = 0L;
        while (u < max) {
            long ut;
            int px = (int)axis.pixels(u);
            if ((long)px != (ut = (long)axis.units(px))) {
                // empty if block
            }
            ++u;
        }
        System.out.println(String.valueOf(axis.getClazz()) + "  : " + 1.0 * (double)(System.currentTimeMillis() - millies) / (double)max);
    }

    public static IDomainAxis parseDomainAxis(String value) {
        block16: {
            if (value == null) break block16;
            String[] splitted = value.split("/");
            switch (splitted[0]) {
                case "Linear": {
                    return new LinearDomainAxis(splitted);
                }
                case "Log10": {
                    return new Log10DomainAxis(splitted);
                }
                case "Chart": {
                    return new ChartDomainAxis(splitted);
                }
                case "Unknown": {
                    return unknownAxis;
                }
            }
        }
        return null;
    }

    public static IValueAxis parseValueAxis(String value) {
        block7: {
            if (value == null) break block7;
            String[] splitted = value.split("/");
            switch (splitted[0]) {
                case "ValueAxis": {
                    return new ValueAxis(splitted);
                }
            }
        }
        return null;
    }
}

