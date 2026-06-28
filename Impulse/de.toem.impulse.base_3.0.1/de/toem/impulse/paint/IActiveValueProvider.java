/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint;

import de.toem.toolkits.ui.tlk.ITlkPainter;

public interface IActiveValueProvider {
    public boolean isActive();

    public String format(int var1);

    public boolean paint(ITlkPainter var1, int var2, int var3, int var4, int var5);
}

