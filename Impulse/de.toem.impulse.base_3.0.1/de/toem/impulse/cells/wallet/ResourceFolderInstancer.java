/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.wallet;

import de.toem.impulse.cells.wallet.ResourceFileInstancer;
import de.toem.impulse.cells.wallet.ResourceFolder;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.instancer.IBaseSourcedInstancer;
import de.toem.toolkits.pattern.element.instancer.IInstancer;
import de.toem.toolkits.pattern.element.instancer.IInstancerInformation;
import de.toem.toolkits.pattern.information.IInformationGroup;
import de.toem.toolkits.ui.instancer.AbstractDefaultInstancer;
import java.util.ArrayList;
import java.util.List;

public class ResourceFolderInstancer
extends AbstractDefaultInstancer
implements IBaseSourcedInstancer {
    private List<IElement> sources;

    @Override
    public String getCellType() {
        return "wallet.resource.folder";
    }

    public ResourceFolderInstancer() {
    }

    public ResourceFolderInstancer(ResourceFolderInstancer base, List<IElement> sources) {
        super(base);
        this.sources = sources;
    }

    @Override
    public List<ICell> create(String id, ICell container, IElement preferences) {
        if (this.sources != null) {
            ArrayList<ICell> cells = new ArrayList<ICell>();
            for (IElement source : this.sources) {
                ICell cell;
                if (source.isBound() && (source.isFolder() || source.isProject())) {
                    cell = ResourceFolderInstancer.createCellFromSource(source, preferences);
                    if (cell == null) continue;
                    cells.add(cell);
                    continue;
                }
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
        ResourceFolder cell = (ResourceFolder)Elements.cells.create("wallet.resource.folder");
        if (cell != null) {
            cell.setName(source.getName());
            for (IElement child : source.getChildren()) {
                ICell created;
                if (child.isFolder()) {
                    created = ResourceFolderInstancer.createCellFromSource(child, preferences);
                    if (created == null) continue;
                    cell.addChild(created);
                    continue;
                }
                if (!child.isDocument() || (created = ResourceFileInstancer.createCellFromSource(child, preferences)) == null) continue;
                cell.addChild(created);
            }
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
                if (e.isDocument() || e.isFolder() || e.isProject()) continue;
                return null;
            }
            return new ResourceFolderInstancer(this, elements);
        }
        return null;
    }
}

