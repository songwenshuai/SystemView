/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.ui;

import de.toem.impulse.ui.IRecordViewer;

public interface IRecordPortViewer
extends IRecordViewer {
    public void connectPort(boolean var1);

    public void streamPort(boolean var1);

    public void cancelPort();
}

