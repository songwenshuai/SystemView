/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.parts;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cell.parts.SamplesViewPreferences;
import de.toem.impulse.cell.parts.SubViewPersitence;
import de.toem.impulse.cells.preferences.AbstractSubViewPreferenceCell;
import de.toem.impulse.dialog.view.AbstractSignalControlProvider;
import de.toem.impulse.parts.views.SubView;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.controller.commands.CommandButtonController;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;

public class SubViewPersitenceDialog
extends ControlProviderElementDialog {
    public static Object configurationOwner = new Object();

    public SubViewPersitenceDialog(ITlkPartContainer parent, int style) {
        super(parent, SubViewPersitenceDialog.getControls(parent), style == -1 ? 65536 : style);
    }

    public SubViewPersitenceDialog() {
    }

    public static IControlProvider getControls(final ITlkPartContainer partContainer) {
        AbstractSignalControlProvider persistenceControls = new AbstractSignalControlProvider(){

            @Override
            protected boolean fillThis() {
                try {
                    ITlkComposite configuration = this.tlk().addComposite(this.container(), null, 3, this.cols(), 0x100000, I18n.General_Configuration_, null);
                    this.tlk().addText(configuration, new TextController(this.editor(), SubViewPersitence.class.getField("partPreferences")){

                        @Override
                        protected Object convert(Object value) {
                            if (value instanceof Link) {
                                return ((Link)value).getPath();
                            }
                            return "";
                        }

                        @Override
                        protected void doUpdateExternal() {
                            super.doUpdateExternal();
                            if (partContainer instanceof SubView) {
                                AbstractSubViewPreferenceCell preferences = ((SubView)partContainer).getPreferences();
                                this.tlk().update(preferences, null, configurationOwner);
                            }
                        }
                    }, this.tlk().ld(1, 524288, 1), 8192, null);
                    AbstractControlProvider configurationControls = new AbstractControlProvider(){
                        IController cellController;

                        @Override
                        protected boolean fillThis() {
                            try {
                                this.cellController = new CellTableController(this.editor(), SubViewPersitence.class.getField("partPreferences")){

                                    @Override
                                    protected boolean filterAddNewTypes(IElement target, String type) {
                                        return type != "impulse.subview.samples";
                                    }

                                    @Override
                                    public void selectionChanged() {
                                        super.selectionChanged();
                                    }
                                }.initCells(ImpulsePreferences.partsPreferences, SamplesViewPreferences.class).initColumnDataSources(true, new Object[]{SamplesViewPreferences.class.getField("description")});
                                this.tlk().addTable(this.container(), this.cellController, this.tlk().ld(this.cols() - 1, 4, 1, 4, 1), 67138, null, new String[]{I18n.General_Name, I18n.General_Description});
                                ITlkComposite buttonComp = this.tlk().addComposite(this.container(), null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
                                CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this.editor(), this.cellController, true, true, true, false);
                            }
                            catch (NoSuchFieldException | SecurityException e) {
                                SystemLog.log(e);
                            }
                            return false;
                        }

                        @Override
                        public void setFocus(boolean force) {
                            this.tlk().setFocus(this.cellController, force);
                        }
                    };
                    this.tlk().addInPlaceDialog(this.container(), configuration, configurationControls, 2, this.cols(), I18n.General_Configuration_);
                }
                catch (NoSuchFieldException | SecurityException e) {
                    SystemLog.log(e);
                }
                return true;
            }
        };
        if (partContainer instanceof SubView) {
            IControlProvider preferenceControls = ((SubView)partContainer).getPreferencesControls(configurationOwner);
            persistenceControls.add(preferenceControls);
        }
        if (partContainer instanceof ITlkPart) {
            persistenceControls.setHintPrefix(((ITlkPart)((Object)partContainer)).getId());
        }
        return persistenceControls;
    }
}

