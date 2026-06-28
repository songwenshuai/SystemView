/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.parts;

import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.ui.controller.base.ImageController;
import de.toem.toolkits.ui.controller.base.TextBoxController;
import de.toem.toolkits.ui.part.dialog.ControlProviderDialog;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class AboutDialog
extends ControlProviderDialog {
    public AboutDialog(ITlkPartContainer container, int style) {
        super(container, AboutDialog.getControls(), style == -1 ? 65536 : style);
    }

    public AboutDialog() {
    }

    public static AboutControls getControls() {
        return new AboutControls();
    }

    public static class AboutControls
    extends AbstractControlProvider {
        @Override
        public boolean fillThis() {
            try {
                ImageController image = this.tlk().addImage(this.container(), new ImageController(this.editor(), null).initScale(true, true), this.tlk().ld(1, 4, 128, 4, 128), 0, null);
                image.setValue("de.toem.impulse.images.about");
                TextBoxController text = new TextBoxController(this.editor(), null);
                this.tlk().addTextBox(this.container(), text, this.tlk().ld(this.cols() - 1, 4, 1, 524288, 1), 8216, null);
                text.setValue(I18n.AboutText);
            }
            catch (SecurityException securityException) {}
            return true;
        }
    }
}

