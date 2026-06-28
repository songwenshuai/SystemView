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
import java.util.List;
import org.eclipse.swt.graphics.Rectangle;

public class VectorPaintPlanner
extends AbstractVolatilePaintPlanner {
    protected VectorPaintPlanner(ITheme theme, IPlotItem item, IDomainAxis axis, Rectangle area, int style) {
        super(theme, item, axis, area, style);
    }

    protected VectorPaintPlanner(MultiPaintPlanner multi, IPlotItem item) {
        super(multi, item);
    }

    @Override
    protected final void planNormal(IPlan.IPaintPlanGenerator generator, IDomainAxis axis, int idx, long t, int x, long nt) {
        CompoundValue compound = this.samples.compoundAt(idx, ((this.paintStyle.diagramMods & 0x1000) != 0 && !this.relationLocked ? 4 : 0) | ((this.paintStyle.diagramMods & 0x2000) != 0 && !this.labelsLocked ? 8 : 0));
        if (compound != null) {
            int tag = compound.getTag() & 0xF;
            if (!compound.isNone()) {
                String value = compound.format(this.paintStyle.format);
                this.lastPaint = 0x2000 | tag;
                generator.add(this.lastPaint, x, idx, value);
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

