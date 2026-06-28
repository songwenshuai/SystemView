/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.cells.view.PlotConfigurationTemplate;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.ui.instancer.AbstractDefaultInstancer;

public class PlotConfigurationTemplateInstancer
extends AbstractDefaultInstancer {
    @Override
    public String getCellType() {
        return "template.configuration.samples";
    }

    @Override
    protected void initOne(String id, ICell cell, IElement preferences) {
        super.initOne(id, cell, preferences);
        if (cell instanceof PlotConfigurationTemplate) {
            ((PlotConfigurationTemplate)cell).doNotUpdate = true;
        }
    }
}

