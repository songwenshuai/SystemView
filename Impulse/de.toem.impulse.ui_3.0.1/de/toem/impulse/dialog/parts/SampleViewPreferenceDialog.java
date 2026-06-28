/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.parts;

import de.toem.impulse.cell.parts.SampleViewMemberPreferences;
import de.toem.impulse.cell.parts.SampleViewPreferences;
import de.toem.impulse.cell.parts.SampleViewTypePreferences;
import de.toem.impulse.dialog.view.AbstractSignalControlProvider;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.ElementModifierEvent;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.CheckSetController;
import de.toem.toolkits.ui.controller.base.GroupController;
import de.toem.toolkits.ui.controller.base.TabFolderController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.controls.ITlkGroup;
import de.toem.toolkits.ui.tlk.controls.ITlkTabFolder;
import java.util.ArrayList;
import java.util.List;

public class SampleViewPreferenceDialog
extends ControlProviderElementDialog {
    public static final int VARIANT_PART_FILTERED = 1;
    public static final int VARIANT_DIALOG_FILTERED = 2;
    public static final int VARIANT_DIALOG_UNFILTERED = 3;

    public SampleViewPreferenceDialog(ITlkPartContainer parent, int style) {
        super(parent, SampleViewPreferenceDialog.getControls(), style == -1 ? SampleViewPreferenceDialog.defaultStyle() & 0xFFFFFFF7 : style);
    }

    public SampleViewPreferenceDialog() {
    }

    public static IControlProvider getControls() {
        NameDescriptionEnableProvider provider = new NameDescriptionEnableProvider();
        provider.add(SampleViewPreferenceDialog.getControls(null, new ITypeFilter(){

            @Override
            public boolean filter(ICell column) {
                return false;
            }
        }, 3));
        provider.setCellClass(SampleViewPreferences.class);
        return provider;
    }

    public static IControlProvider getControls(final Object owner, final ITypeFilter filter, final int variant) {
        return new AbstractSignalControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.sampleview_dialog";
            }

            @Override
            protected boolean fillThis() {
                try {
                    this.tlk().setDefaultOwner(owner);
                    this.setCellClass(SampleViewPreferences.class);
                    ITlkTabFolder tabFolder = this.tlk().addTabFolder(this.container(), new TabFolderController(this.editor(), String.valueOf(this.hintPrefix()) + ".config.folder"), this.cols(), 0, null);
                    ITlkComposite standard = null;
                    standard = variant == 1 ? this.tlk().addComposite(tabFolder, null, this.cols(), null, 8, I18n.SampleView_StandardSettings, null) : this.tlk().addComposite(tabFolder, null, this.cols(), null, 0, I18n.SampleView_StandardSettings, null);
                    this.tlk().addButton(standard, new CheckController(this.editor(), this.source("showLabels")), this.cols(), 2048, "Show labels (L)", null);
                    this.tlk().addButton(standard, new CheckController(this.editor(), this.source("showAssocs")), this.cols(), 2048, "Show relations (A)", null);
                    this.tlk().addButton(standard, new CheckController(this.editor(), this.source("showDefaultFormat")), this.cols(), 2048, "Show default format (F)", null);
                    this.tlk().addButton(standard, new CheckController(this.editor(), this.source("showNonAvailableFormats")), this.cols(), 2048, "Show non-available formats/infos (F)", null);
                    this.tlk().addButton(standard, new CheckController(this.editor(), this.source("showGroupSamples")), this.cols(), 2048, "Show group samples (S)", null);
                    ArrayList<String> labels = new ArrayList<String>();
                    ArrayList<Long> options = new ArrayList<Long>();
                    Object[] objectArray = SampleConverter.formatValueOptions();
                    int n = objectArray.length;
                    int n2 = 0;
                    while (n2 < n) {
                        Object o = objectArray[n2];
                        int f = (Integer)o;
                        if (f >= 16 && f <= 20) {
                            labels.add(SampleConverter.getFormatLabel(f));
                            options.add(1L << f);
                        }
                        ++n2;
                    }
                    ITlkGroup group = this.tlk().addGroup(standard, null, this.cols(), this.cols(), 0, "Show infos (I)", null);
                    this.tlk().addButtonSet(group, new CheckSetController(this.editor(), this.source("showInfos"), options.toArray()), this.cols(), this.cols(), 2048, null, labels.toArray(new String[labels.size()]), null);
                    ITlkComposite type = this.tlk().addComposite(tabFolder, null, this.cols(), null, 8, I18n.SampleView_TypeSettings, null);
                    if (variant == 1 || variant == 2) {
                        group = this.tlk().addGroup(type, new GroupController(this.editor(), null){
                            ICell filterCell;
                            {
                                super($anonymous0, $anonymous1);
                                this.filterCell = null;
                            }

                            @Override
                            public boolean isAffected(ElementModifierEvent event) {
                                return true;
                            }

                            @Override
                            public boolean needsUpdate() {
                                return true;
                            }

                            @Override
                            protected void doUpdatePre() {
                                if (!this.checkControl()) {
                                    return;
                                }
                                ICell filterCell = null;
                                if (this.getCell() != null) {
                                    for (ICell child : this.getCell().getChildren()) {
                                        if (!(child instanceof SampleViewTypePreferences) || filter.filter(child)) continue;
                                        filterCell = child;
                                        break;
                                    }
                                }
                                if (!Utils.equals(filterCell, this.filterCell)) {
                                    this.filterCell = filterCell;
                                }
                                this.tlk().update(this.filterCell, null, this.getControl(), true);
                                super.doUpdateExternal();
                            }

                            @Override
                            protected Object value() {
                                return this.filterCell != null ? this.filterCell.getName() : null;
                            }
                        }, this.cols(), this.cols(), 0, I18n.SampleView_TypeSettings, null);
                        this.tlk().setDefaultOwner(group);
                        this.setCellClass(SampleViewTypePreferences.class);
                        this.tlk().addButton(group, new CheckController(this.editor(), this.source("showBytes")), this.cols(), 2048, "Show bytes", null);
                        this.tlk().addButton(group, new CheckController(this.editor(), this.source("showImage")), this.cols(), 2048, "Show image", null);
                        labels = new ArrayList();
                        options = new ArrayList();
                        Object[] objectArray2 = SampleConverter.formatValueOptions();
                        int n3 = objectArray2.length;
                        int n4 = 0;
                        while (n4 < n3) {
                            Object o = objectArray2[n4];
                            int f = (Integer)o;
                            if (f >= 1 && (f < 16 || f > 20) && f <= 39) {
                                labels.add(SampleConverter.getFormatLabel(f));
                                options.add(1L << f);
                            }
                            ++n4;
                        }
                        this.tlk().addLabel(group, this.cols(), 0, "Show formats (F):", null);
                        this.tlk().addButtonSet(group, new CheckSetController(this.editor(), this.source("showFormats"), options.toArray()), this.cols(), this.cols(), 2048, null, labels.toArray(new String[labels.size()]), null);
                        this.tlk().setDefaultOwner(owner);
                    } else {
                        this.tlk().addTable(type, new CellTableController(this.editor(), null){

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

                            @Override
                            protected List<ICell> getRawTableCells() {
                                if (this.getValueBaseCell() != null) {
                                    return this.getValueBaseCell().getTribe(false, SampleViewTypePreferences.class);
                                }
                                return ICell.EMPTY_LIST;
                            }
                        }.initCells(null, SampleViewTypePreferences.class).initColumnDataSources(true, new Object[]{SampleViewTypePreferences.class.getMethod("getDescription", new Class[0])}), this.tlk().ld(this.cols(), 4, 1, 524288, 1), 67138, I18n.SampleView_TypeSettings, new String[]{I18n.General_Name, I18n.General_Description});
                    }
                    ITlkComposite member = this.tlk().addComposite(tabFolder, null, this.cols(), null, 0, I18n.SampleView_MemberSettings, null);
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

                        @Override
                        protected List<ICell> getRawTableCells() {
                            if (this.getValueBaseCell() != null) {
                                return this.getValueBaseCell().getTribe(false, SampleViewMemberPreferences.class);
                            }
                            return ICell.EMPTY_LIST;
                        }
                    }.initCells(null, SampleViewMemberPreferences.class).initColumnDataSources(true, new Object[]{SampleViewMemberPreferences.class.getMethod("getDescription", new Class[0])}), this.cols(), 67138, null, new String[]{I18n.General_Name, I18n.General_Description});
                    this.tlk().setDefaultOwner(null);
                }
                catch (NoSuchMethodException noSuchMethodException) {
                }
                catch (SecurityException securityException) {}
                return true;
            }
        };
    }

    public static interface ITypeFilter {
        public boolean filter(ICell var1);
    }
}

