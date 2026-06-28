/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.view;

import de.toem.impulse.cells.charts.ScriptChart;
import de.toem.impulse.scripting.ScriptControls;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;

public class ScriptChartDialog
extends ControlProviderElementDialog {
    public ScriptChartDialog(ITlkPartContainer parent, int style) {
        super(parent, ScriptChartDialog.getControls(), style);
    }

    public ScriptChartDialog() {
    }

    public static IControlProvider getControls() {
        IControlProvider provider = new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.scriptChart_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    ScriptControls.fillScriptControls(this.tlk(), this.container(), this.editor(), ScriptChart.class.getField("script"), null, this.tlk().ld(this.cols(), 4, 1, 4, 350), "Backend Script");
                    ScriptControls.fillScriptControls(this.tlk(), this.container(), this.editor(), ScriptChart.class.getField("frontendScript"), null, this.tlk().ld(this.cols(), 4, 1, 4, 350), "Frontend Script");
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
                return true;
            }
        }.insertBefore(new NameDescriptionEnableProvider());
        provider.setCellClass(ScriptChart.class);
        return provider;
    }

    protected static void addOptions(ITlkComposite options, AbstractControlProvider provider) {
    }
}

