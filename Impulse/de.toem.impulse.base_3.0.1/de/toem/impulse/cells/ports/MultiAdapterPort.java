/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.impulse.cells.ports.AbstractPortAdapterBaseCell;
import de.toem.impulse.cells.ports.IPortAdapter;
import de.toem.impulse.cells.ports.IRecordPort;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.ICell;

@CellAnnotation(type="port.records")
public class MultiAdapterPort
extends AbstractPortAdapterBaseCell
implements IRecordPort {
    public static final String TYPE = "port.records";

    @Override
    public boolean isPort() {
        return true;
    }

    @Override
    public int getNature() {
        int nature = this.hasChildren() ? Integer.MAX_VALUE : 0;
        for (ICell iCell : this.getChildren(IPortAdapter.class)) {
            nature &= ((IPortAdapter)iCell).getNature();
        }
        return nature;
    }
}

