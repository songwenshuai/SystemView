/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.serializer;

import de.toem.impulse.cells.serializer.ScriptedReaderConfiguration;
import de.toem.impulse.dialog.serializer.ReaderConfigurationDialog;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.scripting.ScriptControls;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class ScriptedReaderConfigurationDialog
extends ControlProviderElementDialog {
    public ScriptedReaderConfigurationDialog(ITlkPartContainer parent, int style) {
        super(parent, ScriptedReaderConfigurationDialog.getControls(), style == -1 ? ScriptedReaderConfigurationDialog.defaultStyle() & 0xFFFFFFF7 : style);
    }

    public ScriptedReaderConfigurationDialog() {
    }

    public static IControlProvider getControls() {
        ITlkControlProvider provider = (ITlkControlProvider)new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.scripted_reader_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    ScriptControls.fillScriptControls(this.tlk(), this.container(), this.editor(), this.clazz().getField("script"), null, this.tlk().ld(this.cols(), 4, 1, 524288, 1), I18n.General_Script);
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException e) {
                    SystemLog.log(e);
                }
                return true;
            }
        }.insertBefore(ReaderConfigurationDialog.getControls(false));
        provider.setCellClass(ScriptedReaderConfiguration.class);
        return provider;
    }
}

