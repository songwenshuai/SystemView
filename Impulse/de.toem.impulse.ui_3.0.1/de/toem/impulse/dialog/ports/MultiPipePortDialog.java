/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.ports;

import de.toem.impulse.cells.ports.AbstractPortAdapterBaseCell;
import de.toem.impulse.cells.ports.MultiPipePort;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.commands.CommandButtonController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;

public class MultiPipePortDialog
extends ControlProviderElementDialog {
    public MultiPipePortDialog(ITlkPartContainer parent, int style) {
        super(parent, MultiPipePortDialog.getControls(), style);
    }

    public MultiPipePortDialog() {
    }

    public static IControlProvider getControls() {
        IControlProvider provider = new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.multi_port_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    CellTableController cellContoller = new CellTableController(this.editor(), null, AbstractPortAdapterBaseCell.class){

                        @Override
                        public void selectionChanged() {
                            this.tlk().updateEnable();
                            super.selectionChanged();
                        }
                    }.initCheckSource(AbstractPortAdapterBaseCell.class.getField("enabled")).initColumnDataSources(true, new Object[]{AbstractPortAdapterBaseCell.class.getField("description")});
                    this.tlk().addTable(this.container(), cellContoller, this.tlk().ld(2, 524288, 1, 4, 0), 67650, null, new String[]{I18n.General_Name, I18n.General_Description});
                    ITlkComposite buttonComp = this.tlk().addComposite(this.container(), null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
                    CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this.editor(), cellContoller, true, false, true, true);
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
                return true;
            }
        }.insertBefore(new NameDescriptionEnableProvider());
        provider.setCellClass(MultiPipePort.class);
        return provider;
    }
}

