/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.controller;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.cells.view.CursorConfiguration;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.paint.ICursorItem;
import de.toem.impulse.paint.IPlotTree;
import de.toem.impulse.paint.controller.PlotTreeController;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.synchronization.AbstractCell2ObjectSynchronizer;
import de.toem.toolkits.ui.tlk.TLK;

public class ViewerCursorItemSynchronizer
extends AbstractCell2ObjectSynchronizer<ICursorItem>
implements PlotTreeController.ICursorItemSynchronizer {
    private TLK tlk;
    private IPlotTree plotTree;

    public ViewerCursorItemSynchronizer(TLK tlk, IPlotTree plotTree) {
        super(false, true, new Class[]{CursorConfiguration.class});
        this.tlk = tlk;
        this.plotTree = plotTree;
    }

    @Override
    public void synchronize(ICell presentation) {
        this.synchronize(presentation, this.plotTree);
    }

    @Override
    protected Iterable<ICursorItem> getTargetChildren(Object sourceObject, Object targetObject) {
        if (targetObject instanceof IPlotTree) {
            return ((IPlotTree)targetObject).getCursorItems();
        }
        return this.EMPTY;
    }

    @Override
    protected boolean remove(ICursorItem object) {
        object.dispose();
        return true;
    }

    @Override
    protected ICursorItem add(ICell sourceChild, Object targetObject, int index) {
        if (targetObject instanceof IPlotTree && sourceChild instanceof ICell) {
            ICursorItem item = this.plotTree.create(ICursorItem.class, 0);
            item.setData(sourceChild);
            if (sourceChild.getElement().isBound()) {
                DomainValue value = DomainValue.parse(sourceChild.getElement().getHint("POSITION"));
                if (value != null) {
                    item.setPosition(value);
                } else {
                    IDomainAxis axis = ((IPlotTree)targetObject).getActiveAxis();
                    if (axis != null && axis.getDomainBase() != DomainBase.Unknown) {
                        value = new DomainValue(axis.getDomainBase(), (long)axis.units(axis.getPadding()));
                        item.setPosition(value);
                    } else {
                        value = new DomainValue(TimeBase.s, 0L);
                        item.setPosition(value);
                    }
                }
            }
            return item;
        }
        return null;
    }

    @Override
    protected boolean matches(ICell sourceChild, ICursorItem targetChild, int index) {
        return sourceChild == targetChild.getData();
    }

    @Override
    public boolean sync(ICell sourceChild, ICursorItem targetChild, int index) {
        boolean changed = false;
        ICursorItem item = targetChild;
        String label = "";
        Object image = null;
        int color = -1;
        if (sourceChild instanceof CursorConfiguration) {
            CursorConfiguration configuration = (CursorConfiguration)sourceChild;
            color = configuration.color;
            label = configuration.getName();
        }
        changed |= item.setText(label);
        changed |= item.setColor(color);
        return changed |= item.setImage(image);
    }
}

