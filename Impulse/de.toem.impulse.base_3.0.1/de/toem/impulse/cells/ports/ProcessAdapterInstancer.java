/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.impulse.cells.ports.ProcessAdapter;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.ui.instancer.AbstractDefaultInstancer;

public class ProcessAdapterInstancer
extends AbstractDefaultInstancer {
    @Override
    public String getCellType() {
        return "port.record.process";
    }

    @Override
    protected void initOne(String id, ICell cell, IElement preferences) {
        cell.setValue("syncScript", (Object)Scripting.loadScriptFromResources(ProcessAdapter.class, "sync.js"));
        cell.setValue("stimulationScript", (Object)Scripting.loadScriptFromResources(ProcessAdapter.class, "stimulate_process.js"));
    }
}

