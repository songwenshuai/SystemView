/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.PaintPlanProvision;
import de.toem.impulse.paint.plan.SinglePaintPlanProvision;
import de.toem.toolkits.pattern.json.IJsonBase;
import de.toem.toolkits.pattern.json.Json;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.HashMap;

public class MultiPaintPlanProvision
extends ArrayList<IPlan.ISinglePaintPlanProvision>
implements IPlan.IPaintPlanProvision {
    private static final long serialVersionUID = 1L;
    private int delta;

    public MultiPaintPlanProvision() {
    }

    protected MultiPaintPlanProvision(Object jsonObject, IJsonBase.IObjectConversion<Object, Object> replacement) {
        try {
            if (Json.has(jsonObject, "multi")) {
                this.delta = Json.get(jsonObject, 0, "delta");
                Object multi = Json.get(jsonObject, "multi");
                int n = 0;
                while (n < Json.length(multi)) {
                    Object item = Json.get(multi, n);
                    if (Json.has(item, "message")) {
                        this.add(new PaintPlanProvision.MessageProvision(item, replacement));
                    } else {
                        this.add(new SinglePaintPlanProvision(item, replacement));
                    }
                    ++n;
                }
            }
        }
        catch (Throwable throwable) {}
    }

    @Override
    public ITreeItem getItem(int n) {
        return ((IPlan.ISinglePaintPlanProvision)this.get(n)).getItem();
    }

    @Override
    public IPlan.IPaintPlan preparePlan(int n) {
        return ((IPlan.ISinglePaintPlanProvision)this.get(n)).preparePlan();
    }

    @Override
    public Object toJson(IJsonBase.IObjectConversion<Object, Object> replacement) {
        HashMap<String, Serializable> map = new HashMap<String, Serializable>();
        ArrayList<Object> list = new ArrayList<Object>();
        for (IPlan.ISinglePaintPlanProvision p : this) {
            list.add(p.toJson(replacement));
        }
        map.put("multi", list);
        map.put("delta", Integer.valueOf(this.delta));
        return map;
    }

    @Override
    public IPlan.IPaintPlanProvision extend(IPlan.IPaintPlanProvision previous) {
        if (previous == this) {
            return this;
        }
        if (!this.isDelta()) {
            return this;
        }
        return null;
    }

    @Override
    public boolean isDelta() {
        return this.delta != 0;
    }

    @Override
    public int getDelta() {
        return this.delta;
    }
}

