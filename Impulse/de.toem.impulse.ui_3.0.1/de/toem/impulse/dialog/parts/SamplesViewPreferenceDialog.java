/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.parts;

import de.toem.impulse.cell.parts.SamplesViewMemberPreferences;
import de.toem.impulse.cell.parts.SamplesViewPreferences;
import de.toem.impulse.dialog.view.AbstractSignalControlProvider;
import de.toem.impulse.parts.views.SamplesView;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.ElementModifierEvent;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.RadioSetController;
import de.toem.toolkits.ui.controller.base.TabFolderController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.controls.ITlkGroup;
import de.toem.toolkits.ui.tlk.controls.ITlkTabFolder;

public class SamplesViewPreferenceDialog
extends ControlProviderElementDialog {
    public SamplesViewPreferenceDialog(ITlkPartContainer parent, int style) {
        super(parent, SamplesViewPreferenceDialog.getControls(), style == -1 ? SamplesViewPreferenceDialog.defaultStyle() & 0xFFFFFFF7 : style);
    }

    public SamplesViewPreferenceDialog() {
    }

    public static IControlProvider getControls() {
        NameDescriptionEnableProvider provider = new NameDescriptionEnableProvider();
        provider.add(SamplesViewPreferenceDialog.getControls(null, new IMemberColumnFilter(){

            @Override
            public boolean filter(ICell column) {
                return false;
            }
        }, true));
        provider.setCellClass(SamplesViewPreferences.class);
        return provider;
    }

    public static IControlProvider getControls(final Object owner, final IMemberColumnFilter filter, boolean asDialog) {
        return new AbstractSignalControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.samplesview_dialog";
            }

            @Override
            protected boolean fillThis() {
                try {
                    this.setCellClass(SamplesViewPreferences.class);
                    this.tlk().setDefaultOwner(owner);
                    ITlkTabFolder tabFolder = this.tlk().addTabFolder(this.container(), new TabFolderController(this.editor(), String.valueOf(this.hintPrefix()) + ".config.folder"), this.cols(), 0x100000, null);
                    ITlkComposite scrolled = this.tlk().addComposite(tabFolder, null, this.cols(), null, 8, I18n.SamplesView_StandardColumns, null);
                    ITlkGroup group = this.tlk().addGroup(scrolled, null, this.cols(), this.cols(), 0, SamplesView.POSITION_COLUMN, null);
                    this.addHideAlignFont(group, SamplesViewPreferences.class, "domain", false, null, false);
                    this.tlk().addButtonSet(group, new RadioSetController(this.editor(), SamplesViewPreferences.class.getField("domainUnitMode")), 3, this.cols(), 17, I18n.SamplesView_UnitMode_, new String[]{I18n.General_Raw, I18n.General_Preferred, I18n.General_Auto}, null);
                    group = this.tlk().addGroup(scrolled, null, this.cols(), this.cols(), 0, SamplesView.VALUE_COLUMN, null);
                    this.addHideAlignFont(group, SamplesViewPreferences.class, "value", true, I18n.SamplesView_IfNoMembers, true);
                    this.addFormatCombos(group, "valueFormat", I18n.General_Format_);
                    group = this.tlk().addGroup(scrolled, null, this.cols(), this.cols(), 0, SamplesView.SIGNAL_COLUMN, null);
                    this.addHideAlignFont(group, SamplesViewPreferences.class, "signal", true, I18n.SamplesView_ifMultiple, false);
                    group = this.tlk().addGroup(scrolled, null, this.cols(), this.cols(), 0, SamplesView.GROUP_COLUMN, null);
                    this.addHideAlignFont(group, SamplesViewPreferences.class, "group", true, I18n.SamplesView_ifGrouped, false);
                    group = this.tlk().addGroup(scrolled, null, this.cols(), this.cols(), 0, SamplesView.ORDER_COLUMN, null);
                    this.addHideAlignFont(group, SamplesViewPreferences.class, "order", true, I18n.SamplesView_ifGrouped, false);
                    group = this.tlk().addGroup(scrolled, null, this.cols(), this.cols(), 0, SamplesView.SAMPLES_COLUMN, null);
                    this.addHideAlignFont(group, SamplesViewPreferences.class, "samples", true, I18n.SamplesView_ifGrouped, false);
                    group = this.tlk().addGroup(scrolled, null, this.cols(), this.cols(), 0, SamplesView.LABEL_COLUMN, null);
                    this.addHideAlignFont(group, SamplesViewPreferences.class, "label", true, null, false);
                    group = this.tlk().addGroup(scrolled, null, this.cols(), this.cols(), 0, SamplesView.RELATION_COLUMN, null);
                    this.addHideAlignFont(group, SamplesViewPreferences.class, "relation", true, null, false);
                    group = this.tlk().addGroup(scrolled, null, this.cols(), this.cols(), 0, SamplesView.TAG_COLUMN, null);
                    this.addHideAlignFont(group, SamplesViewPreferences.class, "tag", true, null, false);
                    this.tlk().addButton(group, new CheckController(this.editor(), SamplesViewPreferences.class.getField("tagBackground")), this.cols(), 2049, I18n.General_ShowBackground_, null);
                    ITlkComposite member = this.tlk().addComposite(tabFolder, null, this.cols(), null, 0, I18n.SamplesView_MemberColumns, null);
                    this.tlk().addTable(member, new CellTableController(this.editor(), null){

                        @Override
                        public boolean isAffected(ElementModifierEvent event) {
                            return true;
                        }

                        @Override
                        public boolean needsUpdate() {
                            return true;
                        }

                        @Override
                        protected boolean filterCells(ICell child) {
                            return filter.filter(child);
                        }
                    }.initCells(null, SamplesViewMemberPreferences.class).initColumnDataSources(true, new Object[]{SamplesViewMemberPreferences.class.getMethod("getDescription", new Class[0])}).initCheckSource(SamplesViewMemberPreferences.class.getField("memberShowMode"), 1, 0), this.cols(), 69186, null, new String[]{I18n.General_Name, I18n.General_Description});
                    this.tlk().setDefaultOwner(null);
                }
                catch (NoSuchFieldException | SecurityException e) {
                    SystemLog.log(e);
                }
                catch (NoSuchMethodException e) {
                    SystemLog.log(e);
                }
                return true;
            }

            void addHideAlignFont(Object group, Class<?> clazz, String column, boolean addShowMode, String showCondition, boolean showWrap) throws NoSuchFieldException, SecurityException {
                if (addShowMode) {
                    if (showCondition != null) {
                        this.tlk().addButtonSet(group, new RadioSetController(this.editor(), clazz.getField(String.valueOf(column) + "ShowMode")), 3, this.cols(), 17, I18n.General_Show_, new String[]{I18n.General_Hide, I18n.General_Show, showCondition}, null);
                    } else {
                        this.tlk().addButtonSet(group, new RadioSetController(this.editor(), clazz.getField(String.valueOf(column) + "ShowMode")), 2, this.cols(), 17, I18n.General_Show_, new String[]{I18n.General_Hide, I18n.General_Show}, null);
                    }
                }
                this.tlk().addButtonSet(group, new RadioSetController(this.editor(), clazz.getField(String.valueOf(column) + "Alignment")), 3, this.cols(), 17, I18n.General_Alignment_, new String[]{I18n.General_Left, I18n.General_Center, I18n.General_Right}, null);
                this.tlk().addButton(group, new CheckController(this.editor(), clazz.getField(String.valueOf(column) + "FixedFont")), this.cols(), 2049, I18n.General_UseFixedSizeFont_, null);
                if (showWrap) {
                    this.tlk().addButton(group, new CheckController(this.editor(), clazz.getField(String.valueOf(column) + "Wrap")), this.cols(), 2049, I18n.General_Wrap_, null);
                }
            }
        };
    }

    public static interface IMemberColumnFilter {
        public boolean filter(ICell var1);
    }
}

