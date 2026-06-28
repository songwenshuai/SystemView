/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.plan.MultiPaintPlanner;
import java.util.List;
import org.eclipse.swt.graphics.Rectangle;

public class MultiChartPaintPlanner
extends MultiPaintPlanner {
    public MultiChartPaintPlanner(ITheme theme, List<? extends IPlotItem> plannables, IDomainAxis axis, Rectangle area, int style) {
        super(plannables, axis, theme, area, style);
        this.createDefaultPlanners(plannables);
    }
}

