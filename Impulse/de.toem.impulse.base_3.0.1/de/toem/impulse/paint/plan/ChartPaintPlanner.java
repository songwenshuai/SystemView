/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.cells.charts.AbstractChartCell;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.MultiPaintPlanner;
import de.toem.impulse.paint.plan.SinglePaintPlanner;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import org.eclipse.swt.graphics.Rectangle;

public class ChartPaintPlanner
extends SinglePaintPlanner {
    protected static final int SCHEME_CHART_DEFAULT = 192;
    long started = Utils.millies();
    private Object color;

    protected ChartPaintPlanner(ITheme theme, IPlotItem item, IDomainAxis axis, Rectangle area, int style) {
        super(theme, item, axis, area, style);
        this.scheme = 192;
    }

    protected ChartPaintPlanner(MultiPaintPlanner multi, IPlotItem item, Rectangle area) {
        super(multi, item, area);
        this.scheme = 192;
    }

    @Override
    protected int checkAndUpdatePaintStyle(int style) {
        int checked = super.checkAndUpdatePaintStyle(style);
        if ((this.scheme & 0x80) != 0 && this.state != 0 && this.color != null && !Utils.equals(this.color, this.item.getColor())) {
            return 0;
        }
        return checked;
    }

    @Override
    public int getPriority() {
        long waiting = Utils.millies() - this.started;
        if (waiting < 250L) {
            return -1;
        }
        return (int)(waiting / 1000L);
    }

    @Override
    protected void createPlan(IProgress progress, IPlan.IPaintPlanGenerator generator, IDomainAxis axis, Rectangle area, boolean extend) {
        ICell descriptor;
        int px1 = area.x;
        int cfr_ignored_0 = area.width;
        this.color = this.item.getColor();
        generator.setEndX(Integer.MAX_VALUE);
        ICell iCell = descriptor = this.paintStyle.descriptor != null ? this.paintStyle.descriptor.resolveCell(ImpulsePreferences.chartPreferences) : null;
        if (this.paintStyle != null && descriptor instanceof AbstractChartCell) {
            Rectangle childArea;
            IPropertyModel parameters = ((AbstractChartCell)descriptor).getPropertyModel(true);
            if (parameters != null) {
                parameters.setTotal(this.paintStyle.additional);
            }
            AbstractChartCell chartCell = (AbstractChartCell)descriptor;
            Rectangle rectangle = childArea = this.master != null ? this.master.childArea(this) : area;
            if (chartCell instanceof IPlan.IExternalPlanner) {
                ((IPlan.IExternalPlanner)((Object)chartCell)).plan(this, progress, generator, parameters, axis, childArea, extend);
            }
        }
    }
}

