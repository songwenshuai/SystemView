/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.sample;

import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.ui.controller.base.CellTreeController;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.controller.source.HintSource;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.proposal.PatternContentProposal;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkGroup;
import de.toem.toolkits.ui.tlk.controls.ITlkTreeItem;

public class CompareDialog
extends ControlProviderElementDialog {
    public CompareDialog(ITlkPartContainer parent, int style) {
        super(parent, CompareDialog.getControls(), style);
    }

    public CompareDialog() {
    }

    public static IControlProvider getControls() {
        AbstractControlProvider provider = new AbstractControlProvider(){

            @Override
            public boolean fillThis() {
                try {
                    this.tlk().addLabel(this.container(), 3, 0, I18n.CompareDialog_SelectScopes_, null);
                    CellTreeController cfr_ignored_0 = (CellTreeController)this.tlk().addTree(this.container(), new CellTreeController(this.editor(), null){

                        @Override
                        protected int readCheckState(ICell cell) {
                            if (Boolean.TRUE.toString().equals(cell.getElement().getHint("compare.disable"))) {
                                return 0;
                            }
                            ICell parent = cell.getParent();
                            while (parent != null) {
                                if (this.readCheckState(parent) == 0) {
                                    return 0;
                                }
                                parent = parent.getParent();
                            }
                            return 1;
                        }

                        @Override
                        protected void storeCheckState(ITlkTreeItem item, int check) {
                            Object data = item.getData();
                            if (data instanceof ICell && ((ICell)data).getElement().isBound()) {
                                try {
                                    ICell parent = ((ICell)data).getParent();
                                    while (parent != null) {
                                        if (this.readCheckState(parent) == 0) {
                                            return;
                                        }
                                        parent = parent.getParent();
                                    }
                                    if (check == 1) {
                                        ((ICell)data).getElement().setHint("compare.disable", null);
                                    } else {
                                        ((ICell)data).getElement().setHint("compare.disable", Boolean.TRUE.toString());
                                    }
                                }
                                finally {
                                    this.doUpdateControl();
                                }
                            }
                        }
                    }.initCells(null, new Class[]{Record.class, Scope.class}).setKeepFirstItemExpanded(true), this.tlk().ld(3, 4, 450, 4, 250), 268437506, null, null);
                    this.tlk().addText(this.container(), new TextController(this.editor(), new HintSource(1, "compare.filter", "")).add(new PatternContentProposal()), 2, 0x100001, I18n.CompareDialog_SignalFilter_);
                    this.tlk().addButton(this.container(), new CheckController(this.editor(), new HintSource(1, "compare.filter.delta", Boolean.FALSE.toString())), 1, 2048, I18n.General_Regular, null);
                    ITlkGroup options = this.tlk().addGroup(this.container(), null, 3, 3, 0, I18n.General_Options, null);
                    this.tlk().addButton(options, new CheckController(this.editor(), new HintSource(1, "compare.ignore.more", Boolean.FALSE.toString())), 3, 2048, I18n.Producer_Diff_IgnoreLonger, null);
                    this.tlk().addButton(options, new CheckController(this.editor(), new HintSource(1, "compare.ignore.less", Boolean.FALSE.toString())), 3, 2048, I18n.Producer_Diff_IgnoreShorter, null);
                    this.tlk().addButton(options, new CheckController(this.editor(), new HintSource(1, "compare.hide.identical", Boolean.TRUE.toString())), 3, 2048, I18n.Producer_Diff_HideIdenticalContent, null);
                    this.tlk().addButton(options, new CheckController(this.editor(), new HintSource(1, "compare.integer.delta", Boolean.FALSE.toString())), 3, 2048, I18n.Producer_Diff_PrepareDeltaInteger, null);
                    this.tlk().addButton(options, new CheckController(this.editor(), new HintSource(1, "compare.float.delta", Boolean.FALSE.toString())), 3, 2048, I18n.Producer_Diff_PrepareDeltaFloat, null);
                    this.tlk().addButton(options, new CheckController(this.editor(), new HintSource(1, "compare.original.append", Boolean.TRUE.toString())), 3, 2048, I18n.Producer_Diff_AppendOriginalInputs, null);
                }
                catch (SecurityException securityException) {}
                return true;
            }
        };
        return provider;
    }
}

