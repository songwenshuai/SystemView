/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.cells.view.AxisConfiguration;
import de.toem.impulse.i18n.I18n;
import de.toem.toolkits.pattern.element.CellAnnotation;

@CellAnnotation(type="configuration.folder", dynamicChildren={"configuration.samples", "configuration.folder"}, properties={"imageExtension"})
public class FolderConfiguration
extends AxisConfiguration {
    public static final String TYPE = "configuration.folder";
    public static final int FOLDER_NORMAL = 0;
    public static final int FOLDER_ACCORDION = 1;
    public static final String[] FOLDER_OPTIONS = new String[]{I18n.Folder_Normal, I18n.Folder_Accordion};
    public int folderMode = 0;

    public String imageExtension() {
        return String.valueOf(this.folderMode == 1 ? ".acc" : "") + (this.axisMode != 0 ? ".ruler" : "");
    }
}

