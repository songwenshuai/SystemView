/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.ui;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.provider.ISignalContext;
import de.toem.impulse.provider.ISimpleSamplesProvider;
import de.toem.impulse.ui.DomainPosition;
import de.toem.impulse.ui.IImpulsePart;
import de.toem.impulse.ui.IRecordViewerChildListener;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.part.editor.ITlkEditor;
import java.util.List;

public interface IRecordViewer
extends ITlkEditor,
IImpulsePart,
ISignalContext {
    public void setHint(String var1, String var2);

    public String getHint(String var1);

    public IElement getView();

    public void setView(Link var1);

    public Object command(String var1, Object var2, int var3, Object var4);

    public DomainValue getActiveCursorPosition();

    public void moveActiveCursor(DomainValue var1, ITlkPart var2);

    public void moveActiveCursor(DomainPosition var1, ITlkPart var2);

    public void highlightAttachment(ISimpleSamplesProvider var1, IAttachment var2);

    public List<ISimpleSamplesProvider> getSelectedSamplesProvider();

    @Override
    public void gotoTarget(Link var1);

    public void requestTarget(Link var1, IRecordViewerChildListener var2);

    public void addChildListener(IRecordViewerChildListener var1);

    public void removeChildListener(IRecordViewerChildListener var1);
}

