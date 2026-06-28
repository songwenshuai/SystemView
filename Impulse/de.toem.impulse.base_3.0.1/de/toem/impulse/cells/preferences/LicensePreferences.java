/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.preferences;

import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellAnnotation;

@CellAnnotation(type="preferences.licence")
public class LicensePreferences
extends Cell {
    public static final String TYPE = "preferences.licence";
    public String key;
    public String server;
    public String port;
    public String properties;
    public long evalStarted;
}

