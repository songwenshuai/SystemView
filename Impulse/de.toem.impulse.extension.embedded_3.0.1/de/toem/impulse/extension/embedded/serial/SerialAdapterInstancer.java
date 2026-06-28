/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.serial;

import de.toem.impulse.cells.ports.PipeAdapter;
import de.toem.impulse.extension.embedded.serial.SerialAdapter;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.ui.instancer.AbstractDefaultInstancer;

public class SerialAdapterInstancer
extends AbstractDefaultInstancer {
    @Override
    public String getCellType() {
        return "port.record.serial";
    }

    @Override
    protected void initOne(String id, ICell cell, IElement preferences) {
        cell.setValue("syncScript", (Object)Scripting.loadScriptFromResources(PipeAdapter.class, "sync.js"));
        cell.setValue("stimulationScript", (Object)Scripting.loadScriptFromResources(SerialAdapter.class, "stimulate.js"));
    }
}

