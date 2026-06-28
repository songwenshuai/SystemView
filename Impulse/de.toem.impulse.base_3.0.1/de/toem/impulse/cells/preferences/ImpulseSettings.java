/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.preferences;

import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.preferences.AbstractPreferenceCell;

@CellAnnotation(type="preferences.impulse.settings", dynamicChildren={})
public class ImpulseSettings
extends AbstractPreferenceCell {
    public static final String TYPE = "preferences.impulse.settings";
    public String formatDecimal0Label = "Simple Decimal";
    public String formatDecimal0Format = "0.00";
    public String formatDecimal1Label = "Currency";
    public String formatDecimal1Format = ",###.## \u20ac";
    public String formatDecimal2Label = "Scientific";
    public String formatDecimal2Format = "0.00E0";
    public String formatDecimal3Label;
    public String formatDecimal3Format;
    public String formatDecimal4Label;
    public String formatDecimal4Format;
    public String formatDecimal5Label;
    public String formatDecimal5Format;
    public String formatDecimal6Label;
    public String formatDecimal6Format;
    public String formatDecimal7Label;
    public String formatDecimal7Format;
    public String preferredDomainUnit;
}

