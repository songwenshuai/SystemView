/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.record;

import de.toem.impulse.cells.ports.TcpAdapter;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.serializer.Message;
import de.toem.toolkits.ui.controller.base.TextBoxController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class MessageDialog
extends ControlProviderElementDialog {
    public MessageDialog(ITlkPartContainer parent, int style) {
        super(parent, MessageDialog.getControls(), style == -1 ? 65536 : style);
    }

    public MessageDialog() {
    }

    public static IControlProvider getControls() {
        AbstractControlProvider provider = new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.record_message_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    this.tlk().addTextBox(this.container(), new TextBoxController(this.editor(), Message.class.getField("message")), this.tlk().ld(this.cols(), 4, 1, 524288, 1), 8216, null);
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
                return true;
            }
        };
        provider.setCellClass(TcpAdapter.class);
        return provider;
    }
}

