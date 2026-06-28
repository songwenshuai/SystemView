/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.sample;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.view.SearchConfiguration;
import de.toem.impulse.dialog.sample.SearchConfigurationDialog;
import de.toem.impulse.paint.controller.ViewerFindContext;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.ElementHierarchyModifier;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.temp.TemporaryCellElement;
import de.toem.toolkits.ui.controller.base.ButtonController;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.base.ExpandableController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.controller.commands.AddCommandButtonController;
import de.toem.toolkits.ui.controller.commands.CommandButtonController;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.part.dialog.AbstractDialog;
import de.toem.toolkits.ui.part.dialog.ITlkDialog;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.TLK;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.controls.ITlkExpandable;
import de.toem.toolkits.ui.util.NameUtil;
import java.util.List;

public class FindDialog
extends AbstractDialog {
    private ViewerFindContext context;
    private IElement searchBase;
    private IElement tmpSearch;
    private IElement search;
    private TextController status;

    public FindDialog(ITlkPartContainer container, int style) {
        super(container, style);
    }

    public FindDialog() {
    }

    public boolean open(ViewerFindContext context, ITlkDialog.ITlkDialogListener listener) {
        this.context = context;
        this.searchBase = ImpulsePreferences.searchPreferences;
        SearchConfiguration cell = new SearchConfiguration();
        context.prepareInitialSearchConfiguration(cell);
        this.search = this.tmpSearch = new TemporaryCellElement(2, cell);
        return super.open(listener);
    }

    @Override
    protected void createControls(ITlkComposite container) {
        try {
            final ExpandableController expandable = new ExpandableController(this, TLK.HINT(2, "de.toem.impulse.search.manage", "false"), false);
            ITlkExpandable expandableComp = this.tlk().addExpandable(container, expandable, 3, 3, 0, I18n.FindDialog_Manage, null);
            CellTableController cellController = new CellTableController(this, (Object)null){

                @Override
                public void selectionChanged() {
                    this.tlk().updateEnable();
                    super.selectionChanged();
                }

                @Override
                protected boolean filterAddNewTypes(IElement target, String type) {
                    return !"configuration.search".equals(type);
                }

                @Override
                protected Object doEdit(Object data, int doIt, Object sender) {
                    if (doIt == 0 || doIt == 1) {
                        List<IElement> selected = this.getSelectedElements();
                        if (selected.isEmpty()) {
                            return false;
                        }
                        if (doIt == 0) {
                            FindDialog.this.search = selected.get(0);
                            FindDialog.this.updateControls(null);
                        }
                        return true;
                    }
                    return null;
                }
            }.initCells(null, SearchConfiguration.class).initColumnDataSources(true, new Object[]{SearchConfiguration.class.getField("description")});
            this.tlk().addTable(expandableComp, cellController, this.tlk().ld(2, 524288, 1, 4, -1), 65602, null, new String[]{I18n.General_Name, I18n.General_Description});
            ITlkComposite buttonComp = this.tlk().addComposite(expandableComp, null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            this.tlk().addButton(buttonComp, new AddCommandButtonController((ITlkPart)this, cellController), this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            this.tlk().addButton(buttonComp, new CommandButtonController(this, "de.toem.toolkits.commands.controller.clone", cellController, false), this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            this.tlk().addSpace(buttonComp, 1, 0, 10, 30);
            this.tlk().addButton(buttonComp, new CommandButtonController(this, "de.toem.toolkits.commands.controller.edit", cellController, false), this.tlk().ld(1, 4, -1, 128, -1), 0, I18n.FindDialog_Select, null);
            AddCommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this, cellController, false, false, true, false);
            ITlkComposite searchConfigComp = this.tlk().addComposite(container, null, 3, this.tlk().ld(2, 4, -1, 128, -1), 0, null, null);
            IControlProvider provider = SearchConfigurationDialog.getControls();
            this.tlk().setDefaultOwner(this.tmpSearch);
            provider.setLayout(3, 3);
            provider.setCellClass(SearchConfiguration.class);
            this.tlk().addControls(searchConfigComp, provider);
            this.tlk().setDefaultOwner(null);
            ITlkComposite searchButtonComp = this.tlk().addComposite(container, null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            2 button = this.tlk().addButton(searchButtonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    if (!super.enabled()) {
                        return false;
                    }
                    return FindDialog.this.searchBase.isBound() && FindDialog.this.search.isBound() && FindDialog.this.search.hasCell() && FindDialog.this.search.getCell().hasName() && !Utils.isEmpty(FindDialog.this.search.getCell().getValue("expression", String.class));
                }

                @Override
                public void execute(String id, Object data) {
                    ElementHierarchyModifier modifier = ElementHierarchyModifier.add(FindDialog.this.searchBase, FindDialog.this.search, NameUtil.uniqueName(FindDialog.this.searchBase, FindDialog.this.search));
                    FindDialog.this.apply(null, null, new ElementHierarchyModifier[]{modifier}, false);
                    FindDialog.this.search = modifier.getResult();
                    expandable.setExpanded(true);
                    super.execute(id, data);
                }
            }, this.tlk().ld(1, 4, -1, 128, -1), 0, I18n.FindDialog_Add, null);
            button.setTooltip(I18n.FindDialog_AddTooltip);
            this.tlk().addSpace(searchButtonComp, 1, 0, 10, 30);
            this.tlk().addButton(searchButtonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    if (!super.enabled()) {
                        return false;
                    }
                    if (!FindDialog.this.search.isBound() || !FindDialog.this.search.hasCell()) {
                        return false;
                    }
                    String expression = FindDialog.this.search.getCell().getValue("expression", String.class);
                    return !Utils.isEmpty(expression);
                }

                @Override
                public void execute(String id, Object data) {
                    FindDialog.this.status.setValue(FindDialog.this.context.find(FindDialog.this.search));
                    super.execute(id, data);
                }
            }, this.tlk().ld(1, 4, -1, 128, -1), 0, I18n.FindDialog_FindNext, null);
            this.status = this.tlk().addText(container, new TextController(this, null), this.tlk().ld(3, 4, 300, 4, 75), 8266, "");
        }
        catch (SecurityException securityException) {
        }
        catch (NoSuchFieldException noSuchFieldException) {}
        super.createControls(container);
    }
}

