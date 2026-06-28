/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.svdat;

import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.extension.embedded.svdat.SvDatConfiguration;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.ui.controller.base.ComboController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class SvDatConfigurationDialog
extends ControlProviderElementDialog {
    public SvDatConfigurationDialog(ITlkPartContainer container, int style) {
        super(container, SvDatConfigurationDialog.getControls(), style);
    }

    public SvDatConfigurationDialog() {
    }

    public static IControlProvider getControls() {
        IControlProvider provider = new NameDescriptionEnableProvider().add(new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.extension.embedded.svdat_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    this.tlk().addCombo(this.container(), new ComboController(this.editor(), SvDatConfiguration.class.getField("domainBase"), TimeBase.USUAL_OPTIONS, TimeBase.USUAL_OPTIONS), 3, 8193, "Domain Base:");
                    this.tlk().addText(this.container(), new TextController(this.editor(), SvDatConfiguration.class.getField("userEvents")), this.tlk().ld(this.cols(), 4, 550, 4, 250), 0x10000B, "User/System Events:");
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

