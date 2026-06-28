/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.flux;

import de.toem.impulse.cells.serializer.FluxReaderConfiguration;
import de.toem.impulse.scripting.ScriptControls;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class FluxReaderConfigurationDialog
extends ControlProviderElementDialog {
    public FluxReaderConfigurationDialog(ITlkPartContainer parent, int style) {
        super(parent, FluxReaderConfigurationDialog.getControls(), style == -1 ? FluxReaderConfigurationDialog.defaultStyle() & 0xFFFFFFF7 : style);
    }

    public FluxReaderConfigurationDialog() {
    }

    public static IControlProvider getControls() {
        ITlkControlProvider provider = (ITlkControlProvider)new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.flux_reader_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    ScriptControls.fillScriptControls(this.tlk(), this.container(), this.editor(), this.clazz().getField("script"), null, this.tlk().ld(this.cols(), 4, 1, 524288, 1), "Flux handler");
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException e) {
                    SystemLog.log(e);
                }
                return true;
            }
        }.insertBefore(new NameDescriptionEnableProvider());
        provider.setCellClass(FluxReaderConfiguration.class);
        return provider;
    }
}

