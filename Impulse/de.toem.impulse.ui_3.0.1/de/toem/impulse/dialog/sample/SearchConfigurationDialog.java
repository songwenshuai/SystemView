/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.sample;

import de.toem.impulse.cells.view.SearchConfiguration;
import de.toem.impulse.cells.view.SourceReference;
import de.toem.impulse.scripting.ScriptContentProposalExtension;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.ui.colorizer.JavaScriptColorizer;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.TextBoxController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionEnableProvider;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class SearchConfigurationDialog
extends ControlProviderElementDialog {
    public SearchConfigurationDialog(ITlkPartContainer parent, int style) {
        super(parent, SearchConfigurationDialog.getControls(), style);
    }

    public SearchConfigurationDialog() {
    }

    public static IControlProvider getControls() {
        ITlkControlProvider provider = (ITlkControlProvider)new AbstractControlProvider(){

            @Override
            public boolean fillThis() {
                try {
                    this.tlk().addTable(this.container(), new CellTableController(this.editor(), SourceReference.class.getField("reference")).initCells(null, SourceReference.class).initCheckSource(SourceReference.class.getField("enabled")).initColumnDataSources(false, new Object[]{SourceReference.class.getField("reference"), SourceReference.class.getField("description")}).initLead("s", 40, 0), this.cols(), 1116259, I18n.SearchConfigurationDialog_References, new String[]{I18n.SearchConfigurationDialog_Reference, I18n.General_Location});
                    this.tlk().addTextBox(this.container(), new TextBoxController(this.editor(), SearchConfiguration.class.getField("expression")).add(new JavaScriptColorizer()).add(new ScriptContentProposalExtension()), this.tlk().ld(this.cols() - 1, 4, 300, 4, 100), 0x100009, I18n.General_Expression_);
                    this.tlk().addLabel(this.container(), null, 1, 0, I18n.General_Options_, null);
                    this.tlk().addButton(this.container(), new CheckController(this.editor(), SearchConfiguration.class.getField("reverse")), 1, 2048, I18n.General_Reverse, null);
                    this.tlk().addButton(this.container(), new CheckController(this.editor(), SearchConfiguration.class.getField("wrap")), 1, 2048, I18n.General_Wrap, null);
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
                return true;
            }
        }.insertBefore(new NameDescriptionEnableProvider());
        provider.setCellClass(SearchConfiguration.class);
        return provider;
    }
}

