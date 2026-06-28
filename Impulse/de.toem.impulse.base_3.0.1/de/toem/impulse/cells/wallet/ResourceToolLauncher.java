/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.wallet;

import de.toem.impulse.cells.wallet.Wallet;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.constant.ConstantByteArrayElement;
import de.toem.toolkits.text.MultilineText;
import de.toem.toolkits.ui.tlk.TLK;

@CellAnnotation(type="wallet.resource.tool", dynamicChildren={})
public class ResourceToolLauncher
extends Cell {
    public static final String TYPE = "wallet.resource.tool";
    public String description;
    public String linux;
    public String win32;
    public String osx;
    public String port;
    public int mode;
    public int launchAction;
    public boolean launchRestart;
    public int launchDelay;
    public boolean launchActivate;
    public int terminateAction;
    public boolean terminateActivate;
    public String launchAddon;

    public ConstantByteArrayElement[] createConstantElements() {
        ConstantByteArrayElement[] result = new ConstantByteArrayElement[2];
        String path = ((Wallet)this.getRoot()).replace(String.valueOf(this.getElement().getDocumentContainer().getContainer().getPath(this.getElement().getRootContainer())) + this.getElement().getContainer().getPath(this.getElement().getDocumentContainer()).replace("\\", "/"));
        String working = "${workspace_loc:/" + path + "}";
        String execute = "";
        String arguments = "";
        String script = null;
        String scriptName = ((Wallet)this.getRoot()).replace(this.getName());
        if (TLK.isLinux()) {
            script = this.linux;
            scriptName = String.valueOf(scriptName) + ".sh";
            execute = "/bin/bash";
            arguments = "&quot;${workspace_loc:/" + path + "/" + scriptName + "}&quot;";
        } else if (TLK.isWin32()) {
            script = this.win32;
            scriptName = String.valueOf(scriptName) + ".bat";
            execute = "${workspace_loc:/" + path + "/" + scriptName + "}";
            arguments = "";
        } else if (TLK.isOsX()) {
            script = this.osx;
            scriptName = String.valueOf(scriptName) + ".sh";
            execute = "/bin/bash";
            arguments = "&quot;${workspace_loc:/" + path + "/" + scriptName + "}&quot;";
        }
        script = ((Wallet)this.getRoot()).replace(script);
        result[0] = new ConstantByteArrayElement(scriptName, 2, script != null ? MultilineText.toAscii(script) : Utils.lineSeparator + Utils.lineSeparator);
        String launcherName = String.valueOf(((Wallet)this.getRoot()).replace(this.getName())) + ".launch";
        String launcher = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" + Utils.lineSeparator + "<launchConfiguration type=\"org.eclipse.ui.externaltools.ProgramLaunchConfigurationType\">" + Utils.lineSeparator + "<booleanAttribute key=\"de.toem.impulse.launchactivateLaunch\" value=\"" + this.launchActivate + "\"/>" + Utils.lineSeparator + "<booleanAttribute key=\"de.toem.impulse.launchactivateTermination\" value=\"" + this.terminateActivate + "\"/>" + Utils.lineSeparator + "<intAttribute key=\"de.toem.impulse.launchdelayLaunch\" value=\"" + this.launchDelay + "\"/>" + Utils.lineSeparator + "<intAttribute key=\"de.toem.impulse.launchlaunch\" value=\"" + this.launchAction + "\"/>" + Utils.lineSeparator + "<intAttribute key=\"de.toem.impulse.launchmode\" value=\"" + this.mode + "\"/>" + Utils.lineSeparator + "<stringAttribute key=\"de.toem.impulse.launchport\" value=\"" + ((Wallet)this.getRoot()).replace(this.port) + "\"/>" + Utils.lineSeparator + "<booleanAttribute key=\"de.toem.impulse.launchrestart\" value=\"" + this.launchRestart + "\"/>" + Utils.lineSeparator + "<intAttribute key=\"de.toem.impulse.launchterminate\" value=\"" + this.terminateAction + "\"/>" + Utils.lineSeparator + "<listAttribute key=\"org.eclipse.debug.ui.favoriteGroups\">" + Utils.lineSeparator + "<listEntry value=\"org.eclipse.ui.externaltools.launchGroup\"/>" + Utils.lineSeparator + "</listAttribute>" + Utils.lineSeparator + "<stringAttribute key=\"org.eclipse.ui.externaltools.ATTR_LAUNCH_CONFIGURATION_BUILD_SCOPE\" value=\"${none}\"/>" + Utils.lineSeparator + "<stringAttribute key=\"org.eclipse.ui.externaltools.ATTR_LOCATION\" value=\"" + execute + "\"/>" + Utils.lineSeparator + "<stringAttribute key=\"org.eclipse.ui.externaltools.ATTR_WORKING_DIRECTORY\" value=\"" + working + "\"/>" + Utils.lineSeparator + "<stringAttribute key=\"org.eclipse.ui.externaltools.ATTR_TOOL_ARGUMENTS\" value=\"" + arguments + "\"/>" + (this.launchAddon != null ? this.launchAddon : "") + Utils.lineSeparator + "</launchConfiguration>" + Utils.lineSeparator;
        result[1] = new ConstantByteArrayElement(launcherName, 2, launcher);
        return result;
    }
}

