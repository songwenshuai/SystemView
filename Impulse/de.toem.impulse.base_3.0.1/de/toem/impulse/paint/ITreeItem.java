/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.paint.IActiveValueProvider;
import de.toem.impulse.paint.IPaintItem;
import de.toem.impulse.paint.IPaintStyle;
import de.toem.impulse.paint.IPlotTree;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.pattern.element.exploits.Markers;
import de.toem.toolkits.ui.tlk.controls.ITlkTreeItemBase;
import java.util.List;
import java.util.Map;
import org.eclipse.swt.graphics.Rectangle;

public interface ITreeItem
extends IPaintItem,
ITlkTreeItemBase<IPlotTree> {
    public static final int CHANGED_EXPANDED = 256;
    public static final int CHANGED_EXPANDABLE = 512;
    public static final int CHANGED_ORDER_PLOT = 1024;
    public static final int CHANGED_ORDER_TREE = 2048;
    public static final int CHANGED_AXIS = 4096;
    public static final int CHANGED_HEIGHT = 8192;
    public static final int CHANGED_COMBINE = 16384;
    public static final int CHANGED_PAINTER = 32768;
    public static final int CHANGED_MARKERS = 65536;
    public static final int CHANGED_ACTIVE_CURSORVALUE = 131072;
    public static final int CHANGED_ACTIVE_CHANGEUNDERMOUSE = 262144;
    public static final String DomainAxis = "DomainAxis";
    public static final String DomainAxisMap = "DomainAxisMap";
    public static final String PreferedHeight = "PreferedHeight";
    public static final String Combined = "Combined";
    public static final String ActiveCursorValue = "ActiveCursorValue";
    public static final String ActiveChangeUnderMouse = "ActiveChangeUnderMouse";
    public static final String Markers = "Markers";
    public static final String MarkerStyle = "MarkerStyle";
    public static final String[] Contents = new String[]{"DomainAxis", "DomainAxisMap", "PreferedHeight", "Combined", "Markers", "MarkerStyle"};

    public Rectangle getBounds();

    public int getHeight();

    public Rectangle getUsedPlotBounds();

    @Override
    public List<? extends ITreeItem> getChildItems();

    public ITreeItem getParentItem();

    public ITreeItem getChildItem(int var1);

    public boolean hasDomainAxis();

    public void setDomainAxis(boolean var1);

    public Map<String, IDomainAxis> getDomainAxisMap();

    public void setDomainAxisMap(Map<String, IDomainAxis> var1);

    public IDomainAxis getAxis();

    public boolean hasLocalAxis();

    public int getPreferedHeight();

    public void setPreferedHeight(int var1);

    public boolean isCombined();

    public void setCombined(boolean var1);

    public boolean isCombinable(ITreeItem var1);

    public static boolean isCombinable(int style) {
        return style == 4 || style == 8 || style == 9;
    }

    public IPaintStyle getPaintStyle();

    public int getValueColumnFormat();

    public int getDiagramType();

    public int getValueDiagramFormat();

    public int getPaintStyleMods();

    public boolean hasPaintStyleMod(int var1);

    public IActiveValueProvider getActiveValueProvider();

    public void setActiveValuePainter(IActiveValueProvider var1);

    public void setActiveCursorValue(boolean var1, String var2, Number var3, IDomainBase var4, long var5, long var7, long var9);

    public void setActiveChangeUnderMouse(String var1, Number var2, List<IAttachment> var3, IDomainBase var4, long var5, long var7, long var9);

    public Markers getMarkers();

    public void setMarkers(Markers var1);

    public boolean hasMarkers();

    public int getMarkerStyle();

    public void setMarkerStyle(int var1);

    public boolean hasNonEmptyPlan();
}

