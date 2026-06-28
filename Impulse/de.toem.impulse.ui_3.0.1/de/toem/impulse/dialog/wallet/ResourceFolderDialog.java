/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.wallet;

import de.toem.impulse.cells.wallet.ResourceFolder;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class ResourceFolderDialog
extends ControlProviderElementDialog {
    public ResourceFolderDialog(ITlkPartContainer parent, int style) {
        super(parent, ResourceFolderDialog.getControls(), style);
    }

    public ResourceFolderDialog() {
    }

    public static IControlProvider getControls() {
        ITlkControlProvider provider = (ITlkControlProvider)new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.wallet_folder_dialog";
            }

            @Override
            public boolean fillThis() {
                return true;
            }
        }.insertBefore(new NameDescriptionEnableProvider());
        provider.setCellClass(ResourceFolder.class);
        return provider;
    }
}

