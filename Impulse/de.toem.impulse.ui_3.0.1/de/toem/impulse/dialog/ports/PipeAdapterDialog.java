/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.ports;

import de.toem.impulse.cells.ports.PipeAdapter;
import de.toem.impulse.dialog.ports.AbstractStreamSerializerControlProvider;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.ui.controller.base.RadioSetController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class PipeAdapterDialog
extends ControlProviderElementDialog {
    public PipeAdapterDialog(ITlkPartContainer parent, int style) {
        super(parent, PipeAdapterDialog.getControls(), style);
    }

    public PipeAdapterDialog() {
    }

    public static IControlProvider getControls() {
        AbstractStreamSerializerControlProvider provider = new AbstractStreamSerializerControlProvider(4096, false, true){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.pipe_port_dialog";
            }

            @Override
            protected void fillStream() throws NoSuchFieldException, SecurityException {
                this.tlk().addButtonSet(this.container(), new RadioSetController(this.editor(), PipeAdapter.class.getField("mode")), 2, this.cols(), 17, I18n.General_Mode_, new String[]{I18n.General_Normal, I18n.Adapter_ReadUntilPortStopped}, null);
            }
        };
        provider.setCellClass(PipeAdapter.class);
        return provider;
    }
}

