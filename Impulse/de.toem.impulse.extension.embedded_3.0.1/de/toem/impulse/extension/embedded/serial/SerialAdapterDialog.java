/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.serial;

import de.toem.impulse.dialog.ports.AbstractStreamSerializerControlProvider;
import de.toem.impulse.extension.embedded.serial.SerialAdapter;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.ui.controller.base.RadioSetController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class SerialAdapterDialog
extends ControlProviderElementDialog {
    public SerialAdapterDialog(ITlkPartContainer parent, int style) {
        super(parent, SerialAdapterDialog.getControls(), style);
    }

    public SerialAdapterDialog() {
    }

    public static IControlProvider getControls() {
        AbstractStreamSerializerControlProvider provider = new AbstractStreamSerializerControlProvider(4096, false, true){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.extension.embedded.serial_dialog";
            }

            @Override
            protected void fillStream() throws NoSuchFieldException, SecurityException {
                this.tlk().addText(this.container(), new TextController(this.editor(), SerialAdapter.class.getField("port")).setNullText("e.g. COM1, /dev/ttyS0"), this.cols(), 0x100001, "Port:");
                this.tlk().addText(this.container(), new TextController(this.editor(), SerialAdapter.class.getField("baudRate")), this.cols(), 0x100001, "Baud rate:");
                this.tlk().addText(this.container(), new TextController(this.editor(), SerialAdapter.class.getField("dataBits")), this.cols(), 0x100001, "Data bits:");
                this.tlk().addButtonSet(this.container(), new RadioSetController(this.editor(), SerialAdapter.class.getField("stopBits")), 3, this.cols(), 17, "Stop bits:", new String[]{"1", "2", "1,5"}, null);
                this.tlk().addButtonSet(this.container(), new RadioSetController(this.editor(), SerialAdapter.class.getField("parity")), 5, this.cols(), 17, "Parity:", new String[]{"None", "Odd", "Even", "Mark", "Space"}, null);
            }
        };
        provider.setCellClass(SerialAdapter.class);
        return provider;
    }
}

