/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.ports;

import de.toem.impulse.cells.ports.UdpAdapter;
import de.toem.impulse.dialog.ports.AbstractStreamSerializerControlProvider;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.ui.controller.base.RadioSetController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class UdpAdapterDialog
extends ControlProviderElementDialog {
    public UdpAdapterDialog(ITlkPartContainer parent, int style) {
        super(parent, UdpAdapterDialog.getControls(), style);
    }

    public UdpAdapterDialog() {
    }

    public static IControlProvider getControls() {
        AbstractStreamSerializerControlProvider provider = new AbstractStreamSerializerControlProvider(4096, false, true){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.udp_port_dialog";
            }

            @Override
            public void fillStream() {
                try {
                    this.tlk().addText(this.container(), new TextController(this.editor(), UdpAdapter.class.getField("socket")), this.cols(), 0x100001, I18n.General_Socket);
                    this.tlk().addButtonSet(this.container(), new RadioSetController(this.editor(), UdpAdapter.class.getField("mode")), 1, this.cols(), 17, I18n.General_Mode_, new String[]{I18n.UdpAdapterDialog_Normal, I18n.UdpAdapterDialog_ManualFeed}, null);
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
            }
        };
        provider.setCellClass(UdpAdapter.class);
        return provider;
    }
}

