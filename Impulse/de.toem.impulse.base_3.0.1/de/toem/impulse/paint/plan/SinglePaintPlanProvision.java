/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.PaintPlan;
import de.toem.impulse.paint.plan.PaintPlanBinary;
import de.toem.toolkits.pattern.json.IJsonBase;
import java.util.Map;
import org.eclipse.swt.graphics.Rectangle;

public class SinglePaintPlanProvision
extends PaintPlanBinary
implements IPlan.ISinglePaintPlanProvision {
    protected PaintPlan plan = null;

    public SinglePaintPlanProvision(IPlotItem item, IDomainAxis axis, Rectangle area, int style, int scheme) {
        super(item, axis, area, style, scheme);
    }

    protected SinglePaintPlanProvision(Object jsonObject, IJsonBase.IObjectConversion<Object, Object> replacement) {
        super(jsonObject, replacement);
    }

    @Override
    public int size() {
        return 1;
    }

    @Override
    public ITreeItem getItem(int n) {
        if (n == 0) {
            return this.item;
        }
        return null;
    }

    @Override
    public IPlan.IPaintPlan preparePlan(int n) {
        if (n == 0) {
            return this.preparePlan();
        }
        return null;
    }

    @Override
    public ITreeItem getItem() {
        return this.item;
    }

    @Override
    public IPlan.IPaintPlan preparePlan() {
        if (this.plan == null) {
            this.plan = new PaintPlan(this);
        } else {
            this.plan.assign(this);
        }
        return this.plan;
    }

    @Override
    public Map<String, Object> toJson(IJsonBase.IObjectConversion<Object, Object> replacement) {
        return super.toJson((IJsonBase.IObjectConversion)replacement);
    }

    @Override
    public boolean isDelta() {
        return this.delta != 0;
    }

    @Override
    public int getDelta() {
        return this.delta;
    }

    @Override
    public IPlan.IPaintPlanProvision extend(IPlan.IPaintPlanProvision previous) {
        if (previous == this) {
            return this;
        }
        if (!this.isDelta()) {
            return this;
        }
        if (previous instanceof SinglePaintPlanProvision) {
            ((SinglePaintPlanProvision)previous).assign(this);
            return previous;
        }
        return null;
    }
}

