/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.impulse.cells.ports.IPortProviderFactory;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.serializer.ICellReader;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.Closeable;

public interface IPortAdapter
extends IPortProviderFactory,
ICell {
    public boolean validate(ICell var1);

    public Closeable getInput(IProgress var1);

    public ICellReader newReader(Closeable var1);

    public int getNature();
}

