/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.parts.views;

import de.toem.impulse.ImpulseLicense;
import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cell.parts.PortsViewPreferences;
import de.toem.impulse.cell.parts.SubViewPersitence;
import de.toem.toolkits.general.i18n.I18n;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.CellTreeController;
import de.toem.toolkits.ui.part.view.AbstractViewPart;
import de.toem.toolkits.ui.tlk.layout.TlkShareData;
import java.util.List;

public class PortsView
extends AbstractViewPart {
    CellTreeController treeController;
    IElement preferences = ImpulsePreferences.portPreferences;

    @Override
    protected void createControls(Object container) {
        this.preferences.addListener(this);
        this.treeController = (CellTreeController)this.tlk().addTree(container, new CellTreeController(this, null){

            @Override
            protected void setContextMenus() {
                this.tlk().addMenu(this.control, this, "MENU", "de.toem.eclipse.toolkits.popupmenu.tree", "de.toem.impulse.menu.ports.context", new String[0]);
            }

            @Override
            public int getSelectionTypes() {
                return 8;
            }

            @Override
            protected Object doExecute(Object data, int doIt, Object sender) {
                return this.doOpen(sender, doIt, sender);
            }

            @Override
            protected void selectionChanged() {
                super.selectionChanged();
            }

            @Override
            protected Object doOpen(Object data, int doIt, Object sender) {
                List<IElement> selected = this.getSelectedElements();
                if (selected.size() < 1 || ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.ports", "de.toem.impulse.feature.default", selected.get(0).getCellType())) {
                    return null;
                }
                return super.doOpen(data, doIt, sender);
            }
        }.initColumnDataSources(true, new Object[]{"description", "definition"}).setShowRoot(false).setOwner(this.preferences), new TlkShareData(1), 0x110002, null, new String[]{I18n.General_Name, I18n.General_Description, I18n.General_Definition});
        this.treeController.addDragSupport(7, null);
        this.treeController.addDropSupport(7, null);
    }

    @Override
    protected String getPersitanceType() {
        return "persitence.impulse.subview";
    }

    @Override
    public SubViewPersitence getPersistence() {
        return (SubViewPersitence)super.getPersistence();
    }

    protected PortsViewPreferences getPreferences() {
        if (this.getPersistence() != null) {
            return (PortsViewPreferences)ImpulsePreferences.partsPreferences.getCellByLink(this.getPersistence().partPreferences, PortsViewPreferences.class);
        }
        return null;
    }

    @Override
    protected IController getControllerHandler() {
        return this.treeController;
    }

    /*
     * Enabled force condition propagation
     * Lifted jumps to return sites
     */
    @Override
    public Object command(String id, Object data, int doIt, Object sender) {
        if (id.equals("de.toem.impulse.commands.about")) {
            if (doIt != 5) {
                return this.doAbout(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.support.preferences")) {
            if (doIt != 5) {
                return this.doShowPreferences(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.ports.add")) {
            if (doIt != 5) {
                if (doIt == 6) {
                    return this.treeController.getAddNewInformations(this.preferences, -1);
                }
                if (doIt != 0 || data != null) return this.treeController.doAddNewElement(data, doIt, sender, this.preferences, -1);
                this.treeController.getAddNewInformations(this.preferences, -1);
            }
        } else if (id.equals("de.toem.impulse.commands.ports.edit")) {
            if (doIt != 5 && doIt != 4) {
                return this.treeController.command(IController.Commands.Edit.commandId, sender, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.ports.open")) {
            if (doIt != 5) {
                return this.treeController.command(IController.Commands.Open.commandId, sender, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.ports.save")) {
            if (doIt != 5) {
                return this.doSourcesSave(data, doIt, sender);
            }
        } else {
            if (!id.equals("de.toem.impulse.commands.ports.reset")) return super.command(id, data, doIt, sender);
            if (doIt != 5) {
                return this.doSourcesReset(data, doIt, sender);
            }
        }
        if (doIt != 5) return null;
        return true;
    }

    protected Object doAbout(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            return true;
        }
        return null;
    }

    private Object doShowPreferences(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            return true;
        }
        return null;
    }

    private Object doSourcesSave(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (this.preferences == null || !this.preferences.isBound() || !this.preferences.isDirty()) {
                return false;
            }
            if (doIt == 0) {
                this.preferences.save();
            }
            return true;
        }
        return null;
    }

    private Object doSourcesReset(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (this.preferences == null || !this.preferences.isBound() || !this.preferences.isDirty()) {
                return false;
            }
            if (doIt == 0) {
                this.preferences.reset();
            }
            return true;
        }
        return null;
    }
}

