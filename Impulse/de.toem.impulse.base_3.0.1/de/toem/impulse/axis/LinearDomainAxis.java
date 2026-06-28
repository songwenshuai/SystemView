/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.axis;

import de.toem.impulse.axis.AbstractDomainAxis;
import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.domain.IDomainBase;

public class LinearDomainAxis
extends AbstractDomainAxis {
    private static final long ZOOM_MAX = 0xFFFFFFFFFFFFFFL;
    private static final long ZOOM_MIN = -256L;
    private static final long[] ZOOM = new long[]{-256L, -128L, -64L, -32L, -16L, -8L, -4L, -2L, 1L, 2L};
    protected static final String CLAZZ = "Linear";

    public LinearDomainAxis(int rootPixel, IDomainBase domainBase) {
        super(rootPixel, domainBase);
    }

    public LinearDomainAxis(LinearDomainAxis axis) {
        super(axis);
    }

    protected LinearDomainAxis(String[] value) {
        super(value);
    }

    @Override
    public LinearDomainAxis clone() {
        return new LinearDomainAxis(this);
    }

    @Override
    public boolean isLinear() {
        return true;
    }

    @Override
    public String getClazz() {
        return CLAZZ;
    }

    @Override
    public long checkZoom(long zoom) {
        if (zoom == 0L) {
            zoom = 1L;
        }
        if (zoom < -256L) {
            zoom = -256L;
        }
        if (zoom > 0xFFFFFFFFFFFFFFL) {
            zoom = 0xFFFFFFFFFFFFFFL;
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
        long pz = -256L;
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
    protected void handleZoomChange() {
        this.calcLinearScaleFactors();
    }

    @Override
    public double pixels(double units) {
        return this.zoom <= 0L ? (units - (double)this.unitOffset) * (double)(-this.zoom) + (double)this.rootPixel + (double)this.adjust : (units - (double)this.unitOffset) / (double)this.zoom + (double)this.rootPixel + (double)this.adjust;
    }

    @Override
    public double units(double pixels) {
        return this.zoom >= 0L ? (pixels - (double)this.rootPixel - (double)this.adjust) * (double)this.zoom + (double)this.unitOffset : (pixels - (double)this.rootPixel - (double)this.adjust) / (double)(-this.zoom) + (double)this.unitOffset;
    }

    @Override
    protected long calcZoom(double units1, double units2, int deltaPixels) {
        if (deltaPixels == 0) {
            return 1L;
        }
        double zoom = Math.abs((units2 - units1) / (double)deltaPixels);
        if (zoom < 1.0 && zoom > 0.0) {
            return Math.round(Math.max(-1.0 / zoom, -256.0));
        }
        if (zoom == 0.0) {
            return -256L;
        }
        return Math.round(zoom);
    }

    @Override
    public double[] getLinearScalingFrom(IDomainAxis axis) {
        if (axis != null && this.getClazz().equals(axis.getClazz())) {
            double u0 = axis.units(0.0);
            double u1000 = axis.units(1000.0);
            double b = this.pixels(u0);
            double a = this.zoom != ((LinearDomainAxis)axis).zoom ? (this.pixels(u1000) - b) / 1000.0 : 1.0;
            return new double[]{b, a};
        }
        return null;
    }

    @Override
    public boolean equals(Object obj) {
        if (!super.equals(obj)) {
            return false;
        }
        if (obj instanceof LinearDomainAxis) {
            LinearDomainAxis cfr_ignored_0 = (LinearDomainAxis)obj;
            return true;
        }
        return false;
    }
}

