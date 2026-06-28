/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.charts;

import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.properties.IPropertyModel;

public abstract class AbstractChartCell
extends Cell {
    public boolean enabled = true;
    public String description;
    public String[][] parameters;

    public IPropertyModel getPropertyModel(boolean configured) {
        return null;
    }
}

