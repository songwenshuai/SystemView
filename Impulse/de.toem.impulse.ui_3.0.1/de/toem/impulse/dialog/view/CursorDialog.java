/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.view;

import de.toem.impulse.cells.view.CursorConfiguration;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.registry.Registration;
import de.toem.toolkits.ui.controller.base.RadioSetController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class CursorDialog
extends ControlProviderElementDialog {
    public CursorDialog(ITlkPartContainer parent, int style) {
        super(parent, CursorDialog.getControls(), style);
    }

    public CursorDialog() {
    }

    public static IControlProvider getControls() {
        AbstractControlProvider provider = new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.cursor_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    this.tlk().addText(this.container(), new TextController(this.editor(), CursorConfiguration.class.getField("name")), this.tlk().ld(this.cols() - 1, 4, 200), 0x100001, I18n.General_Name_);
                    this.tlk().addText(this.container(), new TextController(this.editor(), CursorConfiguration.class.getField("description")), this.cols(), 0x100001, I18n.General_Description_);
                    String[] options = new String[10];
                    final Object[] colors = new Object[10];
                    int n = 0;
                    while (n < colors.length) {
                        colors[n] = Registration.colors.getRgbInt("de.toem.impulse.color.cursor." + n);
                        ++n;
                    }
                    this.tlk().addRadioSet(this.container(), new RadioSetController(this.editor(), CursorConfiguration.class.getField("color")){

                        @Override
                        protected Object convert(Object value) {
                            int n = 0;
                            while (n < colors.length) {
                                if (Utils.equals(value, colors[n])) {
                                    return n;
                                }
                                ++n;
                            }
                            return -1;
                        }

                        @Override
                        protected Object revert(Object value) {
                            int n;
                            if (value instanceof Integer && (n = ((Integer)value).intValue()) >= 0 && n < colors.length) {
                                return colors[n];
                            }
                            return 0;
                        }
                    }, this.tlk().lt(5, 0), this.cols(), 4225, I18n.General_Color_, options, colors);
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

