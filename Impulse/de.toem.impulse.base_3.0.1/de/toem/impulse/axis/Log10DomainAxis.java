/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.axis;

import de.toem.impulse.axis.AbstractDomainAxis;
import de.toem.impulse.domain.IDomainBase;
import java.util.LinkedHashMap;
import java.util.Map;
import org.eclipse.swt.graphics.Rectangle;

public class Log10DomainAxis
extends AbstractDomainAxis {
    private static final int MAX_LOG_ZOOM_WIDTH = 1000;
    private static final int DEFAULT_ZOOM_WIDTH = 100;
    private static final int MIN_ZOOM_WIDTH = 10;
    private static final long ZOOM_MAX = 2147483547L;
    private static final long ZOOM_MIN = -90L;
    private static final long[] ZOOM;
    protected static final String CLAZZ = "Log10";

    static {
        long[] lArray = new long[15];
        lArray[0] = -90L;
        lArray[1] = -80L;
        lArray[2] = -60L;
        lArray[3] = -40L;
        lArray[5] = 100L;
        lArray[6] = 200L;
        lArray[7] = 300L;
        lArray[8] = 400L;
        lArray[9] = 500L;
        lArray[10] = 600L;
        lArray[11] = 700L;
        lArray[12] = 800L;
        lArray[13] = 900L;
        lArray[14] = 1000L;
        ZOOM = lArray;
    }

    public Log10DomainAxis(int rootPixel, IDomainBase domainBase) {
        super(rootPixel, domainBase);
    }

    public Log10DomainAxis(Log10DomainAxis axis) {
        super(axis);
    }

    protected Log10DomainAxis(String[] value) {
        super(value);
    }

    @Override
    public Log10DomainAxis clone() {
        return new Log10DomainAxis(this);
    }

    @Override
    public String getClazz() {
        return CLAZZ;
    }

    @Override
    public long getMinUnitOffset() {
        return 1L;
    }

    @Override
    public long getMaxUnitOffset() {
        return Long.MAX_VALUE;
    }

    @Override
    public void handleUnitOffsetChange() {
        if (this.zoom > 1000L) {
            this.calcLinearScaleFactors();
        }
    }

    @Override
    public void setUnitFrame(long minUnits, long maxUnits) {
        super.setUnitFrame(minUnits >= 1L ? minUnits : 1L, maxUnits >= 1L ? maxUnits : 1L);
    }

    @Override
    protected void handleZoomChange() {
        if (this.zoom > 1000L) {
            this.calcLinearScaleFactors();
        }
        this.extra = (int)(this.zoom + 100L);
    }

    @Override
    public long checkZoom(long zoom) {
        if (zoom < -90L) {
            zoom = -90L;
        }
        if (zoom > 2147483547L) {
            zoom = 2147483547L;
        }
        return zoom;
    }

    @Override
    public long getIncZoom() {
        long[] lArray = ZOOM;
        int n = ZOOM.length;
        int n2 = 0;
        while (n2 < n) {
            long z = lArray[n2];
            if (z > this.zoom) {
                return z;
            }
            ++n2;
        }
        return Long.highestOneBit(this.zoom) << 1;
    }

    @Override
    public long getDecZoom() {
        long pz = -90L;
        long[] lArray = ZOOM;
        int n = ZOOM.length;
        int n2 = 0;
        while (n2 < n) {
            long z = lArray[n2];
            if (z >= this.zoom) {
                return pz;
            }
            pz = z;
            ++n2;
        }
        return Long.highestOneBit(this.zoom - 1L);
    }

    @Override
    public Map<Long, String> getScale(Rectangle clientArea) {
        long un;
        if (this.zoom > 1000L) {
            return super.getScale(clientArea);
        }
        LinkedHashMap<Long, String> map = new LinkedHashMap<Long, String>();
        Rectangle bounds = this.limitBounds(clientArea);
        long u0 = (long)this.units(bounds.x);
        if (u0 < 1L) {
            u0 = 1L;
        }
        if ((un = (long)this.units(bounds.x + bounds.width)) < 1L) {
            return null;
        }
        int l0 = (int)Math.floor(Math.log10(u0));
        int ln = (int)Math.ceil(Math.log10(un));
        int l = l0;
        while (l <= ln) {
            long u = Math.round(Math.pow(10.0, l));
            if (u == Long.MAX_VALUE) break;
            map.put(u, this.toString(u));
            int n = 2;
            while (n <= 9) {
                map.put(u * (long)n, "");
                ++n;
            }
            ++l;
        }
        return map;
    }

    @Override
    public long nextScaleStep(long units, boolean forward, boolean majorStep) {
        long next;
        int lNext;
        if (this.zoom > 1000L) {
            return super.nextScaleStep(units, forward, majorStep);
        }
        int lBase = (int)Math.floor(Math.log10(units));
        if (lBase == (lNext = (int)Math.ceil(Math.log10(units)))) {
            --lBase;
            ++lNext;
        }
        if (lBase <= 0) {
            lBase = 0;
        }
        if ((next = Math.round(Math.pow(10.0, forward ? lNext : lBase))) < this.minUnits) {
            next = this.minUnits;
        } else if (next > this.maxUnits) {
            next = this.maxUnits;
        }
        return next;
    }

    @Override
    public double pixels(double units) {
        if (units < 1.0) {
            return this.pixels(1.0) - 1.0;
        }
        return (double)(this.rootPixel + this.adjust) + (Math.log10(units) - Math.log10(this.unitOffset >= 1L ? this.unitOffset : 1L)) * (double)this.extra;
    }

    @Override
    public double units(double pixel) {
        return Math.pow(10.0, (pixel - (double)this.rootPixel - (double)this.adjust + Math.log10(this.unitOffset) * (double)this.extra) / (double)this.extra);
    }

    @Override
    public long calcZoom(double units1, double units2, int deltaPixels) {
        double l = Math.abs(Math.log10(units2) - Math.log10(units1));
        int f = (int)((double)deltaPixels / l);
        long zoom = this.checkZoom(f - 100);
        return zoom;
    }
}

