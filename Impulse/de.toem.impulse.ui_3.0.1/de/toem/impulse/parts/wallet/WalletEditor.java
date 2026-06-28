/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.parts.wallet;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.wallet.Wallet;
import de.toem.impulse.scripting.ScriptControls;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.ElementModifierEvent;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.IElementModifier;
import de.toem.toolkits.pattern.ide.Ide;
import de.toem.toolkits.pattern.scripting.IScripting;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.ButtonController;
import de.toem.toolkits.ui.controller.base.CellTreeController;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.HtmlController;
import de.toem.toolkits.ui.controller.base.TabFolderController;
import de.toem.toolkits.ui.controller.base.TextBoxController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.controller.commands.CommandButtonController;
import de.toem.toolkits.ui.controller.layout.ShareLayoutController;
import de.toem.toolkits.ui.part.editor.AbstractEditorPart;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.controls.ITlkTabFolder;
import de.toem.toolkits.ui.tlk.controls.ITlkTreeItem;
import de.toem.toolkits.ui.tlk.layout.TlkShareData;
import java.io.File;
import java.util.ArrayList;

public class WalletEditor
extends AbstractEditorPart {
    private ShareLayoutController mainLayoutController;
    private ShareLayoutController topLayoutController;
    private IController walletController;
    private TabFolderController tabFolderController;
    private IController viewsController;
    private IController portController;
    private IController serializerController;
    private IController chartsController;
    private IController templatesController;
    private IController searchController;
    private IController partsController;
    private IController nativesController;
    protected IElement viewPreferences = IElement.NONE;
    protected IElement portPreferences = IElement.NONE;
    protected IElement serializerPreferences = IElement.NONE;
    protected IElement chartsPreferences = IElement.NONE;
    protected IElement templatesPreferences = IElement.NONE;
    protected IElement complementary = IElement.NONE;
    protected IElement searchPreferences = IElement.NONE;
    protected IElement partsPreferences = IElement.NONE;
    protected IElement nativePreferences = IElement.NONE;
    protected IElement resources = IElement.NONE;

    public WalletEditor(ITlkPartContainer partContainer, int style) {
        super(partContainer, style);
    }

    public WalletEditor(ITlkPartContainer.ITlkEditorSession session, int style) {
        super(session, style);
    }

    public WalletEditor() {
    }

    @Override
    protected void installListener() {
        super.installListener();
        if (this.viewPreferences == IElement.NONE) {
            this.viewPreferences = ImpulsePreferences.viewPreferences;
            this.viewPreferences.addListener(this);
        }
        if (this.portPreferences == IElement.NONE) {
            this.portPreferences = ImpulsePreferences.portPreferences;
            this.portPreferences.addListener(this);
        }
        if (this.serializerPreferences == IElement.NONE) {
            this.serializerPreferences = ImpulsePreferences.serializerPreferences;
            this.serializerPreferences.addListener(this);
        }
        if (this.chartsPreferences == IElement.NONE) {
            this.chartsPreferences = ImpulsePreferences.chartPreferences;
            this.chartsPreferences.addListener(this);
        }
        if (this.templatesPreferences == IElement.NONE) {
            this.templatesPreferences = ImpulsePreferences.templatePreferences;
            this.templatesPreferences.addListener(this);
        }
        if (this.searchPreferences == IElement.NONE) {
            this.searchPreferences = ImpulsePreferences.searchPreferences;
            this.searchPreferences.addListener(this);
        }
        if (this.partsPreferences == IElement.NONE) {
            this.partsPreferences = ImpulsePreferences.partsPreferences;
            this.partsPreferences.addListener(this);
        }
        if (this.nativePreferences == IElement.NONE) {
            this.nativePreferences = ImpulsePreferences.nativePreferences;
            this.nativePreferences.addListener(this);
        }
        this.resources = Elements.getElement(new File("/"));
    }

    @Override
    protected void removeListener(boolean open) {
        super.removeListener(open);
        if (!open && this.viewPreferences != IElement.NONE) {
            this.viewPreferences.removeListener(this);
            this.viewPreferences = IElement.NONE;
        }
        if (!open && this.portPreferences != IElement.NONE) {
            this.portPreferences.removeListener(this);
            this.portPreferences = IElement.NONE;
        }
        if (!open && this.serializerPreferences != IElement.NONE) {
            this.serializerPreferences.removeListener(this);
            this.serializerPreferences = IElement.NONE;
        }
        if (!open && this.chartsPreferences != IElement.NONE) {
            this.chartsPreferences.removeListener(this);
            this.chartsPreferences = IElement.NONE;
        }
        if (!open && this.templatesPreferences != IElement.NONE) {
            this.templatesPreferences.removeListener(this);
            this.templatesPreferences = IElement.NONE;
        }
        if (!open && this.searchPreferences != IElement.NONE) {
            this.searchPreferences.removeListener(this);
            this.searchPreferences = IElement.NONE;
        }
        if (!open && this.partsPreferences != IElement.NONE) {
            this.partsPreferences.removeListener(this);
            this.partsPreferences = IElement.NONE;
        }
        if (!open && this.nativePreferences != IElement.NONE) {
            this.nativePreferences.removeListener(this);
            this.nativePreferences = IElement.NONE;
        }
    }

    @Override
    public void createControls(Object container) {
        try {
            this.mainLayoutController = (ShareLayoutController)new ShareLayoutController(this, "de.toem.impulse.editor.wallet.v", 2, 1).initDefaults(100.0f, 0).setOwner(IController.OWNER_UNDEFINED);
            ITlkComposite main = this.tlk().addComposite(container, this.mainLayoutController, null, new TlkShareData(1), 0, null, null);
            this.topLayoutController = (ShareLayoutController)new ShareLayoutController(this, "de.toem.impulse.editor.wallet.h", 1, 0).initDefaults(350.0f, 0).setOwner(IController.OWNER_UNDEFINED);
            ITlkComposite top = this.tlk().addComposite(main, this.topLayoutController, null, new TlkShareData(1), 0, null, null);
            ITlkTabFolder walletFolder = this.tlk().addTabFolder(top, new TabFolderController(this, "de.toem.impulse.editor.wallet.wallet"), new TlkShareData(1), 0x100400, null);
            ITlkComposite wallet = this.tlk().addComposite(walletFolder, null, 2, null, 0x100000, I18n.General_Elements, "wallet");
            this.walletController = this.tlk().addTree(wallet, new CellTreeController(this, null){

                @Override
                protected void selectionChanged() {
                    super.selectionChanged();
                    this.tlk().updateEnable();
                }
            }.initCells(null, new Class[]{Cell.class}).setKeepFirstItemExpanded(true).setShowRoot(true), this.tlk().ld(1, 524288, 1), 0x100002, null, null);
            this.walletController.addDragSupport(1, null);
            this.walletController.addDropSupport(5, null);
            ITlkComposite buttonComp = this.tlk().addComposite(wallet, null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this, this.walletController, false, true, true, true);
            ITlkComposite script = this.tlk().addComposite(walletFolder, null, this.cols(), null, 0x100000, I18n.General_Install, "de.toem.impulse.images.tab.installation");
            this.tlk().addText(script, new TextController(this, Wallet.class.getField("minVersion")), this.tlk().ld(this.cols() - 1, true, false), 0x100001, I18n.General_MinVersion_);
            this.tlk().addText(script, new TextController(this, Wallet.class.getField("maxVersion")), this.tlk().ld(this.cols() - 1, true, false), 0x100001, I18n.General_MaxVersion_);
            this.tlk().addLabel(script, this.cols() - 1, 0, I18n.General_Script_, null);
            this.tlk().addButton(script, new CheckController(this, Wallet.class.getField("installable")), 1, 2048, I18n.General_Installable, null);
            ScriptControls.fillScriptControls(this.tlk(), script, this, Wallet.class.getField("script"), null, this.tlk().ld(this.cols(), 4, 1, 524288, 1));
            ITlkComposite descriptionTab = this.tlk().addComposite(walletFolder, null, 1, null, 0, I18n.General_Description, "de.toem.impulse.images.tab.description");
            this.tlk().addTextBox(descriptionTab, new TextBoxController(this, Wallet.class.getField("description")).initMultilineConversion(false), 1, 0x100018, null);
            this.tlk().addSash(top, null, null, 4);
            this.tabFolderController = (TabFolderController)new TabFolderController(this, "de.toem.impulse.editor.wallet.tabs").setOwner(IController.OWNER_UNDEFINED);
            ITlkTabFolder tabFolder = this.tlk().addTabFolder(top, this.tabFolderController, new TlkShareData(2), 0x100400, null);
            ITlkComposite configTab = this.tlk().addComposite(tabFolder, null, 2, null, 0, I18n.General_Views, "preferences.views");
            this.viewsController = this.tlk().addTree(configTab, new CellTreeController(this, "configCells"){

                @Override
                protected void selectionChanged() {
                    super.selectionChanged();
                    this.tlk().updateEnable();
                }

                @Override
                protected boolean sync(ICell cell, ITlkTreeItem item, int index) {
                    boolean changed = super.sync(cell, item, index);
                    if (item.getParentItem() == null) {
                        item.setText(I18n.General_Preferences);
                    }
                    return changed;
                }
            }.setKeepFirstItemExpanded(true).setShowRoot(true), this.tlk().ld(1, 524288, 1), 0x10000002, null, null).setOwner(IController.OWNER_UNDEFINED);
            this.viewsController.addDragSupport(1, null);
            this.viewsController.addDropSupport(1, null);
            buttonComp = this.tlk().addComposite(configTab, null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this, this.viewsController, false, true, true, true);
            this.tlk().addSpace(buttonComp, 1);
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.viewPreferences != null && WalletEditor.this.viewPreferences.isBound() && WalletEditor.this.viewPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.viewPreferences.save();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Store, "de-toem-toolkits-tlk-css-general-save-image");
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.viewPreferences != null && WalletEditor.this.viewPreferences.isBound() && WalletEditor.this.viewPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.viewPreferences.load();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Reset, null);
            ITlkComposite portTab = this.tlk().addComposite(tabFolder, null, 2, null, 0, I18n.General_Ports, "preferences.impulse.ports");
            this.portController = this.tlk().addTree(portTab, new CellTreeController(this, "portCells"){

                @Override
                protected void selectionChanged() {
                    super.selectionChanged();
                    this.tlk().updateEnable();
                }

                @Override
                protected boolean sync(ICell cell, ITlkTreeItem item, int index) {
                    boolean changed = super.sync(cell, item, index);
                    if (item.getParentItem() == null) {
                        item.setText(I18n.General_Preferences);
                    }
                    return changed;
                }
            }.setKeepFirstItemExpanded(true).setShowRoot(true), this.tlk().ld(1, 524288, 1), 0x10000002, null, null).setOwner(IController.OWNER_UNDEFINED);
            this.portController.addDragSupport(1, null);
            this.portController.addDropSupport(1, null);
            buttonComp = this.tlk().addComposite(portTab, null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this, this.portController, false, true, true, true);
            this.tlk().addSpace(buttonComp, 1);
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.portPreferences != null && WalletEditor.this.portPreferences.isBound() && WalletEditor.this.portPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.portPreferences.save();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Store, "de-toem-toolkits-tlk-css-general-save-image");
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.portPreferences != null && WalletEditor.this.portPreferences.isBound() && WalletEditor.this.portPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.portPreferences.load();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Reset, null);
            ITlkComposite serializerTab = this.tlk().addComposite(tabFolder, null, 2, null, 0, I18n.General_Serializer, "preferences.impulse.serializers");
            this.serializerController = this.tlk().addTree(serializerTab, new CellTreeController(this, "serializerCells"){

                @Override
                protected void selectionChanged() {
                    super.selectionChanged();
                    this.tlk().updateEnable();
                }

                @Override
                protected boolean sync(ICell cell, ITlkTreeItem item, int index) {
                    boolean changed = super.sync(cell, item, index);
                    if (item.getParentItem() == null) {
                        item.setText(I18n.General_Preferences);
                    }
                    return changed;
                }
            }.setKeepFirstItemExpanded(true).setShowRoot(true), this.tlk().ld(1, 524288, 1), 0x10000002, null, null).setOwner(IController.OWNER_UNDEFINED);
            this.serializerController.addDragSupport(1, null);
            this.serializerController.addDropSupport(1, null);
            buttonComp = this.tlk().addComposite(serializerTab, null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this, this.serializerController, false, true, true, true);
            this.tlk().addSpace(buttonComp, 1);
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.serializerPreferences != null && WalletEditor.this.serializerPreferences.isBound() && WalletEditor.this.serializerPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.serializerPreferences.save();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Store, "de-toem-toolkits-tlk-css-general-save-image");
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.serializerPreferences != null && WalletEditor.this.serializerPreferences.isBound() && WalletEditor.this.serializerPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.serializerPreferences.load();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Reset, null);
            ITlkComposite chartsTab = this.tlk().addComposite(tabFolder, null, 2, null, 0, I18n.General_Charts, "preferences.impulse.charts");
            this.chartsController = this.tlk().addTree(chartsTab, new CellTreeController(this, "chartsCells"){

                @Override
                protected void selectionChanged() {
                    super.selectionChanged();
                    this.tlk().updateEnable();
                }

                @Override
                protected boolean sync(ICell cell, ITlkTreeItem item, int index) {
                    boolean changed = super.sync(cell, item, index);
                    if (item.getParentItem() == null) {
                        item.setText(I18n.General_Preferences);
                    }
                    return changed;
                }
            }.setKeepFirstItemExpanded(true).setShowRoot(true), this.tlk().ld(1, 524288, 1), 0x10000002, null, null).setOwner(IController.OWNER_UNDEFINED);
            this.chartsController.addDragSupport(1, null);
            this.chartsController.addDropSupport(1, null);
            buttonComp = this.tlk().addComposite(chartsTab, null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this, this.chartsController, false, true, true, true);
            this.tlk().addSpace(buttonComp, 1);
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.chartsPreferences != null && WalletEditor.this.chartsPreferences.isBound() && WalletEditor.this.chartsPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.chartsPreferences.save();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Store, "de-toem-toolkits-tlk-css-general-save-image");
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.chartsPreferences != null && WalletEditor.this.chartsPreferences.isBound() && WalletEditor.this.chartsPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.chartsPreferences.load();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Reset, null);
            ITlkComposite templatesTab = this.tlk().addComposite(tabFolder, null, 2, null, 0, I18n.General_Templates, "preferences.impulse.templates");
            this.templatesController = this.tlk().addTree(templatesTab, new CellTreeController(this, "templatesCells"){

                @Override
                protected void selectionChanged() {
                    super.selectionChanged();
                    this.tlk().updateEnable();
                }

                @Override
                protected boolean sync(ICell cell, ITlkTreeItem item, int index) {
                    boolean changed = super.sync(cell, item, index);
                    if (item.getParentItem() == null) {
                        item.setText(I18n.General_Preferences);
                    }
                    return changed;
                }
            }.setKeepFirstItemExpanded(true).setShowRoot(true), this.tlk().ld(1, 524288, 1), 0x10000002, null, null).setOwner(IController.OWNER_UNDEFINED);
            this.templatesController.addDragSupport(1, null);
            this.templatesController.addDropSupport(1, null);
            buttonComp = this.tlk().addComposite(templatesTab, null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this, this.templatesController, false, true, true, true);
            this.tlk().addSpace(buttonComp, 1);
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.templatesPreferences != null && WalletEditor.this.templatesPreferences.isBound() && WalletEditor.this.templatesPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.templatesPreferences.save();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Store, "de-toem-toolkits-tlk-css-general-save-image");
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.templatesPreferences != null && WalletEditor.this.templatesPreferences.isBound() && WalletEditor.this.templatesPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.templatesPreferences.load();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Reset, null);
            ITlkComposite searchTab = this.tlk().addComposite(tabFolder, null, 2, null, 0, I18n.General_Search, "preferences.impulse.search");
            this.searchController = this.tlk().addTree(searchTab, new CellTreeController(this, "searchCells"){

                @Override
                protected void selectionChanged() {
                    super.selectionChanged();
                    this.tlk().updateEnable();
                }

                @Override
                protected boolean sync(ICell cell, ITlkTreeItem item, int index) {
                    boolean changed = super.sync(cell, item, index);
                    if (item.getParentItem() == null) {
                        item.setText(I18n.General_Preferences);
                    }
                    return changed;
                }
            }.setKeepFirstItemExpanded(true).setShowRoot(true), this.tlk().ld(1, 524288, 1), 0x10000002, null, null).setOwner(IController.OWNER_UNDEFINED);
            this.searchController.addDragSupport(1, null);
            this.searchController.addDropSupport(1, null);
            buttonComp = this.tlk().addComposite(searchTab, null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this, this.searchController, false, true, true, true);
            this.tlk().addSpace(buttonComp, 1);
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.searchPreferences != null && WalletEditor.this.searchPreferences.isBound() && WalletEditor.this.searchPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.searchPreferences.save();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Store, "de-toem-toolkits-tlk-css-general-save-image");
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.searchPreferences != null && WalletEditor.this.searchPreferences.isBound() && WalletEditor.this.searchPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.searchPreferences.load();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Reset, null);
            ITlkComposite partsTab = this.tlk().addComposite(tabFolder, null, 2, null, 0, I18n.General_Parts, "preferences.impulse.parts");
            this.partsController = this.tlk().addTree(partsTab, new CellTreeController(this, "partsCells"){

                @Override
                protected void selectionChanged() {
                    super.selectionChanged();
                    this.tlk().updateEnable();
                }

                @Override
                protected boolean sync(ICell cell, ITlkTreeItem item, int index) {
                    boolean changed = super.sync(cell, item, index);
                    if (item.getParentItem() == null) {
                        item.setText(I18n.General_Preferences);
                    }
                    return changed;
                }
            }.setKeepFirstItemExpanded(true).setShowRoot(true), this.tlk().ld(1, 524288, 1), 0x10000002, null, null).setOwner(IController.OWNER_UNDEFINED);
            this.partsController.addDragSupport(1, null);
            this.partsController.addDropSupport(1, null);
            buttonComp = this.tlk().addComposite(partsTab, null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this, this.partsController, false, true, true, true);
            this.tlk().addSpace(buttonComp, 1);
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.partsPreferences != null && WalletEditor.this.partsPreferences.isBound() && WalletEditor.this.partsPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.partsPreferences.save();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Store, "de-toem-toolkits-tlk-css-general-save-image");
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.partsPreferences != null && WalletEditor.this.partsPreferences.isBound() && WalletEditor.this.partsPreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.partsPreferences.load();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Reset, null);
            ITlkComposite nativesTab = this.tlk().addComposite(tabFolder, null, 2, null, 0, I18n.General_Native, "preferences.impulse.natives");
            this.nativesController = this.tlk().addTree(nativesTab, new CellTreeController(this, "nativesCells"){

                @Override
                protected void selectionChanged() {
                    super.selectionChanged();
                    this.tlk().updateEnable();
                }

                @Override
                protected boolean sync(ICell cell, ITlkTreeItem item, int index) {
                    boolean changed = super.sync(cell, item, index);
                    if (item.getParentItem() == null) {
                        item.setText(I18n.General_Preferences);
                    }
                    return changed;
                }
            }.setKeepFirstItemExpanded(true).setShowRoot(true), this.tlk().ld(1, 524288, 1), 0x10000002, null, null).setOwner(IController.OWNER_UNDEFINED);
            this.nativesController.addDragSupport(1, null);
            this.nativesController.addDropSupport(1, null);
            buttonComp = this.tlk().addComposite(nativesTab, null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this, this.nativesController, false, true, true, true);
            this.tlk().addSpace(buttonComp, 1);
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.nativePreferences != null && WalletEditor.this.nativePreferences.isBound() && WalletEditor.this.nativePreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.nativePreferences.save();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Store, "de-toem-toolkits-tlk-css-general-save-image");
            this.tlk().addButton(buttonComp, new ButtonController(this, null){

                @Override
                public boolean enabled() {
                    return WalletEditor.this.nativePreferences != null && WalletEditor.this.nativePreferences.isBound() && WalletEditor.this.nativePreferences.isDirty();
                }

                @Override
                public void execute(String id, Object data) {
                    WalletEditor.this.nativePreferences.load();
                }
            }, this.tlk().ld(1, true, false), 0, I18n.General_Reset, null);
            this.tlk().addSash(main, null, null, 2);
            ITlkComposite description = this.tlk().addComposite(main, null, 2, new TlkShareData(2), 0, null, null);
            this.tlk().addHtml(description, new HtmlController(this, Wallet.class.getField("description")), this.tlk().ld(1, 524288, 1), 0x100000, null);
            buttonComp = this.tlk().addComposite(description, null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
            this.tlk().addButton(buttonComp, new CommandButtonController(this, "de.toem.impulse.commands.wallet.install", this, false), this.tlk().ld(1, true, false), 0, I18n.General_Install, null);
            this.tlk().addButton(buttonComp, new CommandButtonController(this, "de.toem.impulse.commands.wallet.uninstall", this, false), this.tlk().ld(1, true, false), 0, I18n.General_Uninstall, null);
            this.tlk().addButton(buttonComp, new CommandButtonController(this, "de.toem.impulse.commands.wallet.update", this, false), this.tlk().ld(1, true, false), 0, I18n.General_Update, null);
            super.createControls(container);
        }
        catch (Throwable e) {
            SystemLog.log(e);
        }
    }

    @Override
    protected boolean filterUpdate(ElementModifierEvent event) {
        return false;
    }

    @Override
    protected void updateControls(ElementModifierEvent event) {
        super.updateControls(event);
    }

    @Override
    protected void updateLocalControls(ElementModifierEvent event) {
        super.updateLocalControls(event);
        if (this.mainLayoutController == null || this.topLayoutController == null || this.tabFolderController == null) {
            return;
        }
        if (this.mainLayoutController != null && this.topLayoutController != null && this.tabFolderController != null && this.editorElement.isBound() && this.editorElement.hasCell()) {
            this.mainLayoutController.update(this.editorElement.getCell(), false);
            this.topLayoutController.update(this.editorElement.getCell(), false);
            this.tabFolderController.update(this.editorElement.getCell(), false);
        }
        if (this.viewsController != null && this.viewPreferences.isBound() && this.viewPreferences.hasCell()) {
            this.viewsController.update(this.viewPreferences.getCell(), false);
        }
        if (this.portController != null && this.portPreferences.isBound() && this.portPreferences.hasCell()) {
            this.portController.update(this.portPreferences.getCell(), false);
        }
        if (this.serializerController != null && this.serializerPreferences.isBound() && this.serializerPreferences.hasCell()) {
            this.serializerController.update(this.serializerPreferences.getCell(), false);
        }
        if (this.chartsController != null && this.chartsPreferences.isBound() && this.chartsPreferences.hasCell()) {
            this.chartsController.update(this.chartsPreferences.getCell(), false);
        }
        if (this.templatesController != null && this.templatesPreferences.isBound() && this.templatesPreferences.hasCell()) {
            this.templatesController.update(this.templatesPreferences.getCell(), false);
        }
        if (this.searchController != null && this.searchPreferences.isBound() && this.searchPreferences.hasCell()) {
            this.searchController.update(this.searchPreferences.getCell(), false);
        }
        if (this.partsController != null && this.partsPreferences.isBound() && this.partsPreferences.hasCell()) {
            this.partsController.update(this.partsPreferences.getCell(), false);
        }
        if (this.nativesController != null && this.nativePreferences.isBound() && this.nativePreferences.hasCell()) {
            this.nativesController.update(this.nativePreferences.getCell(), false);
        }
    }

    @Override
    public Object command(String id, Object data, int doIt, Object sender) {
        if (id.equals("de.toem.impulse.commands.configuration.add")) {
            if (doIt != 5) {
                return false;
            }
        } else if (id.equals("de.toem.impulse.commands.wallet.install")) {
            if (doIt != 5) {
                return this.doWalletInstall(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.wallet.uninstall")) {
            if (doIt != 5) {
                return this.doWalletUninstall(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.wallet.update")) {
            if (doIt != 5) {
                return this.doWalletUpdate(data, doIt, sender);
            }
        } else if (doIt == 5) {
            return false;
        }
        if (doIt == 5) {
            return true;
        }
        return null;
    }

    private Object doWalletInstall(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            Wallet wallet = null;
            if (this.getElement().isBound() && this.getElement().hasCell(Wallet.class) && this.resources.isBound()) {
                wallet = (Wallet)this.getElement().getCell();
                if (wallet == null || !wallet.installable) {
                    return false;
                }
                if (doIt == 0) {
                    ArrayList<IElementModifier> modifiers = new ArrayList<IElementModifier>();
                    StringBuilder checkMessages = new StringBuilder();
                    boolean ok = wallet.createModifiers(1, null, checkMessages, null);
                    if (!ok) {
                        String cfr_ignored_0 = "Found installation problems:" + Utils.lineSeparator + checkMessages.toString() + Utils.lineSeparator + "Shall continue/force installation?";
                    }
                    StringBuilder installMessages = new StringBuilder();
                    wallet.createModifiers(1, modifiers, installMessages, null);
                    this.apply(I18n.General_Install, null, modifiers.toArray(new IElementModifier[modifiers.size()]), true);
                    IExecutable stimulation = new IExecutable(){

                        @Override
                        public void execute(IProgress p) {
                            IScripting scripting = Scripting.create((Wallet)WalletEditor.this.getElement().getCell(), "script", s -> {});
                            scripting.run(null);
                        }
                    };
                    Actives.run(stimulation);
                    Ide.openInformation(I18n.General_Install, installMessages.toString());
                }
            } else {
                return true;
            }
            return true;
        }
        return null;
    }

    private Object doWalletUninstall(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            Wallet wallet = null;
            if (this.getElement().isBound() && this.getElement().hasCell(Wallet.class) && this.resources.isBound()) {
                wallet = (Wallet)this.getElement().getCell();
                if (wallet == null || !wallet.installable) {
                    return false;
                }
                if (doIt == 0) {
                    ArrayList<IElementModifier> modifiers = new ArrayList<IElementModifier>();
                    StringBuilder installMessages = new StringBuilder();
                    wallet.createModifiers(2, modifiers, installMessages, null);
                    this.apply(I18n.General_Uninstall, null, modifiers.toArray(new IElementModifier[modifiers.size()]), true);
                    Ide.openInformation(I18n.General_Uninstall, installMessages.toString());
                }
            } else {
                return true;
            }
            return true;
        }
        return null;
    }

    private Object doWalletUpdate(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            Wallet wallet = null;
            if (this.getElement().isBound() && this.getElement().hasCell(Wallet.class) && this.resources.isBound()) {
                wallet = (Wallet)this.getElement().getCell();
                if (wallet == null || !wallet.installable) {
                    return false;
                }
                if (doIt == 0) {
                    ArrayList<IElementModifier> modifiers = new ArrayList<IElementModifier>();
                    StringBuilder installMessages = new StringBuilder();
                    wallet.createModifiers(2, modifiers, installMessages, null);
                    this.apply(I18n.General_Update, null, modifiers.toArray(new IElementModifier[modifiers.size()]), true);
                    Ide.openInformation(I18n.General_Update, installMessages.toString());
                }
            } else {
                return true;
            }
            return true;
        }
        return null;
    }
}

