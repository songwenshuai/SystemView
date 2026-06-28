/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.swv;

import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.extension.embedded.swv.SwvConfiguration;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.ComboController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkGroup;

public class SwvConfigurationDialog
extends ControlProviderElementDialog {
    public SwvConfigurationDialog(ITlkPartContainer container, int style) {
        super(container, SwvConfigurationDialog.getControls(), style);
    }

    public SwvConfigurationDialog() {
    }

    public static IControlProvider getControls() {
        IControlProvider provider = new NameDescriptionEnableProvider().add(new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.extension.embedded.swv_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    this.tlk().addText(this.container(), new TextController(this.editor(), SwvConfiguration.class.getField("tsFrequency")), this.tlk().ld(1, 524288, -1), 0x100001, "Frequency [Hz] / Devider:");
                    this.tlk().addText(this.container(), new TextController(this.editor(), SwvConfiguration.class.getField("tsDevider")), this.tlk().ld(1, 0x2000000, 100), 0x100000, null);
                    this.tlk().addButton(this.container(), new CheckController(this.editor(), SwvConfiguration.class.getField("waitForSync")), this.cols(), 2049, "Wait for sync:", null);
                    ITlkGroup itm = this.tlk().addGroup(this.container(), null, 12, this.cols(), 0, "ITM", null);
                    int n = 0;
                    while (n < 32) {
                        this.tlk().addText(itm, new TextController(this.editor(), SwvConfiguration.class.getField("itmLabel_" + n)), this.tlk().ld(1, 4, 50), 0x100001, "ITM" + n);
                        this.tlk().addCombo(itm, new ComboController(this.editor(), SwvConfiguration.class.getField("itmMode_" + n), SwvConfiguration.ITM_MODE_LABELS, SwvConfiguration.ITM_MODE_OPTIONS), 1, 8192, null);
                        ++n;
                    }
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException e) {
                    SystemLog.log(e);
                }
                return true;
            }
        });
        provider.setCellClass(ReaderConfiguration.class);
        return provider;
    }
}

