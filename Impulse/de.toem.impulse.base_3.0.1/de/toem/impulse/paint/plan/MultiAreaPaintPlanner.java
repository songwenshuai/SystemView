/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.MultiPaintPlanProvision;
import de.toem.impulse.paint.plan.MultiPaintPlanner;
import de.toem.impulse.paint.plan.PaintStyle;
import de.toem.impulse.paint.plan.SinglePaintPlanner;
import java.util.List;
import org.eclipse.swt.graphics.Rectangle;

public class MultiAreaPaintPlanner
extends MultiPaintPlanner {
    public MultiAreaPaintPlanner(ITheme theme, List<? extends IPlotItem> plannables, IDomainAxis axis, Rectangle area, int style) {
        super(plannables, axis, theme, area, style);
        this.createDefaultPlanners(plannables);
    }

    @Override
    protected boolean extend(MultiPaintPlanProvision multiProvision) {
        for (IPlan.IPaintPlanProvision provision : multiProvision) {
            PaintStyle paintStyle;
            IPlan.IPaintPlanGenerator generator;
            SinglePaintPlanner planner;
            if (!(provision instanceof IPlan.IPaintPlanGenerator) || (planner = (SinglePaintPlanner)this.planners.get((generator = (IPlan.IPaintPlanGenerator)provision).getItem())) == null || (paintStyle = planner.getPaintStyle()) == null) continue;
            if ((paintStyle.getMods() & 0x40) != 0) {
                generator.applyScale(paintStyle.getScaleType(), paintStyle.getScaleUnit(), paintStyle.getScaleFrom(), paintStyle.getScaleTo());
                continue;
            }
            generator.applyScale(paintStyle.getScaleType(), paintStyle.getScaleUnit(), null, null);
        }
        return true;
    }
}

