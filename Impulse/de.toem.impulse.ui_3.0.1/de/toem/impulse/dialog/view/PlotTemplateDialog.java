/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.view;

import de.toem.impulse.cells.view.AbstractViewConfiguration;
import de.toem.impulse.cells.view.PlotConfigurationTemplate;
import de.toem.impulse.cells.view.SourceReference;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.CellTreeController;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.ComboController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.controller.commands.CommandButtonController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.proposal.PatternContentProposal;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.TLK;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.controls.ITlkGroup;

public class PlotTemplateDialog
extends ControlProviderElementDialog {
    public PlotTemplateDialog(ITlkPartContainer parent, int style) {
        super(parent, PlotTemplateDialog.getControls(), style);
    }

    public PlotTemplateDialog() {
    }

    public static IControlProvider getControls() {
        ITlkControlProvider provider = (ITlkControlProvider)new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.serializer_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    this.tlk().addText(this.container(), new TextController(this.editor(), PlotConfigurationTemplate.class.getField("group")), 3, 0x100001, I18n.General_Group_);
                    final CheckController usePattern = this.tlk().addButton(this.container(), new CheckController(this.editor(), PlotConfigurationTemplate.class.getField("usePattern")), 3, 2048, I18n.PlotTemplateDialog_UsePatternAutomaticInstantiation, null);
                    ITlkGroup pattern = this.tlk().addGroup(this.container(), null, 3, 3, 0, I18n.PlotTemplateDialog_SignalPattern, null);
                    this.tlk().addCombo(pattern, new ComboController(this.editor(), PlotConfigurationTemplate.class.getField("processType"), ISamples.ProcessType.getOptions(false), ISamples.ProcessType.getOptions(false)){

                        @Override
                        public boolean enabled() {
                            return super.enabled() && usePattern.getValueAsBoolean();
                        }
                    }.setNullItem(I18n.General_Any), this.tlk().ld(2, 524288, -1), 8193, I18n.Samples_ProcessType_);
                    this.tlk().addCombo(pattern, new ComboController(this.editor(), PlotConfigurationTemplate.class.getField("signalType"), ISamples.SignalType.getOptions(false), ISamples.SignalType.getOptions(false)){

                        @Override
                        public boolean needsUpdate() {
                            return true;
                        }

                        @Override
                        public boolean enabled() {
                            return super.enabled() && usePattern.getValueAsBoolean();
                        }
                    }.setNullItem(I18n.General_Any), 3, 8193, I18n.Samples_SignalType_);
                    TLK tLK = this.tlk();
                    IController iController = new TextController(this.editor(), PlotConfigurationTemplate.class.getField("namePattern")){

                        @Override
                        public boolean enabled() {
                            return super.enabled() && usePattern.getValueAsBoolean();
                        }
                    }.add(new PatternContentProposal());
                    Object object = this.tlk().ld(1, 4, 150);
                    this.tlk();
                    tLK.addText(pattern, iController, object, 0x100000 | 1, I18n.PlotTemplateDialog_NamePattern_);
                    this.tlk().addButton(pattern, new CheckController(this.editor(), PlotConfigurationTemplate.class.getField("nameRegular")){

                        @Override
                        public boolean enabled() {
                            return super.enabled() && usePattern.getValueAsBoolean();
                        }
                    }, 1, 2048, I18n.General_Regular, null);
                    TLK tLK2 = this.tlk();
                    IController iController2 = new TextController(this.editor(), PlotConfigurationTemplate.class.getField("descriptionPattern")){

                        @Override
                        public boolean enabled() {
                            return super.enabled() && usePattern.getValueAsBoolean();
                        }
                    }.add(new PatternContentProposal());
                    Object object2 = this.tlk().ld(1, 4, 150);
                    this.tlk();
                    tLK2.addText(pattern, iController2, object2, 0x100000 | 1, I18n.PlotTemplateDialog_DescriptionPattern_);
                    this.tlk().addButton(pattern, new CheckController(this.editor(), PlotConfigurationTemplate.class.getField("descriptionRegular")){

                        @Override
                        public boolean enabled() {
                            return super.enabled() && usePattern.getValueAsBoolean();
                        }
                    }, 1, 2048, I18n.General_Regular, null);
                    TLK tLK3 = this.tlk();
                    IController iController3 = new TextController(this.editor(), PlotConfigurationTemplate.class.getField("signalDescriptor")){

                        @Override
                        public boolean enabled() {
                            return super.enabled() && usePattern.getValueAsBoolean();
                        }
                    }.add(new PatternContentProposal());
                    Object object3 = this.tlk().ld(1, 4, 150);
                    this.tlk();
                    tLK3.addText(pattern, iController3, object3, 0x100000 | 1, I18n.PlotTemplateDialog_SignalDescriptorPattern_);
                    this.tlk().addButton(pattern, new CheckController(this.editor(), PlotConfigurationTemplate.class.getField("descriptorRegular")){

                        @Override
                        public boolean enabled() {
                            return super.enabled() && usePattern.getValueAsBoolean();
                        }
                    }, 1, 2048, I18n.General_Regular, null);
                    this.tlk().addButton(this.container(), new CheckController(this.editor(), PlotConfigurationTemplate.class.getField("useInMenu")), 3, 2048, I18n.PlotTemplateDialog_ShowContextMenu, null);
                    CellTreeController cellContoller = new CellTreeController(this.editor(), null){

                        @Override
                        public void selectionChanged() {
                            this.tlk().updateEnable();
                            super.selectionChanged();
                        }

                        @Override
                        protected boolean filterAddNewTypes(IElement target, String type) {
                            return false;
                        }
                    }.initCells(null, new Class[]{AbstractViewConfiguration.class, SourceReference.class});
                    this.tlk().addTree(this.container(), cellContoller, this.tlk().ld(2, 524288, -1, 4, 0), 10, null, new String[]{I18n.General_Name, I18n.General_Description});
                    ITlkComposite buttonComp = this.tlk().addComposite(this.container(), null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
                    CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this.editor(), cellContoller, true, true, true, false);
                    ITlkGroup updates = this.tlk().addGroup(this.container(), null, 3, 3, 0, I18n.General_Updates, null);
                    this.tlk().addButton(updates, new CheckController(this.editor(), this.clazz().getField("doNotUpdate")), 1, 2048, I18n.PlotTemplateDialog_DoNotUpdate, null);
                    this.tlk().addText(updates, new TextController(this.editor(), this.clazz().getField("version")), 2, 0x100001 | ("true".equals(System.getProperty("admin", "false")) ? 0 : 8192), I18n.General_CurrentVersion_);
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
                return true;
            }
        }.insertBefore(new NameDescriptionEnableProvider());
        provider.setCellClass(PlotConfigurationTemplate.class);
        return provider;
    }
}

