/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.impulse.cells.ports.AbstractSyncablePortAdapterCell;
import de.toem.impulse.cells.ports.IPortAdapter;
import de.toem.impulse.cells.ports.IPortAdapter2;
import de.toem.impulse.cells.ports.IRecordPort;
import de.toem.impulse.cells.preferences.ImpulsePorts;

public abstract class AbstractSyncableAdapter
extends AbstractSyncablePortAdapterCell
implements IRecordPort,
IPortAdapter,
IPortAdapter2 {
    @Override
    public boolean isPort() {
        return this.getParent() instanceof ImpulsePorts;
    }

    @Override
    public Object getProvider(Class<?> clazz, Object subject) {
        return null;
    }
}

