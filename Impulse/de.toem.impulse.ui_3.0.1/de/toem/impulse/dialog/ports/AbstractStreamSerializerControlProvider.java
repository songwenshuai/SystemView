/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.ports;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.preferences.ImpulseSerializers;
import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.cells.serializer.Serializer;
import de.toem.impulse.dialog.ports.AbstractPortAdapterControlProvider;
import de.toem.impulse.scripting.ScriptControls;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.information.IGroupedInformation;
import de.toem.toolkits.pattern.information.IGroupedInformations;
import de.toem.toolkits.pattern.information.IInformationGroup;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.ExpandableController;
import de.toem.toolkits.ui.controller.commands.CommandButtonController;
import de.toem.toolkits.ui.dialog.InformationSelectorDialog;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.controls.ITlkExpandable;

public class AbstractStreamSerializerControlProvider
extends AbstractPortAdapterControlProvider {
    private Object serializerSupport;
    private boolean hasDefaultSerializer;
    private boolean supportsStimulation;

    public AbstractStreamSerializerControlProvider(Object serializerSupport, boolean hasDefaultSerializer) {
        this.serializerSupport = serializerSupport;
        this.hasDefaultSerializer = hasDefaultSerializer;
    }

    public AbstractStreamSerializerControlProvider(Object serializerSupport, boolean hasDefaultSerializer, boolean supportsStimulation) {
        this.serializerSupport = serializerSupport;
        this.hasDefaultSerializer = hasDefaultSerializer;
        this.supportsStimulation = supportsStimulation;
    }

    @Override
    public void fillPreContent() throws NoSuchFieldException, SecurityException {
        this.fillStream();
        this.tlk().addGroup(this.container(), null, this.cols(), this.cols(), 0, I18n.General_Serializer, null);
        IGroupedInformations<? extends IGroupedInformation, IInformationGroup> information = Elements.serializers.select(true, true, this.serializerSupport);
        AbstractControlProvider controls = InformationSelectorDialog.getControls(I18n.General_Reader, information, this.clazz().getField("serializer"), 25, 12);
        controls.setLayout(this.cols(), this.cols());
        controls.setCellClass(this.clazz());
        controls = new AbstractControlProvider(){
            IController cellController;

            @Override
            protected boolean fillThis() {
                try {
                    this.cellController = new CellTableController(this.editor(), this.clazz().getField("configuration")){

                        @Override
                        public ICell getValueBaseCell() {
                            if (ImpulsePreferences.serializerPreferences.isBound() && ImpulsePreferences.serializerPreferences.hasCell(ImpulseSerializers.class)) {
                                for (ICell iCell : ImpulsePreferences.serializerPreferences.getCell().getChildren(Serializer.class)) {
                                    if (!((Serializer)iCell).id.equals(this.getCellValue("serializer", String.class))) continue;
                                    return iCell;
                                }
                            }
                            return null;
                        }

                        @Override
                        protected boolean filterAddNewTypes(IElement target, String type) {
                            ICell confuguration = (ICell)Elements.cells.create(type);
                            ICell serializer = this.getValueBaseCell();
                            return !(confuguration instanceof ReaderConfiguration) || !(serializer instanceof Serializer) || !((ReaderConfiguration)confuguration).supports((Serializer)serializer);
                        }
                    }.initCells(null, ReaderConfiguration.class).initColumnDataSources(true, new Object[]{ReaderConfiguration.class.getField("description")});
                    this.tlk().addTable(this.container(), this.cellController, this.tlk().ldc(this.cols() - 1, 4, 30, 4, 10), 67136, null, new String[]{I18n.General_Name, I18n.General_Description});
                    ITlkComposite buttonComp = this.tlk().addComposite(this.container(), null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
                    CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this.editor(), this.cellController, true, true, true, false);
                }
                catch (NoSuchFieldException | SecurityException exception) {}
                return false;
            }

            @Override
            public void setFocus(boolean force) {
                this.tlk().setFocus(this.cellController, force);
            }
        };
        controls.setLayout(this.cols(), this.cols());
        controls.setCellClass(this.clazz());
    }

    protected void fillStream() throws NoSuchFieldException, SecurityException {
    }

    @Override
    public void fillPostContent() throws NoSuchFieldException, SecurityException {
        ITlkExpandable expandable = this.tlk().addExpandable(this.container(), new ExpandableController(this.editor(), "dialog.port.sync", false), this.cols(), this.tlk().ld(this.cols(), 524288, -1), 0, I18n.General_Synchronisation, null);
        this.tlk().addButton(expandable, new CheckController(this.editor(), this.clazz().getField("enableSync")){

            @Override
            public boolean enabled() {
                return !this.getCellValueAsBoolean("insertAsRoot");
            }
        }, this.cols(), 2048, I18n.Adapter_EnableSync, null);
        ScriptControls.fillScriptControls(this.tlk(), expandable, this.editor(), this.clazz().getField("syncScript"), null, this.tlk().ld(this.cols(), 524288, 1, 4, 200));
        if (this.supportsStimulation) {
            expandable = this.tlk().addExpandable(this.container(), new ExpandableController(this.editor(), "dialog.port.socket.stimulation", false), this.cols(), this.tlk().ld(this.cols(), 524288, -1), 0, I18n.Adapter_Stimulation, null);
            this.tlk().addButton(expandable, new CheckController(this.editor(), this.clazz().getField("enableStimulation")), this.cols(), 2048, I18n.Adapter_EnableStimulationScript, null);
            ScriptControls.fillScriptControls(this.tlk(), expandable, this.editor(), this.clazz().getField("stimulationScript"), null, this.tlk().ld(this.cols(), 524288, 1, 4, 200));
        }
    }
}

