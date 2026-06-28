/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.view.SourceReference;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.instancer.IBaseSourcedInstancer;
import de.toem.toolkits.ui.instancer.AbstractDefaultInstancer;
import java.util.ArrayList;
import java.util.List;

public class SourceReferenceInstancer
extends AbstractDefaultInstancer
implements IBaseSourcedInstancer {
    private List<IElement> sources;

    public SourceReferenceInstancer() {
    }

    public SourceReferenceInstancer(SourceReferenceInstancer base, List<IElement> sources) {
        super(base);
        this.sources = sources;
    }

    @Override
    public String getCellType() {
        return "configuration.srcref";
    }

    @Override
    public List<ICell> create(String id, ICell container, IElement preferences) {
        if (this.sources != null) {
            ArrayList<ICell> cells = new ArrayList<ICell>();
            for (IElement source : this.sources) {
                ICell cell;
                if (!source.isBound() || !source.hasCell(AbstractSignal.class) || (cell = SourceReferenceInstancer.createCell((AbstractSignal)source.getCell(), preferences)) == null) continue;
                cells.add(cell);
            }
            return cells;
        }
        return super.create(id, container, preferences);
    }

    public static ICell createCell(AbstractSignal source, IElement preferences) {
        if (source == null) {
            return null;
        }
        Signal signal = source.getSignal(null);
        if (signal == null) {
            return null;
        }
        SourceReference cell = (SourceReference)Elements.cells.create("configuration.srcref");
        if (cell != null && source != null) {
            cell.setName(source.getName());
            cell.reference = source.getLink();
            return cell;
        }
        return null;
    }

    public SourceReferenceInstancer forSource(Object[] source) {
        List<IElement> elements;
        if (source != null && source.length > 0 && Elements.haveCells(elements = Elements.getElements(source), AbstractSignal.class)) {
            return new SourceReferenceInstancer(this, elements);
        }
        return null;
    }
}

