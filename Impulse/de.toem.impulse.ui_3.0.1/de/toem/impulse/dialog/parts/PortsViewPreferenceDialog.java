/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.parts;

import de.toem.impulse.cell.parts.PortsViewPreferences;
import de.toem.impulse.dialog.view.AbstractSignalControlProvider;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class PortsViewPreferenceDialog
extends ControlProviderElementDialog {
    public PortsViewPreferenceDialog(ITlkPartContainer parent, int style) {
        super(parent, PortsViewPreferenceDialog.getControls(), style);
    }

    public PortsViewPreferenceDialog() {
    }

    public static IControlProvider getControls() {
        return PortsViewPreferenceDialog.getControls(null, new IMemberColumnFilter(){

            @Override
            public boolean filter(ICell column) {
                return false;
            }
        }, true);
    }

    public static IControlProvider getControls(final Object owner, IMemberColumnFilter filter, final boolean asDialog) {
        return new AbstractSignalControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.record_dialog";
            }

            @Override
            protected boolean fillThis() {
                try {
                    if (asDialog) {
                        this.tlk().ld(this.cols(), 4, -1, 4, 250);
                    } else {
                        this.cols();
                    }
                    this.setCellClass(PortsViewPreferences.class);
                    this.tlk().setDefaultOwner(owner);
                    this.tlk().setDefaultOwner(null);
                }
                catch (SecurityException e) {
                    SystemLog.log(e);
                }
                return true;
            }
        };
    }

    public static interface IMemberColumnFilter {
        public boolean filter(ICell var1);
    }
}

