/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.paint.IPaintItem;

public interface IPlotTreeMouseListener {
    public boolean mouseDoubleClick(int var1, int var2, int var3, int var4, IPaintItem var5, DomainValue var6);

    public boolean mouseDown(int var1, int var2, int var3, int var4, IPaintItem var5, DomainValue var6);

    public boolean mouseUp(int var1, int var2, int var3, int var4, IPaintItem var5, DomainValue var6);

    public boolean mouseHover(int var1, int var2, int var3, int var4, IPaintItem var5, DomainValue var6);
}

