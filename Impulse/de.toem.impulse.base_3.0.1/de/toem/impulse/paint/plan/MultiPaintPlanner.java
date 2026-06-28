/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.plan.AbstractPaintPlanner;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.MultiPaintPlanProvision;
import de.toem.impulse.paint.plan.PaintPlanner;
import de.toem.impulse.paint.plan.SinglePaintPlanner;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import org.eclipse.swt.graphics.Rectangle;

public abstract class MultiPaintPlanner
extends AbstractPaintPlanner
implements IPlan.IMultiPaintPlanner<IPlotItem> {
    protected List<IPlotItem> plannables = new ArrayList<IPlotItem>();
    protected LinkedHashMap<IPlotItem, SinglePaintPlanner> planners = new LinkedHashMap();

    protected MultiPaintPlanner(List<? extends IPlotItem> plannables, IDomainAxis axis, ITheme theme, Rectangle area, int style) {
        super(theme, axis, area, style);
        this.plannables.addAll(plannables);
    }

    protected void createDefaultPlanners(List<? extends IPlotItem> plannables) {
        for (IPlotItem iPlotItem : plannables) {
            SinglePaintPlanner planner = PaintPlanner.create(this, iPlotItem, this.area);
            this.planners.put(iPlotItem, planner);
        }
    }

    @Override
    public Rectangle childArea(IPlan.ISinglePaintPlanner<IPlotItem> child) {
        IPlotItem item;
        IPlotItem iPlotItem = item = child != null ? child.getItem() : null;
        if (item == null || this.area == null) {
            return this.area;
        }
        Rectangle used = item.getUsedPlotBounds();
        if (used == null) {
            return this.area;
        }
        return new Rectangle(this.area.x + used.x, this.area.y + used.y, used.width, used.height);
    }

    @Override
    public int getPriority() {
        int prio = -1;
        for (SinglePaintPlanner p : this.planners.values()) {
            if (p == null) continue;
            prio = Math.max(prio, p.getPriority());
        }
        return prio;
    }

    @Override
    public List<IPlotItem> getItems() {
        return this.plannables;
    }

    @Override
    public int checkAndUpdate(IDomainAxis axis, Rectangle area, int style) {
        int checked = 1;
        for (IPlotItem item : this.plannables) {
            SinglePaintPlanner planner = this.planners.get(item);
            if (planner != null) {
                checked = IPlan.CHECK_MERGE(checked, planner.checkAndUpdate(axis, area, style));
                continue;
            }
            return 0;
        }
        return checked;
    }

    @Override
    public IPlan.IPaintPlanProvision plan(IProgress p) {
        if (this.state != 0) {
            return null;
        }
        MultiPaintPlanProvision plans = new MultiPaintPlanProvision();
        this.planAxis = this.axis != null ? this.axis.clone() : null;
        for (IPlotItem item : this.planners.keySet()) {
            SinglePaintPlanner planner = this.planners.get(item);
            if (planner == null) continue;
            IPlan.ISinglePaintPlanProvision planned = planner.plan(p, this.planAxis);
            plans.add(planned);
        }
        if (!p.isCanceled()) {
            this.extend(plans);
        }
        return plans;
    }

    protected boolean extend(MultiPaintPlanProvision provision) {
        return false;
    }

    @Override
    public IPlan.IPaintPlanProvision createTimeoutProvision(boolean killed) {
        MultiPaintPlanProvision provision = new MultiPaintPlanProvision();
        for (IPlotItem item : this.planners.keySet()) {
            provision.add(this.createTimeoutProvision(item, killed));
        }
        return provision;
    }
}

