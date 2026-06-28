/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.provider;

import de.toem.impulse.cells.record.Signal;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.provider.IContext;
import de.toem.toolkits.pattern.provider.IProvider;

public interface ISignalProvider
extends ICell,
IProvider {
    public Signal getSignal();

    public Signal getSignal(IContext var1);
}

