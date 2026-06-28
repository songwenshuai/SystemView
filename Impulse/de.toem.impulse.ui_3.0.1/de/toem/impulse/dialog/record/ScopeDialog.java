/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.record;

import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.parts.viewer.RecordViewer;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class ScopeDialog
extends ControlProviderElementDialog {
    public ScopeDialog(ITlkPartContainer parent, int style) {
        super(parent, ScopeDialog.getControls(), style);
    }

    public ScopeDialog() {
    }

    public static IControlProvider getControls() {
        AbstractControlProvider provider = new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.scope_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    this.tlk().addText(this.container(), new TextController(this.editor(), Scope.class.getField("name")), this.tlk().ld(this.cols() - 1, 4, 200), 0x100001, I18n.General_Name_);
                    this.tlk().addText(this.container(), new TextController(this.editor(), Scope.class.getField("description")), this.cols(), 0x100001, I18n.General_Description_);
                    this.tlk().addText(this.container(), new TextController(this.editor(), Scope.class.getField("domainType")), this.cols(), 0x100001, "Type:");
                    ITlkPart par = this.editor();
                    while (par.getContainerPart() != null) {
                        par = par.getContainerPart();
                    }
                    if (par instanceof RecordViewer) {
                        this.tlk().setEnabled(false);
                    }
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
                return true;
            }
        };
        return provider;
    }
}

