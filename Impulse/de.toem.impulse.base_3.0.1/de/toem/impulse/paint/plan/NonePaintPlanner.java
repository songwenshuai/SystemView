/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.MultiPaintPlanner;
import de.toem.impulse.paint.plan.SinglePaintPlanner;
import de.toem.toolkits.pattern.threading.IProgress;
import org.eclipse.swt.graphics.Rectangle;

public class NonePaintPlanner
extends SinglePaintPlanner {
    protected static final int SCHEME_NONE_DEFAULT = 192;
    public static final int NONE_PAINTSTYLE = 1;
    public static final int LICENSE_LOCK = 2;
    private int type;

    protected NonePaintPlanner(ITheme theme, IPlotItem item, IDomainAxis axis, Rectangle area, int style, int type) {
        super(theme, item, axis, area, style);
        this.scheme = 192;
        this.type = type;
    }

    protected NonePaintPlanner(MultiPaintPlanner multi, IPlotItem item, int type) {
        super(multi, item);
        this.scheme = 192;
        this.type = type;
    }

    @Override
    protected String checkPlan() {
        if (this.type == 2) {
            return I18n.General_NoLicense;
        }
        return null;
    }

    @Override
    protected void createPlan(IProgress progress, IPlan.IPaintPlanGenerator generator, IDomainAxis axis, Rectangle area, boolean extend) {
    }
}

