/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.wallet;

import de.toem.impulse.cells.wallet.ResourceToolLauncher;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.RadioSetController;
import de.toem.toolkits.ui.controller.base.TabFolderController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.provider.control.NameDescriptionProvider;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.controls.ITlkGroup;
import de.toem.toolkits.ui.tlk.controls.ITlkTabFolder;

public class ResourceToolLauncherDialog
extends ControlProviderElementDialog {
    public ResourceToolLauncherDialog(ITlkPartContainer parent, int style) {
        super(parent, ResourceToolLauncherDialog.getControls(), style);
    }

    public ResourceToolLauncherDialog() {
    }

    public static IControlProvider getControls() {
        ITlkControlProvider provider = (ITlkControlProvider)new AbstractControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.wallet_tool_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    ITlkTabFolder scriptFolder = this.tlk().addTabFolder(this.container(), new TabFolderController(this.editor(), "de.toem.impulse.dialog.wallet.tool"), this.tlk().ldc(this.cols(), 4, 80, 4, 20), 0x100080, null);
                    ITlkComposite scriptTab = this.tlk().addComposite(scriptFolder, null, null, null, 0, String.valueOf(I18n.General_Linux) + " (bash)", null);
                    this.tlk().addText(scriptTab, new TextController(this.editor(), this.clazz().getField("linux")), null, 26, null);
                    scriptTab = this.tlk().addComposite(scriptFolder, null, null, null, 0, String.valueOf(I18n.General_Win32) + " (cmd)", null);
                    this.tlk().addText(scriptTab, new TextController(this.editor(), this.clazz().getField("win32")), null, 26, null);
                    scriptTab = this.tlk().addComposite(scriptFolder, null, null, null, 0, String.valueOf(I18n.General_Osx) + " (bash)", null);
                    this.tlk().addText(scriptTab, new TextController(this.editor(), this.clazz().getField("osx")), null, 26, null);
                    this.tlk().addText(this.container(), new TextController(this.editor(), this.clazz().getField("port")), 3, 0x100001, "Port:");
                    this.tlk().addButtonSet(this.container(), new RadioSetController(this.editor(), this.clazz().getField("mode")), 4, 3, 17, "Mode:", new String[]{"Disabled", "Debug only", "Run only", "Always"}, null);
                    ITlkGroup group = this.tlk().addGroup(this.container(), null, 3, 3, 0, "On Launch", null);
                    this.tlk().addButtonSet(group, new RadioSetController(this.editor(), this.clazz().getField("launchAction")), 4, 3, 17, "Action:", new String[]{"No action", "Connect only", "Start Streaming"}, null);
                    this.tlk().addButton(group, new CheckController(this.editor(), this.clazz().getField("launchRestart")), 3, 2048, "Restart if already running/connected", null);
                    this.tlk().addText(group, new TextController(this.editor(), this.clazz().getField("launchDelay")), 3, 0x100001, "Delay[ms]:");
                    this.tlk().addButton(group, new CheckController(this.editor(), this.clazz().getField("launchActivate")), 3, 2048, "Activate impulse viewer", null);
                    group = this.tlk().addGroup(this.container(), null, 3, 3, 0, "On Termination", null);
                    this.tlk().addButtonSet(group, new RadioSetController(this.editor(), this.clazz().getField("terminateAction")), 4, 3, 17, "Action:", new String[]{"No action", "Stop Streaming"}, null);
                    this.tlk().addButton(group, new CheckController(this.editor(), this.clazz().getField("terminateActivate")), 3, 2048, "Activate impulse viewer", null);
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
                return true;
            }
        }.insertBefore(new NameDescriptionProvider());
        provider.setCellClass(ResourceToolLauncher.class);
        return provider;
    }
}

