/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.controller;

import de.toem.impulse.axis.Axes;
import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.cells.ports.IPortProviderFactory;
import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.view.AxisConfiguration;
import de.toem.impulse.cells.view.FolderConfiguration;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.cells.view.ViewConfiguration;
import de.toem.impulse.dialog.view.DerivedSamplesContext;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.paint.IActiveValueProvider;
import de.toem.impulse.paint.IFolderItem;
import de.toem.impulse.paint.IPaintStyle;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.IPlotTree;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.controller.PlotTreeController;
import de.toem.impulse.provider.ISamplesCache;
import de.toem.impulse.provider.ISamplesContext;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesDisplayInformation;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.element.exploits.Markers;
import de.toem.toolkits.pattern.element.synchronization.AbstractCell2ObjectSynchronizer;
import de.toem.toolkits.pattern.element.synchronization.AbstractSynchronizer;
import de.toem.toolkits.ui.tlk.TLK;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class ViewerTreeItemSynchronizer
extends AbstractCell2ObjectSynchronizer<ITreeItem>
implements PlotTreeController.ITreeItemSynchronizer {
    private TLK tlk;
    private IPlotTree plotTree;
    private ISamplesContext context;
    private ICell record;
    private ICell view;
    private ICell top;
    private int sync;
    private boolean resetOffset;
    private Map<Object, MakeVisible> makeVisible;
    private Set<ICell> makeVisibleCells;
    private Map<Object, ITreeItem> madeVisible;
    private ViewerHierarchicalSamplesTreeItemSynchronizer hierarchicalSamplesSynchronizer;

    public ViewerTreeItemSynchronizer(TLK tlk, IPlotTree plotTree) {
        super(true, true, new Class[]{ViewConfiguration.class, FolderConfiguration.class, PlotConfiguration.class});
        this.tlk = tlk;
        this.plotTree = plotTree;
        this.hierarchicalSamplesSynchronizer = new ViewerHierarchicalSamplesTreeItemSynchronizer();
    }

    public ICell getRecord() {
        return this.record;
    }

    public ICell getPresentation() {
        return this.view;
    }

    @Override
    public ICell getTop() {
        return this.top;
    }

    @Override
    public boolean synchronize(ICell record, ICell presentation, ICell top, ISamplesContext context, int sync) {
        if (context == null || context.getSamplesCache() == null) {
            return false;
        }
        this.record = record;
        this.view = presentation;
        this.top = top;
        this.sync = sync;
        if (sync == 0 || sync == -1) {
            this.context = new DerivedSamplesContext(context, 1);
            ISamplesCache cache = this.context.getSamplesCache();
            HashSet<String> domainStrings = new HashSet<String>();
            if (record != null) {
                for (ICell cell : record.getTribe(true, Signal.class)) {
                    domainStrings.add(((Signal)cell).domainBase);
                }
            }
            if (presentation != null) {
                for (ICell cell : presentation.getTribe(true, PlotConfiguration.class)) {
                    domainStrings.add(((PlotConfiguration)cell).domainBase);
                }
            }
            for (String domainString : domainStrings) {
                IDomainBase base = DomainBase.parse(domainString);
                if (base == DomainBase.Unknown) continue;
                if (cache.get(base.getClazz()) != null) {
                    if (!base.isFinerThan(cache.get(base.getClazz()))) continue;
                    cache.put(base.getClazz(), base);
                    continue;
                }
                cache.put(base.getClazz(), base);
            }
        } else if (sync != 4 && (sync == 1 || sync == 2)) {
            this.context = new DerivedSamplesContext(context, 2);
        }
        boolean result = this.synchronizeFromFirstChild(top, this.plotTree);
        if (this.context instanceof DerivedSamplesContext) {
            ((DerivedSamplesContext)this.context).apply();
        }
        return result;
    }

    @Override
    public boolean synchronize(int sync) {
        this.sync = sync;
        return this.synchronizeFromFirstChild(this.top, this.plotTree);
    }

    @Override
    public boolean synchronize(ICell sourceChild, ITreeItem targetChild, int sync) {
        this.sync = sync;
        boolean changed = this.sync(sourceChild, targetChild, targetChild.index());
        boolean recursive = this.recursive(sourceChild, targetChild);
        if (recursive) {
            changed |= this.synchronize((Object)sourceChild, (Object)targetChild, false);
        }
        return changed;
    }

    @Override
    public Map<Object, ITreeItem> makeVisible(List<?> sources) {
        this.makeVisible = new HashMap<Object, MakeVisible>();
        for (Object source : sources) {
            if (source instanceof ICell) {
                this.makeVisible.put((ICell)source, new MakeVisible((ICell)source, null));
                continue;
            }
            if (!(source instanceof Link)) continue;
            PlotConfiguration cell = null;
            String cid = null;
            Link link = (Link)source;
            if (PlotConfiguration.targetsPlot(link)) {
                cell = PlotConfiguration.getSourcePlot(link, this.view);
                cid = PlotConfiguration.getSourceChildId(link);
            } else if (PlotConfiguration.targetsSignal(link)) {
                for (ICell c : this.view.getTribe(false, PlotConfiguration.class)) {
                    PlotConfiguration p = (PlotConfiguration)c;
                    if (p.samples == null || p.isProduction() || !p.samples.equalTargets(link, this.record)) continue;
                    cell = p;
                    break;
                }
            }
            if (cell == null) continue;
            this.makeVisible.put(link, new MakeVisible(cell, cid));
        }
        this.makeVisibleCells = new HashSet<ICell>();
        for (MakeVisible mv : this.makeVisible.values()) {
            this.makeVisibleCells.add(mv.cell);
        }
        this.sync = 0;
        this.madeVisible = new HashMap<Object, ITreeItem>();
        this.synchronizeFromFirstChild(this.top, this.plotTree);
        this.makeVisibleCells = null;
        this.makeVisible = null;
        return this.madeVisible;
    }

    @Override
    public void resetOffset(boolean reset) {
        this.resetOffset = reset;
    }

    protected void initExpandState(ITreeItem item) {
        Object data = item.getData();
        if (data instanceof ICell && ((ICell)data).getElement().isBound()) {
            String hint = ((ICell)data).getElement().getHint("EXPANDED");
            boolean expanded = Boolean.TRUE.toString().equals(hint) || !Boolean.FALSE.toString().equals(hint) && item.getParentItem() == null;
            item.setExpanded(expanded);
        }
    }

    @Override
    public void storeExpandState(ITreeItem item, boolean expanded) {
        Object virtual = item.getData("SOURCECHILD");
        Object data = null;
        if (virtual != null) {
            ITreeItem parent = item.getParentItem();
            while (parent != null && !(parent.getData() instanceof ICell)) {
                parent = parent.getParentItem();
            }
            data = parent != null ? parent.getData() : null;
        } else {
            data = item.getData();
        }
        if (data instanceof ICell && ((ICell)data).getElement().isBound()) {
            ((ICell)data).getElement().setHint("EXPANDED" + (virtual != null ? String.valueOf(virtual) : ""), String.valueOf(expanded));
        }
    }

    @Override
    protected Iterable<ITreeItem> getTargetChildren(Object sourceObject, Object targetObject) {
        if (targetObject instanceof IPlotTree) {
            return ((IPlotTree)targetObject).getItems();
        }
        if (targetObject instanceof ITreeItem) {
            return ((ITreeItem)targetObject).getChildItems();
        }
        return this.EMPTY;
    }

    @Override
    protected boolean matches(ICell sourceChild, ITreeItem targetChild, int index) {
        return sourceChild == targetChild.getData();
    }

    @Override
    protected boolean remove(ITreeItem object) {
        object.dispose();
        return true;
    }

    @Override
    protected boolean recursive(ICell sourceChild, ITreeItem targetChild) {
        if (targetChild.isExpandable() && !targetChild.isExpanded() && this.makeVisibleCells != null && this.makeVisible(sourceChild)) {
            targetChild.setExpanded(true);
        }
        if (targetChild.isExpandable() && !targetChild.isExpanded()) {
            if (targetChild.areChildrenSynced()) {
                targetChild.setChildrenSynced(false);
            }
            return false;
        }
        if (!targetChild.areChildrenSynced()) {
            targetChild.setChildrenSynced(true);
        }
        return !this.isCompound(sourceChild, targetChild);
    }

    protected boolean makeVisible(ICell sourceChild) {
        for (ICell v : this.makeVisibleCells) {
            if (!v.getParents(null).contains(sourceChild)) continue;
            return true;
        }
        return false;
    }

    private boolean isCompound(ICell sourceChild, ITreeItem targetChild) {
        IPlotItem samplesItem;
        IReadableSamples readable;
        if (sourceChild.hasChildren(PlotConfiguration.class)) {
            return false;
        }
        if (targetChild instanceof IPlotItem && (readable = (samplesItem = (IPlotItem)targetChild).getSamples()) != null) {
            return sourceChild instanceof PlotConfiguration && ((PlotConfiguration)sourceChild).hasChildSamples(null, readable);
        }
        return false;
    }

    @Override
    protected ITreeItem add(ICell sourceChild, Object targetObject, int index) {
        ITreeItem item = null;
        if (sourceChild instanceof PlotConfiguration) {
            if (targetObject instanceof ITreeItem) {
                item = this.plotTree.create(IPlotItem.class, (ITreeItem)targetObject, 0);
            } else if (targetObject instanceof IPlotTree) {
                item = this.plotTree.create(IPlotItem.class, 0);
            }
        } else if (sourceChild instanceof FolderConfiguration) {
            if (targetObject instanceof ITreeItem) {
                item = this.plotTree.create(IFolderItem.class, (ITreeItem)targetObject, 0);
            } else if (targetObject instanceof IPlotTree) {
                item = this.plotTree.create(IFolderItem.class, 0);
            }
        } else if (sourceChild instanceof ViewConfiguration && targetObject instanceof IPlotTree) {
            item = this.plotTree.create(IFolderItem.class, 0);
        }
        if (item != null) {
            item.setData(sourceChild);
            this.initExpandState(item);
        }
        return item;
    }

    @Override
    protected boolean sync(ICell sourceChild, ITreeItem targetChild, int index) {
        IPlotItem samplesItem;
        IReadableSamples readable;
        boolean changed = false;
        boolean tag = false;
        ISamplesCache cached = this.context.getSamplesCache();
        if (cached == null) {
            return false;
        }
        if (this.sync == 3) {
            IPlotItem plotItem;
            if (targetChild instanceof IPlotItem && (plotItem = (IPlotItem)targetChild).getSamples() != null && plotItem.getSamples().getRelease() != plotItem.getSamplesRelease()) {
                changed |= plotItem.setSamples(plotItem.getSamples());
            }
        } else if (this.sync == 2 || this.sync == 1) {
            if (sourceChild instanceof PlotConfiguration && targetChild instanceof IPlotItem) {
                PlotConfiguration configuration = (PlotConfiguration)sourceChild;
                IPlotItem plotItem = (IPlotItem)targetChild;
                if (plotItem.getSamples() == null || !plotItem.getSamples().isReleased() || plotItem.getSamples().isVolatile() || this.sync == 1) {
                    IReadableSamples readable2 = configuration.getSamples(this.context);
                    changed |= plotItem.setSamples(readable2);
                    tag = readable2 != null ? readable2.hasTag() : false;
                    ISamples.TagDomain tagDomain = tag && readable2 != null ? readable2.getTagDomain() : null;
                }
            }
        } else {
            boolean needToSetAxisMap;
            AxisConfiguration dc;
            if (targetChild.index() != index) {
                if (targetChild.getParentItem() != null) {
                    targetChild.getParentItem().moveItem(targetChild, index);
                }
                changed = true;
            }
            changed |= targetChild.setImage(sourceChild.getIconId());
            changed |= targetChild.setText(sourceChild.getLabel());
            if (sourceChild instanceof PlotConfiguration) {
                changed |= targetChild.setColor(((PlotConfiguration)sourceChild).color);
            }
            HashMap<String, IDomainAxis> axisMap = null;
            if (sourceChild instanceof AxisConfiguration && ((dc = (AxisConfiguration)sourceChild) == this.top || dc.axisMode != 0) && this.record != null) {
                axisMap = new HashMap<String, IDomainAxis>();
                IDomainBase extraBase = this.getExtraBase(dc);
                if (extraBase != null) {
                    axisMap.put(extraBase.getClazz(), Axes.create(dc.axisType, extraBase));
                } else {
                    for (String string : cached.getDomainClasses()) {
                        IDomainBase base = cached.get(string);
                        axisMap.put(string, Axes.create(dc.axisType, base));
                    }
                }
            }
            boolean bl = needToSetAxisMap = targetChild.getDomainAxisMap() == null && axisMap != null || targetChild.getDomainAxisMap() != null && axisMap == null;
            if (!needToSetAxisMap && targetChild.getDomainAxisMap() != null && axisMap != null) {
                boolean bl2 = needToSetAxisMap = !targetChild.getDomainAxisMap().keySet().equals(axisMap.keySet());
                if (!needToSetAxisMap) {
                    for (String dc2 : axisMap.keySet()) {
                        needToSetAxisMap |= !Utils.equals(targetChild.getDomainAxisMap().get(dc2).getDomainBase(), ((IDomainAxis)axisMap.get(dc2)).getDomainBase());
                        if (!(needToSetAxisMap |= !Utils.equals(targetChild.getDomainAxisMap().get(dc2).getClass(), ((IDomainAxis)axisMap.get(dc2)).getClass()))) break;
                    }
                }
            }
            if (needToSetAxisMap) {
                if (axisMap != null) {
                    for (String domainClazz : axisMap.keySet()) {
                        long offset;
                        ICell hintCell = ViewerTreeItemSynchronizer.getAxisHintCell(sourceChild);
                        if (hintCell == null || !hintCell.getElement().isBound()) continue;
                        long factor = Utils.parseLong(hintCell.getElement().getHint("FACTOR" + ((IDomainAxis)axisMap.get(domainClazz)).getClazz() + domainClazz), Long.MIN_VALUE);
                        if (factor == Long.MIN_VALUE) {
                            factor = Utils.parseInt(hintCell.getElement().getHint("FACTOR" + domainClazz), 0);
                        }
                        if ((offset = Utils.parseLong(hintCell.getElement().getHint("OFFSET" + ((IDomainAxis)axisMap.get(domainClazz)).getClazz() + domainClazz), Long.MIN_VALUE)) == Long.MIN_VALUE) {
                            offset = Utils.parseLong(hintCell.getElement().getHint("OFFSET" + domainClazz), 0L);
                        }
                        if (offset != 0L && this.resetOffset) {
                            offset = 0L;
                            hintCell.getElement().setHint("OFFSET" + domainClazz, "");
                        }
                        ((IDomainAxis)axisMap.get(domainClazz)).setZoom(factor);
                        ((IDomainAxis)axisMap.get(domainClazz)).setUnitOffset(offset);
                    }
                }
                targetChild.setDomainAxisMap(axisMap);
                changed = true;
            }
            if (sourceChild instanceof PlotConfiguration && targetChild instanceof IPlotItem) {
                int preferedHeight;
                Signal signal;
                PlotConfiguration configuration = (PlotConfiguration)sourceChild;
                IPlotItem iPlotItem = (IPlotItem)targetChild;
                if (!Utils.equals(targetChild.getDescription(), configuration.description)) {
                    targetChild.setDescription(configuration.description);
                    changed = true;
                }
                if (!configuration.isProduction() && (signal = this.getSignal(configuration.samples)) != null) {
                    Object serializer = signal.getData("SERIALIZER");
                    if (serializer instanceof IPortProviderFactory) {
                        IActiveValueProvider painter = (IActiveValueProvider)((IPortProviderFactory)serializer).getProvider(IActiveValueProvider.class, signal);
                        if (iPlotItem.getActiveValueProvider() != painter) {
                            iPlotItem.setActiveValuePainter(painter);
                            changed = true;
                        }
                    }
                    Markers markers = null;
                    if (signal.hasMarkers()) {
                        markers = new Markers();
                        signal.fillMarkers(markers);
                    }
                    if (!Utils.equals(iPlotItem.getMarkers(), markers)) {
                        iPlotItem.setMarkers(markers);
                        changed = true;
                    }
                    if (!Utils.equals(iPlotItem.getMarkerStyle(), configuration.markerPresentation)) {
                        iPlotItem.setMarkerStyle(configuration.markerPresentation);
                        changed = true;
                    }
                    ISamples.SignalType.valueOf(signal);
                    ISamples.SignalDescriptor.valueOf(signal);
                }
                Object readable3 = null;
                readable3 = configuration.getSamples(this.context);
                changed |= iPlotItem.setSamples((IReadableSamples)readable3);
                tag = readable3 != null ? readable3.hasTag() : false;
                ISamples.TagDomain tagDomain = tag && readable3 != null ? readable3.getTagDomain() : null;
                IPaintStyle style = configuration.getPaintStyle();
                changed |= iPlotItem.setPaintStyle(style, style.getType() == 8 && this.sync == -1);
                int n = preferedHeight = configuration.preferedHeight ? configuration.preferedHeightValue : -1;
                if (preferedHeight != iPlotItem.getPreferedHeight()) {
                    iPlotItem.setPreferedHeight(preferedHeight);
                    changed = true;
                }
                boolean combined = false;
                if (ITreeItem.isCombinable(configuration.style)) {
                    combined = configuration.combine;
                }
                if (combined != iPlotItem.isCombined()) {
                    iPlotItem.setCombined(combined);
                    changed = true;
                }
                boolean domainAxis = true;
                if (configuration.style == 8) {
                    domainAxis = false;
                }
                if (domainAxis != iPlotItem.hasDomainAxis()) {
                    iPlotItem.setDomainAxis(domainAxis);
                    changed = true;
                }
            }
            boolean expandable = this.hasSourceChildren(sourceChild, null) || this.isCompound(sourceChild, targetChild);
            changed |= targetChild.setExpandable(expandable);
            if (this.makeVisible != null && this.makeVisibleCells != null && this.madeVisible != null && this.makeVisibleCells.contains(sourceChild)) {
                for (Object e : this.makeVisible.keySet()) {
                    MakeVisible mv = this.makeVisible.get(e);
                    if (mv.cell != sourceChild || mv.cid != null) continue;
                    this.madeVisible.put(e, targetChild);
                }
            }
            if (sourceChild instanceof FolderConfiguration && targetChild instanceof IFolderItem) {
                FolderConfiguration folderConfiguration = (FolderConfiguration)sourceChild;
                IFolderItem folderItem = (IFolderItem)targetChild;
                if (folderItem.getFolderMode() != folderConfiguration.folderMode) {
                    changed = true;
                    folderItem.setFolderMode(folderConfiguration.folderMode);
                }
            }
        }
        if (targetChild.isExpandable() && targetChild.isExpanded() && this.isCompound(sourceChild, targetChild) && (readable = (samplesItem = (IPlotItem)targetChild).getSamples()) != null && sourceChild instanceof PlotConfiguration && ((PlotConfiguration)sourceChild).hasChildSamples(null, readable)) {
            changed |= this.hierarchicalSamplesSynchronizer.synchronize(sourceChild, samplesItem, this.sync);
        }
        return changed;
    }

    protected static ICell getAxisHintCell(ICell child) {
        while (child != null) {
            if (child instanceof AxisConfiguration && ((AxisConfiguration)child).axisMode != 0) {
                return child;
            }
            if (child instanceof ViewConfiguration) {
                return child;
            }
            child = child.getParent();
        }
        return null;
    }

    protected IDomainBase getExtraBase(ICell child) {
        while (child != null) {
            if (child instanceof AxisConfiguration && ((AxisConfiguration)child).axisMode == 2) {
                return DomainBase.parse(((AxisConfiguration)child).domainBase);
            }
            child = child.getParent();
        }
        return null;
    }

    protected Signal getSignal(Link reference) {
        AbstractSignal source = null;
        if (reference != null) {
            if (source == null && this.record != null) {
                source = (AbstractSignal)reference.resolveCell(this.record, AbstractSignal.class);
            }
            if (source != null) {
                return source.getSignal(null);
            }
        }
        return null;
    }

    public static String getConfigurationImageId(String cellType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor) {
        String id = String.valueOf(cellType) + "." + String.valueOf((Object)signalType).toLowerCase();
        if (signalType == ISamples.SignalType.Logic && signalDescriptor.getScale() > 1) {
            id = String.valueOf(id) + ".n.16";
        } else if (signalType == ISamples.SignalType.Logic && signalDescriptor.getScale() == 1) {
            id = String.valueOf(id) + ".1.16";
        } else if (signalType != null) {
            id = String.valueOf(id) + ".16";
        }
        return id;
    }

    public static String getConfigurationImageId(String production) {
        return String.valueOf(production) + ".16";
    }

    class MakeVisible {
        ICell cell;
        String cid;
        ITreeItem item;

        public MakeVisible(ICell cell, String cid) {
            this.cell = cell;
            this.cid = Utils.isEmpty(cid) ? null : cid;
        }
    }

    class ViewerHierarchicalSamplesTreeItemSynchronizer
    extends AbstractSynchronizer<String, IPlotItem>
    implements PlotTreeController.ITreeItemSynchronizer.ICompoundTreeItemSynchronizer<IPlotItem> {
        protected ICell rootCell;
        protected IPlotItem rootItem;
        protected int sync;
        protected IReadableSamples readable;
        protected ISamples.TagDomain tagDomain;
        protected boolean added;
        private Set<String> makeVisibleCids;

        public ViewerHierarchicalSamplesTreeItemSynchronizer() {
            super(true, true);
        }

        @Override
        public boolean synchronize(ICell rootCell, IPlotItem rootItem, int sync) {
            this.rootCell = rootCell;
            this.rootItem = rootItem;
            this.sync = sync;
            this.added = false;
            this.readable = rootItem instanceof IPlotItem ? rootItem.getSamples() : null;
            boolean tag = this.readable != null ? this.readable.hasTag() : false;
            ISamples.TagDomain tagDomain = this.tagDomain = tag && this.readable != null ? this.readable.getTagDomain() : null;
            if (ViewerTreeItemSynchronizer.this.makeVisible != null && ViewerTreeItemSynchronizer.this.makeVisibleCells != null && ViewerTreeItemSynchronizer.this.makeVisibleCells.contains(rootCell)) {
                this.makeVisibleCids = new HashSet<String>();
                for (MakeVisible mv : ViewerTreeItemSynchronizer.this.makeVisible.values()) {
                    if (mv.cell != rootCell || mv.cid == null) continue;
                    this.makeVisibleCids.add(mv.cid);
                }
            }
            if (this.makeVisibleCids != null && this.makeVisibleCids.isEmpty()) {
                this.makeVisibleCids = null;
            }
            boolean result = this.synchronize(rootCell, rootItem);
            this.makeVisibleCids = null;
            return result;
        }

        @Override
        protected boolean matches(String sourceChild, IPlotItem targetChild, int index) {
            return sourceChild.equals(targetChild.getData("SOURCECHILD")) && this == targetChild.getData("SYNCHRONIZER");
        }

        @Override
        protected Iterable<IPlotItem> getTargetChildren(Object sourceObject, Object targetObject) {
            if (targetObject instanceof ITreeItem) {
                return ((ITreeItem)targetObject).getChildItems();
            }
            return this.EMPTY;
        }

        @Override
        protected boolean hasSourceChildren(Object sourceObject, Object targetObject) {
            if (!(this.rootCell instanceof PlotConfiguration)) {
                return false;
            }
            if (sourceObject == this.rootCell) {
                return ((PlotConfiguration)this.rootCell).hasChildSamples(null, this.readable);
            }
            if (sourceObject instanceof String) {
                return ((PlotConfiguration)this.rootCell).hasChildSamples((String)sourceObject, this.readable);
            }
            return false;
        }

        @Override
        protected Iterable<String> getSourceChildren(Object sourceObject, Object targetObject) {
            if (!(this.rootCell instanceof PlotConfiguration)) {
                return this.EMPTY;
            }
            if (sourceObject == this.rootCell) {
                return ((PlotConfiguration)this.rootCell).getChildSampleIds(null, this.readable);
            }
            if (sourceObject instanceof String) {
                return ((PlotConfiguration)this.rootCell).getChildSampleIds((String)sourceObject, this.readable);
            }
            return this.EMPTY;
        }

        @Override
        protected IPlotItem add(String sourceChild, Object targetObject, int index) {
            IPlotItem item = null;
            if (targetObject instanceof IPlotItem) {
                item = ViewerTreeItemSynchronizer.this.plotTree.create(IPlotItem.class, (IPlotItem)targetObject, 0);
                item.setData("SOURCECHILD", sourceChild);
                item.setData("SYNCHRONIZER", this);
                if (this.recursive) {
                    boolean expanded = Utils.parseBoolean(this.rootCell.getElement().getHint("EXPANDED" + String.valueOf(sourceChild)), false);
                    if (item.isExpanded() != expanded) {
                        item.setExpanded(expanded);
                    }
                }
            }
            this.added = true;
            return item;
        }

        @Override
        protected boolean remove(IPlotItem object) {
            object.dispose();
            return true;
        }

        @Override
        protected boolean sync(String sourceChild, IPlotItem targetChild, int index) {
            IReadableSamples samples;
            boolean changed = false;
            if (!this.added && this.sync == 3) {
                if (targetChild.getSamples() != null && targetChild.getSamples().getRelease() != targetChild.getSamplesRelease()) {
                    changed |= targetChild.setSamples(targetChild.getSamples());
                }
            } else if (!this.added && this.sync == 2 || this.sync == 1) {
                if (!targetChild.getSamples().isReleased() || targetChild.getSamples().isVolatile() || this.sync == 1) {
                    samples = ((PlotConfiguration)this.rootCell).getSamples(sourceChild, this.readable);
                    changed |= targetChild.setSamples(samples);
                }
            } else {
                IPaintStyle paintStyle;
                Object color;
                String label;
                samples = ((PlotConfiguration)this.rootCell).getSamples(sourceChild, this.readable);
                changed |= targetChild.setSamples(samples);
                boolean changeCompoundCell = false;
                String string = label = samples != null ? samples.getLabel() : sourceChild;
                if (!Utils.equals(targetChild.getText(), label)) {
                    targetChild.setText(label);
                    changed = true;
                    changeCompoundCell = true;
                }
                changed |= targetChild.setImage(samples.getIconId() != null ? samples.getIconId() : "configuration.samples.sub");
                Object object = color = samples instanceof ISamplesDisplayInformation ? ((ISamplesDisplayInformation)((Object)samples)).getColor() : null;
                if (color == null) {
                    color = this.rootItem.getColor();
                }
                if (!Utils.equals(targetChild.getColor(), color)) {
                    targetChild.setColor(color);
                    changed = true;
                    changeCompoundCell = true;
                }
                IPaintStyle iPaintStyle = paintStyle = samples instanceof ISamplesDisplayInformation ? ((ISamplesDisplayInformation)((Object)samples)).getPaintStyle() : null;
                if (paintStyle == null) {
                    IPaintStyle iPaintStyle2 = paintStyle = samples != null ? PlotConfiguration.getPaintStyle(samples) : null;
                }
                if (!Utils.equals(targetChild.getPaintStyle(), paintStyle)) {
                    targetChild.setPaintStyle(paintStyle, false);
                    changed = true;
                    changeCompoundCell = true;
                }
                int height = Utils.parseInt(this.rootCell.getElement().getHint("virtual.child.preferedHeight." + String.valueOf(sourceChild)), 0);
                if (!Utils.equals(targetChild.getPreferedHeight(), height)) {
                    targetChild.setPreferedHeight(height);
                    changed = true;
                }
                if (changeCompoundCell) {
                    targetChild.setData("COMPOUNDCELL", ((PlotConfiguration)this.rootCell).createSubConfiguration(sourceChild, this.readable));
                }
                if (ViewerTreeItemSynchronizer.this.makeVisible != null && this.makeVisibleCids != null && ViewerTreeItemSynchronizer.this.madeVisible != null && this.makeVisibleCids.contains(sourceChild)) {
                    for (Object obj : ViewerTreeItemSynchronizer.this.makeVisible.keySet()) {
                        MakeVisible mv = (MakeVisible)ViewerTreeItemSynchronizer.this.makeVisible.get(obj);
                        if (mv.cell != this.rootCell || !Utils.equals(mv.cid, sourceChild)) continue;
                        ViewerTreeItemSynchronizer.this.madeVisible.put(obj, targetChild);
                    }
                }
            }
            if (this.recursive) {
                boolean expandable = this.hasSourceChildren(sourceChild, null);
                if (targetChild.isExpandable() != expandable) {
                    targetChild.setExpandable(expandable);
                    changed = true;
                }
            }
            return changed;
        }

        @Override
        protected boolean recursive(String sourceChild, IPlotItem targetChild) {
            if (targetChild.isExpandable() && !targetChild.isExpanded() && this.makeVisibleCids != null && this.makeVisible(sourceChild)) {
                targetChild.setExpanded(true);
            }
            if (targetChild.isExpandable() && !targetChild.isExpanded()) {
                if (targetChild.areChildrenSynced()) {
                    targetChild.setChildrenSynced(false);
                }
                return false;
            }
            if (!targetChild.areChildrenSynced()) {
                targetChild.setChildrenSynced(true);
            }
            return true;
        }

        private boolean makeVisible(String sourceChild) {
            for (String cid : this.makeVisibleCids) {
                if (!cid.startsWith(sourceChild)) continue;
                return true;
            }
            return false;
        }
    }
}

