/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.axis.IValueAxis;
import de.toem.impulse.paint.IPaint;
import de.toem.impulse.paint.IPaintStyle;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.PaintPlanBinary;
import de.toem.impulse.paint.plan.PaintPlanIterator;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.scripting.PaintScriptContextProvider;
import de.toem.toolkits.core.Assert;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.scripting.IScripting;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.ui.tlk.ITlkPainter;
import org.eclipse.swt.graphics.Rectangle;

public class PaintPlan
extends PaintPlanBinary
implements IPlan.IPaintPlan {
    private IPaintStyle paintStyle;
    protected double[] scale = new double[]{0.0, 1.0};
    protected IScripting scripting;

    public PaintPlan(PaintPlanBinary that) {
        super(that);
        this.area = new Rectangle(this.area.x, this.area.y, this.area.width, this.area.height);
        this.paintStyle = this.item.getPaintStyle();
    }

    @Override
    public boolean isEmpty() {
        return this.pLen == 0 && this.iLen == 0 && !this.hasPaintStyle();
    }

    @Override
    public boolean hasPaintStyle() {
        return this.paintStyle != null;
    }

    @Override
    public IPaintStyle getPaintStyle() {
        return this.paintStyle;
    }

    @Override
    public boolean hasValueAxis() {
        return this.vaxis != null && this.paintStyle != null && this.paintStyle.hasValueAxis();
    }

    @Override
    public IValueAxis getValueAxis() {
        return this.vaxis;
    }

    @Override
    public int getEndX() {
        return this.xEnd;
    }

    @Override
    public int getLayers() {
        return this.layers;
    }

    @Override
    public ISamples.TagDomain getTagDomaim() {
        return this.tagDomain;
    }

    @Override
    public double getA() {
        return 0.0;
    }

    private void scale(double[] scale) {
        this.scale[1] = scale[1] * this.scale[1];
        this.scale[0] = scale[1] * this.scale[0] + scale[0];
        this.xEnd = (int)((double)this.xEnd * scale[1] + scale[0]);
        this.area.x = (int)((double)this.area.x * scale[1] + scale[0]);
        this.area.width = (int)((double)this.area.width * scale[1]);
    }

    public void log() {
        PaintPlanIterator iter = new PaintPlanIterator(this, false);
        while (iter.hasNext()) {
            iter.next();
            Utils.lprint(iter.paint(), iter.index(), iter.x(), iter.a(), iter.b());
        }
    }

    @Override
    public String getStatus() {
        return this.status;
    }

    @Override
    public int yields(IDomainAxis axis, Rectangle area, int style, Object image) {
        int yields = 51;
        if (image == null || !(image instanceof IPaint.CachedImage) || ((IPaint.CachedImage)image).image == null) {
            yields = IPlan.YIELD_REMOVE(yields, 48);
        }
        if ((this.scheme & 0x20) != 0) {
            double[] scaling;
            double[] dArray = scaling = this.axis != null && axis != null ? axis.getLinearScalingFrom(this.axis) : null;
            if (scaling == null) {
                yields = 0;
            } else {
                if (scaling[1] != 1.0 || scaling[0] != 0.0 && Math.abs(scaling[0]) < (double)(this.area.width / 2)) {
                    this.scale(scaling);
                    this.axis = axis.clone();
                    yields |= 4;
                }
                if ((int)scaling[0] != 0) {
                    yields = IPlan.YIELD_REMOVE(yields, 48);
                }
            }
        }
        if (image instanceof Rectangle) {
            boolean copy;
            Rectangle newBounds = area;
            Rectangle newImageBounds = (Rectangle)image;
            boolean bl = copy = newImageBounds.width == 0;
            if (newImageBounds.width != 0 && !newImageBounds.equals(newBounds)) {
                if ((newBounds = newImageBounds.intersection(newBounds)) == null || newBounds.width == 0 || newBounds.height == 0) {
                    return 0;
                }
                copy = true;
            }
            if (copy) {
                newImageBounds.x = newBounds.x;
                newImageBounds.y = newBounds.x;
                newImageBounds.width = newBounds.width;
                newImageBounds.height = newBounds.height;
            }
        }
        return yields;
    }

    @Override
    public int isApplicable(int changed, IDomainAxis axis, Rectangle area, int style, IPaint.CachedImage image) {
        Assert.isTrue(axis != null && area != null);
        int applicable = 51;
        if (image == null) {
            applicable = IPlan.YIELD_REMOVE(applicable, 48);
        }
        if ((applicable & 1) != 0 && (changed & 0x10) != 0 && (this.scheme & 0x40) != 0) {
            applicable = IPlan.YIELD_REMOVE(applicable, 2);
        }
        if ((applicable & 1) != 0 && (changed & 3) != 0 && (this.scheme & 0x20) != 0) {
            double[] deltaPixels = axis.getLinearScalingFrom(this.axis);
            if (deltaPixels == null) {
                applicable = 0;
            } else if (deltaPixels[0] != 0.0 || deltaPixels[1] != 1.0) {
                applicable = IPlan.YIELD_REMOVE(applicable, 48);
                if (deltaPixels[1] != 1.0) {
                    applicable = IPlan.YIELD_REMOVE(applicable, 2);
                }
                if (!this.area.contains((int)((double)area.x - deltaPixels[0]), this.area.y) || !this.area.contains((int)((double)(area.x + area.width - 1) - deltaPixels[0]), this.area.y)) {
                    applicable = IPlan.YIELD_REMOVE(applicable, 2);
                    if (style == 0 && !this.area.contains((int)((double)area.x - deltaPixels[0]), this.area.y) && !this.area.contains((int)((double)(area.x + area.width - 1) - deltaPixels[0]), this.area.y)) {
                        applicable = IPlan.YIELD_REMOVE(applicable, 1);
                    }
                }
            }
        }
        if ((applicable & 1) != 0 && (changed & 2) != 0 && (this.scheme & 0x100) != 0 && !this.area.equals(area)) {
            applicable = IPlan.YIELD_REMOVE(applicable, 2);
        }
        if ((applicable & 1) != 0 && (changed & 2) != 0 && (this.scheme & 0x2000) != 0 && !this.area.equals(area)) {
            applicable = IPlan.YIELD_REMOVE(applicable, 32);
        }
        if ((applicable & 1) != 0 && (changed & 0x20) != 0 && (this.scheme & 0x80) != 0) {
            if ((changed & 0x20) != 0) {
                applicable = IPlan.YIELD_REMOVE(applicable, 1);
            }
            applicable = IPlan.YIELD_REMOVE(applicable, 2);
        }
        if ((applicable & 1) != 0 && (changed & 4) != 0 && (this.scheme & 0x10) != 0 && (changed & 4) != 0) {
            applicable = IPlan.YIELD_REMOVE(applicable, 2);
        }
        if ((applicable & 1) != 0 && (changed & 4) != 0 && (this.scheme & 0x1000) != 0 && (changed & 4) != 0) {
            applicable = IPlan.YIELD_REMOVE(applicable, 32);
        }
        if ((applicable & 1) != 0 && (changed & 0x80) != 0 && (this.scheme & 0x200) != 0) {
            applicable = IPlan.YIELD_REMOVE(applicable, 2);
        }
        if ((applicable & 1) != 0 && (changed & 0x100) != 0 && (this.scheme & 0x400) != 0) {
            applicable = IPlan.YIELD_REMOVE(applicable, 2);
        }
        return applicable;
    }

    @Override
    public boolean hasMessage() {
        return false;
    }

    @Override
    public String getMessage() {
        return null;
    }

    @Override
    public boolean hasScripting() {
        return !Utils.isEmpty(this.script);
    }

    @Override
    public Object invoke(String function, ITlkPainter painter, IPlan.IPaintPlanIterator iter, ITreeItem item, Rectangle area, Object ... args) {
        if (this.hasScripting()) {
            if (this.scripting == null) {
                this.scripting = Scripting.create(new PaintScriptContextProvider(), s -> s.setScript(this.script, null));
                this.scripting.run(null);
            }
            return this.scripting.invoke(function, painter, iter, item, area, args);
        }
        return null;
    }
}

