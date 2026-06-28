/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.cells.view.AxisConfiguration;
import de.toem.toolkits.pattern.element.CellAnnotation;

@CellAnnotation(type="configuration.record", dynamicChildren={"configuration.samples", "configuration.folder", "configuration.cursor", "configuration.search"})
public class ViewConfiguration
extends AxisConfiguration {
    public static final String TYPE = "configuration.record";
    public boolean enabled = true;

    public ViewConfiguration() {
        this.axisMode = 1;
    }
}

