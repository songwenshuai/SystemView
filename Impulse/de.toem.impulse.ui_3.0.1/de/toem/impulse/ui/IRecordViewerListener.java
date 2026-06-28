/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.ui;

import de.toem.impulse.ui.IImpulsePartListener;
import de.toem.impulse.ui.IRecordViewer;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;

public interface IRecordViewerListener
extends IImpulsePartListener {
    public void inputSet(IRecordViewer var1, IElement var2, IElement var3, IElement var4);

    public void viewSet(IRecordViewer var1, Link var2, IElement var3);

    public void aboutToRefresh(IRecordViewer var1);
}

