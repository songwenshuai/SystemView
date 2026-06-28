/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.ImpulseLicense;
import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.view.AbstractTreeConfiguration;
import de.toem.impulse.cells.view.AxisConfiguration;
import de.toem.impulse.cells.view.SourceReference;
import de.toem.impulse.cells.view.ViewConfiguration;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.paint.IPaintStyle;
import de.toem.impulse.paint.plan.PaintStyle;
import de.toem.impulse.provider.IHierarchicalSamplesProvider;
import de.toem.impulse.provider.ISamplesCache;
import de.toem.impulse.provider.ISamplesContext;
import de.toem.impulse.provider.ISignalContext;
import de.toem.impulse.provider.ISignalProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.IReferencedReadableSamples;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesCharacteristic;
import de.toem.impulse.samples.ISamplesDisplayInformation;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.base.ReferencedReadableSamples;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.impulse.samples.convert.SampleConverterConfiguration;
import de.toem.impulse.samples.producer.LogicExtract;
import de.toem.impulse.samples.producer.MemberExtract;
import de.toem.impulse.samples.producer.NoneProducer;
import de.toem.impulse.samples.producer.SamplesProducerDescriptor;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.FieldAnnotation;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.provider.IContext;
import de.toem.toolkits.pattern.scripting.IScriptContextProvider;
import de.toem.toolkits.ui.tlk.TLK;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

