/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.serializer;

import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.serializer.SerializerDescriptor;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.ui.controller.base.PropertyTableController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.TLK;

public class ReaderConfigurationDialog
extends ControlProviderElementDialog {
    public ReaderConfigurationDialog(ITlkPartContainer parent, int style) {
        super(parent, ReaderConfigurationDialog.getControls(), style == -1 ? ReaderConfigurationDialog.defaultStyle() & 0xFFFFFFF7 : style);
    }

    public ReaderConfigurationDialog() {
    }

    public static IControlProvider getControls() {
        return ReaderConfigurationDialog.getControls(true);
    }

    public static IControlProvider getControls(final boolean tableGrabVert) {
        ITlkControlProvider provider = (ITlkControlProvider)new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.serializer_configuration dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    this.tlk().addTable(this.container(), new PropertyTableController(this.editor(), ReaderConfiguration.class.getField("parameters")){

                        @Override
                        public boolean needsUpdate() {
                            return true;
                        }

                        @Override
                        protected void doUpdatePre() {
                            IPropertyModel propertyModel;
                            SerializerDescriptor descriptor;
                            super.doUpdatePre();
                            this.setModel(null);
                            ICell cell = this.getCell();
                            if (cell != null && cell.getParent() != null && (descriptor = (SerializerDescriptor)Elements.serializers.get(cell.getParent().getValue("id", String.class))) != null && (propertyModel = descriptor.getPropertyModel(ReaderConfiguration.class)) != null) {
                                this.setModel(propertyModel);
                            }
                        }
                    }, this.tlk().ld(this.cols(), 4, 1, tableGrabVert ? 524288 : 4, tableGrabVert ? 1 : 100), 0x100020, null, new String[]{I18n.General_Parameter, I18n.General_Value});
                    this.tlk();
                    if (TLK.isHtml()) {
                        this.tlk().addText(this.container(), new TextController(this.editor(), this.clazz().getField("namePattern")), this.tlk().ld(this.cols() - 1, 524288, 1), 0x100001, I18n.General_FilePattern_);
                    }
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException e) {
                    SystemLog.log(e);
                }
                return true;
            }
        }.insertBefore(new NameDescriptionEnableProvider());
        provider.setCellClass(ReaderConfiguration.class);
        return provider;
    }
}

