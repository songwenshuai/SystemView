/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.view;

import de.toem.impulse.cells.view.AxisConfiguration;
import de.toem.impulse.cells.view.FolderConfiguration;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.ComboController;
import de.toem.toolkits.ui.controller.base.RadioSetController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkGroup;

public class FolderDialog
extends ControlProviderElementDialog {
    public FolderDialog(ITlkPartContainer parent, int style) {
        super(parent, FolderDialog.getControls(), style);
    }

    public FolderDialog() {
    }

    public static IControlProvider getControls() {
        AbstractControlProvider provider = new AbstractControlProvider(){
            IController axisMode;
            IController folderMode;
            IController domainBase;
            IController domainClass;

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.folder_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    this.tlk().addText(this.container(), new TextController(this.editor(), FolderConfiguration.class.getField("name")), this.tlk().ld(this.cols() - 1, 4, 200), 0x100001, I18n.General_Name_);
                    this.tlk().addText(this.container(), new TextController(this.editor(), FolderConfiguration.class.getField("description")), this.cols(), 0x100001, I18n.General_Description_);
                    this.axisMode = this.tlk().addButtonSet(this.container(), new RadioSetController(this.editor(), AxisConfiguration.class.getField("axisMode")), 3, this.cols(), 17, I18n.Axis_DomainAxis_, FolderConfiguration.AXIS_OPTIONS, null);
                    this.folderMode = this.tlk().addButtonSet(this.container(), new RadioSetController(this.editor(), FolderConfiguration.class.getField("folderMode")), 3, this.cols(), 17, I18n.Folder_FolderType, FolderConfiguration.FOLDER_OPTIONS, null);
                    ITlkGroup dedicated = this.tlk().addGroup(this.container(), null, this.cols(), this.cols(), 0, I18n.Folder_DedicatedAxis, null);
                    this.tlk().addButtonSet(dedicated, new RadioSetController(this.editor(), AxisConfiguration.class.getField("axisType")){

                        @Override
                        public boolean enabled() {
                            return super.enabled() && axisMode.getValueAsInt() >= 1;
                        }
                    }, 3, this.cols(), 17, I18n.Axis_AxisType_, FolderConfiguration.AXIS_TYPE_OPTIONS, null);
                    this.domainClass = this.tlk().addCombo(dedicated, new ComboController(this.editor(), "domainClass", DomainBase.CLASS_LABELS, DomainBase.CLASSES){

                        @Override
                        public Object value() {
                            if (super.value() == null) {
                                if (this.getCellValue("domainBase", String.class) != null) {
                                    return DomainBase.parse((String)this.getCellValue("domainBase", String.class)).getClass();
                                }
                                return TimeBase.class;
                            }
                            return super.value();
                        }

                        @Override
                        protected void doUpdateExternal() {
                            domainBase.updateControl(true);
                        }

                        @Override
                        public boolean enabled() {
                            return super.enabled() && axisMode.getValueAsInt() > 1;
                        }
                    }, this.cols() - 1, 8193, I18n.Samples_DomainClassBase);
                    this.domainBase = this.tlk().addCombo(dedicated, new ComboController(this.editor(), AxisConfiguration.class.getField("domainBase"), DomainBase.ALL_LABELS, DomainBase.ALL_OPTIONS){

                        @Override
                        protected boolean filterItem(String label, Object value) {
                            return value != null && !DomainBase.parse((String)value).getClass().equals(domainClass.getValue());
                        }

                        @Override
                        public boolean enabled() {
                            return super.enabled() && axisMode.getValueAsInt() > 1;
                        }
                    }.setNullItem(I18n.General_Unknown), 1, 8192, null);
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

