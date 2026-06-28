/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer.vcd;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.record.SignalProxy;
import de.toem.impulse.serializer.vcd.VcdVariable;
import de.toem.toolkits.pattern.element.ICell;
import java.util.List;
import java.util.Map;

public interface IVcdVariableSignalExtender<E> {
    public void extend(VcdVariable<E> var1, ICell var2, Signal var3, Map<ICell, List<VcdVariable<E>>> var4);

    public void extend(VcdVariable<E> var1, ICell var2, SignalProxy var3, Map<ICell, List<VcdVariable<E>>> var4);
}

