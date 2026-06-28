/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cell.parts;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.preferences.AbstractSubViewPreferenceCell;
import de.toem.impulse.dialog.sample.SampleDialog;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.preferences.ClassPreferences;

@CellAnnotation(type="impulse.subview.sample", dynamicChildOf={"preferences.impulse.parts"})
public class SampleViewPreferences
extends AbstractSubViewPreferenceCell {
    public static final String TYPE = "impulse.subview.sample";
    public boolean showDefaultFormat = true;
    public boolean showNonAvailableFormats;
    public boolean showLabels = true;
    public boolean showAssocs = true;
    public boolean showGroupSamples = true;
    public long showInfos = 131072L;
    public static IElement defaultPersistence;
    public static SampleViewPreferences defaultPreferences;

    static {
        defaultPreferences = new SampleViewPreferences();
    }

    public static IElement getDefaultPersistence() {
        if (defaultPersistence == null) {
            defaultPersistence = ClassPreferences.initializeElement(SampleDialog.ISampleDialogInput.class, "persitence.impulse.subview");
        }
        return defaultPersistence;
    }

    public static SampleViewPreferences getDefaultPreferences() {
        ICell sampleViewInputPersistenceCell;
        if (SampleViewPreferences.getDefaultPersistence().isBound() && (sampleViewInputPersistenceCell = SampleViewPreferences.getDefaultPersistence().getCell()) != null) {
            return (SampleViewPreferences)ImpulsePreferences.partsPreferences.getCellByLink(sampleViewInputPersistenceCell.getValue("partPreferences", Link.class), SampleViewPreferences.class);
        }
        return defaultPreferences;
    }
}

