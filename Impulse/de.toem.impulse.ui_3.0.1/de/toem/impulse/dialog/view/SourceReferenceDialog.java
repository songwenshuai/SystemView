/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.view;

import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.cells.view.FolderConfiguration;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.cells.view.SourceReference;
import de.toem.impulse.cells.view.ViewConfiguration;
import de.toem.impulse.ui.IRecordViewer;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.ElementModifierEvent;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.element.serializer.Message;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.base.CellTreeController;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.TabFolderController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.TLK;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.controls.ITlkTabFolder;
import java.util.LinkedList;
import java.util.List;

public class SourceReferenceDialog
extends ControlProviderElementDialog {
    public SourceReferenceDialog(ITlkPartContainer parent, int style) {
        super(parent, SourceReferenceDialog.getControls(), style);
    }

    public SourceReferenceDialog() {
    }

    private static IControlProvider getControls() {
        try {
            return SourceReferenceDialog.getControls(SourceReference.class.getField("reference"), SourceReference.class.getField("description"), SourceReference.class.getField("reference"));
        }
        catch (NoSuchFieldException | SecurityException e) {
            e.printStackTrace();
            return null;
        }
    }

    public static IControlProvider getControls(final Object linkSource, final Object commentSource, final Object tableSource) {
        AbstractControlProvider provider = new AbstractControlProvider(){
            private TabFolderController tabController;
            private CellTreeController signalController;
            private IController signalTableController;
            private List<ICell> signalFilterCells;
            private CellTreeController plotController;
            private IController plotTableController;
            private List<ICell> plotFilterCells;
            private IController imageController;

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.reference_dialog";
            }

            private IElement getSignalsBase() {
                ITlkPart editor = this.editor();
                while (editor != null) {
                    if (editor instanceof IRecordViewer) {
                        IElement base = editor.getElement();
                        if (!base.isBound() || !base.hasCell()) continue;
                        return base;
                    }
                    editor = editor.getContainerPart();
                }
                return IElement.NONE;
            }

            @Override
            public void setFocus(boolean force) {
                super.setFocus(force);
            }

            @Override
            public boolean fillThis() {
                try {
                    if (linkSource != null) {
                        this.tlk().addText(this.container(), new TextController(this.editor(), linkSource){

                            @Override
                            protected Object convert(Object value) {
                                if (value instanceof Link) {
                                    return ((Link)value).toString();
                                }
                                return value;
                            }

                            @Override
                            protected Object revert(Object value) {
                                if (value instanceof String) {
                                    Link link = Link.parse((String)value);
                                    return link;
                                }
                                return value;
                            }
                        }, this.tlk().ld(this.cols() - 1, 524288, 1), 0x100001, I18n.General_Path_);
                    }
                    if (commentSource != null) {
                        this.tlk().addText(this.container(), new TextController(this.editor(), commentSource), this.tlk().ld(this.cols() - 1, 524288, 1), 0x100001, I18n.General_Description_);
                    }
                    this.tabController = new TabFolderController(this.editor(), "referenceDialog.tab");
                    ITlkTabFolder tab = this.tlk().addTabFolder(this.container(), this.tabController, this.cols(), 0, null);
                    ITlkComposite signals = this.tlk().addComposite(tab, null, 1, null, 0, I18n.SourceReferenceDialog_Signals, null);
                    this.signalController = (CellTreeController)this.tlk().addTree(signals, new CellTreeController(this.editor(), tableSource){

                        @Override
                        public ICell getRoot() {
                            IElement base = this.getSignalsBase();
                            if (base.isBound() && base.hasCell()) {
                                return base.getCell();
                            }
                            return null;
                        }

                        @Override
                        public void selectionChanged() {
                            signalFilterCells = this.getSelectedCells();
                            signalTableController.setUpdateRequired();
                            signalTableController.update(this.getRoot(), true);
                        }

                        @Override
                        protected void doUpdateExternal() {
                            signalTableController.update(this.getRoot(), true);
                        }

                        @Override
                        protected boolean allowModification(IController.Commands command, Object data, Object context) {
                            return false;
                        }

                        @Override
                        protected boolean filterPasteElements(IElement element) {
                            return !element.isBound() && element.hasCell(ViewConfiguration.class);
                        }

                        @Override
                        public void doUpdateControl() {
                            ICell cell;
                            super.doUpdateControl();
                            Link link = this.getValueAsLink();
                            ICell iCell = cell = link != null ? PlotConfiguration.getSourceCell(link, this.getRoot()) : null;
                            if (cell == null) {
                                ICell iCell2 = cell = link != null ? PlotConfiguration.getSourceContainer(link, this.getRoot()) : null;
                            }
                            if (cell instanceof AbstractSignal) {
                                cell = cell.getParent();
                            }
                            if (cell != null) {
                                this.select(cell, false);
                                this.selectionChanged();
                            } else {
                                this.clearSelection();
                            }
                        }
                    }.initCells(null, new Class[]{Message.class, Scope.class, Record.class, ViewConfiguration.class}).setKeepFirstItemExpanded(true).setShowRoot(true), this.tlk().ld(1, 4, 1, 4, 200), 0x100000, null, null);
                    final CheckController childrenCheck = this.tlk().addButton(signals, new CheckController(this.editor(), TLK.HINT(2, "referenceDialog.check.children", "false")){

                        @Override
                        public void changed(boolean finalized) {
                            signalTableController.update(signalTableController.getCell(), true);
                            super.changed(finalized);
                        }
                    }, this.tlk().ld(1, 131072, -1), 2048, I18n.General_AllChildren, null);
                    this.signalTableController = this.tlk().addTable(signals, new CellTableController(this.editor(), null){

                        @Override
                        public boolean needsUpdate() {
                            return super.needsUpdate();
                        }

                        @Override
                        public boolean isAffected(ElementModifierEvent event) {
                            return super.isAffected(event);
                        }

                        @Override
                        protected boolean allowModification(IController.Commands command, Object data, Object context) {
                            return false;
                        }

                        @Override
                        protected List<ICell> getRawTableCells() {
                            if (signalFilterCells == null || signalFilterCells.isEmpty()) {
                                return ICell.EMPTY_LIST;
                            }
                            LinkedList<ICell> cells = new LinkedList<ICell>();
                            for (ICell cell : signalFilterCells) {
                                if (childrenCheck.isChecked()) {
                                    for (ICell iCell : cell.getTribe(false, AbstractSignal.class)) {
                                        if (signalFilterCells.size() != 1 && cells.contains(iCell)) continue;
                                        cells.add(iCell);
                                    }
                                    continue;
                                }
                                for (ICell iCell : cell.getChildren(AbstractSignal.class)) {
                                    if (signalFilterCells.size() != 1 && cells.contains(iCell)) continue;
                                    cells.add(iCell);
                                }
                            }
                            return cells;
                        }

                        @Override
                        public void doUpdateControl() {
                            super.doUpdateControl();
                            ICell cell = PlotConfiguration.getSourceCell(signalController.getValueAsLink(), this.getCell());
                            if (cell instanceof AbstractSignal) {
                                this.select(cell, false);
                            } else {
                                this.clearSelection();
                            }
                        }

                        @Override
                        protected void selectionChanged() {
                            super.selectionChanged();
                            if (!this.updating && this.getSelectedCell() != null) {
                                signalController.setValue(PlotConfiguration.getSourceLink(this.getSelectedCell()), true, false, false, false);
                            }
                        }
                    }.initColumnDataSources(true, new Object[]{AbstractSignal.class.getField("description"), Cell.class.getMethod("getContainerPath", new Class[0]), Elements.disclosures.get("de.toem.impulse.disclosures.signalType"), Elements.disclosures.get("de.toem.impulse.disclosures.signalTag"), Elements.disclosures.get("de.toem.impulse.disclosures.signalClass"), Elements.disclosures.get("de.toem.impulse.disclosures.processType"), Elements.disclosures.get("de.toem.impulse.disclosures.signalDescriptor"), Elements.disclosures.get("de.toem.impulse.disclosures.domain")}).initCells(null, AbstractSignal.class).setOwner(this.signalController), this.tlk().ld(1, 4, 1, 4, 200), 67138, null, new String[]{I18n.General_Signal, I18n.General_Description, I18n.General_Location, I18n.Samples_SignalType, I18n.Samples_SignalTag, I18n.Samples_SignalClass, I18n.Samples_ProcessType, I18n.Samples_SignalDescriptor, I18n.Samples_DomainBase});
                    ITlkComposite plots = this.tlk().addComposite(tab, null, 1, null, 0, I18n.SourceReferenceDialog_Plots, null);
                    this.plotController = (CellTreeController)this.tlk().addTree(plots, new CellTreeController(this.editor(), tableSource){

                        @Override
                        public ICell getRoot() {
                            if (super.getCell() != null) {
                                ICell cell = super.getCell().getParent(ViewConfiguration.class);
                                return cell;
                            }
                            return null;
                        }

                        @Override
                        public void selectionChanged() {
                            plotFilterCells = this.getSelectedCells();
                            plotTableController.setUpdateRequired();
                            plotTableController.update(this.getRoot(), true);
                        }

                        @Override
                        protected void doUpdateExternal() {
                            plotTableController.update(this.getRoot(), true);
                        }

                        @Override
                        protected boolean allowModification(IController.Commands command, Object data, Object context) {
                            return false;
                        }

                        @Override
                        protected boolean filterPasteElements(IElement element) {
                            return !element.isBound() && element.hasCell(ViewConfiguration.class);
                        }

                        @Override
                        public void doUpdateControl() {
                            ICell cell;
                            super.doUpdateControl();
                            Link link = this.getValueAsLink();
                            ICell iCell = cell = link != null ? PlotConfiguration.getSourceCell(link, this.getRoot()) : null;
                            if (cell == null) {
                                ICell iCell2 = cell = link != null ? PlotConfiguration.getSourceContainer(link, this.getRoot()) : null;
                            }
                            if (cell != null) {
                                this.select(cell, false);
                                this.selectionChanged();
                            } else {
                                this.clearSelection();
                            }
                        }
                    }.initCells(null, new Class[]{FolderConfiguration.class, ViewConfiguration.class}).setKeepFirstItemExpanded(true).setShowRoot(true), this.tlk().ld(1, 4, 1, 4, 200), 0x100000, null, null);
                    childrenCheck = this.tlk().addButton(plots, new CheckController(this.editor(), TLK.HINT(2, "referenceDialog.check.children", "false")){

                        @Override
                        public void changed(boolean finalized) {
                            plotTableController.update(plotTableController.getCell(), true);
                            super.changed(finalized);
                        }
                    }, this.tlk().ld(1, 131072, -1), 2048, I18n.General_AllChildren, null);
                    this.plotTableController = this.tlk().addTable(plots, new CellTableController(this.editor(), null){

                        @Override
                        public boolean needsUpdate() {
                            return true;
                        }

                        @Override
                        public boolean isAffected(ElementModifierEvent event) {
                            this.needsUpdate();
                            return true;
                        }

                        @Override
                        protected boolean allowModification(IController.Commands command, Object data, Object context) {
                            return false;
                        }

                        @Override
                        protected List<ICell> getRawTableCells() {
                            if (plotFilterCells == null || plotFilterCells.isEmpty()) {
                                return ICell.EMPTY_LIST;
                            }
                            LinkedList<ICell> cells = new LinkedList<ICell>();
                            for (ICell cell : plotFilterCells) {
                                if (childrenCheck.isChecked()) {
                                    for (ICell iCell : cell.getTribe(false, PlotConfiguration.class)) {
                                        if (plotFilterCells.size() != 1 && cells.contains(iCell)) continue;
                                        cells.add(iCell);
                                    }
                                    continue;
                                }
                                for (ICell iCell : cell.getChildren(PlotConfiguration.class)) {
                                    if (plotFilterCells.size() != 1 && cells.contains(iCell)) continue;
                                    cells.add(iCell);
                                }
                            }
                            return cells;
                        }

                        @Override
                        public void doUpdateControl() {
                            super.doUpdateControl();
                            ICell cell = PlotConfiguration.getSourceCell(plotController.getValueAsLink(), this.getCell());
                            if (cell instanceof PlotConfiguration) {
                                this.select(cell, false);
                            } else {
                                this.clearSelection();
                            }
                        }

                        @Override
                        protected void selectionChanged() {
                            super.selectionChanged();
                            if (!this.updating && this.getSelectedCell() != null) {
                                plotController.setValue(PlotConfiguration.getSourceLink(this.getSelectedCell()), true, false, false, false);
                            }
                        }
                    }.initColumnDataSources(true, new Object[]{AbstractSignal.class.getField("description"), Cell.class.getMethod("getPath", new Class[0])}).initCells(null, AbstractSignal.class).setOwner(this.plotController), this.tlk().ld(1, 4, 1, 4, 200), 67138, null, new String[]{I18n.General_Signal, I18n.General_Description, I18n.General_Location});
                }
                catch (Throwable throwable) {}
                return true;
            }
        };
        return provider;
    }
}

