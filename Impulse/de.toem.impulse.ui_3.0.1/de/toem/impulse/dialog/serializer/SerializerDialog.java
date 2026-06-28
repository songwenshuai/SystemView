/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.serializer;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.cells.serializer.Serializer;
import de.toem.impulse.flux.FluxNative;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.serializer.SerializerDescriptor;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.controller.base.ButtonController;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.base.PropertyTableController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.controller.commands.CommandButtonController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.TLK;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.controls.ITlkGroup;

public class SerializerDialog
extends ControlProviderElementDialog {
    public SerializerDialog(ITlkPartContainer parent, int style) {
        super(parent, SerializerDialog.getControls(), style);
    }

    public SerializerDialog() {
    }

    public static IControlProvider getControls() {
        IControlProvider provider = new NameDescriptionEnableProvider().add(new AbstractControlProvider(){

            @Override
            public boolean fillThis() {
                try {
                    this.tlk().addLabel(this.container(), null, 3, 0, I18n.General_DefaultParameters_, null);
                    this.tlk().addTable(this.container(), new PropertyTableController(this.editor(), Serializer.class.getField("parameters")){

                        @Override
                        public boolean needsUpdate() {
                            return true;
                        }

                        @Override
                        protected void doUpdatePre() {
                            IPropertyModel propertyModel;
                            super.doUpdatePre();
                            this.setModel(null);
                            SerializerDescriptor descriptor = (SerializerDescriptor)Elements.serializers.get(this.getCellValueAsString("id"));
                            if (descriptor != null && (propertyModel = descriptor.getPropertyModel()) != null) {
                                this.setModel(propertyModel);
                            }
                        }
                    }, this.tlk().ld(3, 4, 1, 4, 200), 0x100020, null, new String[]{I18n.General_Parameter, I18n.General_Value});
                    this.tlk().addLabel(this.container(), null, 3, 0, I18n.General_Configurations_, null);
                    CellTableController cellContoller = new CellTableController(this.editor(), null){

                        @Override
                        public void selectionChanged() {
                            this.tlk().updateEnable();
                            super.selectionChanged();
                        }

                        @Override
                        protected boolean filterAddNewTypes(IElement target, String type) {
                            ICell cell = (ICell)Elements.cells.create(type);
                            return !(cell instanceof ReaderConfiguration) || !(this.getCell() instanceof Serializer) || !((ReaderConfiguration)cell).supports((Serializer)this.getCell());
                        }
                    }.initCells(null, ReaderConfiguration.class).initCheckSource(ReaderConfiguration.class.getField("enabled")).initColumnDataSources(true, new Object[]{ReaderConfiguration.class.getField("description")});
                    this.tlk().addTable(this.container(), cellContoller, this.tlk().ld(2, 524288, 1, 4, 1), 1116226, null, new String[]{I18n.General_Name, I18n.General_Description});
                    ITlkComposite buttonComp = this.tlk().addComposite(this.container(), null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
                    CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this.editor(), cellContoller, true, true, false, true);
                    this.tlk();
                    if (TLK.isHtml()) {
                        this.tlk().addText(this.container(), new TextController(this.editor(), this.clazz().getField("namePattern")), this.tlk().ld(this.cols() - 1, 524288, 1), 0x100001, I18n.General_FilePattern_);
                    } else {
                        ITlkGroup contentType = this.tlk().addGroup(this.container(), null, 3, 3, 0, "Content type association", null);
                        this.tlk().addLabel(contentType, null, this.tlk().ld(this.cols(), 4, 1), 0, "To add a file association for this serializer,\npress 'Close and Open Content Type Preferences',\nselect 'Element/Record/[serializer name] Record' and press\n'Add' under 'File associations'.", null);
                        this.tlk().addButton(contentType, new ButtonController(this.editor(), null){

                            @Override
                            public void execute(String id, Object data) {
                                Actives.runInMain(new IExecutable(){

                                    @Override
                                    public void execute(IProgress p) {
                                    }
                                }, 500);
                            }
                        }, this.tlk().ld(this.cols(), 4, 1), 0, "Close and Open Content Type Preferences", null);
                    }
                    this.tlk().addButton(this.container(), new ButtonController(this.editor(), null){

                        @Override
                        public void execute(String id, Object data) {
                            super.execute(id, data);
                            String serializerId = this.getCellValueAsString("id");
                            SerializerDescriptor descriptor = (SerializerDescriptor)Elements.serializers.get(serializerId);
                            String dialogId = descriptor.getNativeDialog();
                            if (dialogId != null) {
                                this.tlk().openDialog(dialogId, this.editor(), Elements.getElements(ImpulsePreferences.getNative(serializerId, new FluxNative())), false, null);
                            }
                        }

                        @Override
                        public boolean enabled() {
                            SerializerDescriptor descriptor = (SerializerDescriptor)Elements.serializers.get(this.getCellValueAsString("id"));
                            return descriptor != null && descriptor.getNativeDialog() != null;
                        }
                    }, this.cols(), 0, I18n.Natives_Open_Native, null);
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
                return true;
            }
        });
        provider.setCellClass(Serializer.class);
        return provider;
    }
}

