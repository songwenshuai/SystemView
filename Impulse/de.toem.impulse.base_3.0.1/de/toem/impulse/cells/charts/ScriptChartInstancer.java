/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.charts;

import de.toem.impulse.cells.charts.ScriptChart;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.text.MultilineText;
import de.toem.toolkits.ui.instancer.AbstractDefaultInstancer;
import java.io.InputStream;

public class ScriptChartInstancer
extends AbstractDefaultInstancer {
    @Override
    public String getCellType() {
        return "chart.script";
    }

    @Override
    protected void initOne(String id, ICell cell, IElement preferences) {
        if (cell instanceof ScriptChart) {
            try {
                InputStream script = ScriptChart.class.getResourceAsStream("script.js");
                if (script != null) {
                    String raw = Utils.readStringFromInputStream(script, "UTF-8", true);
                    if (raw != null) {
                        ((ScriptChart)cell).script = MultilineText.toXml(raw.trim());
                    }
                    script.close();
                }
            }
            catch (Throwable throwable) {}
        }
    }
}

