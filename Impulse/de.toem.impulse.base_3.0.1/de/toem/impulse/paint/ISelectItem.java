/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint;

import de.toem.impulse.paint.IPaintItem;

public interface ISelectItem {
    public IPaintItem item();

    public Object getData();

    public boolean isDisposed();

    public boolean isTreeItemSelection();

    public boolean isPlotItemSelection();

    public boolean isCursorItemSelection();

    public boolean isSamplesSelection();

    public boolean isMarkerSelection();

    public Object object();
}

