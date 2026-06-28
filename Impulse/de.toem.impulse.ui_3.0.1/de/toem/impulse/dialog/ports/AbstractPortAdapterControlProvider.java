/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.ports;

import de.toem.impulse.cells.ports.IRecordPort;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;

public class AbstractPortAdapterControlProvider
extends NameDescriptionEnableProvider {
    @Override
    protected final void fillContent() throws NoSuchFieldException, SecurityException {
        this.fillPreContent();
        this.tlk().addButton(this.container(), new CheckController(this.editor(), this.clazz().getField("insertAsRoot")){

            @Override
            public boolean enabled() {
                return !(this.getCell() instanceof IRecordPort) || !((IRecordPort)this.getCell()).isPort();
            }
        }, this.tlk().ld(this.cols(), 131072, -1), 2048, I18n.Adapter_InsertAsRoot, null);
        this.fillPostContent();
    }

    protected void fillPostContent() throws NoSuchFieldException, SecurityException {
    }

    protected void fillPreContent() throws NoSuchFieldException, SecurityException {
    }
}

