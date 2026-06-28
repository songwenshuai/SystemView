/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.view;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.view.AxisConfiguration;
import de.toem.impulse.cells.view.CursorConfiguration;
import de.toem.impulse.cells.view.FolderConfiguration;
import de.toem.impulse.cells.view.FolderConfigurationInstancer;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.cells.view.PlotConfigurationInstancer;
import de.toem.impulse.cells.view.ViewConfiguration;
import de.toem.impulse.cells.view.ViewConfigurationInstancer;
import de.toem.impulse.dialog.view.AbstractSignalControlProvider;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.parts.viewer.RecordViewer;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.ElementHierarchyModifier;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.IElementModifier;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.base.ComboController;
import de.toem.toolkits.ui.controller.base.RadioSetController;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.part.dialog.ITlkDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkGroup;
import java.util.List;

public class ViewDialog
extends ControlProviderElementDialog {
    public ViewDialog(ITlkPartContainer parent, int style) {
        super(parent, ViewDialog.getControls(), style);
    }

    public ViewDialog() {
    }

    public static IControlProvider getControls() {
        IControlProvider provider = new NameDescriptionEnableProvider().add(new AbstractSignalControlProvider(){
            IElement oldConfiguration = IElement.NONE;
            IController domainBase;
            IController domainClass;
            Object productionConfig;
            Object definition;
            Object domainClassBase;
            Object domainRange;
            Object parameters;

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.view_dialog";
            }

            private void setupConfiguration(ITlkPart parent, ICell cell, int index) {
                ElementHierarchyModifier removeModifier = null;
                IElementModifier modifier = null;
                if (cell != null && cell.getElement().isBound()) {
                    List<ICell> children = cell.getChildren(new Class[]{PlotConfiguration.class, FolderConfiguration.class});
                    if (children.size() > 0) {
                        removeModifier = ElementHierarchyModifier.remove(cell.getElement(), Elements.getElements(children));
                    }
                    String name = ViewConfigurationInstancer.configurationNameForRecord(parent.getElement());
                    switch (index) {
                        case 0: {
                            break;
                        }
                        case 1: {
                            children = parent.getElement().getCell().getChildren(new Class[]{Signal.class, Scope.class});
                            modifier = new FolderConfigurationInstancer(null, Elements.getElements(children)).createAddModifier("default", IElement.NONE, cell.getElement(), null);
                            name = String.valueOf(name) + " hierarchy";
                            break;
                        }
                        case 2: {
                            children = parent.getElement().getCell().getTribe(false, Signal.class);
                            modifier = new PlotConfigurationInstancer(null, Elements.getElements(children)).createAddModifier("default", IElement.NONE, cell.getElement(), null);
                            name = String.valueOf(name) + " flat";
                            break;
                        }
                        default: {
                            children = parent.getElement().getCell().getChildren(ViewConfiguration.class);
                            if (children.isEmpty() || index < 3 || index >= 3 + children.size()) break;
                            List<ICell> clone = children.get(index - 3).clone().getChildren();
                            modifier = ElementHierarchyModifier.add(cell.getElement(), Elements.getElements(clone), null);
                            name = String.valueOf(name) + " " + children.get(index - 3).getName();
                        }
                    }
                    this.tlk().getController("name").setValue(ImpulsePreferences.viewPreferences.uniqueChildName(name));
                    if (removeModifier != null) {
                        this.editor().apply(null, null, new IElementModifier[]{removeModifier}, false);
                    }
                    if (modifier != null) {
                        this.editor().apply(null, null, new IElementModifier[]{modifier}, false);
                    }
                    if (cell.getElement().isBound()) {
                        cell.getElement().setHint("showedAll", Boolean.FALSE.toString());
                        if (parent != null && parent instanceof RecordViewer) {
                            ((RecordViewer)parent).setView(cell.getElement());
                        }
                    }
                }
            }

            @Override
            public boolean fillThis() {
                final ITlkPart parent = this.editor().getContainerPart();
                if (parent != null && parent instanceof RecordViewer && this.editor().getElement().isBound() && parent.getElement().isBound() && parent.getElement().hasCell() && this.editor() instanceof ITlkDialog && ((ITlkDialog)this.editor()).isInstancing()) {
                    RadioSetController selection = this.tlk().addRadioSet(this.container(), new RadioSetController(this.editor(), null){

                        @Override
                        public void changed(boolean finalized) {
                            super.changed(finalized);
                            this.setupConfiguration(parent, this.getCell(), this.getValueAsInt());
                        }
                    }, 3, this.cols(), 4096, null, new String[]{I18n.ViewDialog_Empty, I18n.ViewDialog_Hierarchy, I18n.ViewDialog_Flat, I18n.ViewDialog_Record, I18n.ViewDialog_Record, I18n.ViewDialog_Record, I18n.ViewDialog_Record}, new String[]{"de.toem.impulse.images.option.view.empty", "de.toem.impulse.images.option.view.tree", "de.toem.impulse.images.option.view.flat", "de.toem.impulse.images.option.view.1", "de.toem.impulse.images.option.view.2", "de.toem.impulse.images.option.view.3", "de.toem.impulse.images.option.view.4"});
                    selection.setValue(0);
                    List<ICell> children = parent.getElement().getCell().getTribe(false, Signal.class);
                    if (children.size() > 10000) {
                        selection.setOptionEnabled(1, false, true);
                        selection.setOptionEnabled(2, false, true);
                    }
                    List<ViewConfiguration> configs = parent.getElement().getCell().getChildren(ViewConfiguration.class);
                    int n = 0;
                    while (n < 4) {
                        if (configs.size() > n) {
                            selection.setOptionEnabled(3 + n, true, false);
                            selection.setOptionText(3 + n, String.valueOf(I18n.ViewDialog_Record) + " " + configs.get(n).getName());
                        } else {
                            selection.setOptionEnabled(3 + n, false, true);
                        }
                        ++n;
                    }
                }
                try {
                    this.tlk().addButtonSet(this.container(), new RadioSetController(this.editor(), AxisConfiguration.class.getField("axisType")), 3, this.cols(), 17, I18n.Axis_AxisType_, FolderConfiguration.AXIS_TYPE_OPTIONS, null);
                    ITlkGroup override = this.tlk().addGroup(this.container(), null, this.cols(), this.cols(), 0, I18n.Samples_DomainBase_, null);
                    final RadioSetController axisMode = this.tlk().addButtonSet(override, new RadioSetController(this.editor(), AxisConfiguration.class.getField("axisMode"), 1), 3, this.cols(), 17, I18n.General_Mode_, FolderConfiguration.AXIS_ROOT_OPTIONS, null);
                    this.domainClass = this.tlk().addCombo(override, new ComboController(this.editor(), "domainClass", DomainBase.CLASS_LABELS, DomainBase.CLASSES){

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
                        public boolean enabled() {
                            return super.enabled() && axisMode.getValueAsInt() > 1;
                        }

                        @Override
                        protected void doUpdateExternal() {
                            domainBase.updateControl(true);
                        }
                    }, this.cols() - 1, 8193, I18n.Samples_DomainClass_);
                    this.domainBase = this.tlk().addCombo(override, new ComboController(this.editor(), AxisConfiguration.class.getField("domainBase"), DomainBase.ALL_LABELS, DomainBase.ALL_OPTIONS){

                        @Override
                        protected boolean filterItem(String label, Object value) {
                            return value != null && !DomainBase.parse((String)value).getClass().equals(domainClass.getValue());
                        }

                        @Override
                        public boolean enabled() {
                            return super.enabled() && axisMode.getValueAsInt() > 1;
                        }
                    }.setNullItem(I18n.General_Unknown), 1, 8192, null);
                    this.tlk().addTable(this.container(), new CellTableController(this.editor(), null){

                        @Override
                        protected boolean filterAddNewTypes(IElement target, String type) {
                            return !"configuration.cursor".equals(type);
                        }
                    }.initCells(null, CursorConfiguration.class).initColumnDataSources(true, new Object[]{CursorConfiguration.class.getField("description")}).initLead("", 50, 0), this.tlk().ld(this.cols(), 4, 1, 4, 100), 1114210, null, new String[]{I18n.General_Cursor, I18n.General_Description});
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
                return true;
            }

            @Override
            protected void openThis() {
                ITlkPart parent;
                if (this.editor() instanceof ITlkDialog && ((ITlkDialog)this.editor()).isInstancing() && (parent = this.editor().getContainerPart()) != null && parent instanceof RecordViewer && this.editor().getElement().isBound()) {
                    this.oldConfiguration = ((RecordViewer)parent).getView();
                    ((RecordViewer)parent).setView(this.editor().getElement());
                }
            }

            @Override
            protected void closingThis(boolean canceled) {
                ITlkPart parent;
                if (this.editor() instanceof ITlkDialog && ((ITlkDialog)this.editor()).isInstancing() && (parent = this.editor().getContainerPart()) != null && parent instanceof RecordViewer) {
                    if (canceled) {
                        ((RecordViewer)parent).setView(this.oldConfiguration);
                    } else {
                        ((RecordViewer)parent).setView(this.editor().getElement());
                    }
                }
            }
        });
        provider.setCellClass(ViewConfiguration.class);
        return provider;
    }
}

