/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.sample;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.paint.ICursorItem;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.AbstractDialog;
import de.toem.toolkits.ui.part.dialog.ITlkDialog;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;

public class GotoDialog
extends AbstractDialog {
    TextController controller;
    private IDomainAxis axis;
    private ICursorItem cursorItem;
    DomainValue position;

    public GotoDialog(ITlkPartContainer container, int style) {
        super(container, style);
    }

    public GotoDialog() {
    }

    public boolean open(IDomainAxis axis, ICursorItem cursorItem, ITlkDialog.ITlkDialogListener listener) {
        this.axis = axis;
        this.cursorItem = cursorItem;
        return super.open(listener);
    }

    @Override
    protected void createControls(ITlkComposite container) {
        try {
            this.controller = new TextController(this, null){};
            this.tlk().addText(container, this.controller, this.tlk().ld(3, 524288, -1), 0, null);
            if (this.axis != null && this.cursorItem != null) {
                this.controller.setValue(String.valueOf(this.cursorItem.getPosition()));
            }
        }
        catch (SecurityException securityException) {}
        super.createControls(container);
    }

    @Override
    protected void okPressed() {
        super.okPressed();
    }

    public DomainValue getPosition() {
        return this.position;
    }
}

