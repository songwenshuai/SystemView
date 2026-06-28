/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.controller;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.PixelBase;
import de.toem.impulse.paint.ICursorItem;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.IPlotTree;
import de.toem.impulse.paint.IPlotTreeMouseListener;
import de.toem.impulse.paint.ISelectItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.controller.PlanFactory;
import de.toem.impulse.paint.controller.ViewerTreeItemSynchronizer;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.parts.views.SampleViewDialog;
import de.toem.impulse.provider.ISamplesCache;
import de.toem.impulse.provider.ISamplesContext;
import de.toem.impulse.provider.ISignalContext;
import de.toem.impulse.provider.ISimpleSamplesProvider;
import de.toem.impulse.provider.SamplesCache;
import de.toem.impulse.samples.IGroupPointer;
import de.toem.impulse.samples.IPointer;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.iterator.GroupPointer;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.core.Assert;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.element.synchronization.ICellSynchronizer;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.controller.abstrac.AbstractController;
import de.toem.toolkits.ui.controller.abstrac.IFindReplaceContext;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.tlk.controls.ITlkControl;
import de.toem.toolkits.ui.tlk.controls.ITlkItem;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class PlotTreeController
extends AbstractController<IPlotTree>
implements ISignalContext,
ISamplesContext {
    protected ITreeItemSynchronizer treeItemSynchronizer;
    protected ICursorItemSynchronizer cursorItemSynchronizer;
    protected ISamplesCache cached = new SamplesCache();
    protected IFindReplaceContext findReplaceContext;
    protected PlanFactory planFactory;
    protected SampleDialogInput sampleDialogInput;
    public static final int GOTO_CREATE_NEVER = 0;
    public static final int GOTO_CREATE_ASK = 1;
    public static final int GOTO_CREATE_FORCE = 2;
    Map<ITreeItem, DomainValue> activeCursorValueRequestN = new HashMap<ITreeItem, DomainValue>();
    Map<ITreeItem, DomainValue> activeCursorValueRequestD = new HashMap<ITreeItem, DomainValue>();
    Map<ITreeItem, DomainValue> activeChangeUnderMouseRequest = new HashMap<ITreeItem, DomainValue>();
    IExecutable activeRequested = p -> this.access(plotTree -> {
        for (ITreeItem item : this.activeCursorValueRequestN.keySet()) {
            if (item.isDisposed()) continue;
            this.handleActiveCursorValueRequested(item, this.activeCursorValueRequestN.get(item), false);
        }
        for (ITreeItem item : this.activeCursorValueRequestD.keySet()) {
            if (item.isDisposed()) continue;
            this.handleActiveCursorValueRequested(item, this.activeCursorValueRequestD.get(item), true);
        }
        for (ITreeItem item : this.activeChangeUnderMouseRequest.keySet()) {
            if (item.isDisposed()) continue;
            this.handleActiveChangeUnderMouseRequested(item, this.activeChangeUnderMouseRequest.get(item));
        }
        plotTree.perform();
    });

    public PlotTreeController(ITlkPart editor, String name) {
        super(editor, name);
    }

    @Override
    public void dispose() {
        super.dispose();
        this.cached.clear();
        this.findReplaceContext = null;
        this.planFactory.dispose();
        this.planFactory = null;
        this.sampleDialogInput = null;
    }

    @Override
    public boolean setContextMenuOnRequest() {
        return false;
    }

    @Override
    protected boolean isControlValid() {
        return this.control instanceof IPlotTree;
    }

    @Override
    public void setControl(ITlkControl control, int function) {
        super.setControl(control, function);
        Assert.isTrue(this.isControlValid());
        this.doUpdateTheme();
        this.access(plotTree -> {
            plotTree.onSelection(selection -> this.selectionChanged());
            plotTree.onDefaultSelection(item -> {
                Object object = this.doEdit(item, 0, this);
            });
            plotTree.onCursorDefaultSelection(item -> this.doCursorEdit(null, 0, this));
            plotTree.onItemExpand(item -> this.treeItemSynchronizer.storeExpandState((ITreeItem)item, true));
            plotTree.onItemCollapse(item -> this.treeItemSynchronizer.storeExpandState((ITreeItem)item, false));
            plotTree.onItemSyncRequested(item -> this.access(x -> {
                if (this.treeItemSynchronizer != null) {
                    this.treeItemSynchronizer.synchronize(0);
                }
            }));
            plotTree.onActiveCursorChanged(item -> {
                Object data = item.getData();
                if (data instanceof ICell && ((ICell)data).getElement().isBound()) {
                    this.cursorModified(data, null);
                }
            });
            plotTree.onCursorModified((item, position, delta) -> {
                Object data = item.getData();
                if (!delta && data instanceof ICell && ((ICell)data).getElement().isBound() && position != null) {
                    ((ICell)data).getElement().setHint("POSITION", position.toString(1));
                    this.cursorModified(data, position);
                }
            });
            plotTree.onGeometryModified((configWidth, valueColumnWidth) -> {
                this.setHint("configWidth", String.valueOf(configWidth));
                this.setHint("valueColumnWidth", String.valueOf(valueColumnWidth));
            });
            plotTree.onItemResize((item, height, min) -> {
                for (ISelectItem sel : plotTree.getSelection()) {
                    if (sel == null || sel.isDisposed() || !sel.isTreeItemSelection()) continue;
                    Object virtual = item.getData("SOURCECHILD");
                    if (virtual != null) {
                        ITreeItem parent = ((ITreeItem)sel.item()).getParentItem();
                        while (parent != null && !(parent.getData() instanceof ICell)) {
                            parent = parent.getParentItem();
                        }
                        if (!(parent.getData() instanceof ICell)) continue;
                        ((ICell)parent.getData()).getElement().setHint("virtual.child.preferedHeight." + String.valueOf(virtual), String.valueOf(height > min ? height : -1));
                        continue;
                    }
                    if (!(sel.getData() instanceof ICell)) continue;
                    HashMap<Object, Object> changes = new HashMap<Object, Object>();
                    changes.put("preferedHeight", Integer.valueOf(height) > Integer.valueOf(min));
                    changes.put("preferedHeightValue", height > min ? height : -1);
                    this.setCellValues((ICell)sel.getData(), changes, false);
                }
            });
            plotTree.onDomainAxisModified((item, axis) -> {
                ICell hintCell;
                Object data = item.getData();
                if (data instanceof ICell && ((ICell)data).getElement().isBound() && axis instanceof IDomainAxis && (hintCell = ViewerTreeItemSynchronizer.getAxisHintCell((ICell)data)) != null && hintCell.getElement().isBound()) {
                    IElement element = hintCell.getElement();
                    String domainClass = axis.getDomainBase().getClazz();
                    element.setHint("FACTOR" + axis.getClazz() + domainClass, String.valueOf(axis.getZoom()));
                    element.setHint("OFFSET" + axis.getClazz() + domainClass, String.valueOf(axis.getUnitOffset()));
                    this.treeItemSynchronizer.resetOffset(false);
                }
            });
            plotTree.onActiveCursorValueRequested((item, position, delta) -> {
                (delta ? this.activeCursorValueRequestD : this.activeCursorValueRequestN).put(item, position);
                Actives.runFinally(this.activeRequested, 100);
            });
            plotTree.onActiveChangeUnderMouseRequested((item, position) -> {
                this.activeChangeUnderMouseRequest.put(item, position);
                Actives.runFinally(this.activeRequested, 10);
            });
            plotTree.onGotoRequested(link -> {
                if (link instanceof Link) {
                    this.gotoTarget(link, 1);
                }
            });
            this.planFactory = new PlanFactory(){
                boolean updateRequested;
                protected IExecutable delayedCheckSymples = p -> PlotTreeController.this.access(plotTree -> {
                    PlotTreeController.this.treeItemSynchronizer.synchronize(3);
                    plotTree.perform();
                });

                @Override
                public void provide(Object client, int counter, IPlan.IPaintPlanProvision provision, boolean samplesUpdated) {
                    PlotTreeController.this.access(plotTree -> {
                        plotTree.provide((Integer)client, counter, provision);
                        if (samplesUpdated) {
                            Actives.runFinally(this.delayedCheckSymples, 100);
                        }
                    });
                }

                @Override
                public void notify(Object client, int counter, int reason, String detail) {
                    PlotTreeController.this.access(plotTree -> plotTree.notify((Integer)client, counter, reason, detail));
                }

                @Override
                protected void update() {
                    if (!this.updateRequested) {
                        this.updateRequested = true;
                        Actives.runInMain(new IExecutable(){

                            @Override
                            public void execute(IProgress p) {
                                updateRequested = false;
                                PlotTreeController.this.access(plotTree -> plotTree.perform());
                            }
                        });
                    }
                }
            };
            plotTree.onPlanRequested((contextId, counter, request) -> {
                if (this.planFactory != null && request instanceof IPlan.IPlanRequest) {
                    this.planFactory.request(contextId, counter, request);
                }
            });
            plotTree.onPlanLimitedTo(limits -> {
                if (this.planFactory != null && limits instanceof List) {
                    this.planFactory.limitTo(limits);
                }
            });
        });
    }

    protected void cursorModified(Object data, DomainValue position) {
        if (this.sampleDialogInput != null) {
            this.sampleDialogInput.cursorChanged();
        }
    }

    public boolean getShowCursorDetails() {
        return this.access(false, (T plotTree) -> plotTree.getShowCursorDetails());
    }

    public void setShowCursorDetails(boolean show) {
        this.access(plotTree -> {
            plotTree.setShowCursorDetails(show, true);
            this.setHint("showCursorDetails", String.valueOf(show));
            plotTree.perform();
        });
    }

    public Object canShowCursorDetails() {
        return this.access(false, (T plotTree) -> plotTree.canShowCursorDetails());
    }

    public boolean getShowValueColumn() {
        return this.access(false, (T plotTree) -> plotTree.getShowValueColumn());
    }

    public void setShowValueColumn(boolean show) {
        this.access(plotTree -> {
            plotTree.setShowValueColumn(show);
            this.setHint("showValueColumn", String.valueOf(show));
            plotTree.perform();
        });
    }

    public boolean getShowIcons() {
        return this.access(false, (T plotTree) -> plotTree.getShowIcons());
    }

    public void setShowIcons(boolean show) {
        this.access(plotTree -> {
            plotTree.setShowIcons(show);
            this.setHint("showIcons", String.valueOf(show));
            plotTree.perform();
        });
    }

    public boolean getShowNamesRightAlligned() {
        return this.access(false, (T plotTree) -> plotTree.getShowNamesRightAlligned());
    }

    public void setShowNamesRightAlligned(boolean show) {
        this.access(plotTree -> {
            plotTree.setShowNamesRightAlligned(show);
            this.setHint("showNamesRightAlligned", String.valueOf(show));
            plotTree.perform();
        });
    }

    public boolean getShowAxis() {
        return this.access(false, (T plotTree) -> plotTree.getShowAxis());
    }

    public void setShowAxis(boolean show) {
        this.access(plotTree -> {
            plotTree.setShowAxis(show);
            this.setHint("showAxis", String.valueOf(show));
            plotTree.perform();
        });
    }

    public boolean getShowGrid() {
        return this.access(false, (T plotTree) -> plotTree.getShowGrid());
    }

    public void setShowGrid(boolean show) {
        this.access(plotTree -> {
            plotTree.setShowGrid(show);
            this.setHint("showGrid", String.valueOf(show));
            plotTree.perform();
        });
    }

    public boolean getFitVertical() {
        return this.access(false, (T plotTree) -> plotTree.getFitVertical());
    }

    public void setFitVertical(boolean fit) {
        this.access(plotTree -> {
            plotTree.setFitVertical(fit);
            this.setHint("fitVertical", String.valueOf(fit));
            plotTree.perform();
        });
    }

    public long getZoom() {
        return this.access(0L, (T plotTree) -> plotTree.getZoom());
    }

    public void setZoom(int zoom) {
        this.access(plotTree -> {
            plotTree.setZoom(zoom);
            plotTree.perform();
        });
    }

    public void incZoom() {
        this.access(plotTree -> {
            plotTree.incZoom();
            plotTree.perform();
        });
    }

    public void decZoom() {
        this.access(plotTree -> {
            plotTree.decZoom();
            plotTree.perform();
        });
    }

    public ICursorItem getActiveCursor() {
        return this.access(null, (T plotTree) -> plotTree.getActiveCursor());
    }

    public void moveActiveCursor(DomainValue value, boolean keepVisible, int moveDeltaMode, boolean notify) {
        this.access(plotTree -> {
            plotTree.moveActiveCursor(value, keepVisible, moveDeltaMode, notify);
            plotTree.perform();
        });
    }

    public ITheme getTheme() {
        return this.access(null, (T plotTree) -> plotTree.getTheme());
    }

    @Deprecated
    public void setSamplesMouseListener(IPlotTreeMouseListener listener) {
        this.access(plotTree -> plotTree.setSamplesMouseListener(listener));
    }

    @Override
    public int getSelectionTypes() {
        return 72;
    }

    @Override
    public List<ICell> getSelectedCells() {
        ArrayList<ICell> cells = new ArrayList<ICell>();
        this.access(plotTree -> {
            for (ISelectItem sel : plotTree.getSelection()) {
                if (sel != null && !sel.isDisposed() && sel.isTreeItemSelection() && sel.getData() instanceof ICell) {
                    cells.add((ICell)sel.getData());
                    continue;
                }
                if (sel == null || sel.isDisposed() || sel.isTreeItemSelection()) continue;
                cells.clear();
                break;
            }
        });
        return cells;
    }

    @Override
    public List<IElement> getSelectedElements() {
        return Elements.getElements(this.getSelectedCells());
    }

    @Override
    public List<Object> getSelectedObjects() {
        ArrayList<Object> objects = new ArrayList<Object>();
        this.access(plotTree -> {
            for (ISelectItem sel : plotTree.getSelection()) {
                if (sel == null || sel.isDisposed()) continue;
                objects.add(sel);
            }
        });
        return objects;
    }

    public ICell getSelectedCell() {
        List<ICell> cells = this.getSelectedCells();
        return cells.size() == 1 ? cells.get(0) : null;
    }

    public IElement getSelectedElement() {
        List<IElement> cells = this.getSelectedElements();
        return cells.size() == 1 ? cells.get(0) : null;
    }

    public void clearSelection() {
        this.access(tree -> {
            List items = tree.getItems();
            if (items.size() > 0) {
                tree.select((ISelectItem)items.get(0), false);
            }
        });
        this.selectionChanged();
    }

    public void selectRoot() {
        this.access(tree -> {
            List items = tree.getItems();
            if (items.size() > 0) {
                tree.select((ISelectItem)items.get(0), false);
            }
        });
        this.selectionChanged();
    }

    public void select(ICell source, boolean keepSelection) {
        if (this.treeItemSynchronizer != null) {
            this.access(tree -> {
                ArrayList<ICell> sources = new ArrayList<ICell>();
                sources.add(source);
                Map<Object, ITreeItem> items = this.treeItemSynchronizer.makeVisible(sources);
                tree.select(new ArrayList<ITreeItem>(items.values()), keepSelection);
            });
        }
        this.selectionChanged();
    }

    public void select(List<ICell> sources, boolean keepSelection) {
        if (this.treeItemSynchronizer != null) {
            this.access(tree -> {
                Map<Object, ITreeItem> items = this.treeItemSynchronizer.makeVisible(sources);
                tree.select(new ArrayList<ITreeItem>(items.values()), keepSelection);
            });
        }
        this.selectionChanged();
    }

    public void makeVisible(List<ICell> sources) {
        if (this.treeItemSynchronizer != null) {
            this.access(tree -> this.treeItemSynchronizer.makeVisible(sources));
        }
    }

    @Override
    protected Object getDragData(String dragId, Object data) {
        if (data instanceof ITlkItem && ((ITlkItem)data).getData() instanceof ICell) {
            return Elements.getElements((ICell)((ITlkItem)data).getData());
        }
        return this.getSelectedElements();
    }

    @Override
    protected int getDragDataType(String dragId, Object data) {
        return 1;
    }

    @Override
    protected Object getDropItem(Object item, int location, int operation) {
        if (item instanceof ITlkItem && ((ITlkItem)item).getData() instanceof ICell) {
            return Elements.getElement((ICell)((ITlkItem)item).getData());
        }
        return null;
    }

    @Override
    public Object doPrint(Object data, int doIt, Object sender) {
        return null;
    }

    protected Object doCursorEdit(Object data, int doIt, Object sender) {
        return null;
    }

    public Object doCursorActivate(Object data, int doIt, Object sender) {
        return this.access(null, (T plotTree) -> {
            if (doIt == 0 || doIt == 1) {
                if (!(data instanceof String) || Utils.parseInt((String)data, 0) >= plotTree.getCursorItems().size() || plotTree.getCursorItems().size() <= 1) {
                    return false;
                }
                if (doIt == 0) {
                    plotTree.switchActiveCursor((String)data, true);
                    plotTree.perform();
                }
                return true;
            }
            return null;
        });
    }

    public Object doCursorShow(Object data, int doIt, Object sender) {
        return this.access(null, (T plotTree) -> {
            if (doIt == 0 || doIt == 1) {
                if (!(data instanceof String) || Utils.parseInt((String)data, 0) >= plotTree.getCursorItems().size() || plotTree.getCursorItems().size() < 1) {
                    return false;
                }
                if (doIt == 0) {
                    plotTree.showCursor((String)data);
                    plotTree.perform();
                }
                return true;
            }
            return null;
        });
    }

    public Object doGeometryValueColumn(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                this.setShowValueColumn(!this.getShowValueColumn());
            }
            return true;
        }
        if (doIt == 3) {
            return this.getShowValueColumn();
        }
        return null;
    }

    public Object doGeometryConfigIcon(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                this.setShowIcons(!this.getShowIcons());
            }
            return true;
        }
        if (doIt == 3) {
            return this.getShowIcons();
        }
        return null;
    }

    public Object doGeometryNamesRightAligned(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                this.setShowNamesRightAlligned(!this.getShowNamesRightAlligned());
            }
            return true;
        }
        if (doIt == 3) {
            return this.getShowNamesRightAlligned();
        }
        return null;
    }

    public Object doGeometryFitVertical(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                this.setFitVertical(!this.getFitVertical());
            }
            return true;
        }
        if (doIt == 3) {
            return this.getFitVertical();
        }
        return null;
    }

    public Object doGeometryAxis(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                this.setShowAxis(!this.getShowAxis());
            }
            return true;
        }
        if (doIt == 3) {
            return this.getShowAxis();
        }
        return null;
    }

    public Object doGeometryGrid(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                this.setShowGrid(!this.getShowGrid());
            }
            return true;
        }
        if (doIt == 3) {
            return this.getShowGrid();
        }
        return null;
    }

    public Object doAxisDomainZoomFit(Object data, int doIt, Object sender) {
        return this.access(null, (T plotTree) -> {
            if (doIt == 0 || doIt == 1) {
                if (doIt == 0) {
                    plotTree.showAll(false);
                    plotTree.perform();
                }
                return true;
            }
            return null;
        });
    }

    public Object doAxisDomainZoomOut(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                this.incZoom();
            }
            return true;
        }
        return null;
    }

    public Object doAxisDomainZoomIn(Object data, int doIt, Object sender) {
        return this.access(null, (T plotTree) -> {
            if (doIt == 0 || doIt == 1) {
                if (doIt == 0) {
                    this.decZoom();
                }
                return true;
            }
            return null;
        });
    }

    public Object doRefresh(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0 && ("plots".equals(data) || data == null)) {
                if (this.planFactory != null) {
                    this.planFactory.invalidate();
                }
                this.clearItems();
            }
            return true;
        }
        return null;
    }

    @Override
    public IElement getRecord() {
        return this.getCell() != null ? this.getCell().getElement() : IElement.NONE;
    }

    @Override
    public ISamplesCache getSamplesCache() {
        return this.cached;
    }

    public void gotoTarget(Link link, int createOptions) {
    }

    protected void fireActiveCursorChanged(DomainValue position, ISimpleSamplesProvider samples, int index) {
    }

    protected void openSampleViewDialog() {
        if (this.sampleDialogInput == null) {
            this.sampleDialogInput = new SampleDialogInput();
        } else {
            this.sampleDialogInput.init();
        }
    }

    protected void clearItems() {
        this.access(plotTree -> plotTree.clear());
    }

    protected void synchronizeTreeItems(ISamplesContext context, ICell record, ICell presentation, ICell top, int style) {
        this.access(plotTree -> {
            if (this.treeItemSynchronizer.getTop() != top) {
                plotTree.setScrollOffset(0);
            }
            this.treeItemSynchronizer.synchronize(record, presentation, top, context, style);
        });
    }

    protected void synchronizeCursorItems(ICell presentation) {
        this.access(plotTree -> this.cursorItemSynchronizer.synchronize(presentation));
    }

    public boolean hasTreeFocus() {
        return this.access(false, (T plotTree) -> plotTree.hasTreeFocus());
    }

    public boolean hasPlotFocus() {
        return this.access(false, (T plotTree) -> plotTree.hasPlotFocus());
    }

    public boolean hasDetailsFocus() {
        return this.access(false, (T plotTree) -> plotTree.hasDetailsFocus());
    }

    private void handleActiveCursorValueRequested(ITreeItem item, DomainValue position, boolean delta) {
        IReadableSamples samples;
        if (item instanceof IPlotItem && position != null && (samples = ((IPlotItem)item).getSamples()) != null && samples.isSettled() && (samples.getDomainBase() == position.base || position.isNull())) {
            int index = samples.indexAt(position);
            long from = 0L;
            long to = 0L;
            String activeString = null;
            Number activeNumber = null;
            boolean valid = false;
            if (index >= 0) {
                from = samples.unitsAt(index);
                to = samples.unitsAt(index + 1);
                activeString = samples.formatAt(index, ((IPlotItem)item).getValueColumnFormat());
                activeNumber = samples.numberValueAt(index);
                valid = true;
            } else if (samples.getCount() > 0) {
                index = 0;
                from = Long.MIN_VALUE;
                to = samples.unitsAt(0);
                activeString = "";
                activeNumber = 0;
                valid = true;
            } else if (samples.getCount() == 0) {
                index = 0;
                from = Long.MIN_VALUE;
                to = Long.MAX_VALUE;
                activeString = "";
                activeNumber = 0;
                valid = true;
            }
            if (valid) {
                ((IPlotItem)item).setActiveCursorValue(delta, activeString, activeNumber, position.base, position.units, from, to);
                return;
            }
        }
    }

    private void handleActiveChangeUnderMouseRequested(ITreeItem item, DomainValue position) {
        if (item instanceof IPlotItem && position != null) {
            IReadableSamples samples = ((IPlotItem)item).getSamples();
            if (samples != null && samples.isSettled() && samples.getDomainBase() == position.base) {
                int index = samples.indexAt(position);
                long pos = position.units;
                long units = 0L;
                long next = 0L;
                long previous = 0L;
                int identicalPositions = 0;
                if (index >= 0) {
                    units = samples.unitsAt(index);
                    next = samples.unitsAt(index + 1);
                    previous = 0L;
                    if (next - pos < pos - units) {
                        ++index;
                        previous = units;
                        units = next;
                        while (next != Long.MAX_VALUE && units == next) {
                            next = samples.unitsAt(index + 1 + identicalPositions);
                            ++identicalPositions;
                        }
                    } else {
                        previous = units;
                        while (previous != Long.MAX_VALUE && units == previous) {
                            previous = samples.unitsAt(index - 1 - identicalPositions);
                            ++identicalPositions;
                        }
                        index -= identicalPositions - 1;
                    }
                } else if (samples.getCount() > 0) {
                    index = 0;
                    next = units = samples.unitsAt(index);
                    while (next != Long.MAX_VALUE && units == next) {
                        next = samples.unitsAt(index + 1 + identicalPositions);
                        ++identicalPositions;
                    }
                    previous = Long.MIN_VALUE;
                }
                if (index >= 0) {
                    CompoundValue value = null;
                    boolean grouped = samples.getGroups() > 0;
                    int minGroup = -1;
                    int maxGroup = -1;
                    ArrayList<IAttachment> attachments = new ArrayList<IAttachment>();
                    int i = index;
                    while (i < index + identicalPositions) {
                        value = samples.compoundAt(i, 4);
                        if (value != null) {
                            for (IAttachment a : value.attachments(4)) {
                                attachments.add(a);
                            }
                        }
                        if (grouped && value != null) {
                            int group = value.getGroup();
                            if (group != -1 && (minGroup == -1 || minGroup > group)) {
                                minGroup = group;
                            }
                            if (group != -1 && (maxGroup == -1 || maxGroup < group)) {
                                maxGroup = group;
                            }
                        }
                        ++i;
                    }
                    String message = "@" + index + (identicalPositions > 1 ? "(+" + (identicalPositions - 1) + ")" : "");
                    if (grouped && minGroup != -1) {
                        message = String.valueOf(message) + " #" + minGroup;
                    }
                    if (grouped && maxGroup != -1 && maxGroup > minGroup) {
                        message = String.valueOf(message) + "-#" + maxGroup;
                    }
                    message = String.valueOf(message) + " " + position.base.toString(units);
                    if (identicalPositions == 1) {
                        message = String.valueOf(message) + "=" + value.format(((IPlotItem)item).getValueColumnFormat());
                    }
                    message = Utils.limit(message, 256);
                    Number numerical = value != null ? (Number)value.numberValue() : (Number)null;
                    ((IPlotItem)item).setActiveChangeUnderMouse(message, numerical, attachments, position.base, previous, units, next);
                    return;
                }
            }
            ((IPlotItem)item).setActiveChangeUnderMouse(null, null, null, position.base, position.units, position.units, position.units);
        }
    }

    public static interface ICursorItemSynchronizer
    extends ICellSynchronizer<ICursorItem> {
        public static final String HINT_POSITION = "POSITION";

        public void synchronize(ICell var1);
    }

    public static interface ITreeItemSynchronizer
    extends ICellSynchronizer<ITreeItem> {
        public static final String HINT_EXPANDED = "EXPANDED";
        public static final String HINT_HEIGHT = "virtual.child.preferedHeight.";
        public static final String HINT_AXIS_OFFSET = "OFFSET";
        public static final String HINT_AXIS_ZOOM = "FACTOR";
        public static final int SYNC_DEEP = -1;
        public static final int SYNC_ALL = 0;
        public static final int SYNC_SAMPLES_ONLY = 1;
        public static final int SYNC_VOLATILE_SAMPLES_ONLY = 2;
        public static final int SYNC_SAMPLES_RELEASE_ONLY = 3;
        public static final int SYNC_NO_SAMPLES = 4;

        public ICell getTop();

        @Deprecated
        public void resetOffset(boolean var1);

        public boolean synchronize(ICell var1, ICell var2, ICell var3, ISamplesContext var4, int var5);

        public boolean synchronize(ICell var1, ITreeItem var2, int var3);

        public boolean synchronize(int var1);

        public void storeExpandState(ITreeItem var1, boolean var2);

        public Map<Object, ITreeItem> makeVisible(List<?> var1);

        public static interface ICompoundTreeItemSynchronizer<T extends ITreeItem> {
            public boolean synchronize(ICell var1, T var2, int var3);
        }
    }

    protected class SampleDialogInput
    extends SampleViewDialog.AbstractSampleDialogInput {
        ITreeItem item;

        public SampleDialogInput() {
            this.init();
        }

        @Override
        public void init() {
            PlotTreeController.this.access(plotTree -> {
                List selection = plotTree.getSelection();
                if (!selection.isEmpty() && ((ISelectItem)selection.get(0)).isTreeItemSelection()) {
                    this.item = (ITreeItem)((ISelectItem)selection.get(0)).item();
                    this.pointer = this.getInspectionPointer(this.item);
                }
            });
        }

        @Override
        public String getSourceIconId() {
            return PlotTreeController.this.getEditor().getIconId();
        }

        @Override
        public String getSourceLabel() {
            return PlotTreeController.this.getEditor().getLabel();
        }

        private IPointer getInspectionPointer(ITreeItem item) {
            return PlotTreeController.this.access(null, plotTree -> {
                boolean group;
                ICursorItem activeCursor = plotTree.getActiveCursor();
                IReadableSamples samples = item instanceof IPlotItem ? ((IPlotItem)item).getSamples() : null;
                int diagramType = item instanceof IPlotItem ? ((IPlotItem)item).getDiagramType() : -1;
                boolean bl = group = diagramType == 5;
                if (samples != null) {
                    if (group) {
                        return activeCursor != null && !(activeCursor.getDomainBase() instanceof PixelBase) ? new GroupPointer(samples, activeCursor.getPosition()) : new GroupPointer(samples, 0);
                    }
                    return activeCursor != null && !(activeCursor.getDomainBase() instanceof PixelBase) ? new SamplePointer(samples, activeCursor.getPosition()) : new SamplePointer(samples, 0);
                }
                return null;
            });
        }

        @Override
        public String getLabel() {
            if (this.pointer instanceof ISamplePointer) {
                return "@" + this.pointer.getIndex() + " (" + this.pointer.getCount() + ")";
            }
            if (this.pointer instanceof IGroupPointer) {
                return "#" + this.pointer.getIndex() + " (" + this.pointer.getGroups() + ")";
            }
            return "";
        }

        @Override
        public Object getColor() {
            return this.item != null ? this.item.getColor() : null;
        }

        public void cursorChanged() {
            if (this.inputListener == null) {
                return;
            }
            PlotTreeController.this.access(plotTree -> {
                DomainValue newPosition;
                ICursorItem cursor = plotTree.getActiveCursor();
                if (cursor != null && this.pointer != null && this.item != null && (newPosition = cursor.getPosition()) != null && !(newPosition.base instanceof PixelBase) && !this.pointer.getPosition().equals(newPosition)) {
                    this.pointer.setPosition(newPosition);
                    this.inputListener.indexChanged();
                }
            });
        }

        @Override
        protected void syncBack(IPointer pointer) {
            PlotTreeController.this.access(plotTree -> {
                ICursorItem cursor = plotTree.getActiveCursor();
                if (cursor != null && pointer != null && cursor.getDomainBase() != null && !(cursor.getDomainBase() instanceof PixelBase)) {
                    plotTree.moveActiveCursor(pointer.getPosition(), true, 0, true);
                    plotTree.perform();
                }
            });
        }

        @Override
        public void goTarget(Link link) {
            PlotTreeController.this.gotoTarget(link, 1);
            this.init();
            if (this.inputListener != null) {
                this.inputListener.pointerChanged();
            }
        }

        @Override
        public void goBack() {
        }

        @Override
        public boolean canGoBack() {
            return false;
        }

        @Override
        public void highlightAttachment(IAttachment attachment) {
        }
    }
}

