/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.wallet;

import de.toem.impulse.cells.wallet.ResourceFile;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.instancer.IBaseSourcedInstancer;
import de.toem.toolkits.pattern.element.instancer.IInstancer;
import de.toem.toolkits.pattern.element.instancer.IInstancerInformation;
import de.toem.toolkits.pattern.information.IInformationGroup;
import de.toem.toolkits.ui.instancer.AbstractDialogInstancer;
import java.util.ArrayList;
import java.util.List;

public class ResourceFileInstancer
extends AbstractDialogInstancer<IInstancerInformation, IInformationGroup>
implements IBaseSourcedInstancer {
    private List<IElement> sources;

    @Override
    public String getCellType() {
        return "wallet.resource.file";
    }

    public ResourceFileInstancer() {
    }

    public ResourceFileInstancer(ResourceFileInstancer base, List<IElement> sources) {
        super(base);
        this.sources = sources;
    }

    @Override
    public List<ICell> create(String id, ICell container, IElement preferences) {
        if (this.sources != null) {
            ArrayList<ICell> cells = new ArrayList<ICell>();
            for (IElement source : this.sources) {
                ICell cell;
                if (!source.isBound() || !source.isDocument() || (cell = ResourceFileInstancer.createCellFromSource(source, preferences)) == null) continue;
                cells.add(cell);
            }
            return cells;
        }
        return super.create(id, container, preferences);
    }

    public static ICell createCellFromSource(IElement source, IElement preferences) {
        if (source == null) {
            return null;
        }
        ResourceFile cell = (ResourceFile)Elements.cells.create("wallet.resource.file");
        if (cell != null) {
            cell.setName(source.getName());
            cell.content = Utils.readBinaryFromInputStream(source.getResourceData(null));
            return cell;
        }
        return null;
    }

    @Override
    public IInstancer<IInstancerInformation, IInformationGroup> forSource(Object[] source) {
        if (source != null && source.length > 0) {
            List<IElement> elements = Elements.getElements(source);
            for (IElement e : elements) {
                if (!e.isBound()) {
                    return null;
                }
                if (e.isDocument()) continue;
                return null;
            }
            return new ResourceFileInstancer(this, elements);
        }
        return null;
    }
}

