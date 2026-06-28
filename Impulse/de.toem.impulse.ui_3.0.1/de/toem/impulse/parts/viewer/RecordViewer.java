/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.parts.viewer;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.ports.IRecordPort;
import de.toem.impulse.cells.preferences.ImpulseSerializers;
import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.cells.serializer.Serializer;
import de.toem.impulse.cells.view.FolderConfiguration;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.cells.view.ViewConfiguration;
import de.toem.impulse.dialog.parts.AboutDialog;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.paint.ICursorItem;
import de.toem.impulse.paint.IPlotTree;
import de.toem.impulse.paint.IPlotTreeMouseListener;
import de.toem.impulse.paint.controller.ViewerPlotTreeController;
import de.toem.impulse.parts.viewer.PortInput;
import de.toem.impulse.parts.viewer.ViewerPersitence;
import de.toem.impulse.provider.ISamplesCache;
import de.toem.impulse.provider.ISamplesContext;
import de.toem.impulse.provider.ISignalContext;
import de.toem.impulse.provider.ISimpleSamplesProvider;
import de.toem.impulse.samples.base.RecordComparator;
import de.toem.impulse.ui.DomainPosition;
import de.toem.impulse.ui.IRecordPortViewer;
import de.toem.impulse.ui.IRecordViewerChildListener;
import de.toem.impulse.ui.ImpulseNotifications;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellDescriptor;
import de.toem.toolkits.pattern.element.ElementModifierEvent;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.element.serializer.Message;
import de.toem.toolkits.pattern.element.temp.TemporaryCellElement;
import de.toem.toolkits.pattern.information.BaseGroupedInformations;
import de.toem.toolkits.pattern.information.GroupedInformation;
import de.toem.toolkits.pattern.information.IGroupedInformation;
import de.toem.toolkits.pattern.information.IGroupedInformations;
import de.toem.toolkits.pattern.information.IInformationGroup;
import de.toem.toolkits.pattern.information.InformationGroup;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.base.CellTreeController;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.controller.commands.CommandButtonController;
import de.toem.toolkits.ui.controller.layout.ShareLayoutController;
import de.toem.toolkits.ui.controller.source.HintSource;
import de.toem.toolkits.ui.dialog.InformationSelectorDialog;
import de.toem.toolkits.ui.dnd.ControllerDragListener;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.part.dialog.Dialogs;
import de.toem.toolkits.ui.part.editor.AbstractEditorPart;
import de.toem.toolkits.ui.part.view.AbstractInputViewPart;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.TLK;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.layout.TlkShareData;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class RecordViewer
extends AbstractEditorPart
implements ISignalContext,
ISamplesContext,
IRecordPortViewer {
    protected IElement viewPreferences = IElement.NONE;
    protected IElement chartPreferences = IElement.NONE;
    protected IElement serializerPreferences = IElement.NONE;
    Link view;
    boolean filterConfigurations;
    ViewerPlotTreeController plotTreeController;
    Link top;
    PortInput streams;
    boolean iterating;
    IController filterTableController;
    ShareLayoutController topLayoutController;
    ShareLayoutController leftLayoutController;
    CellTreeController scopeController;
    List<ICell> filterCells;
    String filterPattern;
    private CheckController childrenCheck;
    protected transient List<IRecordViewerChildListener> childListeners = new ArrayList<IRecordViewerChildListener>();
    protected String PART_KEY = "de.toem.impulse.viewer";
    private boolean askingForNewView;
    private boolean openedViews;

    public RecordViewer(ITlkPartContainer partContainer, int style) {
        super(partContainer, style);
    }

    public RecordViewer(ITlkPartContainer.ITlkEditorSession session, int style) {
        super(session, style);
    }

    public RecordViewer() {
    }

    @Override
    protected void beforeCreation() {
        super.beforeCreation();
    }

    @Override
    protected void afterCreation() {
        super.afterCreation();
        ImpulseNotifications.firePartCreated(this);
    }

    @Override
    public void dispose() {
        super.dispose();
        if (this.streams != null) {
            this.streams.dispose();
            this.streams = null;
        }
        this.plotTreeController = null;
        this.filterTableController = null;
        this.topLayoutController = null;
        this.leftLayoutController = null;
        this.scopeController = null;
        this.filterCells = null;
        this.childrenCheck = null;
        if (this.childListeners != null) {
            this.childListeners.clear();
        }
        ImpulseNotifications.firePartDisposed(this);
    }

    @Override
    public String getId() {
        return this.PART_KEY;
    }

    @Override
    protected void installListener() {
        super.installListener();
        if (this.viewPreferences == IElement.NONE) {
            this.viewPreferences = ImpulsePreferences.viewPreferences;
            this.viewPreferences.addListener(this);
        }
        if (this.chartPreferences == IElement.NONE) {
            this.chartPreferences = ImpulsePreferences.chartPreferences;
            this.chartPreferences.addListener(this);
        }
        if (this.serializerPreferences == IElement.NONE) {
            this.serializerPreferences = ImpulsePreferences.serializerPreferences;
            this.serializerPreferences.addListener(this);
        }
    }

    @Override
    protected void removeListener(boolean open) {
        super.removeListener(open);
        if (!open && this.viewPreferences != IElement.NONE) {
            this.viewPreferences.removeListener(this);
            this.viewPreferences = IElement.NONE;
        }
        if (!open && this.chartPreferences != IElement.NONE) {
            this.chartPreferences.removeListener(this);
            this.chartPreferences = IElement.NONE;
        }
        if (!open && this.serializerPreferences != IElement.NONE) {
            this.serializerPreferences.removeListener(this);
            this.serializerPreferences = IElement.NONE;
        }
    }

    @Override
    protected String getPersitanceType() {
        return "persitence.impulse.viewer";
    }

    @Override
    protected ViewerPersitence getPersistence() {
        return (ViewerPersitence)super.getPersistence();
    }

    @Override
    protected void setInput(IElement inputElement, IElement diffElement) {
        if (inputElement.isBound() && inputElement.hasCell(IRecordPort.class)) {
            this.setPortInput(inputElement);
        } else if (inputElement.isBound() && diffElement.isBound() && inputElement.hasCell(Record.class) && diffElement.hasCell(Record.class)) {
            this.setDiffInput(inputElement, diffElement);
        } else {
            super.setInput(inputElement, diffElement);
        }
        ImpulseNotifications.fireViewerInputSet(this, inputElement, this.editorElement, this.baseElement);
    }

    protected void setDiffInput(IElement inputElementA, IElement inputElementB) {
        final Record baseCell = new Record();
        this.editorElement = inputElementA;
        this.baseElement = new TemporaryCellElement(2, "Compare", baseCell, this.editorElement);
        this.PART_KEY = "de.toem.impulse.diffViewer";
        final Record a = (Record)inputElementA.getCell();
        final Record b = (Record)inputElementB.getCell();
        boolean configured = Boolean.TRUE.toString().equals(this.editorElement.getHint("compare.configured"));
        Actives.run(new IExecutable(){

            @Override
            public void execute(IProgress p) {
                RecordComparator.createDiff(p, a, b, baseCell);
            }
        }, null, true);
    }

    protected void setPortInput(IElement inputElement) {
        Record baseCell = new Record();
        this.editorElement = inputElement;
        this.baseElement = new TemporaryCellElement(2, this.editorElement.getName(), baseCell, this.editorElement);
        IRecordPort port = (IRecordPort)this.editorElement.getCell();
        if (this.plotTreeController != null) {
            this.plotTreeController.initStreaming();
        }
        if (this.streams != null) {
            this.streams.dispose();
        }
        this.streams = new PortInput(port, this.baseElement){

            @Override
            protected void started() {
                RecordViewer.this.updateControls(null);
            }

            @Override
            protected void enteredStreaming() {
                if (RecordViewer.this.streams != null && RecordViewer.this.plotTreeController != null) {
                    RecordViewer.this.plotTreeController.enterStreamMode();
                }
            }

            @Override
            protected void enteredUpdating(boolean moveActiveCursor) {
                if (RecordViewer.this.streams != null && RecordViewer.this.plotTreeController != null) {
                    RecordViewer.this.plotTreeController.enterUpdateMode(moveActiveCursor);
                    RecordViewer.this.plotTreeController.updateStreamSamples(false);
                }
            }

            @Override
            protected void leftUpdating() {
                if (RecordViewer.this.streams != null && RecordViewer.this.plotTreeController != null) {
                    RecordViewer.this.plotTreeController.updateStreamTree();
                    RecordViewer.this.plotTreeController.leaveUpdateMode();
                }
            }

            @Override
            protected void updateCurrentValue() {
                if (RecordViewer.this.streams != null && RecordViewer.this.plotTreeController != null) {
                    RecordViewer.this.plotTreeController.updateStreamTree();
                }
            }

            @Override
            protected void update(int changed) {
                if (RecordViewer.this.streams != null && RecordViewer.this.plotTreeController != null) {
                    if (RecordViewer.this.plotTreeController.isUpdateMode() && this.isStreaming()) {
                        RecordViewer.this.plotTreeController.updateStreamSamples(changed < 4);
                    }
                    if (changed >= 4) {
                        RecordViewer.this.updateControls(null);
                    }
                }
            }

            @Override
            protected void checkMode() {
                if (RecordViewer.this.streams != null && RecordViewer.this.plotTreeController != null && RecordViewer.this.streams.isUpdating() && !RecordViewer.this.plotTreeController.checkUpdateMode()) {
                    RecordViewer.this.streams.enterUpdating(false);
                }
            }

            @Override
            protected void finished() {
                if (RecordViewer.this.streams != null && RecordViewer.this.plotTreeController != null) {
                    RecordViewer.this.plotTreeController.updateStreamSamples(false);
                    RecordViewer.this.plotTreeController.updateStreamTree();
                    RecordViewer.this.plotTreeController.leaveUpdateMode();
                    RecordViewer.this.plotTreeController.leaveStreamMode();
                    RecordViewer.this.updateControls(null);
                }
            }
        };
    }

    @Override
    protected void handleInput(boolean inputChanged) {
        if (this.getSamplesCache() != null) {
            this.getSamplesCache().clear();
        }
        Link view = null;
        if (inputChanged && this.baseElement.isBound()) {
            String hint = this.editorElement.getHint(String.valueOf(this.PART_KEY) + ".configuration");
            this.filterConfigurations = Boolean.TRUE.toString().equals(this.editorElement.getHint(String.valueOf(this.PART_KEY) + ".configurationFilter"));
            if (hint instanceof String && (view = Link.fromPath(hint)).resolveCell(this.viewPreferences, ViewConfiguration.class) == null) {
                view = null;
            }
            if (view != null) {
                this.setView(view);
            }
            if (view != null && (hint = this.editorElement.getHint(String.valueOf(this.PART_KEY) + ".top")) instanceof String) {
                this.top = Link.fromPath(hint);
                if (this.top.resolveCell(this.viewPreferences, ViewConfiguration.class) == null) {
                    view = null;
                }
            }
            if (this.getElement().hasCell()) {
                this.onInputReady();
            }
        }
    }

    private void onInputReady() {
        boolean doShowAll;
        boolean gotMessage = false;
        if (this.getElement().isBound() && this.getElement().hasCell() && !this.getElement().getCell().getChildren(Message.class).isEmpty()) {
            this.doGeometrySignals(true, 0, this);
            gotMessage = true;
        }
        if (this.view == null) {
            List<IElement> views = this.viewPreferences.getChildren(null, ViewConfiguration.class);
            IElement fit = IElement.NONE;
            float strength = 0.5f;
            for (IElement e : views) {
                float s = this.viewFits(e);
                if (!(s > strength)) continue;
                fit = e;
            }
            if (fit.isBound()) {
                this.view = fit.getLink(this.viewPreferences);
            }
            if (this.view != null) {
                this.setView(this.view);
            }
        }
        if (!this.openedViews && !gotMessage && this.view == null && this.viewPreferences.hasCell() && this.getCurrentChildEditor() == null) {
            this.doViewsOpen(null, 0, this);
            this.openedViews = true;
        }
        boolean bl = doShowAll = this.editorElement.isBound() && Boolean.TRUE.toString().equals(this.editorElement.getHint(String.valueOf(this.PART_KEY) + ".showAll"));
        if (doShowAll && this.plotTreeController != null) {
            this.plotTreeController.doAxisDomainZoomFit(null, 0, this);
            if (this.editorElement.isBound()) {
                this.editorElement.setHint(String.valueOf(this.PART_KEY) + ".showAll", Boolean.FALSE.toString());
            }
        }
    }

    private String getReader() {
        if (this.getElement().isBound()) {
            String serializer = null;
            IElement element = this.getElement();
            if (element.isBound() && element.hasCover()) {
                ICover cover = element.getCover();
                serializer = cover != null ? cover.getSerializer() : null;
            } else {
                serializer = this.getElement().getHint("SERIALIZER");
                if (!Utils.isEmpty(serializer)) {
                    serializer = "trying " + serializer;
                }
            }
            return serializer;
        }
        return null;
    }

    private String getReaderConfiguration() {
        if (this.getElement().isBound()) {
            String configuration = null;
            IElement element = this.getElement();
            if (element.isBound() && element.hasCover()) {
                ICover cover = element.getCover();
                configuration = cover != null ? cover.getConfiguration() : null;
            } else {
                configuration = this.getElement().getHint("CONFIGURATION");
                if (!Utils.isEmpty(configuration)) {
                    configuration = "trying " + configuration;
                }
            }
            return configuration;
        }
        return null;
    }

    @Override
    public void createControls(Object container) {
        try {
            this.topLayoutController = (ShareLayoutController)new ShareLayoutController(this, String.valueOf(this.PART_KEY) + ".samples.v", 1, 0).initDefaults(350.0f, 0).setOwner(IController.OWNER_UNDEFINED);
            ITlkComposite main = this.tlk().addComposite(container, this.topLayoutController, null, new TlkShareData(1), 0, null, null);
            ITlkComposite left = this.tlk().addComposite(main, null, this.cols(), new TlkShareData(1), 0, null, null);
            ITlkComposite reader = this.tlk().addComposite(left, null, 3, this.cols(), 0x100000 | this.style, I18n.General_Serializer_, null);
            final IGroupedInformations<? extends IGroupedInformation, IInformationGroup> serializers = Elements.serializers.select(true, true, null);
            final 3 readerText = this.tlk().addText(reader, new TextController(this.editor(), null){

                @Override
                protected Object value() {
                    return RecordViewer.this.getReader();
                }

                @Override
                protected Object convert(Object value) {
                    String label = value instanceof String && serializers.get((String)value) != null ? ((IGroupedInformation)serializers.get((String)value)).getLabel() : "";
                    return label;
                }
            }, this.tlk().ld(1, 524288, 1), 8192, null);
            this.tlk().addButton(reader, new CommandButtonController(this.editor(), "de.toem.impulse.commands.samples.reader.edit", null, false).initApplyCmd(false, true), 1, 4096, "", "de-toem-toolkits-tlk-css-general-edit-image");
            this.tlk().addInPlaceDialog(left, reader, InformationSelectorDialog.getControls(I18n.General_Serializer, serializers, new HintSource(1, "SERIALIZER", null){

                @Override
                public void changeValue(Object value, boolean finalize) {
                    super.changeValue(value, finalize);
                    readerText.update(true);
                    RecordViewer.this.doSamplesReload(value != null ? "SERIALIZER" + value : null, 0, this);
                }
            }, -1, 12), 2, this.cols(), I18n.General_Serializer_);
            ITlkComposite configuration = this.tlk().addComposite(left, null, 3, this.cols(), 0x100000 | this.style, I18n.General_Configuration_, null);
            final 5 configurationText = this.tlk().addText(configuration, new TextController(this.editor(), null){

                @Override
                protected Object value() {
                    return RecordViewer.this.getReaderConfiguration();
                }
            }, this.tlk().ld(1, 524288, 1), 8192, null);
            this.tlk().addButton(configuration, new CommandButtonController(this.editor(), "de.toem.impulse.commands.samples.configuration.edit", null, false).initApplyCmd(false, true), 1, 4096, "", "de-toem-toolkits-tlk-css-general-edit-image");
            AbstractControlProvider configurationControls = new AbstractControlProvider(){

                @Override
                protected boolean fillThis() {
                    try {
                        CellTableController table = this.tlk().addTable(this.container(), new CellTableController(this.editor(), new HintSource(1, "CONFIGURATION", null){

                            @Override
                            public void changeValue(Object value, boolean finalize) {
                                super.changeValue(value, finalize);
                                configurationText.update(true);
                                RecordViewer.this.doSamplesReload(value != null ? "CONFIGURATION" + value : null, 0, this);
                            }
                        }){

                            @Override
                            public ICell getValueBaseCell() {
                                String id = RecordViewer.this.getReader();
                                if ((this).RecordViewer.this.serializerPreferences.isBound() && (this).RecordViewer.this.serializerPreferences.hasCell(ImpulseSerializers.class)) {
                                    for (ICell iCell : (this).RecordViewer.this.serializerPreferences.getCell().getChildren(Serializer.class)) {
                                        if (!((Serializer)iCell).id.equals(id)) continue;
                                        return iCell;
                                    }
                                }
                                return null;
                            }

                            @Override
                            protected boolean filterAddNewTypes(IElement target, String type) {
                                ICell doUpdateControl = (ICell)Elements.cells.create(type);
                                ICell serializer = this.getValueBaseCell();
                                return !(doUpdateControl instanceof ReaderConfiguration) || !(serializer instanceof Serializer) || !((ReaderConfiguration)doUpdateControl).supports((Serializer)serializer);
                            }

                            @Override
                            public void doUpdateControl() {
                                super.doUpdateControl();
                                ICell configuration = this.getValueBaseCell() != null && this.converted instanceof String ? this.getValueBaseCell().getChildByName((String)this.converted) : null;
                                this.select(configuration, true);
                            }

                            @Override
                            protected void selectionChanged() {
                                super.selectionChanged();
                                ICell cell = this.getSelectedCell();
                                this.setValue(cell instanceof ReaderConfiguration ? ((ReaderConfiguration)cell).getName() : null, true, false, false, false);
                            }
                        }.initCells(null, ReaderConfiguration.class).initColumnDataSources(true, new Object[]{ReaderConfiguration.class.getField("description")}), this.tlk().ldc(this.cols() - 1, 524288, 1), 1115712, null, new String[]{I18n.General_Name, I18n.General_Description});
                        ITlkComposite buttonComp = this.tlk().addComposite(this.container(), null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
                        CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this.editor(), table, true, true, true, false);
                    }
                    catch (NoSuchFieldException | SecurityException exception) {}
                    return false;
                }
            };
            this.tlk().addInPlaceDialog(left, configuration, configurationControls, 2, this.cols(), I18n.General_Configuration_);
            this.leftLayoutController = (ShareLayoutController)new ShareLayoutController(this, String.valueOf(this.PART_KEY) + ".samples.h", 1, 1).initDefaults(250.0f, 0).setOwner(IController.OWNER_UNDEFINED);
            ITlkComposite signals = this.tlk().addComposite(left, this.leftLayoutController, null, this.tlk().ld(this.cols(), 4, 1, 524288, 1), 0, null, null);
            this.scopeController = (CellTreeController)this.tlk().addTree(signals, new CellTreeController(this, null){

                @Override
                protected void setContextMenus() {
                    this.tlk().addMenu(this.control, this, "MENU", "de.toem.toolkits.popupmenu.tree", "de.toem.impulse.popupmenu.viewer.scope", new String[]{"add", "insert", "clone"});
                }

                @Override
                public void selectionChanged() {
                    RecordViewer.this.filterCells = this.getSelectedCells();
                    if (RecordViewer.this.filterTableController != null) {
                        RecordViewer.this.filterTableController.setUpdateRequired();
                        RecordViewer.this.filterTableController.update(this.getCell(), true);
                    }
                }

                @Override
                protected void doUpdateExternal() {
                    if (RecordViewer.this.filterTableController != null) {
                        RecordViewer.this.filterTableController.update(this.getCell(), true);
                    }
                }

                @Override
                protected Object doExecute(Object data, int doIt, Object sender) {
                    if (doIt == 0 || doIt == 1) {
                        if (RecordViewer.this.plotTreeController == null || !RecordViewer.this.plotTreeController.getView().isBound()) {
                            return false;
                        }
                        if (this.getSelectedCell() == null) {
                            return false;
                        }
                        if (doIt == 0) {
                            RecordViewer.this.plotTreeController.gotoTarget(this.getSelectedCell().getLink(), 2);
                        }
                        return true;
                    }
                    return null;
                }

                @Override
                protected boolean allowModification(IController.Commands command, Object data, Object context) {
                    switch (command) {
                        case Edit: {
                            return true;
                        }
                        case Cut: 
                        case Delete: 
                        case Rename: {
                            List<ICell> cells = this.getSelectedCells();
                            return cells.size() == 1 && cells.get(0) instanceof ViewConfiguration;
                        }
                        case Paste: {
                            return true;
                        }
                    }
                    return false;
                }

                @Override
                protected boolean filterPasteElements(IElement element) {
                    return !element.isBound() && element.hasCell(ViewConfiguration.class);
                }
            }.initCells(null, new Class[]{Message.class, Scope.class, Record.class, ViewConfiguration.class}).setKeepFirstItemExpanded(true).setShowRoot(true), new TlkShareData(1), 0x100002, null, null);
            this.scopeController.addDragSupport(4, new ControllerDragListener(this.scopeController){

                @Override
                public boolean handleStart(int x, int y) {
                    if (RecordViewer.this.plotTreeController != null && !RecordViewer.this.plotTreeController.getView().isBound()) {
                        RecordViewer.this.tlk.openYesNoQuestion(I18n.RecordViewer_ViewMissing, I18n.RecordViewer_NotViewSet, result -> {
                            if (result == 1) {
                                RecordViewer.this.doViewAdd(null, 0, this);
                            }
                        });
                        return false;
                    }
                    return true;
                }
            });
            this.tlk().addSash(signals, null, null, 2);
            ITlkComposite table = this.tlk().addComposite(signals, null, 2, new TlkShareData(2), 0, null, null);
            ITlkComposite filter = this.tlk().addComposite(table, null, 2, 2, 0, null, null);
            filter.setBackground("css.WIDGET_BACKGROUND");
            this.childrenCheck = this.tlk().addButton(filter, new CheckController(this, TLK.HINT(2, String.valueOf(this.PART_KEY) + ".check.children", "false")){

                @Override
                public void changed(boolean finalized) {
                    if (RecordViewer.this.filterTableController != null) {
                        RecordViewer.this.filterTableController.update(RecordViewer.this.filterTableController.getCell(), true);
                    }
                    super.changed(finalized);
                }
            }, this.tlk().ld(2, 131072, -1), 2048, I18n.General_AllChildren, null);
            this.filterTableController = this.tlk().addTable(table, new CellTableController(this, (Object)null){

                @Override
                protected void setContextMenus() {
                    this.tlk().addMenu(this.control, this, "MENU", "de.toem.toolkits.popupmenu.table", "de.toem.impulse.popupmenu.viewer.signals", new String[]{"add", "insert", "clone", "cut", "paste", "delete", "rename", "updown"});
                }

                @Override
                public boolean isAffected(ElementModifierEvent event) {
                    return true;
                }

                @Override
                public boolean needsUpdate() {
                    return true;
                }

                @Override
                protected List<ICell> getRawTableCells() {
                    if (RecordViewer.this.filterCells == null || RecordViewer.this.filterCells.isEmpty()) {
                        return ICell.EMPTY_LIST;
                    }
                    ArrayList<ICell> cells = new ArrayList<ICell>();
                    for (ICell cell : RecordViewer.this.filterCells) {
                        boolean contained = false;
                        if (RecordViewer.this.childrenCheck != null && RecordViewer.this.childrenCheck.isChecked()) {
                            for (ICell c : RecordViewer.this.filterCells) {
                                if (c == cell || !c.containsCell(cell)) continue;
                                contained = true;
                            }
                        }
                        if (contained) break;
                        if (RecordViewer.this.childrenCheck != null && RecordViewer.this.childrenCheck.isChecked()) {
                            cells.addAll(cell.getTribe(false, AbstractSignal.class));
                            continue;
                        }
                        cells.addAll(cell.getChildren(AbstractSignal.class));
                    }
                    return cells;
                }

                @Override
                protected Object doExecute(Object data, int doIt, Object sender) {
                    if (doIt == 0 || doIt == 1) {
                        if (RecordViewer.this.plotTreeController == null || !RecordViewer.this.plotTreeController.getView().isBound()) {
                            return false;
                        }
                        if (this.getSelectedCell() == null) {
                            return false;
                        }
                        if (doIt == 0) {
                            RecordViewer.this.plotTreeController.gotoTarget(this.getSelectedCell().getLink(), 2);
                        }
                        return true;
                    }
                    return null;
                }

                @Override
                protected boolean allowModification(IController.Commands command, Object data, Object context) {
                    return command == IController.Commands.Edit;
                }
            }.initCells(null, AbstractSignal.class).initColumnDataSources(true, new Object[]{AbstractSignal.class.getField("description"), Cell.class.getMethod("getContainerPath", new Class[0]), Elements.disclosures.get("de.toem.impulse.disclosures.signalType"), Elements.disclosures.get("de.toem.impulse.disclosures.signalTag"), Elements.disclosures.get("de.toem.impulse.disclosures.signalClass"), Elements.disclosures.get("de.toem.impulse.disclosures.processType"), Elements.disclosures.get("de.toem.impulse.disclosures.signalDescriptor"), Elements.disclosures.get("de.toem.impulse.disclosures.domain")}).setOwner(this.scopeController), this.tlk().ld(2, true, true), 67170, null, new String[]{I18n.General_Signal, I18n.General_Description, I18n.General_Location, I18n.Samples_SignalType, I18n.Samples_SignalTag, I18n.Samples_SignalClass, I18n.Samples_ProcessType, I18n.Samples_SignalDescriptor, I18n.Samples_DomainBase});
            this.filterTableController.addDragSupport(4, new ControllerDragListener(this.filterTableController){

                @Override
                public boolean handleStart(int x, int y) {
                    if (RecordViewer.this.plotTreeController != null && !RecordViewer.this.plotTreeController.getView().isBound()) {
                        RecordViewer.this.tlk.openYesNoQuestion(I18n.RecordViewer_ViewMissing, I18n.RecordViewer_NotViewSet, result -> {
                            if (result == 1) {
                                RecordViewer.this.doViewAdd(null, 0, this);
                            }
                        });
                        return false;
                    }
                    return true;
                }
            });
            this.tlk().addSash(main, null, null, 4);
            this.plotTreeController = this.tlk().addControl((Object)main, IPlotTree.class, new ViewerPlotTreeController(this, String.valueOf(this.PART_KEY) + ".samples"){

                @Override
                protected void selectionChanged() {
                    super.selectionChanged();
                    this.access(plotTree -> {
                        plotTree.getSelection();
                        RecordViewer.this.fireSelectionChanged();
                    });
                }

                @Override
                protected void cursorModified(Object data, DomainValue position) {
                    super.cursorModified(data, position);
                    this.access(plotTree -> {
                        DomainValue value;
                        ICursorItem cursor = plotTree.getActiveCursor();
                        DomainValue domainValue = value = cursor != null ? cursor.getPosition() : null;
                        if (value != null) {
                            this.fireActiveCursorChanged(new DomainPosition(value, null, -1));
                        }
                    });
                }

                @Override
                protected Object doShowIn(final Object data, int doIt, Object sender) {
                    if (doIt == 0 || doIt == 1) {
                        if (!(data instanceof String)) {
                            return false;
                        }
                        if (doIt == 0) {
                            this.tlk.showView((String)data);
                            ITlkPart part = ITlkPart.getPart(String.valueOf(data));
                            if (part instanceof AbstractInputViewPart) {
                                ((AbstractInputViewPart)part).ShowIn(RecordViewer.this, this.getSelectedObjects());
                            } else {
                                Actives.runInMain(new IExecutable(){

                                    @Override
                                    public void execute(IProgress p) {
                                        ITlkPart part = ITlkPart.getPart(String.valueOf(data));
                                        if (part instanceof AbstractInputViewPart) {
                                            ((AbstractInputViewPart)part).ShowIn(RecordViewer.this, this.getSelectedObjects());
                                        }
                                    }
                                }, 1000);
                            }
                        }
                        return true;
                    }
                    return null;
                }

                protected void fireActiveCursorChanged(DomainPosition position) {
                    RecordViewer.this.fireActiveCursorChanged(position, (ITlkPart)RecordViewer.this);
                }
            }, (Object)new TlkShareData(2), 0, null);
            this.plotTreeController.addDragSupport(7, null);
            this.plotTreeController.addDropSupport(7, null);
            super.createControls(container);
        }
        catch (Throwable e) {
            SystemLog.log(e);
        }
    }

    @Override
    protected boolean filterUpdate(ElementModifierEvent event) {
        if (this.plotTreeController == null || event == null) {
            return false;
        }
        IElement configurationElement = this.plotTreeController.getView();
        return !event.getElement().isBound() || !event.getElement().isTribe(configurationElement) && !event.getElement().isTribe(this.baseElement) && event.getElement() != this.viewPreferences && !event.getElement().isTribe(this.chartPreferences) && !event.getElement().isTribe(this.serializerPreferences);
    }

    @Override
    protected void updateControls(ElementModifierEvent event) {
        super.updateControls(event);
    }

    @Override
    protected void updateLocalControls(ElementModifierEvent event) {
        super.updateLocalControls(event);
        if (this.topLayoutController == null || this.leftLayoutController == null) {
            return;
        }
        if (this.editorElement.isBound() && this.editorElement.hasCell()) {
            this.topLayoutController.update(this.editorElement.getCell(), false);
            this.leftLayoutController.update(this.editorElement.getCell(), false);
        }
        if (this.scopeController != null && !this.scopeController.hasSelection()) {
            this.scopeController.selectRoot();
        }
        if (this.plotTreeController != null) {
            ICell cell;
            IElement element = IElement.NONE;
            if (event != null && event.getElement() == this.plotTreeController.getView()) {
                element = this.plotTreeController.getView();
                if (element.isBound() && this.viewPreferences.isBound()) {
                    this.view = element.getLink(this.viewPreferences);
                }
                if (this.editorElement.isBound()) {
                    this.editorElement.setHint(String.valueOf(this.PART_KEY) + ".configuration", this.view != null ? this.view.toString() : "");
                }
            } else {
                cell = this.viewPreferences.isBound() ? this.viewPreferences.getCellByLink(this.view, ViewConfiguration.class) : null;
                element = cell != null ? cell.getElement() : IElement.NONE;
            }
            this.plotTreeController.setView(element);
            if (element.isBound()) {
                cell = element.getCellByLink(this.top);
                this.plotTreeController.setTopElement(cell != null ? cell.getElement() : IElement.NONE);
            } else {
                this.plotTreeController.setTopElement(IElement.NONE);
            }
        }
    }

    @Override
    public void setHint(String key, String value) {
    }

    @Override
    public String getHint(String key) {
        return null;
    }

    public float viewFits(IElement view) {
        int found = 0;
        int total = 0;
        if (!view.hasCell(ViewConfiguration.class)) {
            return 0.0f;
        }
        ViewConfiguration cell = (ViewConfiguration)view.getCell();
        if (!cell.enabled) {
            return 0.0f;
        }
        List<ICell> samples = cell.getTribe(false, PlotConfiguration.class);
        samples = samples.subList(0, Math.min(samples.size(), 100));
        if (this.baseElement.isBound() && this.baseElement.hasCell()) {
            ICell base = this.baseElement.getCell();
            for (ICell c : samples) {
                Link link = ((PlotConfiguration)c).samples;
                if (link == null || "View".equals(link.getResourceClass())) continue;
                if (link.resolveCell(base) != null) {
                    ++found;
                }
                ++total;
            }
        }
        float fit = total > 0 ? 1.0f * (float)found / (float)total : 0.0f;
        return fit;
    }

    public void setView(IElement element) {
        if (element.isBound() && this.viewPreferences.isBound()) {
            this.setView(element.getLink(this.viewPreferences));
        }
    }

    @Override
    public IElement getView() {
        if (this.viewPreferences.isBound()) {
            ICell cell = this.viewPreferences.getCellByLink(this.view, ViewConfiguration.class);
            return cell != null ? cell.getElement() : IElement.NONE;
        }
        return IElement.NONE;
    }

    @Override
    public void setView(Link view) {
        this.view = view;
        this.top = null;
        if (this.editorElement.isBound()) {
            this.editorElement.setHint(String.valueOf(this.PART_KEY) + ".configuration", view != null ? view.toString() : "");
        }
        this.updateControls(null);
        ImpulseNotifications.fireViewerViewSet(this, view, this.getView());
    }

    public void setTop(Link top) {
        this.top = top;
        if (this.editorElement.isBound()) {
            this.editorElement.setHint(String.valueOf(this.PART_KEY) + ".top", top != null ? top.toString() : "");
        }
        this.updateControls(null);
    }

    public List<Object> getSelectedObjects() {
        if (this.plotTreeController != null) {
            return this.plotTreeController.getSelectedObjects();
        }
        return Collections.EMPTY_LIST;
    }

    @Override
    public List<ISimpleSamplesProvider> getSelectedSamplesProvider() {
        ArrayList<ISimpleSamplesProvider> list = new ArrayList<ISimpleSamplesProvider>();
        for (Object o : this.getSelectedObjects()) {
            if (!(o instanceof ISimpleSamplesProvider)) continue;
            list.add((ISimpleSamplesProvider)o);
        }
        return list;
    }

    @Override
    public void elementLoaded(IElement element, boolean coverOnly) {
        super.elementLoaded(element, coverOnly);
        if (this.isValid() && this.getElement().isBound() && this.getElement().hasCell()) {
            this.onInputReady();
        }
    }

    @Override
    public Object command(String id, Object data, int doIt, Object sender) {
        Object ctrlResult;
        Object object = ctrlResult = this.plotTreeController != null ? this.plotTreeController.command(id, data, doIt, sender) : null;
        if (ctrlResult != null) {
            return ctrlResult;
        }
        if (id.equals("de.toem.impulse.commands.about")) {
            if (doIt != 5) {
                return this.doAbout(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.views.open")) {
            if (doIt != 5) {
                return this.doViewsOpen(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.view.add")) {
            if (doIt != 5) {
                return this.doViewAdd(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.view.clone")) {
            if (doIt != 5) {
                return this.doViewClone(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.view.edit")) {
            if (doIt != 5) {
                return this.doViewEdit(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.view.delete")) {
            if (doIt != 5) {
                return this.doViewDelete(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.view.save")) {
            if (doIt != 5) {
                return this.doViewSave(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.view.reset")) {
            if (doIt != 5) {
                return this.doViewReset(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.geometry.details")) {
            if (doIt != 5) {
                return this.doGeometryCursorDetails(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.geometry.signals")) {
            if (doIt != 5) {
                return this.doGeometrySignals(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.samples.reload")) {
            if (doIt != 5) {
                return this.doSamplesReload(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.samples.reader.edit")) {
            if (doIt != 5) {
                return this.doSamplesEditReader(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.samples.configuration.edit")) {
            if (doIt != 5) {
                return this.doSamplesEditConfiguration(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.navigate.goInto")) {
            if (doIt != 5) {
                return this.doGoInto(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.navigate.up")) {
            if (doIt != 5) {
                return this.doGoUp(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.ports.edit")) {
            if (doIt != 5) {
                return this.doPortEdit(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.ports.connect")) {
            if (doIt != 5) {
                return this.doPortConnect(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.ports.streaming")) {
            if (doIt != 5) {
                return this.doPortStreaming(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.ports.iterating")) {
            if (doIt != 5) {
                return this.doPortIterating(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.ports.updating")) {
            if (doIt != 5) {
                return this.doPortUpdating(data, doIt, sender);
            }
        } else if (id.equals("org.eclipse.ui.file.print")) {
            if (doIt != 5) {
                return this.doPrint(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.support.preferences")) {
            if (doIt != 5) {
                return this.doShowPreferences(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.support.report")) {
            if (doIt != 5) {
                return this.doSupportReport(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.support.help")) {
            if (doIt != 5) {
                return this.doSupportHelp(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.refresh")) {
            if (doIt != 5) {
                return this.doRefresh(data, doIt, sender);
            }
        } else {
            return super.command(id, data, doIt, sender);
        }
        if (doIt == 5) {
            return true;
        }
        return null;
    }

    private Object doViewSave(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (this.viewPreferences == null || !this.viewPreferences.isBound() || !this.viewPreferences.isDirty()) {
                return false;
            }
            if (doIt == 0) {
                this.viewPreferences.save();
            }
            return true;
        }
        return null;
    }

    private Object doViewReset(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (this.viewPreferences == null || !this.viewPreferences.isBound() || !this.viewPreferences.isDirty()) {
                return false;
            }
            if (doIt == 0) {
                this.viewPreferences.load();
            }
            return true;
        }
        return null;
    }

    protected Object doAbout(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                new AboutDialog(this, -1).open();
            }
            return true;
        }
        return null;
    }

    protected Object doViewsOpen(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                if (this.getCurrentChildEditor() != null && "de.toem.impulse.dialog.viewSelect".equals(this.getCurrentChildEditor().getId())) {
                    this.getCurrentChildEditor().close(false, true);
                } else {
                    this.tlk().openDialog("de.toem.impulse.dialog.viewSelect", this, null);
                }
            }
            return true;
        }
        return null;
    }

    protected Object doViewAdd(Object data, int doIt, Object sender) {
        return this.plotTreeController != null ? this.plotTreeController.doAddNewElement(new String[]{"de.toem.impulse.instancer.configuration.record", "configuration.record"}, doIt, sender, this.viewPreferences, -1) : null;
    }

    private Object doViewDelete(Object data, int doIt, Object sender) {
        return this.plotTreeController != null ? this.plotTreeController.doEditDeleteElements(data, doIt, sender, Elements.getElements(this.plotTreeController.getView())) : null;
    }

    private Object doViewEdit(Object data, int doIt, Object sender) {
        return this.plotTreeController != null ? this.plotTreeController.doEditElements(data, doIt, sender, Elements.getElements(this.plotTreeController.getView())) : null;
    }

    private Object doViewClone(Object data, int doIt, Object sender) {
        return this.plotTreeController != null ? this.plotTreeController.doCloneElement(data, doIt, sender, Elements.getElements(this.plotTreeController.getView())) : null;
    }

    private Object doGeometryCursorDetails(Object data, int doIt, Object sender) {
        if (this.plotTreeController == null) {
            return null;
        }
        if (doIt == 0 || doIt == 1) {
            if (doIt != 0) {
                return this.plotTreeController.canShowCursorDetails();
            }
            this.plotTreeController.setShowCursorDetails(!this.plotTreeController.getShowCursorDetails());
            return true;
        }
        if (doIt == 3) {
            return this.plotTreeController.getShowCursorDetails();
        }
        return null;
    }

    private Object doGeometrySignals(Object data, int doIt, Object sender) {
        if (this.topLayoutController == null) {
            return null;
        }
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                if (data instanceof Boolean) {
                    this.topLayoutController.maximize(Boolean.TRUE == data ? 0 : 2);
                } else if (this.topLayoutController.getMaximize() == 0) {
                    this.topLayoutController.maximize(2);
                } else {
                    this.topLayoutController.maximize(0);
                }
            }
            return true;
        }
        if (doIt == 3) {
            if (this.topLayoutController.getMaximize() == 0) {
                return true;
            }
            return false;
        }
        return null;
    }

    private Object doGoInto(Object data, int doIt, Object sender) {
        if (this.plotTreeController == null) {
            return null;
        }
        if (doIt == 0 || doIt == 1) {
            List<ICell> selection = this.plotTreeController.getSelectedCells();
            if (selection.size() != 1) {
                return false;
            }
            if (!selection.get(0).hasChildren()) {
                return false;
            }
            if (!(selection.get(0) instanceof FolderConfiguration)) {
                return false;
            }
            if (this.plotTreeController.getTopElement() == selection.get(0).getElement()) {
                return false;
            }
            if (!this.plotTreeController.getView().isBound()) {
                return false;
            }
            if (doIt == 0) {
                this.setTop(selection.get(0).getLink(this.plotTreeController.getView()));
            }
            return true;
        }
        return null;
    }

    private Object doGoUp(Object data, int doIt, Object sender) {
        if (this.plotTreeController == null) {
            return null;
        }
        if (doIt == 0 || doIt == 1 || doIt == 2) {
            if (!this.plotTreeController.getView().isBound()) {
                return false;
            }
            if (!this.plotTreeController.getTopElement().isBound() && !this.plotTreeController.getTopElement().hasCell()) {
                return false;
            }
            if (this.plotTreeController.getView() == this.plotTreeController.getTopElement()) {
                return false;
            }
            if (!this.plotTreeController.getTopElement().getContainer().isBound()) {
                return false;
            }
            if (doIt == 0) {
                if (this.plotTreeController.getTopElement().getContainer() == this.plotTreeController.getView()) {
                    this.setTop(null);
                } else {
                    this.setTop(this.plotTreeController.getTopElement().getContainer().getLink(this.plotTreeController.getView()));
                }
            }
            return true;
        }
        return null;
    }

    private Object doPortEdit(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (!this.inputElement.isBound() || !this.inputElement.hasCell(IRecordPort.class)) {
                return false;
            }
            return true;
        }
        if (doIt == 4) {
            if (!this.inputElement.isBound() || !this.inputElement.hasCell(IRecordPort.class)) {
                return null;
            }
            ICell cell = this.inputElement.getCell();
            CellDescriptor descriptor = (CellDescriptor)Elements.cells.get(cell.getType());
            return descriptor != null ? descriptor.getIconId() : null;
        }
        if (doIt == 2) {
            if (this.inputElement.isBound() && this.inputElement.hasCell(IRecordPort.class)) {
                return true;
            }
            return false;
        }
        return null;
    }

    private Object doPortConnect(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (!this.inputElement.isBound() || !this.inputElement.hasCell(IRecordPort.class)) {
                return false;
            }
            if ((((IRecordPort)this.inputElement.getCell()).getNature() & 0x20) == 0) {
                return false;
            }
            if (doIt == 0) {
                if (this.streams != null && this.streams.isRunning()) {
                    this.streams.cancel();
                } else {
                    this.reOpen();
                    if (this.streams != null) {
                        this.streams.setIterating(this.iterating);
                        this.streams.start();
                    }
                }
            }
            return true;
        }
        if (doIt == 3) {
            if (this.streams != null && this.streams.isRunning()) {
                return true;
            }
            return false;
        }
        if (doIt == 2) {
            if (this.inputElement.isBound() && this.inputElement.hasCell(IRecordPort.class)) {
                return true;
            }
            return false;
        }
        return null;
    }

    private Object doPortStreaming(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (this.inputElement.isBound() && this.inputElement.hasCell(IRecordPort.class)) {
                if (doIt == 0) {
                    if (this.streams != null && this.streams.isRunning()) {
                        if (!this.streams.isStreaming()) {
                            this.streams.enterStreaming();
                            this.streams.enterUpdating(true);
                        } else {
                            this.streams.cancel();
                        }
                    } else {
                        this.reOpen();
                        if (this.streams != null) {
                            this.streams.setIterating(this.iterating);
                            this.streams.start();
                            this.streams.enterStreaming();
                            this.streams.enterUpdating(true);
                        }
                    }
                }
                return true;
            }
            return false;
        }
        if (doIt == 3) {
            boolean state = this.streams != null && this.streams.isRunning() && this.streams.isStreaming();
            return state;
        }
        if (doIt == 2) {
            if (this.inputElement.isBound() && this.inputElement.hasCell(IRecordPort.class)) {
                return true;
            }
            return false;
        }
        return null;
    }

    private Object doPortIterating(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (!this.inputElement.isBound() || !this.inputElement.hasCell(IRecordPort.class)) {
                return false;
            }
            if ((((IRecordPort)this.inputElement.getCell()).getNature() & 0x18) != 24) {
                return false;
            }
            if (doIt == 0) {
                boolean bl = this.iterating = !this.iterating;
                if (this.streams != null) {
                    this.streams.setIterating(this.iterating);
                }
            }
            return true;
        }
        if (doIt == 3) {
            return this.iterating;
        }
        if (doIt == 2) {
            if (this.inputElement.isBound() && this.inputElement.hasCell(IRecordPort.class)) {
                return true;
            }
            return false;
        }
        return null;
    }

    private Object doPortUpdating(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (!this.inputElement.isBound() || !this.inputElement.hasCell(IRecordPort.class)) {
                return false;
            }
            if ((((IRecordPort)this.inputElement.getCell()).getNature() & 1) == 0) {
                return false;
            }
            if (this.streams == null || !this.streams.isStreaming()) {
                return false;
            }
            if (doIt == 0) {
                this.streams.enterUpdating(!this.streams.isUpdating());
            }
            return true;
        }
        if (doIt == 3) {
            if (this.streams != null && !this.streams.isFinished() && this.streams.isUpdating()) {
                return true;
            }
            return false;
        }
        if (doIt == 2) {
            if (this.inputElement.isBound() && this.inputElement.hasCell(IRecordPort.class)) {
                return true;
            }
            return false;
        }
        return null;
    }

    private Object doSamplesReload(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (!this.inputElement.isBound() || this.inputElement.hasCell(IRecordPort.class) || this.inputElement.isConstant()) {
                return false;
            }
            if (doIt == 0) {
                if (this.baseElement.isBound() && this.baseElement.hasResource()) {
                    if (data instanceof String && ((String)data).startsWith("CONFIGURATION")) {
                        this.baseElement.setHint("CONFIGURATION", ((String)data).substring("CONFIGURATION".length()));
                    }
                    if (data instanceof String && ((String)data).startsWith("SERIALIZER")) {
                        this.baseElement.setHint("SERIALIZER", ((String)data).substring("SERIALIZER".length()));
                    }
                    this.baseElement.load();
                }
                if ("de.toem.impulse.diffViewer".equals(this.PART_KEY) && this.baseElement.isBound()) {
                    this.baseElement.setHint("compare.configured", null);
                }
                this.reOpen();
            }
            return true;
        }
        if (doIt == 6) {
            if (this.baseElement.isBound() && this.baseElement.hasResource() && this.baseElement.hasCell()) {
                ICover cover = this.baseElement.getCover();
                String serializer = cover.getSerializer();
                String configuration = cover.getConfiguration();
                BaseGroupedInformations options = new BaseGroupedInformations();
                InformationGroup serializers = new InformationGroup("otherserializers", I18n.RecordViewer_SelectSerializer, "");
                options.add(new GroupedInformation(serializers, "SERIALIZER", I18n.General_Default, null, "impulse.serializer"));
                for (Serializer cell : ImpulsePreferences.getAllSerializer()) {
                    if (!cell.enabled) continue;
                    options.add(new GroupedInformation(serializers, "SERIALIZER" + cell.id, String.valueOf(cell.getName()) + (cell.id.equals(serializer) ? " [" + I18n.General_Active + "]" : ""), null, "impulse.serializer"));
                }
                InformationGroup configurations = new InformationGroup("otherReader", I18n.RecordViewer_SelectConfiguration, "");
                options.add(new GroupedInformation(configurations, "CONFIGURATION", I18n.General_Default, null, "impulse.serializer.configuration.default"));
                for (ReaderConfiguration cell : ImpulsePreferences.getSerializerAllConfigurations(serializer)) {
                    if (!cell.enabled) continue;
                    options.add(new GroupedInformation(configurations, "CONFIGURATION" + cell.getName(), String.valueOf(cell.getName()) + (Utils.equals(configuration, cell.getName()) ? " [" + I18n.General_Active + "]" : ""), null, "impulse.serializer.configuration.default"));
                }
                return options;
            }
        } else if (doIt == 2) {
            if (this.inputElement.isBound() && !this.inputElement.hasCell(IRecordPort.class)) {
                return true;
            }
            return false;
        }
        return null;
    }

    private Object doSamplesEditReader(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            String dialog;
            if (!this.inputElement.isBound() || this.inputElement.hasCell(IRecordPort.class) || this.inputElement.isConstant()) {
                return false;
            }
            ICover cover = this.baseElement != null ? this.baseElement.getCover() : null;
            String id = cover != null ? cover.getSerializer() : null;
            Serializer cell = ImpulsePreferences.getSerializer(id);
            if (cell == null) {
                return false;
            }
            if (doIt == 0 && (dialog = Dialogs.getForElement(Elements.getElement(cell))) != null) {
                this.tlk().openDialog(dialog, this, Elements.getElements(cell), false, result -> {
                    if (result == 4) {
                        this.baseElement.load();
                        this.reOpen();
                    }
                });
            }
            return true;
        }
        return null;
    }

    private Object doSamplesEditConfiguration(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            String dialog;
            String configuration;
            if (!this.inputElement.isBound() || this.inputElement.hasCell(IRecordPort.class) || this.inputElement.isConstant()) {
                return false;
            }
            ICover cover = this.baseElement != null ? this.baseElement.getCover() : null;
            String id = cover != null ? cover.getSerializer() : null;
            ReaderConfiguration cell = ImpulsePreferences.getSerializerConfiguration(id, configuration = cover != null ? cover.getConfiguration() : null);
            if (cell == null) {
                return false;
            }
            if (doIt == 0 && (dialog = Dialogs.getForElement(Elements.getElement(cell))) != null) {
                this.tlk().openDialog(dialog, this, Elements.getElements(cell), false, result -> {
                    if (result == 4) {
                        this.baseElement.load();
                        this.reOpen();
                    }
                });
            }
            return true;
        }
        return null;
    }

    private Object doPrint(Object data, int doIt, Object sender) {
        if (this.plotTreeController != null) {
            return this.plotTreeController.doPrint(data, doIt, sender);
        }
        return null;
    }

    private Object doSupportHelp(Object data, int doIt, Object sender) {
        return null;
    }

    private Object doSupportReport(Object data, int doIt, Object sender) {
        return null;
    }

    private Object doShowPreferences(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            return true;
        }
        return null;
    }

    public Object doRefresh(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                ImpulseNotifications.fireViewerAboutToRefresh(this);
                if (("signals".equals(data) || data == null) && this.getSamplesCache() != null) {
                    this.getSamplesCache().clear();
                }
                if (this.plotTreeController != null) {
                    this.plotTreeController.doRefresh(data, doIt, sender);
                }
                this.updateControls(null);
            }
            return true;
        }
        return null;
    }

    @Override
    public IElement getRecord() {
        return this.getElement();
    }

    @Override
    public ISamplesCache getSamplesCache() {
        if (this.plotTreeController != null) {
            return this.plotTreeController.getSamplesCache();
        }
        return null;
    }

    @Override
    public void gotoTarget(Link link) {
        if (link == null || this.plotTreeController == null) {
            return;
        }
        this.plotTreeController.gotoTarget(link, 1);
    }

    @Override
    public void highlightAttachment(ISimpleSamplesProvider samples, IAttachment attachment) {
        if (this.plotTreeController == null) {
            return;
        }
        this.plotTreeController.highlightTarget(samples, attachment);
    }

    @Override
    public void requestTarget(Link link, IRecordViewerChildListener listener) {
        if (link == null || this.plotTreeController == null) {
            return;
        }
        this.plotTreeController.requestTarget(link, listener);
    }

    @Override
    public void moveActiveCursor(DomainValue position, ITlkPart sender) {
        if (this.plotTreeController != null) {
            if (this.plotTreeController.isUpdateMode()) {
                this.plotTreeController.leaveUpdateMode();
            }
            this.plotTreeController.moveActiveCursor(position, true, 0, false);
        }
        this.fireActiveCursorChanged(position, sender);
    }

    @Override
    public void moveActiveCursor(DomainPosition position, ITlkPart sender) {
        if (this.plotTreeController != null) {
            if (this.plotTreeController.isUpdateMode()) {
                this.plotTreeController.leaveUpdateMode();
            }
            this.plotTreeController.moveActiveCursor(position.domainValue, true, 0, false);
        }
        this.fireActiveCursorChanged(position, sender);
    }

    @Override
    public DomainValue getActiveCursorPosition() {
        if (this.plotTreeController != null) {
            ICursorItem cursor = this.plotTreeController.getActiveCursor();
            return cursor != null ? cursor.getPosition() : null;
        }
        return null;
    }

    public void setSamplesMouseListener(IPlotTreeMouseListener listener) {
        if (this.plotTreeController != null) {
            this.plotTreeController.setSamplesMouseListener(listener);
        }
    }

    @Override
    public void cancelPort() {
        if (this.inputElement.isBound() && this.inputElement.hasCell(IRecordPort.class) && this.streams != null && this.streams.isRunning() && this.streams.isStreaming()) {
            this.streams.cancel();
        }
    }

    @Override
    public void connectPort(boolean forceRestart) {
        if (this.inputElement.isBound() && this.inputElement.hasCell(IRecordPort.class) && (this.streams == null || !this.streams.isRunning() || forceRestart)) {
            this.reOpen();
            if (this.streams != null) {
                this.streams.setIterating(this.iterating);
                this.streams.start();
            }
        }
    }

    @Override
    public void streamPort(boolean forceRestart) {
        if (this.inputElement.isBound() && this.inputElement.hasCell(IRecordPort.class)) {
            if (this.streams != null && this.streams.isRunning() && !forceRestart) {
                if (!this.streams.isStreaming()) {
                    this.streams.enterStreaming();
                }
            } else {
                this.reOpen();
                if (this.streams != null) {
                    this.streams.setIterating(this.iterating);
                    this.streams.start();
                    this.streams.enterStreaming();
                    this.streams.enterUpdating(true);
                }
            }
        }
    }

    @Override
    public synchronized void addChildListener(IRecordViewerChildListener listener) {
        if (!this.childListeners.contains(listener)) {
            this.childListeners.add(listener);
        }
    }

    @Override
    public synchronized void removeChildListener(IRecordViewerChildListener listener) {
        this.childListeners.remove(listener);
    }

    protected void fireActiveCursorChanged(DomainValue position, ITlkPart sender) {
        this.fireActiveCursorChanged(new DomainPosition(position, null, -1), sender);
    }

    protected void fireActiveCursorChanged(DomainPosition position, ITlkPart sender) {
        for (IRecordViewerChildListener l : this.childListeners) {
            l.positionChanged(position, sender);
        }
    }

    @Override
    protected void fireSelectionChanged() {
        for (IRecordViewerChildListener l : this.childListeners) {
            l.signalsChanged(this.getSelectedObjects(), this);
        }
        super.fireSelectionChanged();
    }
}

