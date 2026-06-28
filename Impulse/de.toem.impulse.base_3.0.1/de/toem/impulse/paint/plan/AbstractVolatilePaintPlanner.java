/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.MultiPaintPlanner;
import de.toem.impulse.paint.plan.SinglePaintPlanner;
import de.toem.impulse.samples.base.SamplesStat;
import de.toem.toolkits.core.Assert;
import de.toem.toolkits.pattern.threading.IProgress;
import org.eclipse.swt.graphics.Rectangle;

public abstract class AbstractVolatilePaintPlanner
extends SinglePaintPlanner {
    protected boolean paintNormalIfNoChange = false;
    protected int lastPaint = 0;

    protected AbstractVolatilePaintPlanner(ITheme theme, IPlotItem item, IDomainAxis axis, Rectangle area, int style) {
        super(theme, item, axis, area, style);
    }

    protected AbstractVolatilePaintPlanner(MultiPaintPlanner multi, IPlotItem item) {
        super(multi, item);
    }

    protected abstract void planNormal(IPlan.IPaintPlanGenerator var1, IDomainAxis var2, int var3, long var4, int var6, long var7);

    @Override
    public void createPlan(IProgress p, IPlan.IPaintPlanGenerator generator, IDomainAxis axis, Rectangle area, boolean extend) {
        Assert.isFalse(extend);
        int px1 = area.x;
        int px2 = px1 + area.width;
        boolean histo = area.height > 40;
        boolean minDtZero = false;
        generator.setEndX((int)axis.pixels(this.samples.getEndUnits()));
        generator.setTagDomain(this.samples.getTagDomain());
        double maxLoad = 0.0;
        double minLoad = Double.MAX_VALUE;
        int count = this.samples.getCount();
        if (count <= 0) {
            return;
        }
        int idx1 = this.samples.indexAt((long)axis.units(px1));
        int idx2 = this.samples.indexAt((long)axis.units(px2));
        if (count > idx2 + 1) {
            ++idx2;
        }
        if (idx1 == -1 && idx2 == -1) {
            return;
        }
        if (idx1 == -1 && idx2 != -1) {
            idx1 = 0;
        } else if (idx1 != -1 && idx2 == -1) {
            idx2 = idx1;
        }
        generator.extendToInfinity(idx1 == 0, idx2 == count - 1);
        int idx = idx1;
        long t = this.samples.unitsAt(idx);
        long nt = this.samples.unitsAt(idx + 1);
        int mindx = this.minSampleWidth;
        long mindt = -1L;
        int state = 0;
        while (idx <= idx2 && !p.isCanceled()) {
            boolean hasValues;
            long diff;
            int x = (int)axis.pixels(t);
            if (mindt == -1L || !axis.isLinear()) {
                mindt = axis.deltaUnits(x, mindx);
                boolean bl = minDtZero = mindt <= 0L;
                if (minDtZero) {
                    mindt = 1L;
                }
            }
            if ((diff = nt - t) >= mindt || diff < 0L) {
                this.planNormal(generator, axis, idx, t, x, nt);
                state = 0;
                t = nt;
                nt = this.samples.unitsAt(++idx + 1);
                continue;
            }
            int nidx = this.samples.indexAt(t + mindt);
            if (minDtZero && nidx > idx + 1) {
                --nidx;
            }
            if (nidx <= idx) {
                nidx = idx + 1;
            }
            nt = this.samples.unitsAt(nidx);
            diff = nt - t;
            SamplesStat stat = this.samples.getStat(idx, nidx - 1, 1 | (this.paintNormalIfNoChange ? 2 : 0));
            int tag = stat.tag & 0xF;
            boolean hasChanges = (stat.flags & 4) != 0;
            boolean hasNone = (stat.flags & 8) != 0;
            boolean bl = hasValues = (stat.flags & 0x10) != 0;
            if (this.paintNormalIfNoChange && !hasChanges || hasNone && !hasValues && !hasChanges) {
                this.planNormal(generator, axis, idx, t, x, nt);
                state = 0;
            } else if (!histo || diff < mindt / 2L) {
                if (state != 1 + tag) {
                    this.lastPaint = 0xF000 | tag;
                    generator.add(this.lastPaint, x, idx);
                    state = 1 + tag;
                }
            } else {
                double load;
                double d = load = diff > 0L ? 1.0 * (double)(nidx - idx) / (double)diff : 0.0;
                if (maxLoad < load) {
                    maxLoad = load;
                }
                if (minLoad > load) {
                    minLoad = load;
                }
                this.lastPaint = 0xF000 | tag;
                generator.add(this.lastPaint, x, load, idx);
                state = 1 + tag;
            }
            idx = nidx;
            t = nt;
            nt = this.samples.unitsAt(idx + 1);
        }
        generator.applyScale(0, null, 0.0, minLoad + maxLoad);
        generator.applyAttachments();
    }
}

