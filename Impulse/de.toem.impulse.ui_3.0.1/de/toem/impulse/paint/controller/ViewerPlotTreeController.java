/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.controller;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.view.AxisConfiguration;
import de.toem.impulse.cells.view.CursorConfigurationInstancer;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.paint.ICursorItem;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.IPlotTree;
import de.toem.impulse.paint.ISelectItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.controller.PlotTreeController;
import de.toem.impulse.paint.controller.ViewerCursorItemSynchronizer;
import de.toem.impulse.paint.controller.ViewerFindContext;
import de.toem.impulse.paint.controller.ViewerTreeItemSynchronizer;
import de.toem.impulse.provider.ISignalProvider;
import de.toem.impulse.provider.ISimpleSamplesProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.base.SamplesStat;
import de.toem.impulse.ui.DomainPosition;
import de.toem.impulse.ui.IRecordViewerChildListener;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ElementFieldModifier;
import de.toem.toolkits.pattern.element.ElementModifierEvent;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.IElementModifier;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.element.exploits.AbstractListedXmlFieldExploit;
import de.toem.toolkits.pattern.element.exploits.IExploit;
import de.toem.toolkits.pattern.element.exploits.Marker;
import de.toem.toolkits.pattern.element.exploits.Markers;
import de.toem.toolkits.pattern.information.BaseGroupedInformations;
import de.toem.toolkits.pattern.information.GroupedInformation;
import de.toem.toolkits.pattern.information.IGroupedInformations;
import de.toem.toolkits.pattern.registry.Registration;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IEvaluable;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.tlk.TLK;
import de.toem.toolkits.ui.tlk.controls.ITlkControl;
import java.math.BigDecimal;
import java.math.MathContext;
import java.math.RoundingMode;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ViewerPlotTreeController
extends PlotTreeController
implements CursorConfigurationInstancer.IRecordViewerController,
ViewerFindContext.IFindReplaceTargetController {
    private IElement view = IElement.NONE;
    private IElement topElement = IElement.NONE;
    private boolean topChanged = false;
    private boolean streamMode;
    private boolean streamUpdateMode;
    private boolean streamModeMoveCursor;
    private boolean syncDeep;

    public ViewerPlotTreeController(ITlkPart editor, String name) {
        super(editor, name);
    }

    @Override
    public void dispose() {
        super.dispose();
        this.view = IElement.NONE;
        this.topElement = IElement.NONE;
    }

    public void setView(IElement view) {
        this.topChanged |= view != this.view;
        this.view = view;
    }

    @Override
    public IElement getView() {
        return this.view;
    }

    @Override
    public IElement getValueBaseElement() {
        return this.view;
    }

    public void setTopElement(IElement element) {
        this.topChanged |= element != this.topElement;
        this.topElement = element;
    }

    public IElement getTopElement() {
        return this.topElement;
    }

    public ICell getTopCell() {
        return this.topElement.isBound() && this.topElement.hasCell() ? this.topElement.getCell() : this.getValueBaseCell();
    }

    @Override
    protected void setContextMenus() {
        this.access(plotTree -> {
            this.tlk().addMenu((ITlkControl)plotTree, this, "MENU", "de.toem.toolkits.popupmenu.tree", "de.toem.impulse.popupmenu.viewer.tree");
            this.tlk().addMenu((ITlkControl)plotTree, this, "Samples", "de.toem.toolkits.popupmenu.tree", "de.toem.impulse.popupmenu.viewer.plots", new String[]{"add", "clone", "rename"});
            this.tlk().addMenu((ITlkControl)plotTree, this, "Cursor", "de.toem.toolkits.popupmenu.control", "de.toem.impulse.popupmenu.viewer.cursor");
            this.tlk().addMenu((ITlkControl)plotTree, this, "Header", "de.toem.toolkits.popupmenu.control", "de.toem.impulse.popupmenu.viewer.header");
        });
    }

    @Override
    public void setControl(ITlkControl control, int function) {
        super.setControl(control, function);
        this.treeItemSynchronizer = new ViewerTreeItemSynchronizer(this.tlk(), (IPlotTree)control);
        this.cursorItemSynchronizer = new ViewerCursorItemSynchronizer(this.tlk(), (IPlotTree)control);
        this.access(plotTree -> {});
    }

    @Override
    public boolean needsUpdate() {
        return true;
    }

    @Override
    public boolean isAffected(ElementModifierEvent event) {
        if (event != null && event.getElement().isBound() && event.getElement().isTribe(ImpulsePreferences.chartPreferences)) {
            this.syncDeep = true;
            return true;
        }
        return super.isAffected(event);
    }

    @Override
    public void doUpdateControl() {
        boolean doShowAll;
        super.doUpdateControl();
        this.access(plotTree -> {
            if (this.getCell() == null || !(this.getValueBaseCell() instanceof AxisConfiguration)) {
                this.clearItems();
            } else {
                this.synchronizeTreeItems(this, this.getCell(), this.getValueBaseCell(), this.getTopCell(), this.syncDeep ? -1 : (this.streamMode && !this.streamUpdateMode ? 4 : 0));
                this.synchronizeCursorItems(this.getValueBaseCell());
                this.syncDeep = false;
            }
            plotTree.perform();
        });
        boolean bl = doShowAll = this.view.isBound() && !Boolean.TRUE.toString().equals(this.view.getHint("showedAll"));
        if (doShowAll) {
            Actives.runInMain(new IExecutable(){

                @Override
                public void execute(IProgress p) {
                    ViewerPlotTreeController.this.access(plotTree -> {
                        plotTree.showAll(true);
                        if (ViewerPlotTreeController.this.view.isBound()) {
                            ViewerPlotTreeController.this.view.setHint("showedAll", Boolean.TRUE.toString());
                        }
                        plotTree.perform();
                    });
                }
            }, 100);
        }
    }

    @Override
    protected void doUpdateHints() {
        super.doUpdateHints();
        this.access(plotTree -> {
            plotTree.setConfigWidth(Utils.parseInt(this.getHint("configWidth"), 250));
            plotTree.setValueColumnWidth(Utils.parseInt(this.getHint("valueColumnWidth"), 50));
            plotTree.setShowCursorDetails(Utils.parseBoolean(this.getHint("showCursorDetails"), false), false);
            plotTree.setShowValueColumn(Utils.parseBoolean(this.getHint("showValueColumn"), true));
            plotTree.setShowIcons(Utils.parseBoolean(this.getHint("showIcons"), true));
            plotTree.setShowNamesRightAlligned(Utils.parseBoolean(this.getHint("showNamesRightAlligned"), false));
            plotTree.setShowAxis(Utils.parseBoolean(this.getHint("showAxis"), true));
            plotTree.setShowGrid(Utils.parseBoolean(this.getHint("showGrid"), true));
            plotTree.setFitVertical(Utils.parseBoolean(this.getHint("fitVertical"), false));
            plotTree.perform();
        });
    }

    @Override
    protected boolean doUpdateTheme() {
        this.access(plotTree -> {
            ITheme theme = plotTree.getTheme();
            if (theme == null) {
                return;
            }
            theme.setColor(0, null);
            theme.setColor(1, null);
            theme.setColor(2, "de.toem.impulse.color.sample.tag1");
            theme.setColor(13, "de.toem.impulse.color.sample.tag3");
            theme.setColor(14, "de.toem.impulse.color.sample.tag4");
            theme.setColor(15, "de.toem.impulse.color.sample.tag5");
            theme.setColor(16, "de.toem.impulse.color.sample.tag6");
            theme.setColor(17, "de.toem.impulse.color.sample.tag7");
            theme.setColor(18, "de.toem.impulse.color.sample.tag8");
            theme.setColor(4, "de.toem.impulse.foreground.relation");
            theme.setColor(5, "de.toem.impulse.color.relation.info");
            theme.setColor(6, "de.toem.impulse.color.relation.text");
            int n = 0;
            while (n < 32) {
                int rgbint = 0;
                rgbint = n < 16 ? TLK.hsb2RgbInt(22.5f * (float)n, 1.0f, 0.6f) : TLK.hsb2RgbInt(22.5f * (float)(n - 16), 0.85f, 1.0f);
                theme.setColor(32 + n, rgbint);
                ++n;
            }
            theme.setMarkerFont(this.tlk().getFont("de.toem.impulse.font.marker"));
            plotTree.setTheme(theme);
            plotTree.perform();
        });
        return true;
    }

    public void enterStreamMode() {
        this.streamMode = true;
        this.streamUpdateMode = false;
        this.doUpdateControl();
    }

    public boolean isUpdateMode() {
        return this.streamUpdateMode;
    }

    public void enterUpdateMode(boolean moveActiveCursor) {
        this.access(plotTree -> {
            this.streamUpdateMode = true;
            this.streamModeMoveCursor = moveActiveCursor;
            plotTree.setStreamMode(moveActiveCursor);
        });
        this.doUpdateControl();
    }

    public void leaveUpdateMode() {
        this.access(plotTree -> {
            this.streamUpdateMode = false;
            plotTree.setStreamMode(false);
        });
        this.doUpdateControl();
    }

    public boolean checkUpdateMode() {
        this.access(plotTree -> {
            if (this.streamUpdateMode && this.streamModeMoveCursor && !plotTree.getStreamMode()) {
                this.streamUpdateMode = false;
            }
        });
        return this.streamUpdateMode;
    }

    public void leaveStreamMode() {
        this.streamUpdateMode = false;
        this.streamMode = false;
        this.doUpdateControl();
    }

    public void initStreaming() {
        this.treeItemSynchronizer.resetOffset(true);
    }

    public void updateStreamSamples(boolean onlyVolatiles) {
        this.access(plotTree -> {
            if (this.checkUpdateMode()) {
                if (this.getCell() == null || !(this.getValueBaseCell() instanceof AxisConfiguration)) {
                    this.clearItems();
                } else {
                    this.synchronizeTreeItems(this, this.getCell(), this.getValueBaseCell(), this.getTopCell(), onlyVolatiles ? 2 : 1);
                }
                plotTree.perform();
            }
        });
    }

    public void updateStreamTree() {
        this.access(plotTree -> {
            plotTree.updateActiveValues();
            plotTree.perform();
        });
    }

    public Markers getSelectedMarkers() {
        Markers markers = new Markers();
        this.access(plotTree -> {
            for (ISelectItem sel : plotTree.getSelection()) {
                Signal signal;
                if (!sel.isMarkerSelection()) continue;
                Marker marker = (Marker)sel.object();
                ICell cell = (ICell)sel.getData();
                if (!(cell instanceof PlotConfiguration) || marker == null || (signal = ((PlotConfiguration)cell).getSignal(this)) == null) continue;
                marker.setData(signal);
                markers.add(marker);
            }
        });
        return markers;
    }

    public boolean hasMarkerSelection() {
        return this.access(false, (T plotTree) -> {
            List selection = plotTree.getSelection();
            for (ISelectItem sel : selection) {
                if (sel.isMarkerSelection()) continue;
                return false;
            }
            return !selection.isEmpty();
        });
    }

    public boolean hasTreeItemSelection() {
        return this.access(false, (T plotTree) -> {
            List selection = plotTree.getSelection();
            for (ISelectItem sel : selection) {
                if (sel.isTreeItemSelection()) continue;
                return false;
            }
            return !selection.isEmpty();
        });
    }

    @Override
    protected Object doEditCopy(Object data, int doIt, Object sender) {
        if (!this.checkControl()) {
            return null;
        }
        if (this.hasTreeFocus()) {
            return super.doEditCopy(data, doIt, sender);
        }
        if (this.hasPlotFocus()) {
            if (doIt == 0 || doIt == 1) {
                if (!this.hasMarkerSelection()) {
                    return false;
                }
                if (doIt == 0) {
                    Markers markers = this.getSelectedMarkers();
                    this.clipboard().setText(markers.toString());
                    return true;
                }
                return true;
            }
            return null;
        }
        this.hasDetailsFocus();
        return null;
    }

    @Override
    protected Object doEditDelete(Object data, int doIt, Object sender) {
        if (!this.checkControl()) {
            return null;
        }
        if (this.hasTreeFocus()) {
            return super.doEditDelete(data, doIt, sender);
        }
        if (this.hasPlotFocus()) {
            if (doIt == 0 || doIt == 1) {
                if (this.streamMode) {
                    return false;
                }
                Markers markers = this.getSelectedMarkers();
                if (markers.isEmpty()) {
                    return false;
                }
                if (doIt == 0) {
                    try {
                        HashMap<IElement, Markers> applyMap = new HashMap<IElement, Markers>();
                        for (Marker marker : markers) {
                            IExploit exploit;
                            ICell cell;
                            IElement element;
                            if (!(marker.getData() instanceof ICell) || !(element = (cell = (ICell)marker.getData()).getElement()).isBound() || !element.canBeModified()) continue;
                            if (!applyMap.keySet().contains(element)) {
                                exploit = cell.getExploit("markers");
                                if (!(exploit instanceof Markers) || !((Markers)exploit).contains(marker)) continue;
                                applyMap.put(element, (Markers)exploit);
                                ((Markers)exploit).remove(marker);
                                continue;
                            }
                            exploit = (Markers)applyMap.get(element);
                            ((AbstractListedXmlFieldExploit)exploit).remove(marker);
                        }
                        ArrayList<ElementFieldModifier> modifiers = new ArrayList<ElementFieldModifier>();
                        for (IElement element : applyMap.keySet()) {
                            modifiers.add(new ElementFieldModifier(element, "markers", null, applyMap.get(element), false));
                        }
                        this.getEditor().apply("Apply markers", null, modifiers.toArray(new IElementModifier[modifiers.size()]), true);
                    }
                    catch (Throwable throwable) {}
                }
                return true;
            }
            return null;
        }
        this.hasDetailsFocus();
        return null;
    }

    @Override
    protected Object doEditPaste(Object data, int doIt, Object sender) {
        if (!this.checkControl()) {
            return null;
        }
        if (this.hasTreeFocus()) {
            return super.doEditPaste(data, doIt, sender);
        }
        if (this.hasPlotFocus()) {
            if (doIt == 0 || doIt == 1) {
                Markers markers;
                String text = this.clipboard().getText();
                if (!Utils.isEmpty(text) && text.startsWith("<markers") && !(markers = new Markers().setValue(text)).isEmpty()) {
                    IElement target = this.getAddPasteTargetElement();
                    if (!target.isBound()) {
                        return false;
                    }
                    if (!target.hasCell(PlotConfiguration.class)) {
                        return false;
                    }
                    if (doIt == 0) {
                        PlotConfiguration cell = (PlotConfiguration)target.getCell();
                        IExploit exploit = cell.getExploit("markers");
                        if (!(exploit instanceof Markers)) {
                            exploit = new Markers();
                        }
                        for (Marker marker : markers) {
                            if (((Markers)exploit).contains(marker)) continue;
                            ((Markers)exploit).add(marker);
                        }
                        Signal signal = ((PlotConfiguration)target.getCell()).getSignal(this);
                        if (signal == null || !signal.getElement().isBound()) {
                            return false;
                        }
                        ArrayList<ElementFieldModifier> modifiers = new ArrayList<ElementFieldModifier>();
                        modifiers.add(new ElementFieldModifier(signal.getElement(), "markers", null, exploit, false));
                        this.getEditor().apply("Paste markers", null, modifiers.toArray(new IElementModifier[modifiers.size()]), true);
                    }
                    return true;
                }
                return false;
            }
            return null;
        }
        this.hasDetailsFocus();
        return null;
    }

    @Override
    protected Object doEdit(Object data, int doIt, Object sender) {
        if (!this.checkControl()) {
            return null;
        }
        if (this.hasTreeFocus()) {
            return super.doEdit(data, doIt, sender);
        }
        if (this.hasPlotFocus()) {
            if (doIt == 0 || doIt == 1) {
                if (this.hasMarkerSelection()) {
                    if (this.streamMode) {
                        return false;
                    }
                    Markers markers = this.getSelectedMarkers();
                    if (markers.isEmpty()) {
                        return false;
                    }
                    return true;
                }
                if (this.hasTreeItemSelection()) {
                    if (doIt == 0) {
                        this.openSampleViewDialog();
                    }
                    return true;
                }
            }
            return null;
        }
        this.hasDetailsFocus();
        return null;
    }

    @Override
    protected IElement getAddPasteTargetElement() {
        List<IElement> elements = this.getSelectedElements();
        if (elements.size() == 1) {
            return elements.get(0);
        }
        return IElement.NONE;
    }

    @Override
    protected Object doAddNew(Object data, int doIt, Object sender) {
        if (!this.checkControl()) {
            return null;
        }
        if (this.hasTreeFocus()) {
            return super.doAddNew(data, doIt, sender);
        }
        if (!this.hasPlotFocus()) {
            this.hasDetailsFocus();
        }
        return null;
    }

    @Override
    protected Object doInsertNew(Object data, int doIt, Object sender) {
        if (!this.checkControl()) {
            return null;
        }
        if (this.hasTreeFocus()) {
            return super.doInsertNew(data, doIt, sender);
        }
        if (this.hasPlotFocus()) {
            if (doIt == 0 || doIt == 1) {
                if (data instanceof String) {
                    int type = 0;
                    if ("bookmark".equals((String)data)) {
                        type = 1;
                    } else if ("task".equals((String)data)) {
                        type = 3;
                    } else if ("annotation".equals((String)data)) {
                        type = 4;
                    }
                    return this.doMarkerAdd(String.valueOf(type), doIt, sender);
                }
                return true;
            }
            if (doIt == 6) {
                BaseGroupedInformations informations = new BaseGroupedInformations();
                informations.add(new GroupedInformation(IGroupedInformations.DEFAULT_GROUP, "bookmark", I18n.General_Bookmark, I18n.General_AddBookmark, Registration.images.getIconId("de.toem.impulse.images.marker.bookmark")));
                informations.add(new GroupedInformation(IGroupedInformations.DEFAULT_GROUP, "task", I18n.General_Task, I18n.General_AddTask, Registration.images.getIconId("de.toem.impulse.images.marker.task")));
                informations.add(new GroupedInformation(IGroupedInformations.DEFAULT_GROUP, "annotation", I18n.General_Annotation, I18n.General_AddAnnotation, Registration.images.getIconId("de.toem.impulse.images.marker.annotation")));
                return informations;
            }
            return null;
        }
        this.hasDetailsFocus();
        return null;
    }

    private Object doMarkerAdd(Object data, int doIt, Object sender) {
        return this.access(null, (T plotTree) -> {
            if (doIt == 0 || doIt == 1) {
                if (this.streamMode) {
                    return false;
                }
                List<ICell> cells = this.getSelectedCells();
                if (cells.size() != 1) {
                    return false;
                }
                ICell config = cells.get(0);
                if (!(config instanceof PlotConfiguration)) {
                    return false;
                }
                if (((PlotConfiguration)config).isProduction()) {
                    return false;
                }
                Signal signal = ((PlotConfiguration)config).getSignal(this);
                if (signal == null || !signal.getElement().isBound() || !signal.getElement().canBeModified()) {
                    return false;
                }
                IDomainBase base = DomainBase.valueOf(signal);
                ICursorItem cursor = plotTree.getActiveCursor();
                if (cursor == null || !cursor.getDomainBase().isCompatible(base)) {
                    return false;
                }
                if (doIt == 0) {
                    if (!(data instanceof String)) {
                        return false;
                    }
                    int type = Utils.parseInt((String)data, 0);
                    if (type == 0) {
                        return false;
                    }
                    Marker marker = new Marker(type, "", 0, 1);
                    switch (type) {
                        case 1: {
                            marker.setMessage("New bookmark");
                            break;
                        }
                        case 3: {
                            marker.setMessage("New task");
                            break;
                        }
                        case 4: {
                            IReadableSamples reader = signal.getSamples(this);
                            int idx = reader.indexAt(cursor.getPosition());
                            if (idx != -1) {
                                marker.setMessage(String.valueOf(config.getName()) + "=" + reader.stringValueAt(idx));
                                break;
                            }
                            marker.setMessage("New annotation");
                        }
                    }
                    Link link = new Link();
                    link.setParameter(base.getDomainLabel(), String.valueOf(cursor.getPosition()));
                    marker.addLocation(link);
                }
                return true;
            }
            return null;
        });
    }

    @Override
    protected Object doEditFindReplace(Object data, int doIt, Object sender) {
        return this.access(null, (T plotTree) -> {
            if (doIt == 0 || doIt == 1) {
                for (ISelectItem sel : plotTree.getSelection()) {
                    if (sel.isTreeItemSelection() && ((ITreeItem)sel.item()).hasDomainAxis() && ((ITreeItem)sel.item()).getData() instanceof ICell) continue;
                    return false;
                }
                if (doIt == 0) {
                    if (this.findReplaceContext == null) {
                        this.findReplaceContext = new ViewerFindContext(this);
                    }
                    this.findReplaceContext.open();
                }
                return true;
            }
            return null;
        });
    }

    @Override
    protected Object doEditFindPrevious(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (this.findReplaceContext == null) {
                return false;
            }
            if (doIt == 0 && this.findReplaceContext != null) {
                this.findReplaceContext.findPrevious();
            }
            return true;
        }
        return null;
    }

    @Override
    protected Object doEditFindNext(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (this.findReplaceContext == null) {
                return false;
            }
            if (doIt == 0 && this.findReplaceContext != null) {
                this.findReplaceContext.findNext();
            }
            return true;
        }
        return null;
    }

    @Override
    protected Object doEditSelectAll(Object data, int doIt, Object sender) {
        return this.access(null, (T plotTree) -> {
            if (doIt == 0 || doIt == 1) {
                if (doIt == 0) {
                    plotTree.selectAll();
                    plotTree.perform();
                }
                return true;
            }
            return null;
        });
    }

    @Override
    protected Object doMarkerDeleteAll(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (this.streamMode) {
                return false;
            }
            List<ICell> selected = this.getSelectedCells();
            if (selected.isEmpty()) {
                return false;
            }
            ArrayList<Signal> signals = new ArrayList<Signal>();
            for (ICell s : selected) {
                Signal signal;
                if (!(s instanceof PlotConfiguration) || (signal = ((PlotConfiguration)s).getSignal(this)) == null || !signal.getElement().isBound() || !signal.getElement().canBeModified() || Utils.isEmpty(signal.markers)) continue;
                signals.add(signal);
            }
            if (signals.isEmpty()) {
                return false;
            }
            if (doIt == 0) {
                ArrayList<ElementFieldModifier> modifiers = new ArrayList<ElementFieldModifier>();
                for (Signal signal : signals) {
                    modifiers.add(new ElementFieldModifier(signal.getElement(), "markers", null, null, false));
                }
                this.getEditor().apply("Remove markers", null, modifiers.toArray(new IElementModifier[modifiers.size()]), true);
            }
            return true;
        }
        return null;
    }

    @Override
    public Object command(String id, Object data, int doIt, Object sender) {
        if (this.view.hasCell() && this.getSelectedCells().contains(this.view.getCell()) && (IController.Commands.Cut.commandId.equals(id) || IController.Commands.Up.commandId.equals(id) || IController.Commands.Down.commandId.equals(id))) {
            return null;
        }
        if (id.equals("de.toem.impulse.commands.cursor.add")) {
            if (doIt != 5) {
                return this.doCursorAdd(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.cursor.delete")) {
            if (doIt != 5) {
                return this.doCursorDelete(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.cursor.deleteAll")) {
            if (doIt != 5) {
                return this.doCursorDeleteAll(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.cursor.edit")) {
            if (doIt != 5) {
                return this.doCursorEdit(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.cursor.show")) {
            if (doIt != 5) {
                return this.doCursorShow(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.cursor.activate")) {
            if (doIt != 5) {
                return this.doCursorActivate(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.goto")) {
            if (doIt != 5) {
                return this.doGoto(data, doIt, sender, false);
            }
        } else if (id.equals("de.toem.impulse.commands.cursor.color")) {
            if (doIt != 5) {
                return this.doCursorColor(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.plot.color")) {
            if (doIt != 5) {
                return this.doSamplesColor(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.axis.domain.zoom.in")) {
            if (doIt != 5) {
                return this.doAxisDomainZoomIn(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.axis.domain.zoom.out")) {
            if (doIt != 5) {
                return this.doAxisDomainZoomOut(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.axis.domain.zoom.fit")) {
            if (doIt != 5) {
                return this.doAxisDomainZoomFit(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.axis.value.range.set")) {
            if (doIt != 5) {
                return this.doAxisValueRangeSet(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.geometry.namesRightAligned")) {
            if (doIt != 5) {
                return this.doGeometryNamesRightAligned(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.geometry.configIcon")) {
            if (doIt != 5) {
                return this.doGeometryConfigIcon(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.geometry.valueColumn")) {
            if (doIt != 5) {
                return this.doGeometryValueColumn(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.geometry.fitVertical")) {
            if (doIt != 5) {
                return this.doGeometryFitVertical(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.geometry.axis")) {
            if (doIt != 5) {
                return this.doGeometryAxis(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.geometry.grid")) {
            if (doIt != 5) {
                return this.doGeometryGrid(data, doIt, sender);
            }
        } else {
            return super.command(id, data, doIt, sender);
        }
        if (doIt == 5) {
            return true;
        }
        return null;
    }

    @Override
    protected boolean filterAddNewTypes(IElement target, String type) {
        PlotConfiguration config;
        if (type.equals("configuration.folder") || type.equals("configuration.samples")) {
            return false;
        }
        return !type.equals("configuration.srcref") || !target.isBound() || !target.hasCell(PlotConfiguration.class) || !(config = (PlotConfiguration)target.getCell()).isProduction();
    }

    public Object doCursorAdd(Object data, int doIt, Object sender) {
        return this.doAddNewElement(new String[]{"de.toem.impulse.instancer.configuration.cursor", "configuration.cursor"}, doIt, sender, this.view, -1);
    }

    private Object doGoto(Object data, int doIt, Object sender, boolean go) {
        return this.access(null, (T plotTree) -> {
            if (doIt == 0 || doIt == 1) {
                if (plotTree.getCursorItems().size() < 1) {
                    return false;
                }
                if (doIt == 0) {
                    if ("prev".equals(data)) {
                        this.moveActiveCursorToNextChange(false, true);
                        plotTree.perform();
                    } else if ("next".equals(data)) {
                        this.moveActiveCursorToNextChange(true, true);
                        plotTree.perform();
                    } else if ("pos1".equals(data)) {
                        plotTree.goToStart();
                        plotTree.perform();
                    } else if ("end".equals(data)) {
                        plotTree.goToEnd();
                        plotTree.perform();
                    }
                }
                return true;
            }
            return null;
        });
    }

    private void moveActiveCursorToNextChange(boolean forward, boolean keepVisible) {
        this.access(plotTree -> {
            ICursorItem activeCursor = plotTree.getActiveCursor();
            IDomainAxis axis = plotTree.getActiveAxis();
            if (activeCursor != null) {
                long next = Long.MIN_VALUE;
                for (ISelectItem sel : plotTree.getSelection()) {
                    long n;
                    IPlotItem item;
                    if (!sel.isPlotItemSelection() || !(item = (IPlotItem)sel.item()).hasDomainAxis() || item.getAxis() != axis || (n = this.getChangePosition(item, activeCursor.getPositionInDomainUnits(), forward)) == activeCursor.getPositionInDomainUnits() || next != Long.MIN_VALUE && Math.abs(activeCursor.getPositionInDomainUnits() - n) >= Math.abs(activeCursor.getPositionInDomainUnits() - next)) continue;
                    next = n;
                }
                if (next == Long.MIN_VALUE) {
                    return;
                }
                plotTree.moveActiveCursor(new DomainValue(axis.getDomainBase(), next), keepVisible, 0, true);
            }
        });
    }

    private long getChangePosition(IPlotItem item, long units, boolean forward) {
        long u = units;
        IReadableSamples samples = item.getSamples();
        if (samples != null) {
            int index = samples.indexAt(units);
            if (index >= 0 && !forward) {
                u = samples.unitsAt(index);
                while (u == units && index > 0) {
                    u = samples.unitsAt(--index);
                }
            } else if (index < samples.getCount() - 1 && forward) {
                u = samples.unitsAt(++index);
                while (u == units && index < samples.getCount() - 1) {
                    u = samples.unitsAt(++index);
                }
            } else if (index == samples.getCount() - 1 && forward) {
                u = samples.getEndUnits();
            }
        }
        return u;
    }

    @Override
    protected Object doCursorEdit(Object data, int doIt, Object sender) {
        return this.access(null, (T plotTree) -> {
            if (doIt == 0 || doIt == 1) {
                if (plotTree.getActiveCursor() == null) {
                    return false;
                }
                ICell cell = (ICell)plotTree.getActiveCursor().getData();
                if (!cell.getElement().isBound()) {
                    return false;
                }
                ArrayList<IElement> elements = new ArrayList<IElement>();
                elements.add(cell.getElement());
                return this.doEditElements(data, doIt, sender, elements);
            }
            return null;
        });
    }

    private Object doCursorDelete(Object data, int doIt, Object sender) {
        return this.access(null, (T plotTree) -> {
            if (doIt == 0 || doIt == 1) {
                ICell cell;
                if (plotTree.getActiveCursor() == null) {
                    return false;
                }
                ICell iCell = cell = plotTree.getActiveCursor() != null && plotTree.getActiveCursor().getData() instanceof ICell ? (ICell)plotTree.getActiveCursor().getData() : null;
                if (!cell.getElement().isBound()) {
                    return false;
                }
                if (doIt == 0) {
                    ArrayList<IElement> elements = new ArrayList<IElement>();
                    elements.add(cell.getElement());
                    return this.doEditDeleteElements(data, doIt, sender, elements);
                }
                return true;
            }
            return null;
        });
    }

    private Object doCursorDeleteAll(Object data, int doIt, Object sender) {
        return this.access(null, (T plotTree) -> {
            if (doIt == 0 || doIt == 1) {
                if (plotTree.getCursorItems().isEmpty()) {
                    return false;
                }
                if (doIt == 0) {
                    ArrayList<IElement> elements = new ArrayList<IElement>();
                    for (ICursorItem item : plotTree.getCursorItems()) {
                        IElement element = ((ICell)item.getData()).getElement();
                        if (!element.isBound()) continue;
                        elements.add(element);
                    }
                    return this.doEditDeleteElements(data, doIt, sender, elements);
                }
                return true;
            }
            return null;
        });
    }

    private Object doCursorColor(Object data, int doIt, Object sender) {
        return this.access(null, (T plotTree) -> {
            if (doIt == 0 || doIt == 1) {
                ICell cell;
                if (plotTree.getActiveCursor() == null) {
                    return false;
                }
                ICell iCell = cell = plotTree.getActiveCursor() != null && plotTree.getActiveCursor().getData() instanceof ICell ? (ICell)plotTree.getActiveCursor().getData() : null;
                if (!cell.getElement().isBound()) {
                    return false;
                }
                if (doIt == 0) {
                    int value = cell.getValueAsInt("color");
                    if (data != null && data instanceof String) {
                        int rgb = Utils.parseInt((String)data, 0);
                        value = rgb == value ? -1 : rgb;
                    }
                    if (value != -1) {
                        this.setCellValue(cell, "color", (Object)value, false);
                    }
                }
                return true;
            }
            return null;
        });
    }

    private Object doSamplesColor(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            List<IElement> elements = this.getSelectedElements();
            if (elements.size() < 1) {
                return false;
            }
            if (!Elements.haveCells(elements, PlotConfiguration.class)) {
                return false;
            }
            if (doIt == 0) {
                List<ICell> cells = Elements.extractCells(elements, false);
                int value = cells.get(0).getValueAsInt("color");
                if (data != null) {
                    if ("multiple".equals(data)) {
                        int index = 0;
                        for (ICell cell : cells) {
                            this.setCellValue(cell, "color", (Object)TLK.getSystemColorRgbInt("de.toem.impulse.color.sample." + index, 65280), false);
                            if (TLK.getSystemColorRgbInt("de.toem.impulse.color.sample." + index, 65280) == 65280) {
                                index = 0;
                            }
                            ++index;
                        }
                        return true;
                    }
                    if (data instanceof String) {
                        int rgb = Utils.parseInt((String)data, 0);
                        value = rgb == value ? -1 : rgb;
                    }
                }
                if (value != -1) {
                    for (ICell cell : cells) {
                        this.setCellValue(cell, "color", (Object)value, false);
                    }
                }
            }
            return true;
        }
        if (doIt == 6) {
            BaseGroupedInformations informations = new BaseGroupedInformations();
            String cfr_ignored_0 = String.valueOf(I18n.General_DefaultColor) + " ";
            String cfr_ignored_1 = String.valueOf(I18n.General_ApplyDefaultColor) + " ";
            return informations;
        }
        return null;
    }

    SamplesStat calcMaxMinRange(PlotConfiguration config) {
        IReadableSamples readable = config.getSamples(this);
        if (readable != null) {
            return readable.getStat(0, readable.getCount() - 1, 8);
        }
        return null;
    }

    public Object doAxisValueRangeSet(Object data, int doIt, Object sender) {
        final List<ICell> selection = this.getSelectedCells();
        if (selection.isEmpty()) {
            return null;
        }
        for (ICell cell : selection) {
            if (cell instanceof PlotConfiguration && ((PlotConfiguration)cell).hasValueAxis()) continue;
            return null;
        }
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                if (!(data instanceof String)) {
                    return false;
                }
                if ("auto".equals(data)) {
                    this.setCellValues(selection, PlotConfiguration.class, "scale", false, true);
                } else {
                    String[] splitted = ((String)data).split("/");
                    if (splitted.length >= 2) {
                        float min = Utils.parseFloat(splitted[0], 0.0f);
                        float max = Utils.parseFloat(splitted[1], 1.0f);
                        if (max > min) {
                            this.setCellValues(selection, PlotConfiguration.class, "scaleFrom", Float.valueOf(min), true);
                            this.setCellValues(selection, PlotConfiguration.class, "scaleTo", Float.valueOf(max), true);
                            this.setCellValues(selection, PlotConfiguration.class, "scale", true, true);
                        }
                    }
                }
            }
            return true;
        }
        if (doIt == 6) {
            try {
                return Actives.timeout(new IEvaluable(){

                    @Override
                    public Object evaluate(IProgress p) {
                        BaseGroupedInformations options = new BaseGroupedInformations();
                        double tmin = 3.4028234663852886E38;
                        double tmax = -3.4028234663852886E38;
                        int added = 0;
                        for (ICell cell : selection) {
                            SamplesStat stat = ViewerPlotTreeController.this.calcMaxMinRange((PlotConfiguration)cell);
                            if (stat == null) continue;
                            double min = new BigDecimal(stat.min).round(new MathContext(3, RoundingMode.FLOOR)).doubleValue();
                            double max = new BigDecimal(stat.max).round(new MathContext(3, RoundingMode.CEILING)).doubleValue();
                            if ((double)stat.min < tmin) {
                                tmin = min;
                            }
                            if ((double)stat.max > tmax) {
                                tmax = max;
                            }
                            String id = String.valueOf(String.valueOf(min)) + "/" + String.valueOf(max) + "/" + cell.getName();
                            String label = String.valueOf(selection.size() > 1 ? String.valueOf(I18n.SamplesComposite_MaxMinOf) + " \"" + cell.getName() + "\"" : I18n.SamplesComposite_MaxMin) + ": " + String.valueOf(min) + " ... " + String.valueOf(max);
                            options.add(new GroupedInformation(IGroupedInformations.DEFAULT_GROUP, id, label, null));
                            ++added;
                        }
                        if (tmax != -3.4028234663852886E38 && tmin != 3.4028234663852886E38 && added > 1) {
                            String id = String.valueOf(String.valueOf(tmin)) + "/" + String.valueOf(tmax);
                            String label = String.valueOf(I18n.SamplesComposite_Combined_) + " " + String.valueOf(tmin) + I18n.General_Dotdotdot + String.valueOf(tmax);
                            options.add(new GroupedInformation(IGroupedInformations.DEFAULT_GROUP, id, label, null));
                        }
                        return options;
                    }
                }, 1000);
            }
            catch (Throwable throwable) {}
        }
        return null;
    }

    public void highlightTarget(ISimpleSamplesProvider samples, IAttachment attachment) {
        this.access(plotTree -> plotTree.highlightAttachment(samples instanceof IPlotItem ? (IPlotItem)samples : null, attachment, 0, false));
    }

    @Override
    public void gotoTarget(Link link, int createOptions) {
        ArrayList<Link> sources = new ArrayList<Link>();
        sources.add(link);
        Map<Object, ITreeItem> items = this.treeItemSynchronizer.makeVisible(sources);
        ITreeItem item = items.get(link);
        if (item == null && PlotConfiguration.targetsSignal(link)) {
            if (createOptions == 2) {
                this.gotoNewTarget(link);
                return;
            }
            if (createOptions == 1) {
                this.tlk.openYesNoQuestion(I18n.SamplesComposite_SignalNotFoundInView, I18n.SamplesComposite_WantToAddNewPlot, result -> {
                    if (result == 1) {
                        this.gotoNewTarget(link);
                    }
                });
            }
        }
        if (item instanceof IPlotItem) {
            String value;
            IDomainBase domainBase;
            DomainValue p = null;
            IDomainBase iDomainBase = domainBase = ((IPlotItem)item).getSamples() != null ? ((IPlotItem)item).getSamples().getDomainBase() : null;
            if (domainBase != null && (value = link.getParameter(domainBase.getDomainLabel())) != null) {
                long units = domainBase.parseUnits(value);
                p = new DomainValue(domainBase, units);
            }
            DomainValue position = p;
            int idx = Utils.parseInt(link.getParameter("idx"), -1);
            this.fireActiveCursorChanged(position, (IPlotItem)item, idx);
            this.access(plotTree -> {
                plotTree.select(item, false);
                plotTree.makeVisible(item);
                if (position != null) {
                    plotTree.moveActiveCursor(position, true, 0, false);
                }
                plotTree.perform();
            });
        }
    }

    void gotoNewTarget(Link link) {
        if (!this.getRecord().isBound()) {
            return;
        }
        ICell target = this.getRecord().getCellByLink(link);
        Signal signal = null;
        if (target instanceof ISignalProvider) {
            signal = ((ISignalProvider)target).getSignal(this);
        }
        if (signal == null) {
            return;
        }
        boolean toInsert = false;
        boolean toAdd = false;
        IElement[] itarget = this.getInsertTargetElement();
        toInsert = itarget != null && itarget.length >= 2 && itarget[0].isBound() && itarget[0].getCell() instanceof AxisConfiguration;
        IElement atarget = this.getAddPasteTargetElement();
        boolean bl = toAdd = atarget != null && atarget.isBound() && atarget.getCell() instanceof AxisConfiguration;
        if (toInsert) {
            int index = itarget[0].indexOf(itarget[1]) + 1;
            this.doAddNewElement(new String[]{"de.toem.impulse.instancer.configuration.samples", "SIGNAL" + signal.getPath()}, 0, this, itarget[0], index);
        } else if (!toInsert && toAdd) {
            this.doAddNew(new String[]{"de.toem.impulse.instancer.configuration.samples", "SIGNAL" + signal.getPath()}, 0, this);
        } else {
            this.doAddNewElement(new String[]{"de.toem.impulse.instancer.configuration.samples", "SIGNAL" + signal.getPath()}, 0, this, this.view, -1);
        }
        this.gotoTarget(link, 0);
    }

    public void requestTarget(Link link, IRecordViewerChildListener listener) {
        String value;
        IElement view = this.getView();
        if (!view.isBound() || !view.hasCell()) {
            return;
        }
        ICell base = view.getCell();
        if (!this.getRecord().isBound()) {
            return;
        }
        this.getRecord();
        ICell target = this.getRecord().getCellByLink(link);
        Signal signal = null;
        if (target instanceof ISignalProvider) {
            signal = ((ISignalProvider)target).getSignal(this);
        }
        if (signal == null) {
            return;
        }
        DomainValue pos = null;
        IDomainBase signalDomainBase = DomainBase.valueOf(signal);
        if (base != null && (value = link.getParameter(signalDomainBase.getDomainLabel())) != null) {
            long units = signalDomainBase.parseUnits(value);
            pos = new DomainValue(signalDomainBase, units);
        }
        DomainValue position = pos;
        ICell plot = null;
        for (ICell c : base.getTribe(false, PlotConfiguration.class)) {
            PlotConfiguration p = (PlotConfiguration)c;
            if (p.samples == null || p.isProduction() || !p.samples.equalTargets(link, this.getRecord().getCell())) continue;
            plot = p;
            break;
        }
        listener.signalsChanged(Arrays.asList(plot != null ? plot : target), null);
        if (position != null) {
            listener.positionChanged(new DomainPosition(position, null, -1), null);
        }
    }
}

