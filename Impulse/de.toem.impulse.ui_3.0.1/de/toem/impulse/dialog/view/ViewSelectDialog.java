/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.view;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.view.ViewConfiguration;
import de.toem.impulse.parts.viewer.RecordViewer;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.commands.CommandButtonController;
import de.toem.toolkits.ui.part.dialog.ControlProviderDialog;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class ViewSelectDialog
extends ControlProviderDialog {
    public ViewSelectDialog(ITlkPartContainer parent, int style) {
        super(parent, ViewSelectDialog.getControls(), style == -1 ? 65536 : style);
    }

    public ViewSelectDialog() {
    }

    @Override
    protected void installListener() {
        super.installListener();
        ImpulsePreferences.viewPreferences.addListener(this);
    }

    @Override
    protected void removeListener(boolean open) {
        super.removeListener(open);
        ImpulsePreferences.viewPreferences.removeListener(this);
    }

    public static IControlProvider getControls() {
        AbstractControlProvider provider = new AbstractControlProvider(){
            CellTableController cellController;

            @Override
            protected boolean fillThis() {
                final RecordViewer viewer = this.editor().getContainer(RecordViewer.class);
                if (viewer != null) {
                    try {
                        this.cellController = new CellTableController(this.editor(), ".views"){

                            @Override
                            protected void doUpdateHints() {
                                super.doUpdateHints();
                            }

                            @Override
                            public Object value() {
                                return viewer.getView();
                            }

                            @Override
                            public void selectionChanged() {
                                super.selectionChanged();
                                ICell cell = this.getSelectedCell();
                                viewer.setView(cell != null ? (this.relative ? cell.getLink(this.getValueBaseCell()) : cell.getLink()) : null);
                                this.tlk.updateEnable();
                            }

                            @Override
                            protected int getRowFormat(int rowIndex, ICell cell) {
                                Object data = this.getDataValue(rowIndex, 2, this.columnIndex2Name(2));
                                if (I18n.General_Good.equals(data)) {
                                    return 6336;
                                }
                                if (I18n.General_Fair.equals(data)) {
                                    return 5376;
                                }
                                if (I18n.General_Weak.equals(data)) {
                                    return 320;
                                }
                                return 0;
                            }
                        }.initCells(ImpulsePreferences.viewPreferences, ViewConfiguration.class).initColumnDataSources(true, new Object[]{ViewConfiguration.class.getField("description"), new CellTableController.ColumnData(){

                            @Override
                            public String columnData(ICell cell) {
                                float fit = viewer.viewFits(cell.getElement());
                                if (fit > 0.9f) {
                                    return I18n.General_Good;
                                }
                                if (fit > 0.4f) {
                                    return I18n.General_Fair;
                                }
                                if (fit > 0.1f) {
                                    return I18n.General_Weak;
                                }
                                return I18n.General_Bad;
                            }
                        }});
                        this.tlk().addTable(this.container(), this.cellController, this.tlk().ld(this.cols(), 4, 1, 524288, 1), 67136, null, new String[]{I18n.General_Name, I18n.General_Description, I18n.General_Valid});
                        this.tlk().addButton(this.container(), new CommandButtonController(this.editor(), "de.toem.impulse.commands.view.add", viewer, false), this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
                        this.tlk().addButton(this.container(), new CommandButtonController(this.editor(), "de.toem.toolkits.commands.controller.clone", this.cellController, true), this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
                        this.tlk().addButton(this.container(), new CommandButtonController(this.editor(), "org.eclipse.ui.edit.delete", this.cellController, false), this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
                    }
                    catch (NoSuchFieldException | SecurityException exception) {}
                }
                return false;
            }

            @Override
            public void setFocus(boolean force) {
                this.tlk().setFocus(this.cellController, force);
            }

            @Override
            protected void openThis() {
                super.openThis();
                RecordViewer viewer = this.editor().getContainer(RecordViewer.class);
                this.cellController.select(Elements.getCell(viewer.getView()), false);
            }
        };
        provider.setCellClass(ViewConfiguration.class);
        return provider;
    }
}

