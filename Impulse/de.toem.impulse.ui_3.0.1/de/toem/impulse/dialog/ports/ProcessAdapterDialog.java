/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.ports;

import de.toem.impulse.cells.ports.ProcessAdapter;
import de.toem.impulse.dialog.ports.AbstractStreamSerializerControlProvider;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class ProcessAdapterDialog
extends ControlProviderElementDialog {
    public ProcessAdapterDialog(ITlkPartContainer parent, int style) {
        super(parent, ProcessAdapterDialog.getControls(), style);
    }

    public ProcessAdapterDialog() {
    }

    public static IControlProvider getControls() {
        AbstractStreamSerializerControlProvider provider = new AbstractStreamSerializerControlProvider(4096, false, true){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.process_port_dialog";
            }

            @Override
            protected void fillStream() throws NoSuchFieldException, SecurityException {
                this.tlk().addText(this.container(), new TextController(this.editor(), this.clazz().getField("command")), this.cols(), 0x100001, I18n.General_Command_);
                this.tlk().addText(this.container(), new TextController(this.editor(), this.clazz().getField("parameter")), this.cols(), 0x100001, I18n.General_Parameter_);
            }
        };
        provider.setCellClass(ProcessAdapter.class);
        return provider;
    }
}

