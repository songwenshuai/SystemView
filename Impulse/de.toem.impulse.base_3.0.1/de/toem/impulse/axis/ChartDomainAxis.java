/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.axis;

import de.toem.impulse.axis.AbstractDomainAxis;
import de.toem.impulse.domain.IDomainBase;
import java.util.LinkedHashMap;
import java.util.Map;
import org.eclipse.swt.graphics.Rectangle;

public class ChartDomainAxis
extends AbstractDomainAxis {
    protected static final String CLAZZ = "Chart";

    public ChartDomainAxis(int rootPixel, IDomainBase base) {
        super(rootPixel, base);
        this.minUnits = Long.MIN_VALUE;
        this.maxUnits = Long.MAX_VALUE;
    }

    public ChartDomainAxis(ChartDomainAxis axis) {
        super(axis);
    }

    protected ChartDomainAxis(String[] value) {
        super(value);
    }

    @Override
    public ChartDomainAxis clone() {
        return new ChartDomainAxis(this);
    }

    @Override
    public boolean isStatic() {
        return true;
    }

    @Override
    public String getClazz() {
        return CLAZZ;
    }

    @Override
    public long checkZoom(long zoom) {
        return 0L;
    }

    @Override
    public long getIncZoom() {
        return 0L;
    }

    @Override
    public long getDecZoom() {
        return 0L;
    }

    @Override
    public Map<Long, String> getScale(Rectangle clientArea) {
        LinkedHashMap<Long, String> map = new LinkedHashMap<Long, String>();
        return map;
    }

    @Override
    public double pixels(double units) {
        return units + (double)this.rootPixel;
    }

    @Override
    public double units(double pixel) {
        return pixel - (double)this.rootPixel;
    }

    @Override
    public boolean equals(Object obj) {
        if (!super.equals(obj)) {
            return false;
        }
        if (obj instanceof ChartDomainAxis) {
            ChartDomainAxis cfr_ignored_0 = (ChartDomainAxis)obj;
            return true;
        }
        return false;
    }
}

