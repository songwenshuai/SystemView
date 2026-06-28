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
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.core.Assert;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.List;
import org.eclipse.swt.graphics.Rectangle;

public class LinePaintPlanner
extends SinglePaintPlanner {
    protected int lastPaint = 0;

    protected LinePaintPlanner(ITheme theme, IPlotItem item, IDomainAxis axis, Rectangle area, int style) {
        super(theme, item, axis, area, style);
    }

    protected LinePaintPlanner(MultiPaintPlanner multi, IPlotItem item) {
        super(multi, item);
    }

    @Override
    public void createPlan(IProgress p, IPlan.IPaintPlanGenerator generator, IDomainAxis axis, Rectangle area, boolean extend) {
        int idx;
        Assert.isFalse(extend);
        int px1 = area.x;
        int px2 = px1 + area.width;
        generator.setEndX((int)axis.pixels(this.samples.getEndUnits()));
        generator.setTagDomain(this.samples.getTagDomain());
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
        int nidx = idx = idx1;
        long t = this.samples.unitsAt(idx1);
        int x = (int)axis.pixels(t);
        int mindx = this.minSampleWidth;
        while (idx <= idx2 && nidx <= idx2 && !p.isCanceled()) {
            long nt;
            int nx;
            if ((nx = (int)axis.pixels(nt = this.samples.unitsAt(++nidx))) >= x + mindx || idx == idx2) {
                int tag = this.samples.getTagAt(idx) & 0xF;
                if (!this.samples.isNoneAt(idx)) {
                    List<IAttachment> attachments;
                    double y = this.samples.floatValueAt(idx);
                    this.lastPaint = 0x4100 | tag;
                    generator.add(this.lastPaint, x, y, idx);
                    if (((this.paintStyle.diagramMods & 0x1000) != 0 && !this.relationLocked || (this.paintStyle.diagramMods & 0x2000) != 0 && !this.labelsLocked) && (attachments = this.samples.attachmentsAt(idx, ((this.paintStyle.diagramMods & 0x1000) != 0 && !this.relationLocked ? 4 : 0) | ((this.paintStyle.diagramMods & 0x2000) != 0 && !this.labelsLocked ? 8 : 0))) != null) {
                        for (IAttachment a : attachments) {
                            generator.attach(a instanceof IAttachment.IAttachedRelation ? 57600 : 57856, x, y, idx, a);
                        }
                    }
                } else {
                    int paint = tag;
                    if (this.lastPaint != paint) {
                        this.lastPaint = paint;
                        generator.add(this.lastPaint, x, idx);
                    }
                }
            } else {
                SamplesStat stat;
                nx = x + mindx;
                nt = (long)axis.units(nx);
                int lidx = this.samples.indexAt(nt - 1L);
                if (lidx < idx) {
                    lidx = idx;
                }
                if (lidx > idx2) {
                    lidx = idx2;
                }
                if ((nidx = lidx + 1) <= idx2) {
                    long tt = this.samples.unitsAt(nidx);
                    long tx = (int)axis.pixels(tt);
                    if (tx > (long)nx) {
                        nidx = lidx;
                    }
                } else {
                    nidx = lidx;
                }
                if ((stat = this.samples.getStat(idx, lidx, 8)) != null) {
                    int tag = stat.tag & 0xF;
                    if (stat.min < stat.first && stat.min < stat.last || stat.max > stat.first && stat.max > stat.last) {
                        this.lastPaint = 0x4400 | tag;
                        generator.add(this.lastPaint, x, stat.first, stat.last, stat.min, stat.max, idx);
                    } else {
                        this.lastPaint = 0x4200 | tag;
                        generator.add(this.lastPaint, x, stat.first, stat.last, idx);
                    }
                } else {
                    int paint = 0;
                    if (this.lastPaint != paint) {
                        this.lastPaint = paint;
                        generator.add(this.lastPaint, x, idx);
                    }
                }
            }
            idx = nidx;
            t = nt;
            x = nx;
        }
        if ((this.paintStyle.getMods() & 0x40) != 0) {
            generator.applyScale(this.paintStyle.getScaleType(), this.paintStyle.getScaleUnit(), this.paintStyle.getScaleFrom(), this.paintStyle.getScaleTo());
        } else {
            generator.applyScale(this.paintStyle.getScaleType(), this.paintStyle.getScaleUnit(), null, null);
        }
    }
}