@CellAnnotation(type="configuration.samples", dynamicChildren={"configuration.srcref"}, properties={"imageExtension"})
public class PlotConfiguration
extends AbstractTreeConfiguration
implements ISignalProvider,
IHierarchicalSamplesProvider,
ISamplesDisplayInformation,
IScriptContextProvider {
    public static final String TYPE = "configuration.samples";
    public static final String RESOURCE_CLASS_VIEW = "View";
    public static final String LINK_KEY_CHILD = "cid";
    public Link samples;
    public String production;
    @FieldAnnotation(affects={"signalType"})
    public String processType;
    @FieldAnnotation(affects={"signalDescriptor"})
    public String signalType;
    public String signalDescriptor;
    public String domainBase;
    public String start;
    public String end;
    public String rate;
    @FieldAnnotation(affectedBy={"production"})
    public String[][] parameters;
    @FieldAnnotation(affectedBy={"production"}, language="language")
    public String definition;
    public String language;
    public static final int LOGIC_INTERPRET_UNSIGNED_INTEGER = 0;
    public static final int LOGIC_INTERPRET_SIGNED_INTEGER = 1;
    public static final int LOGIC_INTERPRET_754 = 2;
    public int dataInterpretation;
    public boolean transformLinear;
    public float transformLinearM;
    public float transformLinearB;
    public int color = 0xCCCCCC;
    public static final String[] STYLE_LABELS = new String[]{I18n.Plot_None, I18n.Plot_Logic, I18n.Plot_Vector, I18n.Plot_Event, I18n.Plot_Line, I18n.Plot_Transaction, I18n.Plot_Log, I18n.Plot_Image, I18n.Plot_Chart, I18n.Plot_Area, I18n.Plot_Gant};
    public static final String[] STYLE_IMAGES = new String[]{"configuration.samples-none", "configuration.samples-logic", "configuration.samples-vector", "configuration.samples-event", "configuration.samples-line", "configuration.samples-transaction", "configuration.samples-log", "configuration.samples-image", "configuration.samples-chart", "configuration.samples-area", "configuration.samples-gannt"};
    public static final int STYLE_NONE = 0;
    public static final int STYLE_LOGIC = 1;
    public static final int STYLE_VECTOR = 2;
    public static final int STYLE_EVENT = 3;
    public static final int STYLE_LINE = 4;
    public static final int STYLE_TRANSACTION = 5;
    public static final int STYLE_LOG = 6;
    public static final int STYLE_IMAGE = 7;
    public static final int STYLE_CHART = 8;
    public static final int STYLE_AREA = 9;
    public static final int STYLE_GANT = 10;
    public int style = 2;
    public boolean combine = false;
    public boolean axis = true;
    public boolean interpolation = false;
    public boolean annotation = false;
    public boolean relation = false;
    public boolean aModifier = false;
    public boolean bModifier = false;
    public Link styleDescriptor;
    @FieldAnnotation(affectedBy={"style", "styleDescriptor"})
    public String[][] styleParameters;
    public boolean preferedHeight = false;
    public int preferedHeightValue = 0;
    public static final String[] markerOptions = new String[]{I18n.Plot_Hide, I18n.Plot_Above, I18n.Plot_Within};
    public static final int MARKER_NONE = 0;
    public static final int MARKER_ABOVE = 1;
    public static final int MARKER_WITHIN = 2;
    public int markerPresentation = 1;
    public static final Object[] formatValueOptions = ISample.formatValueOptions;
    public static final String[] formatValueLabels = ISample.formatValueLabels;
    public static final Object[] formatCollectionOptions = ISample.formatCollectionOptions;
    public static final String[] formatCollectionLabels = ISample.formatCollectionLabels;
    public int aValueFormat = -1;
    public int bValueFormat = -1;
    public int cValueFormat = -1;
    public int columnValueFormat = -1;
    public static final int SCALE_TYPE_LINEAR = 0;
    public static final int SCALE_TYPE_LOG10 = 1;
    public boolean scale = false;
    public int scaleType = 0;
    public float scaleFrom = 0.0f;
    public float scaleTo = 1.0f;
    public String scaleUnit = "";
    boolean recursionDetector;
    public static final int MAX_COMPOUND = 256;

    public static String[] getProductionLabels() {
        ArrayList<String> labels = new ArrayList<String>();
        String[] stringArray = ISamplesProducer.all.getAllIds();
        int n = stringArray.length;
        int n2 = 0;
        while (n2 < n) {
            String id = stringArray[n2];
            labels.add(ISamplesProducer.all.getLabel(id));
            ++n2;
        }
        return labels.toArray(new String[labels.size()]);
    }

    public static Object[] getProductionOptions() {
        ArrayList<String> options = new ArrayList<String>();
        String[] stringArray = ISamplesProducer.all.getAllIds();
        int n = stringArray.length;
        int n2 = 0;
        while (n2 < n) {
            String id = stringArray[n2];
            options.add(id);
            ++n2;
        }
        return options.toArray();
    }

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        IReadableSamples samples;
        if (this.isProduction() && (samples = this.getSamples()) instanceof IScriptContextProvider) {
            ((IScriptContextProvider)((Object)samples)).provideToScriptContext(context);
        }
    }

    public String language() {
        return Utils.isEmpty(this.language) ? "js" : this.language;
    }

    public boolean isProduction() {
        return this.production != null;
    }

    public boolean hasValueAxis() {
        return this.style == 4 || this.style == 9;
    }

    @Override
    public String getDescription() {
        return this.description;
    }

    @Override
    public String getLabel() {
        String label = super.getLabel();
        if (this.isProduction()) {
            ArrayList<Cell> references = new ArrayList<Cell>();
            references.addAll(this.getChildren(SourceReference.class));
            references.add(0, this);
            label = String.valueOf(label) + "<";
            int n = 0;
            for (ICell iCell : references) {
                if (n > 0) {
                    label = String.valueOf(label) + ",";
                }
                if (n > 5) {
                    label = String.valueOf(label) + "...";
                    break;
                }
                if (iCell != null) {
                    Link l = iCell.getValue("samples", Link.class);
                    if (l == null) {
                        l = iCell.getValue("reference", Link.class);
                    }
                    if (l != null) {
                        label = String.valueOf(label) + l.getName();
                    }
                }
                ++n;
            }
            label = String.valueOf(label) + ">";
        }
        return label;
    }

    @Override
    public Signal getSignal() {
        return null;
    }

    @Override
    public Signal getSignal(IContext context) {
        if (this.samples != null && !RESOURCE_CLASS_VIEW.equals(this.samples.getResourceClass())) {
            return this.getSourceSignal(context, this.samples);
        }
        return null;
    }

    @Override
    public IReadableSamples getSamples() {
        return this.getSamples(null);
    }

    @Override
    public IReadableSamples getSamples(IContext context) {
        if (this.recursionDetector) {
            return null;
        }
        try {
            Object compound;
            IReadableSamples current;
            this.recursionDetector = true;
            ISamplesCache samplesCache = context instanceof ISamplesContext ? ((ISamplesContext)context).getSamplesCache() : null;
            int samplesMode = context instanceof ISamplesContext ? ((ISamplesContext)context).getSamplesMode() : 0;
            IReadableSamples iReadableSamples = current = samplesCache != null ? samplesCache.get(this) : null;
            if (current != null) {
                IReadableSamples iReadableSamples2 = current;
                return iReadableSamples2;
            }
            IReadableSamples previous = samplesCache != null ? samplesCache.getInvalidated(this) : null;
            IReadableSamples readable = null;
            IReadableSamples previousSamples = null;
            IReferencedReadableSamples previousConverter = null;
            if (!this.isProduction()) {
                ISamplesReader iSamplesReader = previousSamples = previous != null ? previous.getReader() : null;
                if (previous != previousSamples && previous instanceof IReferencedReadableSamples) {
                    previousConverter = (IReferencedReadableSamples)previous;
                }
                readable = this.getSource(context, samplesCache, samplesMode, this.samples);
            } else {
                SamplesProducerDescriptor descr;
                List<IReadableSamples> sources;
                IReadableSamples iReadableSamples3 = previousSamples = previous != null ? previous.getProducer() : null;
                if (previous != previousSamples && previous instanceof IReferencedReadableSamples) {
                    previousConverter = (IReferencedReadableSamples)previous;
                }
                List<IReadableSamples> previousSources = null;
                if (previousSamples instanceof ISamplesProducer) {
                    previousSources = ((ISamplesProducer)previousSamples).getSources();
                }
                if (Utils.equals(sources = this.getSources(context, samplesCache, samplesMode), previousSources)) {
                    sources = previousSources;
                } else {
                    previousSamples = null;
                }
                readable = !ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.productions", "de.toem.impulse.feature.default", this.production) ? ((descr = (SamplesProducerDescriptor)ISamplesProducer.all.get(this.production)) != null ? this.updateProducer(context, samplesCache, samplesMode, descr, (ISamplesProducer)previousSamples, sources) : this.updateNoneProducer(context, samplesCache, samplesMode, (ISamplesProducer)previousSamples, sources, 1)) : this.updateNoneProducer(context, samplesCache, samplesMode, (ISamplesProducer)previousSamples, sources, 2);
            }
            ISamples result = null;
            if (readable != null) {
                SampleConverterConfiguration formatterConfiguration = SampleConverter.createConverterConfiguration(readable, this);
                if (formatterConfiguration == null) {
                    result = readable;
                } else if (previousConverter != null && Utils.equals(previousConverter.getConverterConfiguration(), formatterConfiguration) && previousConverter.getReference() == readable) {
                    result = previousConverter;
                } else {
                    ReferencedReadableSamples formatter = new ReferencedReadableSamples(readable, formatterConfiguration);
                    result = formatter;
                }
                if (samplesCache != null) {
                    samplesCache.put(this, readable);
                }
            }
            Object object = compound = result != null ? result.getData("COMPOUND") : null;
            if (compound instanceof ISamplesProducer.IMasterProducer && ((ISamplesProducer.IMasterProducer)compound).update(result) == -1) {
                result.setData("COMPOUND", null);
            }
            ISamples iSamples = result;
            return iSamples;
        }
        finally {
            this.recursionDetector = false;
        }
    }

    private List<IReadableSamples> getSources(IContext context, ISamplesCache samplesCache, int samplesMode) {
        ArrayList<IReadableSamples> sources = new ArrayList<IReadableSamples>();
        sources.add(this.getSource(context, samplesCache, samplesMode, this.samples));
        List<SourceReference> references = this.getChildren(SourceReference.class);
        int n = 0;
        while (n < references.size()) {
            SourceReference reference = references.get(n);
            if (reference != null && reference.enabled) {
                sources.add(this.getSource(context, samplesCache, samplesMode, reference.reference));
            }
            ++n;
        }
        return sources;
    }

    private IReadableSamples getSource(IContext context, ISamplesCache samplesCache, int samplesMode, Link reference) {
        ICell source = this.getSourceCell(context, reference);
        if (source instanceof Signal) {
            return this.getSignalSource(context, samplesCache, samplesMode, (Signal)source);
        }
        if (source instanceof PlotConfiguration) {
            return ((PlotConfiguration)source).getSamples(reference.getParameter(LINK_KEY_CHILD), context);
        }
        return null;
    }

    private IReadableSamples getSignalSource(IContext context, ISamplesCache samplesCache, int samplesMode, Signal signal) {
        IReadableSamples readable;
        IDomainBase signalBase = DomainBase.valueOf(signal);
        IDomainBase readerBase = this.getReaderBase(signalBase, samplesCache);
        IReadableSamples iReadableSamples = readable = samplesCache != null ? samplesCache.get(signal) : null;
        if (readable != null && readable.getDomainBase() != readerBase) {
            readable = null;
        }
        if (readable == null) {
            IReadableSamples previous = samplesCache != null ? samplesCache.getInvalidated(signal) : null;
            readable = !(previous instanceof ISamplesReader) || ((ISamplesReader)previous).update(signal, readerBase) < 0 ? PackedSamples.createReader(signal, readerBase) : previous;
            if (samplesCache != null) {
                samplesCache.put(signal, readable);
            }
        }
        return readable;
    }

    public ICell getSourceCell(IContext context, Link reference) {
        if (reference != null) {
            if (this.samples != null && RESOURCE_CLASS_VIEW.equals(this.samples.getResourceClass())) {
                return this.getSourcePlot(context, reference);
            }
            return this.getSourceSignal(context, reference);
        }
        return null;
    }

    private PlotConfiguration getSourcePlot(IContext context, Link reference) {
        ICell base = this.getParent(ViewConfiguration.class);
        return base != null && reference != null ? (PlotConfiguration)reference.resolveCell(base, PlotConfiguration.class) : null;
    }

    private Signal getSourceSignal(IContext context, Link reference) {
        IElement record = context instanceof ISignalContext ? ((ISignalContext)context).getRecord() : null;
        ICell base = record != null && record.isBound() ? record.getCell() : null;
        ISignalProvider signal = base != null && reference != null ? (ISignalProvider)reference.resolveCell(base, ISignalProvider.class) : null;
        return signal != null ? signal.getSignal(context) : null;
    }

    private IDomainBase getReaderBase(IDomainBase signalBase, ISamplesCache cached) {
        ICell child = this;
        while (child != null) {
            if (child instanceof AxisConfiguration && ((AxisConfiguration)child).axisMode == 2) {
                return DomainBase.parse(((AxisConfiguration)child).domainBase);
            }
            child = child.getParent();
        }
        if (signalBase != null && signalBase != DomainBase.Unknown && cached != null && cached.get(signalBase.getClazz()) != null) {
            return cached.get(signalBase.getClazz());
        }
        return signalBase;
    }

    public static Link getSourceLink(ICell source) {
        if (source instanceof AbstractSignal) {
            return source.getLink(Record.class);
        }
        if (source instanceof PlotConfiguration) {
            return source.getLink(ViewConfiguration.class, RESOURCE_CLASS_VIEW);
        }
        return null;
    }

    public static boolean targetsPlot(Link link) {
        return link != null && RESOURCE_CLASS_VIEW.equals(link.getResourceClass());
    }

    public static boolean targetsSignal(Link link) {
        return link != null && !RESOURCE_CLASS_VIEW.equals(link.getResourceClass());
    }

    public static ICell getSourceCell(Link link, ICell base) {
        if (link != null) {
            if (RESOURCE_CLASS_VIEW.equals(link.getResourceClass()) && base instanceof ViewConfiguration) {
                return link.resolveCell(base, PlotConfiguration.class);
            }
            if (base instanceof Record) {
                return link.resolveCell(base, AbstractSignal.class);
            }
        }
        return null;
    }

    public static PlotConfiguration getSourcePlot(Link link, ICell base) {
        return link != null && base instanceof ViewConfiguration ? (PlotConfiguration)link.resolveCell(base, PlotConfiguration.class) : null;
    }

    public static AbstractSignal getSourceSignal(Link link, ICell base) {
        return link != null && base instanceof Record ? (AbstractSignal)link.resolveCell(base, AbstractSignal.class) : null;
    }

    public static String getSourceChildId(Link link) {
        return link != null ? link.getParameter(LINK_KEY_CHILD) : null;
    }

    public static ICell getSourceContainer(Link link, ICell base) {
        if (link != null) {
            if (link.hasPath()) {
                link = Link.fromPath(IElement.extractContainerPath(link.getPath()));
            }
            if (RESOURCE_CLASS_VIEW.equals(link.getResourceClass()) && base instanceof ViewConfiguration) {
                return link.resolveCell(base);
            }
            if (base instanceof Record) {
                return link.resolveCell(base);
            }
        }
        return null;
    }

    @Override
    public boolean hasChildSamples(String subId, IContext context) {
        IReadableSamples samples = this.getSamples(context);
        if (samples == null) {
            return false;
        }
        if (samples.getProducer() instanceof ISamplesProducer.IMasterProducer) {
            return true;
        }
        return this.hasChildSamples(subId, samples);
    }

    @Override
    public List<String> getChildSampleIds(String subId, IContext context) {
        IReadableSamples samples = this.getSamples(context);
        if (samples == null) {
            return Collections.EMPTY_LIST;
        }
        return this.getChildSampleIds(subId, samples);
    }

    @Override
    public IReadableSamples getSamples(String subId, IContext context) {
        IReadableSamples samples = this.getSamples(context);
        if (subId == null) {
            return samples;
        }
        if (samples == null) {
            return null;
        }
        return this.getSamples(subId, samples);
    }

    private ISamplesProducer updateProducer(IContext context, final ISamplesCache samplesCache, int samplesMode, SamplesProducerDescriptor desc, ISamplesProducer previous, List<IReadableSamples> sources) {
        ISamplesProducer producer = null;
        if (samplesMode == 2 && previous instanceof ISamplesProducer && desc.update(previous, sources) >= 0) {
            producer = previous;
        }
        if (producer == null) {
            String id = "View:/" + this.getPath(this.getParent(ViewConfiguration.class));
            String name = this.getName();
            ISamples.ProcessType processType = ISamples.ProcessType.parse(this.processType);
            ISamples.SignalType signalType = ISamples.SignalType.parse(this.signalType);
            ISamples.SignalDescriptor signalDescriptor = Utils.isEmpty(this.signalDescriptor) ? null : ISamples.SignalDescriptor.parse(this.signalDescriptor);
            IDomainBase domainBase = DomainBase.parse(this.domainBase);
            IPropertyModel parameters = desc.getPropertyModel();
            if (parameters != null) {
                parameters.setTotal(this.parameters);
            }
            IDomainBaseProvider readerBaseProvider = new IDomainBaseProvider(){

                @Override
                public IDomainBase getDomainBase(IDomainBase samplesBase) {
                    return PlotConfiguration.this.getReaderBase(samplesBase, samplesCache);
                }
            };
            if (!(previous instanceof ISamplesProducer) || desc.update(previous, id, name, sources, processType, signalType, signalDescriptor, domainBase, this.start, this.end, this.rate, this.definition, this.language, parameters, readerBaseProvider) < 0) {
                producer = desc.newInstance(id, name, sources, processType, signalType, signalDescriptor, domainBase, this.start, this.end, this.rate, this.definition, this.language, parameters, readerBaseProvider);
                if (Utils.equals(producer, previous)) {
                    producer = previous;
                }
            } else {
                producer = previous;
            }
        }
        return producer;
    }

    private ISamplesProducer updateNoneProducer(IContext context, final ISamplesCache samplesCache, int samplesMode, ISamplesProducer previous, List<IReadableSamples> sources, int type) {
        ISamplesProducer producer = null;
        if (!(previous instanceof NoneProducer) || ((NoneProducer)previous).getType() != 1) {
            ISamples.ProcessType processType = ISamples.ProcessType.parse(this.processType);
            ISamples.SignalType signalType = ISamples.SignalType.parse(this.signalType);
            ISamples.SignalDescriptor signalDescriptor = Utils.isEmpty(this.signalDescriptor) ? null : ISamples.SignalDescriptor.parse(this.signalDescriptor);
            IDomainBase domainBase = DomainBase.parse(this.domainBase);
            IPropertyModel parameters = null;
            IDomainBaseProvider readerBaseProvider = new IDomainBaseProvider(){

                @Override
                public IDomainBase getDomainBase(IDomainBase samplesBase) {
                    return PlotConfiguration.this.getReaderBase(samplesBase, samplesCache);
                }
            };
            producer = new NoneProducer(null, null, sources, processType, signalType, signalDescriptor, domainBase, this.start, this.end, this.rate, this.definition, this.language, parameters, readerBaseProvider, type);
        } else {
            producer = previous;
        }
        return producer;
    }

    public boolean hasChildSamples(String subId, IReadableSamples baseSamples) {
        if (baseSamples != null && baseSamples.getProducer() instanceof ISamplesProducer.IMasterProducer) {
            return ((ISamplesProducer.IMasterProducer)baseSamples.getProducer()).hasChildSlaves(subId);
        }
        if (subId == null) {
            boolean isCompound = baseSamples.getSignalType() == ISamples.SignalType.Logic && baseSamples.getScale() > 1 && baseSamples.getScale() <= 256;
            isCompound |= (baseSamples.getSignalType() == ISamples.SignalType.FloatArray || baseSamples.getSignalType() == ISamples.SignalType.IntegerArray || baseSamples.getSignalType() == ISamples.SignalType.TextArray || baseSamples.getSignalType() == ISamples.SignalType.EventArray) && baseSamples.getScale() > 1 && baseSamples.getScale() <= 256;
            return isCompound |= baseSamples.getSignalType() == ISamples.SignalType.Struct;
        }
        Object compound = baseSamples.getData("COMPOUND");
        if (compound instanceof ISamplesProducer.IMasterProducer) {
            return ((ISamplesProducer.IMasterProducer)compound).hasChildSlaves(subId);
        }
        return false;
    }

    public List<String> getChildSampleIds(String subId, IReadableSamples baseSamples) {
        if (baseSamples != null && baseSamples.getProducer() instanceof ISamplesProducer.IMasterProducer) {
            return ((ISamplesProducer.IMasterProducer)baseSamples.getProducer()).getChildSlaveIds(subId);
        }
        Object compound = baseSamples.getData("COMPOUND");
        if (!(compound instanceof ISamplesProducer.IMasterProducer)) {
            compound = null;
            if (baseSamples.getSignalType() == ISamples.SignalType.Struct) {
                compound = new MemberExtract(baseSamples);
            } else if (baseSamples.getSignalType() == ISamples.SignalType.FloatArray || baseSamples.getSignalType() == ISamples.SignalType.IntegerArray || baseSamples.getSignalType() == ISamples.SignalType.TextArray || baseSamples.getSignalType() == ISamples.SignalType.EventArray && baseSamples.getScale() <= 256) {
                compound = new MemberExtract(baseSamples);
            } else if (baseSamples.getSignalType() == ISamples.SignalType.Logic && baseSamples.getScale() > 1 && baseSamples.getScale() <= 256) {
                compound = new LogicExtract(baseSamples);
            }
            baseSamples.setData("COMPOUND", compound);
        }
        if (compound instanceof ISamplesProducer.IMasterProducer) {
            return ((ISamplesProducer.IMasterProducer)compound).getChildSlaveIds(subId);
        }
        return Collections.EMPTY_LIST;
    }

    public IReadableSamples getSamples(String subId, IReadableSamples baseSamples) {
        if (baseSamples != null && baseSamples.getProducer() instanceof ISamplesProducer.IMasterProducer) {
            return ((ISamplesProducer.IMasterProducer)baseSamples.getProducer()).getSlaveProduction(subId);
        }
        Object compound = baseSamples.getData("COMPOUND");
        if (compound instanceof ISamplesProducer.IMasterProducer) {
            return ((ISamplesProducer.IMasterProducer)compound).getSlaveProduction(subId);
        }
        return null;
    }

    public PlotConfiguration createSubConfiguration(String subId, IReadableSamples baseSamples) {
        IReadableSamples samples = this.getSamples(subId, baseSamples);
        if (samples != null) {
            PlotConfiguration configuration = (PlotConfiguration)Elements.cells.create(PlotConfiguration.class);
            configuration.samples = this.getLink(this.getParent(ViewConfiguration.class));
            configuration.samples.setParameter(LINK_KEY_CHILD, subId);
            configuration.name = String.valueOf(this.getName()) + samples.getName();
            Object color = null;
            IPaintStyle paintStyle = null;
            if (samples instanceof ISamplesDisplayInformation) {
                color = ((ISamplesDisplayInformation)((Object)samples)).getColor();
                paintStyle = ((ISamplesDisplayInformation)((Object)samples)).getPaintStyle();
            } else {
                paintStyle = PlotConfiguration.getPaintStyle(samples);
            }
            int n = configuration.color = color instanceof Integer ? (Integer)color : this.color;
            if (paintStyle != null) {
                configuration.style = paintStyle.getType();
                configuration.columnValueFormat = paintStyle.getValueColumnFormat();
                configuration.styleParameters = paintStyle.getAdditional();
                if (paintStyle.getType() == 1) {
                    configuration.aValueFormat = paintStyle.getFormat();
                    configuration.relation = paintStyle.hasMod(4096);
                } else if (paintStyle.getType() == 2) {
                    configuration.aValueFormat = paintStyle.getFormat();
                    configuration.relation = paintStyle.hasMod(4096);
                } else if (paintStyle.getType() == 6) {
                    configuration.relation = paintStyle.hasMod(4096);
                } else if (paintStyle.getType() == 5) {
                    configuration.relation = paintStyle.hasMod(4096);
                } else if (paintStyle.getType() == 4 || paintStyle.getType() == 9) {
                    configuration.aValueFormat = paintStyle.getFormat();
                    configuration.scale = paintStyle.hasMod(64);
                    configuration.annotation = paintStyle.hasMod(32);
                    configuration.interpolation = paintStyle.hasMod(16);
                    configuration.aModifier = paintStyle.hasMod(256);
                    configuration.bModifier = paintStyle.hasMod(512);
                    configuration.scaleFrom = (float)paintStyle.getScaleFrom();
                    configuration.scaleTo = (float)paintStyle.getScaleTo();
                    configuration.scaleType = paintStyle.getScaleType();
                    configuration.scaleUnit = paintStyle.getScaleUnit();
                } else if (paintStyle.getType() == 3) {
                    configuration.cValueFormat = paintStyle.getFormat();
                    configuration.relation = paintStyle.hasMod(4096);
                    configuration.aModifier = paintStyle.hasMod(2048);
                } else if (paintStyle.getType() == 10) {
                    configuration.bValueFormat = paintStyle.getFormat();
                    configuration.relation = paintStyle.hasMod(4096);
                    configuration.aModifier = paintStyle.hasMod(2048);
                    configuration.annotation = paintStyle.hasMod(32);
                } else if (paintStyle.getType() == 8 && configuration.styleDescriptor != null && configuration.styleDescriptor.resolveCell(ImpulsePreferences.chartPreferences) != null) {
                    configuration.styleDescriptor = paintStyle.getDescriptor();
                }
            }
            return configuration;
        }
        return null;
    }

    @Override
    public Object getColor() {
        return this.color;
    }

    @Override
    public int getValueColumnFormat() {
        return this.columnValueFormat;
    }

    @Override
    public IPaintStyle getPaintStyle() {
        PaintStyle paintStyle = new PaintStyle();
        paintStyle.diagramType = this.style;
        paintStyle.valueColumnFormat = this.columnValueFormat;
        paintStyle.additional = this.styleParameters;
        if (paintStyle.diagramType == 1) {
            paintStyle.format = this.aValueFormat;
            paintStyle.diagramMods = paintStyle.diagramMods | (this.relation ? 4096 : 0);
        } else if (paintStyle.diagramType == 2) {
            paintStyle.format = this.bValueFormat;
            paintStyle.diagramMods = paintStyle.diagramMods | (this.relation ? 4096 : 0);
        } else if (paintStyle.diagramType == 6) {
            paintStyle.diagramMods = paintStyle.diagramMods | (this.relation ? 4096 : 0);
        } else if (paintStyle.diagramType == 5) {
            paintStyle.diagramMods = paintStyle.diagramMods | (this.relation ? 4096 : 0);
        } else if (paintStyle.diagramType == 4 || paintStyle.diagramType == 9) {
            paintStyle.diagramMods = paintStyle.diagramMods | (this.scale ? 64 : 0);
            paintStyle.diagramMods = paintStyle.diagramMods | (this.annotation ? 32 : 0);
            paintStyle.diagramMods = paintStyle.diagramMods | (this.interpolation ? 16 : 0);
            paintStyle.diagramMods = paintStyle.diagramMods | (this.axis ? 128 : 0);
            paintStyle.diagramMods = paintStyle.diagramMods | (this.aModifier ? 256 : 0);
            paintStyle.diagramMods = paintStyle.diagramMods | (this.bModifier ? 512 : 0);
            paintStyle.scaleFrom = this.scaleFrom;
            paintStyle.scaleTo = this.scaleTo;
            paintStyle.scaleType = this.scaleType;
            paintStyle.scaleUnit = this.scaleUnit;
            paintStyle.format = this.aValueFormat;
        } else if (paintStyle.diagramType == 3) {
            paintStyle.format = this.cValueFormat;
            paintStyle.diagramMods = paintStyle.diagramMods | (this.relation ? 4096 : 0);
            paintStyle.diagramMods = paintStyle.diagramMods | (this.aModifier ? 2048 : 0);
        } else if (paintStyle.diagramType == 10) {
            paintStyle.format = this.bValueFormat;
            paintStyle.diagramMods = paintStyle.diagramMods | (this.aModifier ? 2048 : 0);
            paintStyle.diagramMods = paintStyle.diagramMods | (this.relation ? 4096 : 0);
            paintStyle.diagramMods = paintStyle.diagramMods | (this.annotation ? 32 : 0);
        } else if (paintStyle.diagramType == 8) {
            if (this.styleDescriptor != null && this.styleDescriptor.resolveCell(ImpulsePreferences.chartPreferences) != null) {
                paintStyle.descriptor = this.styleDescriptor;
            }
        } else {
            paintStyle.format = 0;
        }
        return paintStyle;
    }

    public static IPaintStyle getPaintStyle(ISamplesCharacteristic samples) {
        ISamples.SignalType type = samples.getSignalType();
        ISamples.SignalDescriptor signalDescriptor = samples.getSignalDescriptor();
        PaintStyle paintStyle = new PaintStyle();
        paintStyle.valueColumnFormat = -1;
        paintStyle.format = -1;
        if (type == ISamples.SignalType.Logic && signalDescriptor.getScale() == 1) {
            paintStyle.diagramType = 1;
            paintStyle.format = 0;
        } else if (type == ISamples.SignalType.Logic && signalDescriptor.getScale() > 1) {
            paintStyle.diagramType = 2;
        } else if (type == ISamples.SignalType.Integer || type == ISamples.SignalType.IntegerArray) {
            paintStyle.diagramType = 2;
        } else if (type == ISamples.SignalType.Float) {
            paintStyle.diagramType = 4;
            paintStyle.diagramMods = 16;
        } else if (type == ISamples.SignalType.FloatArray) {
            paintStyle.diagramType = 2;
        } else if (type == ISamples.SignalType.Event || type == ISamples.SignalType.EventArray) {
            if (signalDescriptor.isGantt()) {
                paintStyle.diagramType = 10;
                paintStyle.format = 0;
                paintStyle.diagramMods = 6176;
            } else {
                paintStyle.diagramType = 3;
            }
        } else if (type == ISamples.SignalType.Struct) {
            if (signalDescriptor.isTransaction()) {
                paintStyle.diagramType = 5;
            } else if (signalDescriptor.isGantt()) {
                paintStyle.diagramType = 10;
                paintStyle.format = 0;
                paintStyle.diagramMods = 6176;
            } else {
                paintStyle.diagramType = 6;
            }
        } else {
            paintStyle.diagramType = type == ISamples.SignalType.Text || type == ISamples.SignalType.TextArray ? 2 : (type == ISamples.SignalType.Binary && signalDescriptor.isImage() ? 7 : (type == ISamples.SignalType.Binary ? 2 : 0));
        }
        return paintStyle;
    }

    public static int getColor(ISamplesCharacteristic samples) {
        ISamples.SignalType type = samples.getSignalType();
        ISamples.SignalDescriptor signalDescriptor = samples.getSignalDescriptor();
        int color = 0;
        color = type == ISamples.SignalType.Logic && signalDescriptor.getScale() == 1 ? TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.0", 0) : (type == ISamples.SignalType.Logic && signalDescriptor.getScale() > 1 ? TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.1", 0) : (type == ISamples.SignalType.Integer || type == ISamples.SignalType.IntegerArray ? TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.1", 0) : (type == ISamples.SignalType.Float ? TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.2", 0) : (type == ISamples.SignalType.FloatArray ? TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.2", 0) : (type == ISamples.SignalType.Event || type == ISamples.SignalType.EventArray ? (signalDescriptor.isGantt() ? TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.9", 0) : TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.7", 0)) : (type == ISamples.SignalType.Struct ? (signalDescriptor.isTransaction() ? TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.9", 0) : (signalDescriptor.isGantt() ? TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.9", 0) : TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.9", 0))) : (type == ISamples.SignalType.Text || type == ISamples.SignalType.TextArray ? TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.4", 0) : (type == ISamples.SignalType.Binary ? TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.3", 0) : TLK.getSystemColorRgbInt("de.toem.impulse.color.sample.5", 0)))))))));
        return color;
    }

    public String imageExtension() {
        StringBuilder builder = new StringBuilder();
        String sstyle = "";
        switch (this.style) {
            case 0: {
                sstyle = "none";
                break;
            }
            case 1: {
                sstyle = "logic";
                break;
            }
            case 2: {
                sstyle = "vector";
                break;
            }
            case 3: {
                sstyle = "event";
                break;
            }
            case 4: {
                sstyle = "line";
                break;
            }
            case 5: {
                sstyle = "transaction";
                break;
            }
            case 6: {
                sstyle = "log";
                break;
            }
            case 7: {
                sstyle = "image";
                break;
            }
            case 8: {
                sstyle = "chart";
                break;
            }
            case 9: {
                sstyle = "area";
                break;
            }
            case 10: {
                sstyle = "gannt";
            }
        }
        builder.append("-");
        builder.append(sstyle);
        if (!Utils.isEmpty(this.production)) {
            builder.append("-prod");
        }
        return builder.toString();
    }

    public static String fieldNameForAttribute(String old) {
        if ("script".equals(old)) {
            return "definition";
        }
        if ("relation".equals(old)) {
            return "relation";
        }
        return null;
    }
}

