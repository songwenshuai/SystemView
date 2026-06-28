/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.cells.view.PlotConfigurationTemplate;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesCharacteristic;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.instancer.BaseInstancerInformation;
import de.toem.toolkits.pattern.element.instancer.IBaseContextInstancer;
import de.toem.toolkits.pattern.element.instancer.IBaseSourcedInstancer;
import de.toem.toolkits.pattern.information.InformationGroup;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.instancer.AbstractDefaultInstancer;
import de.toem.toolkits.ui.tlk.TLK;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class PlotConfigurationInstancer
extends AbstractDefaultInstancer
implements IBaseContextInstancer,
IBaseSourcedInstancer {
    private IController context;
    private List<? extends Object> sources;
    private List<PlotConfigurationTemplate> autoTemplates;

    public PlotConfigurationInstancer() {
    }

    protected PlotConfigurationInstancer(PlotConfigurationInstancer base, IController controller) {
        super(base);
        this.informations.clear();
        this.context = controller;
        this.informations.add(new BaseInstancerInformation(this, this.getDefaultGroup(), this.getDefaultId(), this.getLabel(), this.getDescription()));
        InformationGroup signals = new InformationGroup("signals", I18n.Instancer_Signals, null);
        this.groups.add(signals);
        IElement element = controller.getEditor().getElement();
        if (element != null && element.isBound() && element.hasCell()) {
            List<ICell> recordSignals = element.getCell().getTribe(false, AbstractSignal.class);
            int n = 0;
            for (ICell iCell : recordSignals) {
                this.informations.add(new BaseInstancerInformation(this, signals, "SIGNAL" + iCell.getPath(), iCell.getName(), String.valueOf(I18n.Instancer_AddPlotFromSignal) + " \"" + iCell.getName() + "\""));
                if (n++ > 100) break;
            }
        }
        InformationGroup defaultTemplates = new InformationGroup("templates", I18n.Instancer_DefaultTemplates, null);
        HashMap<String, InformationGroup> templateGroupMap = new HashMap<String, InformationGroup>();
        this.groups.add(defaultTemplates);
        if (ImpulsePreferences.templatePreferences != null && ImpulsePreferences.templatePreferences.isBound() && ImpulsePreferences.templatePreferences.hasCell()) {
            for (ICell iCell : ImpulsePreferences.templatePreferences.getCell().getChildren(PlotConfigurationTemplate.class)) {
                if (!((PlotConfigurationTemplate)iCell).enabled || !((PlotConfigurationTemplate)iCell).useInMenu) continue;
                InformationGroup g = defaultTemplates;
                if (!Utils.isEmpty(((PlotConfigurationTemplate)iCell).group)) {
                    String groupLabel = ((PlotConfigurationTemplate)iCell).group;
                    if (templateGroupMap.containsKey(groupLabel)) {
                        g = (InformationGroup)templateGroupMap.get(groupLabel);
                    } else {
                        g = new InformationGroup(groupLabel, String.valueOf(groupLabel) + " " + I18n.General_Templates, null);
                        templateGroupMap.put(groupLabel, g);
                        this.groups.add(g);
                    }
                }
                this.informations.add(new BaseInstancerInformation(this, g, "TEMPLATE" + iCell.getName(), iCell.getName(), String.valueOf(I18n.Instancer_AddPlotFromTemplate) + " \"" + iCell.getName() + "\""));
            }
        }
    }

    public PlotConfigurationInstancer(PlotConfigurationInstancer base, List<? extends Object> sources) {
        super(base);
        this.sources = sources;
    }

    @Override
    public String getCellType() {
        return "configuration.samples";
    }

    @Override
    public boolean has(String id) {
        return super.has(id) || id.startsWith("SIGNAL");
    }

    @Override
    public final String getDialogId(String id) {
        if (id.startsWith("SIGNAL")) {
            return null;
        }
        return super.getDialogId(id);
    }

    @Override
    public List<ICell> create(String id, ICell container, IElement preferences) {
        ArrayList<ICell> cells = new ArrayList<ICell>();
        if (this.sources != null) {
            if (this.autoTemplates == null) {
                this.autoTemplates = PlotConfigurationInstancer.getAutoTemplates();
            }
            for (Object object : this.sources) {
                IElement sourceElement = Elements.getElement(object);
                if (sourceElement.isBound() && sourceElement.hasCell(AbstractSignal.class)) {
                    AbstractSignal asignal = (AbstractSignal)sourceElement.getCell();
                    PlotConfigurationInstancer.createCellsFromSignalSource(asignal, preferences, this.autoTemplates, cells, false);
                }
                if (!(object instanceof ITreeItem)) continue;
                this.createCellsFromTreeItemSource((ITreeItem)object, preferences, this.autoTemplates, cells);
            }
            return cells;
        }
        if (this.context != null && this.context.getEditor() != null && this.context.getEditor().getElement().isBound() && id.startsWith("SIGNAL")) {
            ICell iCell = this.context.getEditor().getElement().getCellByPath(id.substring("SIGNAL".length()));
            if (iCell instanceof AbstractSignal) {
                if (this.autoTemplates == null) {
                    this.autoTemplates = PlotConfigurationInstancer.getAutoTemplates();
                }
                PlotConfigurationInstancer.createCellsFromSignalSource((AbstractSignal)iCell, preferences, this.autoTemplates, cells, false);
            }
            return cells;
        }
        if (id.startsWith("TEMPLATE")) {
            ICell iCell;
            if (ImpulsePreferences.templatePreferences != null && ImpulsePreferences.templatePreferences.isBound() && ImpulsePreferences.templatePreferences.hasCell() && (iCell = ImpulsePreferences.templatePreferences.getCell().getChildByName(id.substring("TEMPLATE".length()))) instanceof PlotConfigurationTemplate && ((PlotConfigurationTemplate)iCell).enabled && ((PlotConfigurationTemplate)iCell).useInMenu) {
                for (ICell tcell : iCell.getChildren()) {
                    cells.add(tcell.clone());
                }
            }
            return cells;
        }
        return super.create(id, container, preferences);
    }

    public static List<PlotConfigurationTemplate> getAutoTemplates() {
        ArrayList<PlotConfigurationTemplate> templates = new ArrayList<PlotConfigurationTemplate>();
        if (ImpulsePreferences.templatePreferences != null && ImpulsePreferences.templatePreferences.isBound() && ImpulsePreferences.templatePreferences.hasCell()) {
            for (ICell iCell : ImpulsePreferences.templatePreferences.getCell().getChildren(PlotConfigurationTemplate.class)) {
                if (!((PlotConfigurationTemplate)iCell).enabled || !((PlotConfigurationTemplate)iCell).usePattern) continue;
                templates.add((PlotConfigurationTemplate)iCell);
            }
        }
        return templates;
    }

    public static boolean createCellsFromAutoTemplate(ISamplesCharacteristic source, IElement preferences, List<PlotConfigurationTemplate> templates, List<ICell> cells, boolean single) {
        boolean found = false;
        if (templates != null && !templates.isEmpty()) {
            for (PlotConfigurationTemplate template : templates) {
                if (!template.matches(source) || single && template.getTribe(false).size() != 1) continue;
                found = true;
                for (ICell tcell : template.getChildren()) {
                    ICell tclone = tcell.clone();
                    for (ICell clone : tclone.getTribe(true)) {
                        if (!clone.hasName()) {
                            clone.setName(source.getLabel());
                        } else if (clone.getName().contains("${name}")) {
                            clone.setName(clone.getName().replace("${name}", source.getLabel()));
                        }
                        if (!Utils.isEmpty(source.getDescription()) && Utils.isEmpty(clone.getValue("description", String.class))) {
                            clone.setValue("description", (Object)source.getDescription());
                        }
                        if (!(clone instanceof PlotConfiguration) || ((PlotConfiguration)clone).samples != null || !(source instanceof AbstractSignal)) continue;
                        ((PlotConfiguration)clone).samples = ((AbstractSignal)source).getLink();
                    }
                    cells.add(tclone);
                }
            }
        }
        return found;
    }

    public static boolean createCellsFromSignalSource(ISamplesCharacteristic source, IElement preferences, List<PlotConfigurationTemplate> templates, List<ICell> cells, boolean single) {
        if (source == null) {
            return false;
        }
        Signal signal = null;
        if (source instanceof AbstractSignal && (signal = ((AbstractSignal)source).getSignal()) == null) {
            return false;
        }
        if (PlotConfigurationInstancer.createCellsFromAutoTemplate(source, preferences, templates, cells, single)) {
            return true;
        }
        PlotConfiguration cell = (PlotConfiguration)Elements.cells.create("configuration.samples");
        if (cell != null && source != null) {
            cells.add(cell);
            cell.setName(source.getLabel());
            cell.samples = source instanceof AbstractSignal ? ((AbstractSignal)source).getLink() : null;
            ISamples.SignalType type = source.getSignalType();
            ISamples.SignalDescriptor signalDescriptor = source.getSignalDescriptor();
            cell.description = source.getDescription();
            cell.columnValueFormat = -1;
            cell.bValueFormat = -1;
            cell.cValueFormat = -1;
            cell.aValueFormat = 0;
            if (type == ISamples.SignalType.Logic && signalDescriptor.getScale() == 1) {
                cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.0", 255);
                cell.style = 1;
            } else if (type == ISamples.SignalType.Logic && signalDescriptor.getScale() > 1) {
                cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.1", 65280);
                cell.style = 2;
            } else if (type == ISamples.SignalType.Integer || type == ISamples.SignalType.IntegerArray) {
                cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.1", 65280);
                cell.style = 2;
            } else if (type == ISamples.SignalType.Float) {
                int colorIdx = 2;
                if (signal != null && signal.getParent() != null) {
                    colorIdx = (colorIdx + signal.getParent().indexOf(signal)) % 10;
                }
                cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample." + colorIdx, 65280);
                cell.style = 4;
                cell.interpolation = true;
                cell.preferedHeight = true;
                cell.preferedHeightValue = 60;
            } else if (type == ISamples.SignalType.FloatArray) {
                int colorIdx = 2;
                if (signal != null && signal.getParent() != null) {
                    colorIdx = (colorIdx + signal.getParent().indexOf(signal)) % 10;
                }
                cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample." + colorIdx, 65280);
                cell.style = 2;
            } else if (type == ISamples.SignalType.Event || type == ISamples.SignalType.EventArray) {
                if (signalDescriptor.isGantt()) {
                    cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.9", 65280);
                    cell.style = 10;
                    cell.aModifier = true;
                    cell.relation = true;
                    cell.annotation = true;
                    cell.preferedHeightValue = 1;
                    cell.preferedHeight = false;
                    cell.bValueFormat = 0;
                    cell.columnValueFormat = -1;
                } else {
                    cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.7", 65280);
                    cell.style = 3;
                }
            } else if (type == ISamples.SignalType.Struct) {
                if (signalDescriptor.isTransaction()) {
                    cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.9", 65280);
                    cell.style = 5;
                    cell.relation = false;
                    cell.preferedHeight = true;
                    cell.preferedHeightValue = 60;
                } else if (signalDescriptor.isGantt()) {
                    cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.9", 65280);
                    cell.style = 10;
                    cell.aModifier = true;
                    cell.relation = true;
                    cell.annotation = true;
                    cell.relation = true;
                    cell.preferedHeightValue = 1;
                    cell.preferedHeight = false;
                    cell.bValueFormat = 0;
                    cell.columnValueFormat = -1;
                } else {
                    cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.9", 65280);
                    cell.style = 6;
                    cell.preferedHeight = true;
                    cell.preferedHeightValue = 60;
                }
            } else if (type == ISamples.SignalType.Text || type == ISamples.SignalType.TextArray) {
                cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.4", 65280);
                cell.style = 2;
            } else if (type == ISamples.SignalType.Binary && signalDescriptor.isImage()) {
                cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.3", 65280);
                cell.style = 7;
                cell.preferedHeight = true;
                cell.preferedHeightValue = 100;
            } else if (type == ISamples.SignalType.Binary) {
                cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.3", 65280);
                cell.style = 2;
                cell.preferedHeight = true;
            } else {
                cell.style = 0;
                cell.color = 0xFFFFFF;
            }
            return true;
        }
        return false;
    }

    private void createCellsFromTreeItemSource(ITreeItem source, IElement preferences, List<PlotConfigurationTemplate> autoTemplates2, List<ICell> cells) {
        if (source != null && source.getData("COMPOUNDCELL") instanceof PlotConfiguration) {
            cells.add(((PlotConfiguration)source.getData("COMPOUNDCELL")).clone());
        }
    }

    public PlotConfigurationInstancer forContext(Object context) {
        if (context instanceof IController) {
            return new PlotConfigurationInstancer(this, (IController)context);
        }
        return null;
    }

    public PlotConfigurationInstancer forSource(Object[] sources) {
        if (sources != null && sources.length > 0) {
            List<IElement> elements = Elements.getElements(sources);
            if (Elements.haveCells(elements, AbstractSignal.class)) {
                return new PlotConfigurationInstancer(this, elements);
            }
            ArrayList<ITreeItem> items = new ArrayList<ITreeItem>();
            Object[] objectArray = sources;
            int n = sources.length;
            int n2 = 0;
            while (n2 < n) {
                Object s = objectArray[n2];
                if (!(s instanceof ITreeItem) || ((ITreeItem)s).getData() instanceof ICell) {
                    return null;
                }
                items.add((ITreeItem)s);
                ++n2;
            }
            return new PlotConfigurationInstancer(this, items);
        }
        return null;
    }
}

