/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.paint.ICursorItem;
import de.toem.impulse.paint.IPaintItem;
import de.toem.impulse.paint.IPlotTreeMouseListener;
import de.toem.impulse.paint.ISelectItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.ui.tlk.ITlkPainter;
import de.toem.toolkits.ui.tlk.controls.ITlkListener;
import de.toem.toolkits.ui.tlk.controls.ITlkMenu;
import de.toem.toolkits.ui.tlk.controls.ITlkTreeBase;
import java.util.List;

public interface IPlotTree
extends ITlkTreeBase<ITreeItem, ISelectItem> {
    public static final String MENU_MAIN = "Main";
    public static final String MENU_PLOT = "Samples";
    public static final String MENU_CURSOR = "Cursor";
    public static final String MENU_HEADER = "Header";
    public static final String Theme = "Theme";
    public static final String ConfigWidth = "ConfigWidth";
    public static final String ValueColumnWidth = "ValueColumnWidth";
    public static final String ShowCursorDetails = "ShowCursorDetails";
    public static final String ShowValueColumn = "ShowValueColumn";
    public static final String ShowIcons = "ShowIcons";
    public static final String ShowNamesRightAlligned = "ShowNamesRightAlligned";
    public static final String ShowAxis = "ShowAxis";
    public static final String ShowGrid = "ShowGrid";
    public static final String FitVertical = "FitVertical";
    public static final String ScrollOffset = "ScrollOffset";
    public static final String AxisItem = "AxisItem";
    public static final String GoToStart = "GoToStart";
    public static final String GoToEnd = "GoToEnd";
    public static final String ShowAll = "ShowAll";
    public static final String Zoom = "Zoom";
    public static final String IncZoom = "IncZoom";
    public static final String DecZoom = "DecZoom";
    public static final String MakeVisible = "MakeVisible";
    public static final String StreamMode = "StreamMode";
    public static final String Selection = "Selection";
    public static final String SelectAll = "SelectAll";
    public static final String SamplesSelection = "SamplesSelection";
    public static final String SwitchActiveCursor = "ActiveCursor";
    public static final String ShowCursor = "ShowCursor";
    public static final String MoveActiveCursor = "MoveActiveCursor";
    public static final String Provide = "Provide";
    public static final String Notify = "Notify";
    public static final String Highlight = "Highlight";
    public static final String UpdateActiveValues = "UpdateActiveValues";
    public static final String Perform = "Perform";
    public static final String DefaultSelection = "DefaultSelection";
    public static final String ItemExpand = "ItemExpand";
    public static final String ItemCollapse = "ItemCollapse";
    public static final String ItemResize = "ItemResize";
    public static final String DomainAxisModified = "DomainAxisModified";
    public static final String ActiveDomainAxisChanged = "ActiveDomainAxisChanged";
    public static final String ValueAxisModified = "ValueAxisModified";
    public static final String CursorModified = "CursorModified";
    public static final String ActiveCursorChanged = "ActiveCursorChanged";
    public static final String CursorDefaultSelection = "CursorDefaultSelection";
    public static final String GeometryModified = "GeometryModified";
    public static final String PlanRequested = "PlanRequested";
    public static final String PlanLimitedTo = "PlanLimitedTo";
    public static final String ItemSyncRequested = "ItemSyncRequested";
    public static final String ActiveCursorValueRequested = "ActiveCursorValueRequested";
    public static final String ActiveChangeUnderMouseRequested = "ActiveChangeUnderMouseRequested";
    public static final String GotoRequested = "GotoRequested";
    public static final String[] Contents = new String[]{"Theme", "ConfigWidth", "ValueColumnWidth", "ShowCursorDetails", "ShowValueColumn", "ShowIcons", "ShowNamesRightAlligned", "ShowAxis", "ShowGrid", "FitVertical", "ScrollOffset", "AxisItem", "GoToStart", "GoToEnd", "ShowAll", "Zoom", "IncZoom", "DecZoom", "MakeVisible", "StreamMode", "Selection", "SelectAll", "SamplesSelection", "ActiveCursor", "ShowCursor", "MoveActiveCursor", "Provide", "Notify", "UpdateActiveValues", "Perform", "DefaultSelection", "ItemExpand", "ItemCollapse", "ItemResize", "DomainAxisModified", "ActiveDomainAxisChanged", "ValueAxisModified", "CursorModified", "ActiveCursorChanged", "CursorDefaultSelection", "GeometryModified", "PlanRequested", "PlanLimitedTo", "ItemSyncRequested", "ActiveCursorValueRequested", "ActiveChangeUnderMouseRequested", "GotoRequested"};

    public void setTheme(ITheme var1);

    public ITheme getTheme();

    public int getConfigWidth();

    public void setConfigWidth(int var1);

    public int getValueColumnWidth();

    public void setValueColumnWidth(int var1);

    public boolean getShowCursorDetails();

    public void setShowCursorDetails(boolean var1, boolean var2);

    public boolean canShowCursorDetails();

    public boolean getShowValueColumn();

    public void setShowValueColumn(boolean var1);

    public boolean getShowIcons();

    public void setShowIcons(boolean var1);

    public boolean getShowNamesRightAlligned();

    public void setShowNamesRightAlligned(boolean var1);

    public boolean getShowAxis();

    public void setShowAxis(boolean var1);

    public boolean getShowGrid();

    public void setShowGrid(boolean var1);

    public boolean getFitVertical();

    public void setFitVertical(boolean var1);

    public void setScrollOffset(int var1);

    public <T extends ITreeItem> T create(Class<T> var1, ITreeItem var2, int var3);

    public <T extends IPaintItem> T create(Class<T> var1, int var2);

    public ITreeItem getTreeItem();

    public List<ITreeItem> getAllTreeItems();

    public List<ICursorItem> getCursorItems();

    public void removeAllTreeItems();

    public void removeAllCursorItems();

    public void setExpanded(ITreeItem var1, boolean var2);

    public IDomainAxis getActiveAxis();

    public void setAxisItem(ITreeItem var1);

    public void goToStart();

    public void goToEnd();

    public void showAll(boolean var1);

    public long getZoom();

    public void setZoom(int var1);

    public void incZoom();

    public void decZoom();

    @Override
    public void makeVisible(ITreeItem var1);

    public void setStreamMode(boolean var1);

    public boolean getStreamMode();

    public void setSamplesMouseListener(IPlotTreeMouseListener var1);

    public void enableSamplesSelection(boolean var1);

    public boolean hasTreeFocus();

    public boolean hasPlotFocus();

    public boolean hasDetailsFocus();

    public ICursorItem getActiveCursor();

    public void switchActiveCursor(String var1, boolean var2);

    public void showCursor(String var1);

    public void moveActiveCursor(DomainValue var1, boolean var2, int var3, boolean var4);

    @Override
    public void setMenu(String var1, ITlkMenu var2);

    public void provide(int var1, int var2, IPlan.IPaintPlanProvision var3);

    public void notify(int var1, int var2, int var3, String var4);

    public void highlightItem(IPaintItem var1, int var2, boolean var3);

    public void highlightAttachment(ITreeItem var1, IAttachment var2, int var3, boolean var4);

    public void updateActiveValues();

    public boolean paint(ITlkPainter var1);

    public void perform();

    @Override
    public void onSelection(ITlkTreeBase.ISelectionListener<ISelectItem> var1);

    @Override
    public void onDefaultSelection(ITlkTreeBase.IDefaultSelectionListener<ISelectItem> var1);

    @Override
    public void onItemExpand(ITlkTreeBase.IItemExpandListener<ITreeItem> var1);

    @Override
    public void onItemCollapse(ITlkTreeBase.IItemCollapseListener<ITreeItem> var1);

    public void onCursorDefaultSelection(ICursorDefaultSelectionListener var1);

    @Override
    public void onItemSyncRequested(ITlkTreeBase.IItemSyncRequestedListener var1);

    public void onActiveCursorChanged(IActiveCursorChangedListener var1);

    public void onCursorModified(ICursorModifiedListener var1);

    public void onGeometryModified(IGeometryModifiedListener var1);

    public void onItemResize(IItemResizeListener var1);

    public void onActiveDomainAxisChanged(IActiveDomainAxisChangedListener var1);

    public void onDomainAxisModified(IDomainAxisModifiedListener var1);

    public void onPlanRequested(IPlanRequestedListener var1);

    public void onPlanLimitedTo(IPlanLimitedToListener var1);

    public void onActiveCursorValueRequested(IActiveCursorValueRequestedListener var1);

    public void onActiveChangeUnderMouseRequested(IActiveChangeUnderMouseRequestedListener var1);

    public void onGotoRequested(IGotoRequestedListener var1);

    public static interface IActiveChangeUnderMouseRequestedListener
    extends ITlkListener {
        public void handle(ITreeItem var1, DomainValue var2);
    }

    public static interface IActiveCursorChangedListener
    extends ITlkListener {
        public void handle(ICursorItem var1);
    }

    public static interface IActiveCursorValueRequestedListener
    extends ITlkListener {
        public void handle(ITreeItem var1, DomainValue var2, boolean var3);
    }

    public static interface IActiveDomainAxisChangedListener
    extends ITlkListener {
        public void handle(ITreeItem var1, IDomainAxis var2);
    }

    public static interface ICursorDefaultSelectionListener
    extends ITlkListener {
        public void handle(ICursorItem var1);
    }

    public static interface ICursorModifiedListener
    extends ITlkListener {
        public void handle(ICursorItem var1, DomainValue var2, boolean var3);
    }

    public static interface IDomainAxisModifiedListener
    extends ITlkListener {
        public void handle(ITreeItem var1, IDomainAxis var2);
    }

    public static interface IGeometryModifiedListener
    extends ITlkListener {
        public void handle(int var1, int var2);
    }

    public static interface IGotoRequestedListener
    extends ITlkListener {
        public void handle(Link var1);
    }

    public static interface IItemResizeListener
    extends ITlkListener {
        public void handle(ITreeItem var1, int var2, int var3);
    }

    public static interface IPlanLimitedToListener
    extends ITlkListener {
        public void handle(List<ITreeItem> var1);
    }

    public static interface IPlanRequestedListener
    extends ITlkListener {
        public void handle(int var1, int var2, IPlan.IPlanRequest<ITreeItem> var3);
    }
}

