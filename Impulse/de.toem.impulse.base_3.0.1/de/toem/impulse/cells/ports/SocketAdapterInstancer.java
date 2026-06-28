/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.impulse.cells.ports.TcpAdapter;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.ui.instancer.AbstractDefaultInstancer;

public class SocketAdapterInstancer
extends AbstractDefaultInstancer {
    @Override
    public String getCellType() {
        return "port.record.socket";
    }

    @Override
    protected void initOne(String id, ICell cell, IElement preferences) {
        cell.setValue("syncScript", (Object)Scripting.loadScriptFromResources(TcpAdapter.class, "sync.js"));
        cell.setValue("stimulationScript", (Object)Scripting.loadScriptFromResources(TcpAdapter.class, "stimulate_socket.js"));
    }
}

