/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint;

import de.toem.impulse.paint.ITreeItem;

public interface IFolderItem
extends ITreeItem {
    public static final String FolderMode = "FolderMode";
    public static final String[] Contents = new String[]{"FolderMode"};

    public int getFolderMode();

    public void setFolderMode(int var1);
}

