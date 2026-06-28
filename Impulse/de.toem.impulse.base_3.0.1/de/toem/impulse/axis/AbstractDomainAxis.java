/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.axis;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.toolkits.core.Utils;
import java.util.LinkedHashMap;
import java.util.Map;
import org.eclipse.swt.graphics.Rectangle;

public abstract class AbstractDomainAxis
implements IDomainAxis {
    protected IDomainBase domainBase;
    protected int rootPixel;
    protected int adjust;
    protected int padding;
    protected long zoom = Long.MAX_VALUE;
    protected long unitOffset;
    protected long minUnits;
    protected long maxUnits;
    protected long scaleMajor = 1L;
    protected long scaleMinor = 1L;
    protected int extra;

    public AbstractDomainAxis(int padding, IDomainBase domainBase) {
        this.padding = this.rootPixel = padding;
        this.domainBase = domainBase;
        this.setUnitFrame(0L, 0L);
        this.setUnitOffset(0L);
        this.setZoom(0L);
    }

    public AbstractDomainAxis(AbstractDomainAxis axis) {
        this.domainBase = axis.domainBase;
        this.rootPixel = axis.rootPixel;
        this.adjust = axis.adjust;
        this.padding = axis.padding;
        this.zoom = axis.zoom;
        this.unitOffset = axis.unitOffset;
        this.minUnits = axis.minUnits;
        this.maxUnits = axis.maxUnits;
        this.scaleMajor = axis.scaleMajor;
        this.scaleMinor = axis.scaleMinor;
        this.extra = axis.extra;
    }

    protected AbstractDomainAxis(String[] value) {
        if (value != null && value.length > 11) {
            this.domainBase = DomainBase.parse(value[1]);
            this.rootPixel = Utils.parseInt(value[2], 0);
            this.adjust = Utils.parseInt(value[3], 0);
            this.padding = Utils.parseInt(value[4], 0);
            this.zoom = Utils.parseLong(value[5], 0L);
            this.unitOffset = Utils.parseLong(value[6], 0L);
            this.minUnits = Utils.parseLong(value[7], 0L);
            this.maxUnits = Utils.parseLong(value[8], 0L);
            this.scaleMajor = Utils.parseLong(value[9], 0L);
            this.scaleMinor = Utils.parseLong(value[10], 0L);
            this.extra = Utils.parseInt(value[11], 0);
        }
    }

    @Override
    public AbstractDomainAxis clone() {
        return null;
    }

    @Override
    public boolean isLinear() {
        return false;
    }

    @Override
    public boolean isStatic() {
        return false;
    }

    public String toString() {
        return String.valueOf(this.getClazz()) + "/" + this.domainBase + "/" + this.rootPixel + "/" + this.adjust + "/" + this.padding + "/" + this.zoom + "/" + this.unitOffset + "/" + this.minUnits + "/" + this.maxUnits + "/" + this.scaleMajor + "/" + this.scaleMinor + "/" + this.extra;
    }

    @Override
    public IDomainBase getDomainBase() {
        return this.domainBase;
    }

    @Override
    public String getDomainClass() {
        return this.domainBase != null ? this.domainBase.getClazz() : null;
    }

    @Override
    public void setDomainBase(IDomainBase domainBase) {
        this.domainBase = domainBase;
    }

    @Override
    public int getRootPixel() {
        return this.rootPixel;
    }

    @Override
    public void setRootPixel(int pixel) {
        this.rootPixel = pixel;
    }

    @Override
    public int getPadding() {
        return this.padding;
    }

    @Override
    public void setPadding(int pixel) {
        this.padding = pixel;
    }

    @Override
    public int getAdjust() {
        return this.adjust;
    }

    @Override
    public Rectangle limitBounds(Rectangle bounds) {
        Rectangle mb = new Rectangle(bounds.x, bounds.y, bounds.width, bounds.height);
        mb.x = this.limitPx(mb.x);
        mb.width = this.limitPx(mb.x + mb.width) - mb.x;
        return mb;
    }

    @Override
    public int limitPx(int px) {
        double max = this.pixels(9.223372036854776E18);
        double min = this.pixels(-9.223372036854776E18);
        px = (int)Math.max((double)px, min);
        px = (int)Math.min((double)px, max);
        return px;
    }

    @Override
    public Rectangle limitFrameBounds(Rectangle bounds) {
        Rectangle mb = new Rectangle(bounds.x, bounds.y, bounds.width, bounds.height);
        mb.x = this.limitFramePx(mb.x);
        mb.width = this.limitFramePx(mb.x + mb.width) - mb.x;
        return mb;
    }

    @Override
    public int limitFramePx(int pixel) {
        double max = this.pixels(this.getMaxUnits());
        double min = this.pixels(this.getMinUnits());
        pixel = (int)Math.max((double)pixel, min);
        pixel = (int)Math.min((double)pixel, max);
        return pixel;
    }

    @Override
    public void setUnitFrame(long minUnits, long maxUnits) {
        this.minUnits = minUnits;
        this.maxUnits = maxUnits;
        if (maxUnits < minUnits) {
            this.maxUnits = minUnits;
        }
    }

    @Override
    public final long getMinUnits() {
        return this.minUnits;
    }

    @Override
    public final long getMaxUnits() {
        return this.maxUnits;
    }

    @Override
    public final void setMinUnits(long minUnits) {
        this.setUnitFrame(minUnits, this.maxUnits);
    }

    @Override
    public final void setMaxUnits(long maxUnits) {
        this.setUnitFrame(this.minUnits, maxUnits);
    }

    @Override
    public void startAdjustUnitFrame() {
        this.minUnits = Long.MAX_VALUE;
        this.maxUnits = Long.MIN_VALUE;
    }

    @Override
    public boolean adjustUnitFrame(long minUnits, long maxUnits) {
        boolean changed = false;
        if (this.minUnits > minUnits) {
            this.minUnits = minUnits;
            changed = true;
        }
        if (this.maxUnits < maxUnits) {
            this.maxUnits = maxUnits;
            changed = true;
        }
        return changed;
    }

    @Override
    public void endAdjustUnitFrame() {
        if (this.maxUnits == Long.MIN_VALUE || this.minUnits == Long.MAX_VALUE) {
            this.setUnitFrame(0L, 0L);
        } else {
            this.setUnitFrame(this.minUnits, this.maxUnits);
        }
    }

    @Override
    public long getUnitOffset() {
        return this.unitOffset;
    }

    @Override
    public boolean setUnitOffset(long offset) {
        if (offset < this.getMinUnitOffset()) {
            offset = this.getMinUnitOffset();
        }
        if (offset > this.getMaxUnitOffset()) {
            offset = this.getMaxUnitOffset();
        }
        if (this.unitOffset != offset || this.adjust != 0) {
            this.unitOffset = offset;
            this.adjust = 0;
            this.handleUnitOffsetChange();
            return true;
        }
        return false;
    }

    protected void handleUnitOffsetChange() {
    }

    @Override
    public boolean setUnitOffset(long units, int px) {
        boolean result = this.setUnitOffset(this.calcOffset(units, px));
        this.adjust = (int)((double)px - this.pixels(units));
        return result;
    }

    @Override
    public boolean setUnitOffset(IDomainAxis previous, int dx) {
        long u0 = (long)previous.units(this.getRootPixel());
        int x0 = (int)previous.pixels(u0);
        u0 = (long)previous.units(x0);
        int x1 = x0 + dx;
        return this.setUnitOffset(u0, x1);
    }

    @Override
    public long getMinUnitOffset() {
        return Long.MIN_VALUE;
    }

    @Override
    public long getMaxUnitOffset() {
        return Long.MAX_VALUE;
    }

    @Override
    public long getMinScrollUnitOffset(Rectangle clientArea) {
        return this.minUnits;
    }

    @Override
    public long getMaxScrollUnitOffset(Rectangle clientArea, boolean rightPadding) {
        long max = this.maxUnits + this.deltaUnitsAt(this.maxUnits, -(clientArea.width - (rightPadding ? 2 : 1) * this.padding));
        return max > this.minUnits ? max : this.minUnits;
    }

    @Override
    public long getZoom() {
        return this.zoom;
    }

    @Override
    public boolean setZoom(long zoom) {
        if ((zoom = this.checkZoom(zoom)) == this.zoom) {
            return false;
        }
        this.zoom = zoom;
        this.adjust = 0;
        this.handleZoomChange();
        return true;
    }

    protected void handleZoomChange() {
    }

    @Override
    public boolean setZoom(long zoom, long units, int pixel) {
        if (this.setZoom(zoom)) {
            this.setUnitOffset(units, pixel);
            return true;
        }
        return false;
    }

    public long checkZoom(long zoom) {
        return zoom;
    }

    @Override
    public long getIncZoom() {
        return this.checkZoom(this.zoom + 1L);
    }

    @Override
    public long getDecZoom() {
        return this.checkZoom(this.zoom - 1L);
    }

    @Override
    public boolean makeVisible(DomainValue position, Rectangle clientArea) {
        if (position.base.isCompatible(this.domainBase)) {
            return this.makeVisible(position.base.convertTo(this.domainBase, position.units), clientArea);
        }
        return false;
    }

    @Override
    public boolean makeVisible(long units, Rectangle clientArea) {
        long minOffset = this.getMinScrollUnitOffset(clientArea);
        long maxOffset = this.getMaxScrollUnitOffset(clientArea, true);
        long u1 = (long)(this.units(this.padding) + 1.0);
        long u2 = (long)this.units(clientArea.width - 2 * this.padding);
        long offset = this.unitOffset;
        boolean outside = false;
        if (units < u1) {
            offset = this.calcOffset(units, this.padding);
            if (offset < minOffset) {
                offset = minOffset;
            }
            outside = true;
        } else if (units > u2) {
            offset = this.calcOffset(units, clientArea.width - 2 * this.padding);
            if (offset > maxOffset) {
                offset = maxOffset;
            }
            outside = true;
        }
        if (offset != this.unitOffset || outside && this.adjust != 0) {
            this.setUnitOffset(offset);
            return true;
        }
        return false;
    }

    @Override
    public boolean makeChangeVisible(DomainValue position, Rectangle clientArea) {
        if (position.base.isCompatible(this.domainBase)) {
            return this.makeChangeVisible(position.base.convertTo(this.domainBase, position.units), clientArea);
        }
        return false;
    }

    @Override
    public boolean makeChangeVisible(long units, Rectangle clientArea) {
        long minOffset = this.getMinScrollUnitOffset(clientArea);
        long maxOffset = this.getMaxScrollUnitOffset(clientArea, true);
        long u1 = (long)(this.units(this.padding) + 1.0);
        long u2 = (long)this.units(clientArea.width - 5 * this.padding);
        long offset = this.unitOffset;
        boolean outside = false;
        if (units < u1) {
            offset = this.calcOffset(units, this.padding);
            if (offset < minOffset) {
                offset = minOffset;
            }
            outside = true;
        } else if (units > u2) {
            offset = this.calcOffset(units, clientArea.width - 5 * this.padding);
            if (offset > maxOffset) {
                offset = maxOffset;
            }
            outside = true;
        }
        if (offset != this.unitOffset || outside && this.adjust != 0) {
            this.setUnitOffset(offset);
            return true;
        }
        return false;
    }

    @Override
    public boolean makeBothVisible(long units1, long units2, Rectangle clientArea) {
        int dx = clientArea.width - 2 * this.getPadding();
        this.setZoom(this.calcZoom(units1, units2, dx));
        this.setUnitOffset(Math.min(units1, units2), this.getPadding());
        return true;
    }

    @Override
    public boolean makeVisible(Rectangle clientArea) {
        return this.makeBothVisible(this.minUnits, this.maxUnits, clientArea);
    }

    @Override
    public Map<Long, String> getScale(Rectangle clientArea) {
        LinkedHashMap<Long, String> map = new LinkedHashMap<Long, String>();
        if (this.scaleMajor <= 0L) {
            return null;
        }
        int steps = this.scaleMinor > 0L ? (int)(this.scaleMajor / this.scaleMinor) : 0;
        Rectangle bounds = this.limitBounds(clientArea);
        long u0 = (long)this.units(bounds.x - 10);
        long un = (long)this.units(bounds.x + bounds.width);
        if (this.scaleMajor > 0L) {
            long u = u0 / this.scaleMajor * this.scaleMajor;
            if (u0 < 0L && u - this.scaleMajor < u) {
                u -= this.scaleMajor;
            }
            while (u <= un) {
                String val = this.toString(u);
                map.put(u, val);
                int n = 0;
                while (n < steps) {
                    long cu = u + (long)n * (this.scaleMajor / (long)steps);
                    if (cu > u) {
                        map.put(cu, "");
                    }
                    ++n;
                }
                if (u + this.scaleMajor < u) break;
                u += this.scaleMajor;
            }
        }
        return map;
    }

    @Override
    public long nextScaleStep(long units, boolean forward, boolean majorStep) {
        if (this.scaleMajor <= 0L) {
            return units;
        }
        long base = majorStep ? this.scaleMajor : this.scaleMinor;
        long next = units / base * base + (forward ? base : -base);
        if (next < this.minUnits) {
            next = this.minUnits;
        } else if (next > this.maxUnits) {
            next = this.maxUnits;
        }
        return next;
    }

    protected void calcLinearScaleFactors() {
        int dx = 300;
        long dt = this.deltaUnits(0, dx);
        this.scaleMajor = 1L;
        long t = dt;
        while ((t /= 10L) != 0L && this.scaleMajor * 100L / 100L == this.scaleMajor) {
            this.scaleMajor *= 10L;
        }
        int f = (int)(dt / this.scaleMajor);
        if (f > 7) {
            this.scaleMinor = this.scaleMajor;
            this.scaleMajor *= 10L;
        } else if (f > 3) {
            this.scaleMinor = this.scaleMajor;
            this.scaleMajor *= 5L;
        } else if (f > 1) {
            this.scaleMinor = this.scaleMajor / 2L;
            this.scaleMajor *= 2L;
        } else {
            this.scaleMinor = this.scaleMajor / 10L;
        }
        if (this.scaleMinor == 0L) {
            this.scaleMinor = this.scaleMajor;
        }
    }

    @Override
    public String toString(long units) {
        return this.domainBase.toString(units, 15);
    }

    @Override
    public String toString(long units, int style) {
        return this.domainBase.toString(units, style);
    }

    @Override
    public String toDeltaString(long units) {
        return this.domainBase.toString(units, 31);
    }

    @Override
    public abstract double pixels(double var1);

    @Override
    public abstract double units(double var1);

    @Override
    public int deltaPixels(long units, long deltaUnits) {
        return (int)(this.pixels(units + deltaUnits) - this.pixels(units));
    }

    @Override
    public int deltaPixelsAt(int pixel, long deltaUnits) {
        return (int)(this.pixels(this.units(pixel) + (double)deltaUnits) - (double)pixel);
    }

    @Override
    public long deltaUnits(int pixel, int deltaPixels) {
        return (long)(this.units(pixel + deltaPixels) - this.units(pixel));
    }

    @Override
    public long deltaUnitsAt(long units, int deltaPixels) {
        return (long)(this.units(this.pixels(units) + (double)deltaPixels) - (double)units);
    }

    @Override
    public double pixels2(DomainValue position) {
        return position.base != null && position.base.isCompatible(this.domainBase) ? this.pixels(position.base.convertTo(this.domainBase, position.units)) : 0.0;
    }

    @Override
    public DomainValue position(double pixels) {
        return new DomainValue(this.domainBase, (long)this.units(pixels));
    }

    protected long calcOffset(long units, int pixel) {
        return (long)((double)units + (double)this.deltaUnitsAt(units, this.rootPixel - pixel));
    }

    protected long calcZoom(double units1, double units2, int deltaPixels) {
        return 0L;
    }

    @Override
    public double[] getLinearScalingFrom(IDomainAxis axis) {
        if (axis != null && this.getClazz().equals(axis.getClazz()) && ((AbstractDomainAxis)axis).zoom == this.zoom) {
            return new double[]{this.pixels(0.0) - axis.pixels(0.0), 0.0};
        }
        return null;
    }

    public boolean equals(Object obj) {
        if (obj instanceof AbstractDomainAxis) {
            AbstractDomainAxis that = (AbstractDomainAxis)obj;
            return that.domainBase == this.domainBase && that.rootPixel == this.rootPixel && that.adjust == this.adjust && that.unitOffset == this.unitOffset && that.minUnits == this.minUnits && that.maxUnits == this.maxUnits && that.scaleMajor == this.scaleMajor && that.scaleMinor == this.scaleMinor && that.zoom == this.zoom && that.extra == this.extra;
        }
        return false;
    }

    @Override
    public boolean syncTo(IDomainAxis obj) {
        if (obj != null && obj.getClazz().equals(this.getClazz())) {
            AbstractDomainAxis that = (AbstractDomainAxis)obj;
            that.domainBase = this.domainBase;
            that.rootPixel = this.rootPixel;
            that.adjust = this.adjust;
            that.unitOffset = this.unitOffset;
            that.minUnits = this.minUnits;
            that.maxUnits = this.maxUnits;
            that.scaleMajor = this.scaleMajor;
            that.scaleMinor = this.scaleMinor;
            that.zoom = this.zoom;
            that.extra = this.extra;
            return true;
        }
        return false;
    }
}

