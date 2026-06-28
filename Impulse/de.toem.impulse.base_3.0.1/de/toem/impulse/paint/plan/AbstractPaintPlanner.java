/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.PaintPlanProvision;
import de.toem.toolkits.core.Assert;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.pattern.threading.IProgressStatus;
import org.eclipse.swt.graphics.Rectangle;

public abstract class AbstractPaintPlanner
implements IPlan.IPaintPlanner<IPlotItem> {
    public static boolean CLONE = true;
    protected int scheme = 4577;
    protected IDomainAxis axis;
    protected ITheme theme;
    protected Rectangle area;
    protected int style;
    protected static final int STATE_CREATED = 0;
    protected static final int STATE_PLANNING = 2;
    protected static final int STATE_SUCCESS = 4;
    protected static final int STATE_CANCELED = -1;
    protected static final int STATE_DISPOSED = -2;
    protected static final int STATE_NOT_APPLICABLE_ERROR = -3;
    protected static final int STATE_PARAMETER_ERROR = -4;
    protected static final int STATE_INPUT_ERROR = -5;
    protected static final int STATE_PLANNING_ERROR = -6;
    protected static final int STATE_SCHEDULE_ERROR = -7;
    protected int state;
    protected IDomainAxis planAxis;
    protected Rectangle planArea;
    Object client;
    int counter;

    public AbstractPaintPlanner(ITheme theme, IDomainAxis axis, Rectangle area, int style) {
        this.axis = axis;
        this.theme = theme;
        this.area = area;
        this.style = style;
        this.state = 0;
        Assert.isTrue(axis != null && theme != null && area != null);
    }

    public AbstractPaintPlanner(IPlan.IMultiPaintPlanner<IPlotItem> master, Rectangle area) {
        this.axis = master.getAxis();
        this.theme = master.getTheme();
        this.area = area != null ? area : master.getArea();
        this.style = master.getStyle();
        this.state = 0;
        Assert.isTrue(this.axis != null && this.theme != null && this.area != null);
    }

    @Override
    public synchronized void dispose() {
        this.state = -2;
    }

    @Override
    public boolean isDisposed() {
        return this.state == -2;
    }

    @Override
    public int getPriority() {
        return 5;
    }

    @Override
    public IDomainAxis getAxis() {
        return this.axis;
    }

    @Override
    public Rectangle getArea() {
        return this.area;
    }

    @Override
    public int getStyle() {
        return this.style;
    }

    @Override
    public ITheme getTheme() {
        return this.theme;
    }

    @Override
    public int checkAndUpdate(IDomainAxis axis, Rectangle area, int style) {
        if (this.state == -2) {
            return 0;
        }
        Assert.isTrue(axis != null && area != null);
        int checked = 1;
        if (this.state == 0 || this.state == 2) {
            checked |= 8;
        }
        checked = IPlan.CHECK_MERGE(checked, this.checkAndUpdateAxisArea(axis, area, style));
        checked = IPlan.CHECK_MERGE(checked, this.checkAndUpdateStyle(style));
        if ((this.scheme & 0x200) != 0 && this.state == 4) {
            checked = IPlan.CHECK_ADD(checked, 2);
        }
        return checked;
    }

    protected int checkAndUpdateAxisArea(IDomainAxis axis, Rectangle area, int style) {
        int checked = 1;
        if ((this.scheme & 0x20) != 0) {
            if (this.state == 0) {
                if (!Utils.equals(this.area, area) || !Utils.equals(this.axis, axis)) {
                    this.area = area;
                    this.axis = axis;
                    checked = IPlan.CHECK_ADD(checked, 4);
                }
            } else if (this.state == 2 || this.state == 4) {
                double[] scaling = this.axis.getLinearScalingFrom(this.planAxis);
                if (scaling == null) {
                    return 0;
                }
                if (scaling[1] != 1.0) {
                    return 0;
                }
                if (!this.planArea.contains((int)((double)area.x - scaling[0]), this.planArea.y) && this.planArea.contains((int)((double)(area.x + area.width - 1) - scaling[0]), this.planArea.y)) {
                    return 0;
                }
            }
        }
        if ((this.scheme & 0x100) != 0) {
            if (this.state == 0) {
                if (!Utils.equals(this.area, area) || !Utils.equals(this.axis, axis)) {
                    this.area = area;
                    this.axis = axis;
                    checked = IPlan.CHECK_ADD(checked, 4);
                }
            } else if (!(this.state != 2 && this.state != 4 || this.planArea.equals(area))) {
                return 0;
            }
        }
        return checked;
    }

    protected int checkAndUpdateStyle(int style) {
        int checked = 1;
        if ((this.scheme & 0x10) != 0) {
            if (this.state == 0) {
                if (this.style != style) {
                    this.style = style;
                    checked = IPlan.CHECK_ADD(checked, 4);
                }
            } else if (this.state == 2 || this.state == 4) {
                return 0;
            }
        }
        return checked;
    }

    protected Rectangle calcPlanArea(Rectangle area) {
        return (this.scheme & 1) != 0 ? new Rectangle(area.x - 250, area.y, area.width + 500, area.height) : new Rectangle(area.x, area.y, area.width, area.height);
    }

    @Override
    public void setClient(Object handle, int counter) {
        this.client = handle;
        this.counter = counter;
    }

    @Override
    public void setCounter(int id) {
        this.counter = id;
    }

    @Override
    public Object getClient() {
        return this.client;
    }

    @Override
    public int getCounter() {
        return this.counter;
    }

    protected IPlan.ISinglePaintPlanProvision createErrorProvision(IPlotItem item, String message) {
        return PaintPlanProvision.newMessage(item, this.scheme, message);
    }

    protected IPlan.ISinglePaintPlanProvision createCancelProvision(IPlotItem item, IProgress p) {
        if (p instanceof IProgressStatus && ((IProgressStatus)p).hadTimeout()) {
            return PaintPlanProvision.newMessage(item, this.scheme, I18n.General_Timeout);
        }
        return PaintPlanProvision.newMessage(item, this.scheme, I18n.General_Cancelled);
    }

    protected IPlan.ISinglePaintPlanProvision createTimeoutProvision(IPlotItem item, boolean killed) {
        return PaintPlanProvision.newMessage(item, this.scheme, killed ? I18n.General_TimeoutKilled : I18n.General_Timeout);
    }
}

