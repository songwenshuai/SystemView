/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.parts;

import de.toem.impulse.cell.parts.SamplesViewMemberPreferences;
import de.toem.impulse.dialog.view.AbstractSignalControlProvider;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.RadioSetController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class SamplesViewMemberPreferenceDialog
extends ControlProviderElementDialog {
    public SamplesViewMemberPreferenceDialog(ITlkPartContainer parent, int style) {
        super(parent, SamplesViewMemberPreferenceDialog.getControls(), style);
    }

    public SamplesViewMemberPreferenceDialog() {
    }

    public static IControlProvider getControls() {
        return new AbstractSignalControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.samplesview_member_dialog";
            }

            @Override
            protected boolean fillThis() {
                this.setCellClass(SamplesViewMemberPreferences.class);
                try {
                    this.addHideAlignFont(this.container(), SamplesViewMemberPreferences.class, "member", true, null);
                    this.addValueFormatCombo(this.container(), SamplesViewMemberPreferences.class.getField("memberFormat"), I18n.General_Format_);
                    this.tlk().setDefaultOwner(null);
                }
                catch (NoSuchFieldException | SecurityException e) {
                    SystemLog.log(e);
                }
                return true;
            }

            void addHideAlignFont(Object group, Class<?> clazz, String column, boolean addShowMode, String showCondition) throws NoSuchFieldException, SecurityException {
                if (addShowMode) {
                    if (showCondition != null) {
                        this.tlk().addButtonSet(group, new RadioSetController(this.editor(), clazz.getField(String.valueOf(column) + "ShowMode")), 3, this.cols(), 17, I18n.General_Show_, new String[]{I18n.General_Hide, I18n.General_Show, showCondition}, null);
                    } else {
                        this.tlk().addButtonSet(group, new RadioSetController(this.editor(), clazz.getField(String.valueOf(column) + "ShowMode")), 2, this.cols(), 17, I18n.General_Show_, new String[]{I18n.General_Hide, I18n.General_Show}, null);
                    }
                }
                this.tlk().addButtonSet(group, new RadioSetController(this.editor(), clazz.getField(String.valueOf(column) + "Alignment")), 3, this.cols(), 17, I18n.General_Alignment_, new String[]{I18n.General_Left, I18n.General_Center, I18n.General_Right}, null);
                this.tlk().addButton(group, new CheckController(this.editor(), clazz.getField(String.valueOf(column) + "FixedFont")), this.cols(), 2049, I18n.General_UseFixedSizeFont_, null);
                this.tlk().addButton(group, new CheckController(this.editor(), clazz.getField(String.valueOf(column) + "Wrap")), this.cols(), 2049, I18n.General_Wrap_, null);
            }
        };
    }
}

