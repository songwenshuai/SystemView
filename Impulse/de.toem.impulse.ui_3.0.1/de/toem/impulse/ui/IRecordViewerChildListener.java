/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.ui;

import de.toem.impulse.ui.DomainPosition;
import de.toem.toolkits.ui.part.ITlkPart;
import java.util.List;

public interface IRecordViewerChildListener {
    public void positionChanged(DomainPosition var1, ITlkPart var2);

    public void signalsChanged(List<Object> var1, ITlkPart var2);
}

