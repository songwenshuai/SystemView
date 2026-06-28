/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.plan.AbstractVolatilePaintPlanner;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.MultiPaintPlanner;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.filter.FilterExpression;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import org.eclipse.swt.graphics.Rectangle;

public class LogPaintPlanner
extends AbstractVolatilePaintPlanner {
    private List<FilterExpression> filters;

    protected LogPaintPlanner(ITheme theme, IPlotItem item, IDomainAxis axis, Rectangle area, int style) {
        super(theme, item, axis, area, style);
    }

    protected LogPaintPlanner(MultiPaintPlanner multi, IPlotItem item) {
        super(multi, item);
    }

    @Override
    public void createPlan(IProgress progress, IPlan.IPaintPlanGenerator generator, IDomainAxis axis, Rectangle area, boolean extend) {
        String filter;
        PropertyModel parameters = new PropertyModel();
        if (parameters != null) {
            parameters.setTotal(this.paintStyle.additional);
        }
        if (!Utils.isEmpty(filter = parameters.get("filter"))) {
            this.filters = new ArrayList<FilterExpression>();
            String[] stringArray = filter.split(",");
            int n = stringArray.length;
            int n2 = 0;
            while (n2 < n) {
                String f = stringArray[n2];
                this.filters.add(new FilterExpression(f, 3));
                ++n2;
            }
        }
        super.createPlan(progress, generator, axis, area, extend);
    }

    @Override
    protected final void planNormal(IPlan.IPaintPlanGenerator generator, IDomainAxis axis, int idx, long t, int x, long nt) {
        CompoundValue compound = this.samples.compoundAt(idx, ((this.paintStyle.diagramMods & 0x1000) != 0 && !this.relationLocked ? 4 : 0) | ((this.paintStyle.diagramMods & 0x2000) != 0 && !this.labelsLocked ? 8 : 0));
        if (compound != null) {
            int tag = compound.getTag() & 0xF;
            if (!compound.isNone()) {
                int nx = (int)axis.pixels(nt);
                CompoundValue members = compound;
                ArrayList<String> values = null;
                if (Math.abs(nx - x) > 30) {
                    values = new ArrayList<String>();
                    if (members.noOfMembers() > 0) {
                        if (this.filters != null) {
                            HashSet<String> added = new HashSet<String>();
                            for (FilterExpression filter : this.filters) {
                                int n = 0;
                                while (n < members.noOfMembers()) {
                                    String name = members.nameOf(n);
                                    if (!added.contains(name) && filter.matches(name)) {
                                        values.add(name);
                                        values.add(members.textOf(n));
                                        added.add(name);
                                    }
                                    ++n;
                                }
                            }
                        } else {
                            int n = 0;
                            while (n < members.noOfMembers()) {
                                values.add(members.nameOf(n));
                                values.add(members.textOf(n));
                                ++n;
                            }
                        }
                    } else {
                        values.add(compound.format(this.paintStyle.format));
                    }
                }
                this.lastPaint = 0x6000 | tag;
                generator.add(this.lastPaint, x, idx, values);
                List<IAttachment> attachments = compound.attachments(12);
                if (attachments != null) {
                    for (IAttachment a : attachments) {
                        generator.attach(a instanceof IAttachment.IAttachedRelation ? 57600 : 57856, x, idx, a);
                    }
                }
            } else {
                int paint = tag;
                if (this.lastPaint != paint) {
                    this.lastPaint = paint;
                    generator.add(this.lastPaint, x, idx);
                }
            }
        }
    }
}

