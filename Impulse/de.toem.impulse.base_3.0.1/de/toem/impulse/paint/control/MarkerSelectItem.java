/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.control;

import de.toem.impulse.paint.IPaintItem;
import de.toem.impulse.paint.ISelectItem;
import de.toem.impulse.paint.ITreeItem;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.exploits.Marker;

public class MarkerSelectItem
implements ISelectItem {
    public ITreeItem item;
    public Marker marker;

    public MarkerSelectItem(ITreeItem item, Marker marker) {
        this.item = item;
        this.marker = marker;
    }

    public boolean equals(Object obj) {
        if (obj instanceof MarkerSelectItem) {
            MarkerSelectItem that = (MarkerSelectItem)obj;
            if (!Utils.equals(this.item, that.item)) {
                return false;
            }
            return Utils.equals(this.marker, that.marker);
        }
        return false;
    }

    @Override
    public IPaintItem item() {
        return this.item;
    }

    @Override
    public Object object() {
        return this.marker;
    }

    @Override
    public boolean isTreeItemSelection() {
        return false;
    }

    @Override
    public boolean isPlotItemSelection() {
        return false;
    }

    @Override
    public boolean isCursorItemSelection() {
        return false;
    }

    @Override
    public boolean isMarkerSelection() {
        return true;
    }

    @Override
    public boolean isSamplesSelection() {
        return false;
    }

    @Override
    public Object getData() {
        return this.item.getData();
    }

    @Override
    public boolean isDisposed() {
        return this.item.isDisposed();
    }
}

