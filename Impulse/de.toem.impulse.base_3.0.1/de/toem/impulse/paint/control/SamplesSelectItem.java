/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.control;

import de.toem.impulse.paint.IPaintItem;
import de.toem.impulse.paint.ISelectItem;
import de.toem.impulse.paint.ITreeItem;
import de.toem.toolkits.core.Utils;

public class SamplesSelectItem
implements ISelectItem {
    public ITreeItem item;
    public int idx1;
    public int idx2;

    public SamplesSelectItem(ITreeItem item, int idx1) {
        this.item = item;
        this.idx1 = idx1;
        this.idx2 = idx1;
    }

    public SamplesSelectItem(ITreeItem item, int idx1, int idx2) {
        this.item = item;
        this.idx1 = idx1;
        this.idx2 = idx2;
    }

    public boolean equals(Object obj) {
        if (obj instanceof SamplesSelectItem) {
            SamplesSelectItem that = (SamplesSelectItem)obj;
            if (!Utils.equals(this.item, that.item)) {
                return false;
            }
            if (!Utils.equals(this.idx1, that.idx1)) {
                return false;
            }
            return Utils.equals(this.idx2, that.idx2);
        }
        return false;
    }

    @Override
    public IPaintItem item() {
        return this.item;
    }

    @Override
    public Object object() {
        return null;
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
        return false;
    }

    @Override
    public boolean isSamplesSelection() {
        return true;
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

