/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.record.SignalProxy;
import de.toem.impulse.cells.view.FolderConfiguration;
import de.toem.impulse.cells.view.PlotConfigurationInstancer;
import de.toem.impulse.cells.view.PlotConfigurationTemplate;
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

public class FolderConfigurationInstancer
extends AbstractDefaultInstancer
implements IBaseSourcedInstancer {
    private List<IElement> sources;
    private List<PlotConfigurationTemplate> autoTemplates;

    @Override
    public String getCellType() {
        return "configuration.folder";
    }

    public FolderConfigurationInstancer() {
    }

    public FolderConfigurationInstancer(FolderConfigurationInstancer base, List<IElement> sources) {
        super(base);
        this.sources = sources;
        this.autoTemplates = base != null ? base.autoTemplates : null;
    }

    @Override
    public List<ICell> create(String id, ICell container, IElement preferences) {
        if (this.sources != null) {
            ArrayList<ICell> cells = new ArrayList<ICell>();
            if (this.autoTemplates == null) {
                this.autoTemplates = PlotConfigurationInstancer.getAutoTemplates();
            }
            for (IElement source : this.sources) {
                if (source.isBound() && source.hasCell(Scope.class)) {
                    ICell cell = FolderConfigurationInstancer.createCellFromSource((Scope)source.getCell(), preferences, this.autoTemplates);
                    if (cell == null) continue;
                    cells.add(cell);
                    continue;
                }
                if (!source.isBound() || !source.hasCell(AbstractSignal.class)) continue;
                AbstractSignal from = (AbstractSignal)source.getCell();
                PlotConfigurationInstancer.createCellsFromSignalSource(from, preferences, this.autoTemplates, cells, false);
            }
            return cells;
        }
        return super.create(id, container, preferences);
    }

    public static ICell createCellFromSource(Scope source, IElement preferences, List<PlotConfigurationTemplate> templates) {
        if (source == null) {
            return null;
        }
        FolderConfiguration cell = (FolderConfiguration)Elements.cells.create("configuration.folder");
        if (cell != null) {
            cell.setName(source.getName());
            cell.description = source.domainType;
            for (ICell child : source.getChildren()) {
                if (child instanceof Scope) {
                    ICell created = FolderConfigurationInstancer.createCellFromSource((Scope)child, preferences, templates);
                    if (created == null) continue;
                    cell.addChild(created);
                    continue;
                }
                if (!(child instanceof Signal) && !(child instanceof SignalProxy)) continue;
                ArrayList<ICell> childCells = new ArrayList<ICell>();
                PlotConfigurationInstancer.createCellsFromSignalSource((AbstractSignal)child, preferences, templates, childCells, false);
                for (ICell c : childCells) {
                    cell.addChild(c);
                }
            }
            return cell;
        }
        return null;
    }

    @Override
    public IInstancer<IInstancerInformation, IInformationGroup> forSource(Object[] source) {
        List<IElement> elements;
        if (source != null && source.length > 0 && Elements.haveCells(elements = Elements.getElements(source), Scope.class)) {
            return new FolderConfigurationInstancer(this, elements);
        }
        return null;
    }
}

