/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.plan.AbstractVolatileGroupPaintPlanner;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.MultiPaintPlanner;
import de.toem.impulse.values.IAttachment;
import de.toem.impulse.values.Transaction;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.filter.FilterExpression;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import org.eclipse.swt.graphics.Rectangle;

public class TransactionPaintPlanner
extends AbstractVolatileGroupPaintPlanner {
    private List<FilterExpression> filters;

    protected TransactionPaintPlanner(ITheme theme, IPlotItem item, IDomainAxis axis, Rectangle area, int style) {
        super(item, axis, theme, area, style);
    }

    protected TransactionPaintPlanner(MultiPaintPlanner multi, IPlotItem item) {
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
    protected void planNormal(IPlan.IPaintPlanGenerator generator, IDomainAxis axis, int group, int layer, int idx, int lidx, long t, int x, long nt) {
        Transaction transaction = null;
        transaction = (Transaction)this.samples.valuesAtGroup(group);
        if (transaction != null) {
            List<IAttachment> attachments;
            long lt = this.samples.unitsAt(lidx);
            int x2 = (int)axis.pixels(lt);
            int paint = 20480;
            if (lt - t == 0L || transaction.length() == 1) {
                paint = 24576;
                x2 = (int)axis.pixels(nt);
            }
            int[] xn = null;
            if (transaction.noOfEvents() > 2) {
                xn = new int[6];
                xn[0] = x;
                xn[1] = x2;
                int m = 2;
                int n = 1;
                while (n < transaction.noOfEvents() - 1 && m < 6) {
                    xn[m] = (int)axis.pixels(transaction.get(n).getUnits());
                    ++n;
                    ++m;
                }
                while (m < 6) {
                    xn[m] = Short.MIN_VALUE;
                    ++m;
                }
            } else {
                xn = new int[]{x, x2};
            }
            Transaction members = transaction;
            ArrayList<String> values = null;
            if (Math.abs(x2 - x) > 30) {
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
                    values.add(transaction.format(this.paintStyle.format));
                }
            }
            this.lastPaint = paint | transaction.getTag() & 0xF;
            generator.add(this.lastPaint, xn, (short)layer, (short)0, group, values);
            if (((this.paintStyle.diagramMods & 0x1000) != 0 && !this.relationLocked || (this.paintStyle.diagramMods & 0x2000) != 0 && !this.labelsLocked) && (attachments = this.samples.attachmentsAt(idx, ((this.paintStyle.diagramMods & 0x1000) != 0 && !this.relationLocked ? 4 : 0) | ((this.paintStyle.diagramMods & 0x2000) != 0 && !this.labelsLocked ? 8 : 0))) != null) {
                for (IAttachment a : attachments) {
                    generator.attach(a instanceof IAttachment.IAttachedRelation ? 57600 : 57856, x, idx, a);
                }
            }
        }
    }
}

