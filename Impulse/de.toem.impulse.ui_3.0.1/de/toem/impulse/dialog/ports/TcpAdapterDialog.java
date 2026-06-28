/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.ports;

import de.toem.impulse.cells.ports.TcpAdapter;
import de.toem.impulse.dialog.ports.AbstractStreamSerializerControlProvider;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.ui.controller.base.RadioSetController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class TcpAdapterDialog
extends ControlProviderElementDialog {
    public TcpAdapterDialog(ITlkPartContainer parent, int style) {
        super(parent, TcpAdapterDialog.getControls(), style);
    }

    public TcpAdapterDialog() {
    }

    public static IControlProvider getControls() {
        AbstractStreamSerializerControlProvider provider = new AbstractStreamSerializerControlProvider(4096, false, true){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.tcp_port_dialog";
            }

            @Override
            public void fillStream() {
                try {
                    final RadioSetController mode = this.tlk().addButtonSet(this.container(), new RadioSetController(this.editor(), TcpAdapter.class.getField("mode")), 1, this.cols(), 17, I18n.General_Mode_, new String[]{I18n.SocketAdapterDialog_Client, I18n.SocketAdapterDialog_ClientWaitServer, I18n.SocketAdapterDialog_Server}, null);
                    this.tlk().addText(this.container(), new TextController(this.editor(), TcpAdapter.class.getField("server")){

                        @Override
                        protected boolean enabled() {
                            return super.enabled() && mode.getValueAsInt() != 2;
                        }
                    }, this.cols(), 0x100001, I18n.General_Server_);
                    this.tlk().addText(this.container(), new TextController(this.editor(), TcpAdapter.class.getField("socket")), this.cols(), 0x100001, I18n.General_Socket);
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
            }
        };
        provider.setCellClass(TcpAdapter.class);
        return provider;
    }
}

