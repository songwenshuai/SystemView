/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.impulse.cells.record.PortScope;
import de.toem.toolkits.pattern.element.ICell;

public interface IPortAdapter3 {
    public void prepareInsertPoint(ICell var1);

    public boolean needsSync();

    public boolean sync(PortScope var1, ICell var2);
}

