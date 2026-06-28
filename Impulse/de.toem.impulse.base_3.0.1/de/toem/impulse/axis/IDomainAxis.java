/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.axis;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import java.util.Map;
import org.eclipse.swt.graphics.Rectangle;

public interface IDomainAxis
extends Cloneable {
    public static final int ZOOM_DEFAULT = 0;
    public static final int NONLINEAR = 0;

    public boolean isLinear();

    public String getClazz();

    public boolean isStatic();

    public IDomainBase getDomainBase();

    public String getDomainClass();

    public void setDomainBase(IDomainBase var1);

    public int getRootPixel();

    public void setRootPixel(int var1);

    public int getPadding();

    public void setPadding(int var1);

    public int getAdjust();

    public Rectangle limitBounds(Rectangle var1);

    public int limitPx(int var1);

    public Rectangle limitFrameBounds(Rectangle var1);

    public int limitFramePx(int var1);

    public void setUnitFrame(long var1, long var3);

    public long getMinUnits();

    public long getMaxUnits();

    public void setMaxUnits(long var1);

    public void setMinUnits(long var1);

    public void startAdjustUnitFrame();

    public boolean adjustUnitFrame(long var1, long var3);

    public void endAdjustUnitFrame();

    public long getUnitOffset();

    public boolean setUnitOffset(long var1);

    public boolean setUnitOffset(long var1, int var3);

    public boolean setUnitOffset(IDomainAxis var1, int var2);

    public long getMinUnitOffset();

    public long getMaxUnitOffset();

    public long getMinScrollUnitOffset(Rectangle var1);

    public long getMaxScrollUnitOffset(Rectangle var1, boolean var2);

    public long getZoom();

    public long getIncZoom();

    public long getDecZoom();

    public boolean setZoom(long var1);

    public boolean setZoom(long var1, long var3, int var5);

    public boolean makeVisible(DomainValue var1, Rectangle var2);

    public boolean makeVisible(long var1, Rectangle var3);

    public boolean makeChangeVisible(DomainValue var1, Rectangle var2);

    public boolean makeChangeVisible(long var1, Rectangle var3);

    public boolean makeBothVisible(long var1, long var3, Rectangle var5);

    public boolean makeVisible(Rectangle var1);

    public double pixels(double var1);

    public double units(double var1);

    public double pixels2(DomainValue var1);

    public DomainValue position(double var1);

    public int deltaPixels(long var1, long var3);

    public int deltaPixelsAt(int var1, long var2);

    public long deltaUnits(int var1, int var2);

    public long deltaUnitsAt(long var1, int var3);

    public Map<Long, String> getScale(Rectangle var1);

    public long nextScaleStep(long var1, boolean var3, boolean var4);

    public String toString(long var1);

    public String toString(long var1, int var3);

    public String toDeltaString(long var1);

    public double[] getLinearScalingFrom(IDomainAxis var1);

    public IDomainAxis clone();

    public boolean syncTo(IDomainAxis var1);
}

