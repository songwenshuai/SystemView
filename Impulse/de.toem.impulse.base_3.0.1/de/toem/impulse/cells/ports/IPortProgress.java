/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.toolkits.pattern.threading.IProgress;

public interface IPortProgress
extends IProgress {
    public boolean isStreaming();

    public void startStreaming();

    public boolean isUpdating();

    public boolean isIterating();

    public void doUpdate();
}

