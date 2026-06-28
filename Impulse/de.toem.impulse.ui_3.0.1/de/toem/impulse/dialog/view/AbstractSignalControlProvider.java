/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.view;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.charts.AbstractChartCell;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.producer.SamplesProducerDescriptor;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.ElementModifierEvent;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.information.IGroupedInformation;
import de.toem.toolkits.pattern.information.IGroupedInformations;
import de.toem.toolkits.pattern.information.IInformationGroup;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.registry.IRegisteredObjects;
import de.toem.toolkits.pattern.registry.IRegistryObject;
import de.toem.toolkits.pattern.scan.TextScanResult;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.ui.controller.abstrac.AbstractController;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.AbstractCompositeController;
import de.toem.toolkits.ui.controller.base.ComboController;
import de.toem.toolkits.ui.controller.base.ImageController;
import de.toem.toolkits.ui.controller.base.PropertyTableController;
import de.toem.toolkits.ui.controller.base.TabFolderController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.controller.source.PropertyFieldSource;
import de.toem.toolkits.ui.controller.source.PropertySource;
import de.toem.toolkits.ui.dialog.InformationSelectorDialog;
import de.toem.toolkits.ui.proposal.ContentProposal;
import de.toem.toolkits.ui.proposal.ContentProposalExtension;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.controls.ITlkControl;
import de.toem.toolkits.ui.tlk.controls.ITlkTabFolder;
import java.lang.reflect.Field;

public abstract class AbstractSignalControlProvider
extends AbstractControlProvider {
    protected TextController production;
    protected ComboController processType;
    protected ComboController signalType;
    protected TextController signalDescriptor;
    protected ComboController domainBase;
    protected IDomainBase derivedDomainBase;
    protected ComboController domainClass;
    protected TextController domainStart;
    protected TextController domainEnd;
    protected TextController domainRate;

    protected void addProduction(Object container, Object source, Object ld, int style, final IGroupedInformations<? extends IGroupedInformation, IInformationGroup> information, final IExecutable onChange) {
        ITlkComposite main = this.tlk().addComposite(container, null, 3, ld, 0x100000 | style, I18n.Samples_Production_, null);
        this.tlk().addImage(main, new ImageController(this.editor(), source){

            @Override
            protected Object convert(Object value) {
                return value instanceof String ? information.get((String)value) : null;
            }
        }, this.tlk().ld(1, 0x1000000, 20), 0, null);
        this.production = new TextController(this.editor(), source){

            @Override
            protected void doUpdateExternal() {
                if (onChange != null) {
                    onChange.execute(null);
                }
            }

            @Override
            protected Object convert(Object value) {
                String label = value instanceof String && information.get((String)value) != null ? ((IGroupedInformation)information.get((String)value)).getLabel() : "";
                return label;
            }
        };
        this.tlk().addText(main, this.production, this.tlk().ld(1, 524288, 1), 8192, null);
        this.tlk().addInPlaceDialog(container, main, InformationSelectorDialog.getControls(I18n.Samples_Production, information, source, -1, 12), 2, this.cols(), I18n.Samples_Production_);
    }

    protected void addProcessType(Object container, Object source, Object ld, int style) {
        this.processType = this.tlk().addCombo(container, new ComboController(this.editor(), source, ISamples.ProcessType.getOptions(false), ISamples.ProcessType.getOptions(false)){

            @Override
            protected void doUpdateExternal() {
                AbstractSignalControlProvider.this.showControl(ISamples.ProcessType.parse(this.getValueAsString()).equals((Object)ISamples.ProcessType.Continuous), (IController)AbstractSignalControlProvider.this.domainRate);
            }
        }, ld, style | 0x2000, I18n.Samples_ProcessType_);
    }

    protected void addSignalType(Object container, Object source, Object ld, int style) {
        this.signalType = this.tlk().addCombo(container, new ComboController(this.editor(), source, ISamples.SignalType.getOptions(false), ISamples.SignalType.getOptions(false)){

            @Override
            public boolean needsUpdate() {
                return true;
            }

            @Override
            protected void doUpdateExternal() {
            }
        }, ld, style | 0x2000, I18n.Samples_SignalType_);
    }

    protected void addSignalDescriptor(Object container, Object source, Object ld, int style) {
        try {
            this.signalDescriptor = (TextController)this.tlk().addText(container, new TextController(this.editor(), source){

                @Override
                protected Object revert(Object value) {
                    ISamples.SignalType type = ISamples.SignalType.parse(AbstractSignalControlProvider.this.signalType.getValueAsString());
                    if (value instanceof String && !Utils.isEmpty((String)value)) {
                        return ISamples.SignalDescriptor.parseUser(type, String.valueOf(value)).toString();
                    }
                    return null;
                }

                @Override
                protected Object convert(Object value) {
                    ISamples.SignalType type = ISamples.SignalType.parse(AbstractSignalControlProvider.this.signalType.getValueAsString());
                    if (value instanceof String && !Utils.isEmpty((String)value)) {
                        return ISamples.SignalDescriptor.parse((String)value).toUserString(type);
                    }
                    return null;
                }

                @Override
                public boolean needsUpdate() {
                    return true;
                }

                @Override
                protected TextScanResult doCheck(String formatted, int options) {
                    ISamples.SignalType type = ISamples.SignalType.parse(AbstractSignalControlProvider.this.signalType.getValueAsString());
                    if (Utils.isEmpty(formatted)) {
                        return TextScanResult.SCAN_OK;
                    }
                    return ISamples.SignalDescriptor.checkUser(type, formatted) ? TextScanResult.SCAN_OK : TextScanResult.SCAN_ERROR;
                }
            }.add(new ContentProposalExtension(true){

                @Override
                public ContentProposal[] getProposals(String contents, int position) {
                    this.clear();
                    ISamples.SignalType type = ISamples.SignalType.parse(AbstractSignalControlProvider.this.signalType.getValueAsString());
                    this.add("default<>", null, null);
                    switch (type) {
                        case Logic: {
                            this.add("default<bits=16>", null, null);
                            this.add("default<bits=16,df=Hex>", null, null);
                            this.add("default<bits=16,df=Binary>", null, null);
                            this.add("default<bits=16,df=Octal>", null, null);
                            this.add("default<bits=16,df=ASCII>", null, null);
                            break;
                        }
                        case Event: {
                            this.add("default<df=Event>", null, null);
                            this.add("default<df=Text>", null, null);
                            break;
                        }
                        case EventArray: {
                            this.add("default<dim=2>", null, null);
                            this.add("default<dim=2,df=Index>", null, null);
                            this.add("default<dim=2,df=Event>", null, null);
                            this.add("default<dim=2,df=Text>", null, null);
                            break;
                        }
                        case Integer: 
                        case Float: {
                            this.add("default<df=Decimal>", null, null);
                            this.add("default<df=UserDec0>", null, null);
                            break;
                        }
                        case IntegerArray: 
                        case FloatArray: {
                            this.add("default<dim=8>", null, null);
                            this.add("default<dim=8,df=Decimal>", null, null);
                            this.add("default<dim=8,df=UserDec0>", null, null);
                            break;
                        }
                        case Text: {
                            this.add("default<df=Text>", null, null);
                            break;
                        }
                        case TextArray: {
                            this.add("default<dim=2>", null, null);
                            this.add("default<dim=2,df=Text>", null, null);
                            break;
                        }
                        case Struct: {
                            this.add("transaction<>", null, null);
                            break;
                        }
                        case Binary: {
                            this.add("image<>", null, null);
                            break;
                        }
                    }
                    this.add("default<df=None>", null, null);
                    this.add("default<df=Index>", null, null);
                    this.add("default<df=\u0394Domain>", null, null);
                    this.add("default<df=\u0394Value>", null, null);
                    return super.getProposals(contents, position);
                }
            }), ld, style, I18n.Samples_SignalDescriptor_);
        }
        catch (Throwable throwable) {}
    }

    protected void addDomainClass(Object container, Object ld, int style) {
        this.domainClass = this.tlk().addCombo(container, new ComboController(this.editor(), "domainClass", DomainBase.CLASS_LABELS, DomainBase.CLASSES){

            @Override
            protected void doUpdateHints() {
                if (this.value == null && AbstractSignalControlProvider.this.domainBase.getValue() instanceof String) {
                    this.source.changeValue(DomainBase.parse((String)AbstractSignalControlProvider.this.domainBase.getValue()).getClazz(), true);
                    this.update(true);
                    AbstractSignalControlProvider.this.domainBase.updateControl(true);
                }
            }

            @Override
            protected void doUpdateExternal() {
                AbstractSignalControlProvider.this.domainBase.updateControl(true);
            }
        }, ld, style | 0x2000, I18n.Samples_DomainClass_);
    }

    protected void addDomainBase(Object container, Object source, Object ld, int style) {
        this.domainBase = this.tlk().addCombo(container, new ComboController(this.editor(), source, DomainBase.ALL_LABELS, DomainBase.ALL_OPTIONS){

            @Override
            protected boolean filterItem(String label, Object value) {
                Object dclass = AbstractSignalControlProvider.this.domainClass.getValue();
                if (dclass == null && AbstractSignalControlProvider.this.derivedDomainBase != null) {
                    dclass = AbstractSignalControlProvider.this.derivedDomainBase.getClazz();
                }
                IDomainBase base = DomainBase.parse((String)value);
                if (value == null) {
                    return false;
                }
                if (value != null && Utils.equals(value, String.valueOf(AbstractSignalControlProvider.this.derivedDomainBase))) {
                    return false;
                }
                if (value != null && Utils.equals(value, this.value)) {
                    return false;
                }
                return base == null || !base.getClazz().equals(dclass) || base.userLevel() < 1;
            }

            @Override
            protected void doUpdateExternal() {
            }
        }, ld, style | 0x2000, I18n.Samples_DomainBase_);
    }

    protected void addDomainStart(Object container, Object source, Object ld, int style) {
        this.domainStart = this.tlk().addText(container, new TextController(this.editor(), source){

            @Override
            protected TextScanResult doCheck(String formatted, int options) {
                try {
                    if (Utils.isEmpty(formatted) || AbstractSignalControlProvider.this.derivedDomainBase == null) {
                        return TextScanResult.SCAN_OK;
                    }
                    AbstractSignalControlProvider.this.derivedDomainBase.parseUnits(formatted);
                    return TextScanResult.SCAN_OK;
                }
                catch (Throwable throwable) {
                    return TextScanResult.SCAN_ERROR;
                }
            }
        }, ld, style, I18n.Samples_DomainStart_);
    }

    protected void addDomainEnd(Object container, Object source, Object ld, int style) {
        this.domainEnd = this.tlk().addText(container, new TextController(this.editor(), source){

            @Override
            protected TextScanResult doCheck(String formatted, int options) {
                try {
                    if (Utils.isEmpty(formatted) || AbstractSignalControlProvider.this.derivedDomainBase == null) {
                        return TextScanResult.SCAN_OK;
                    }
                    AbstractSignalControlProvider.this.derivedDomainBase.parseUnits(formatted);
                    return TextScanResult.SCAN_OK;
                }
                catch (Throwable throwable) {
                    return TextScanResult.SCAN_ERROR;
                }
            }
        }, ld, style, I18n.Samples_DomainEnd_);
    }

    protected void addDomainRate(Object container, Object source, Object ld, int style) {
        this.domainRate = this.tlk().addText(container, new TextController(this.editor(), source){

            @Override
            protected TextScanResult doCheck(String formatted, int options) {
                try {
                    if (Utils.isEmpty(formatted) || AbstractSignalControlProvider.this.derivedDomainBase == null) {
                        return TextScanResult.SCAN_OK;
                    }
                    if (AbstractSignalControlProvider.this.derivedDomainBase.parseUnits(formatted) > 0L) {
                        return TextScanResult.SCAN_OK;
                    }
                }
                catch (Throwable throwable) {}
                return TextScanResult.SCAN_ERROR;
            }
        }, ld, style, I18n.Samples_DomainRate_);
    }

    protected ITlkTabFolder addParameterTabFolder(Object container, String name, String field, final Object modelSource, final String modelDescriptor) throws NoSuchFieldException, SecurityException {
        final TabFolderController parameterTabController = new TabFolderController(this.editor(), name);
        ITlkTabFolder tabFolder = this.tlk().addTabFolder(container, parameterTabController, this.cols(), 131072, null);
        PropertyFieldSource source = new PropertyFieldSource(this.clazz().getField(field)){

            @Override
            protected IPropertyModel createModel() {
                return AbstractSignalControlProvider.this.getPropertyModel((AbstractController)this.controller, modelSource, modelDescriptor);
            }
        };
        this.tlk().addComposite(tabFolder, new AbstractCompositeController(this.editor(), source){
            IPropertyModel currentModel;
            Object currentSource;
            Object currentSourceModel;

            @Override
            public boolean needsUpdate() {
                return true;
            }

            @Override
            protected void doUpdateControl() {
                IPropertyModel newModel = AbstractSignalControlProvider.this.getPropertyModel(this, modelSource, modelDescriptor);
                if (!Utils.equals(newModel, this.currentModel) || !Utils.equals(this.source, this.currentSource)) {
                    this.currentSourceModel = newModel != null ? PropertySource.getPropertySourceModel(this.source) : null;
                    this.currentSource = this.source;
                    IPropertyModel previousModel = this.currentModel;
                    this.currentModel = newModel;
                    this.access(composite -> {
                        IControlProvider controls;
                        if (previousModel != null) {
                            composite.clear();
                            this.tlk().removeController(previousModel);
                        }
                        IControlProvider iControlProvider = controls = this.currentModel != null ? this.currentModel.getControls() : null;
                        if (controls != null) {
                            controls.setLayout(AbstractSignalControlProvider.this.cols(), AbstractSignalControlProvider.this.cols());
                            Object previous = this.tlk().setDefaultOwner(this.currentModel);
                            this.tlk().addControls(composite, controls);
                            this.tlk().setDefaultOwner(previous);
                            parameterTabController.selectIndex(0);
                            this.tlk().reflow(true, true);
                            this.tlk().update(this.currentSourceModel, null, (Object)this.currentModel);
                        } else {
                            parameterTabController.selectIndex(1);
                            this.tlk().reflow(true, true);
                        }
                    });
                } else {
                    this.tlk().update(this.currentSourceModel, null, (Object)this.currentModel);
                }
                super.doUpdateControl();
            }
        }, this.cols(), this.cols(), 0, "", "codicon-symbol-string");
        ITlkComposite gridTab = this.tlk().addComposite(tabFolder, null, this.cols(), this.cols(), 0, "", "codicon-list-unordered");
        source = new PropertyFieldSource(this.clazz().getField(field)){

            @Override
            protected IPropertyModel createModel() {
                return AbstractSignalControlProvider.this.getPropertyModel((AbstractController)this.controller, modelSource, modelDescriptor);
            }
        };
        this.tlk().addTable(gridTab, new PropertyTableController(this.editor(), source){

            @Override
            public boolean needsUpdate() {
                return true;
            }
        }, this.tlk().ld(this.cols(), 4, 1, 4, 150), 32, null, new String[]{I18n.General_Parameter, I18n.General_Value});
        return tabFolder;
    }

    protected ITlkComposite addDefinition(Object container, Field source, Field language) {
        final Field fieldSource = source;
        final Field languageSource = language;
        return this.tlk().addComposite(container, new AbstractCompositeController(this.editor(), source){
            SamplesProducerDescriptor currentDescriptor;

            @Override
            public boolean needsUpdate() {
                return true;
            }

            @Override
            protected void doUpdateControl() {
                SamplesProducerDescriptor descriptor = (SamplesProducerDescriptor)ISamplesProducer.all.get(AbstractSignalControlProvider.this.production.getValueAsString());
                if (!Utils.equals(descriptor, this.currentDescriptor)) {
                    this.access(composite -> {
                        try {
                            IControlProvider controls;
                            if (this.currentDescriptor != null && this.currentDescriptor.getDefinitionControls(fieldSource, languageSource) != null) {
                                composite.clear();
                                this.tlk().removeController(this.currentDescriptor);
                            }
                            this.currentDescriptor = descriptor;
                            IControlProvider iControlProvider = controls = this.currentDescriptor != null ? this.currentDescriptor.getDefinitionControls(fieldSource, languageSource) : null;
                            if (controls != null) {
                                controls.setLayout(AbstractSignalControlProvider.this.cols(), AbstractSignalControlProvider.this.cols());
                                Object previous = this.tlk().setDefaultOwner(this.currentDescriptor);
                                this.tlk().addControls(composite, controls);
                                this.tlk().setDefaultOwner(previous);
                                this.tlk().reflow(true, true);
                                this.tlk().update(this.getCells(), (ElementModifierEvent)null, (Object)this.currentDescriptor);
                            } else {
                                this.tlk().reflow(true, true);
                            }
                        }
                        catch (SecurityException e) {
                            SystemLog.log(e);
                        }
                    });
                } else {
                    this.tlk().update(null, this.currentDescriptor);
                }
                super.doUpdateControl();
            }
        }, this.cols(), this.cols(), 0, null, null);
    }

    private IPropertyModel getPropertyModel(AbstractController controller, Object modelSource, String modelDescriptor) {
        if (modelSource instanceof IRegisteredObjects) {
            String value = (String)controller.getCellsValue(modelDescriptor, String.class);
            IRegistryObject descriptor = (IRegistryObject)((IRegisteredObjects)modelSource).get(value);
            return descriptor != null ? descriptor.getPropertyModel() : null;
        }
        if (modelSource instanceof IElement) {
            ICell cell;
            Link link = (Link)controller.getCellsValue(modelDescriptor, Link.class);
            ICell iCell = cell = link != null ? link.resolveCell(ImpulsePreferences.chartPreferences) : null;
            if (cell instanceof AbstractChartCell) {
                return ((AbstractChartCell)cell).getPropertyModel(true);
            }
        } else if (modelSource instanceof IPropertyModelProvider) {
            return ((IPropertyModelProvider)modelSource).getPropertyModel(modelDescriptor);
        }
        return null;
    }

    boolean showControl(boolean show, IController controller) {
        return this.tlk().showControl(show, controller);
    }

    boolean showControl(boolean show, IController ... controllers) {
        return this.tlk().showControl(show, controllers);
    }

    boolean showControl(boolean show, ITlkControl control) {
        return this.tlk().showControl(show, control);
    }

    boolean showControl(boolean show, ITlkControl ... controls) {
        return this.tlk().showControl(show, controls);
    }

    protected IController addFormatCombos(Object container, Object field, String label) throws NoSuchFieldException, SecurityException {
        ComboController collectionFormatsController = new ComboController(this.editor(), field instanceof String ? this.clazz().getField((String)field) : field, PlotConfiguration.formatCollectionLabels, PlotConfiguration.formatCollectionOptions){

            @Override
            protected boolean enabled(Object option) {
                if (this.converted instanceof Integer) {
                    int format = (Integer)this.converted & 0xFFFF;
                    if (format >= 16 && format <= 20) {
                        return false;
                    }
                    return format != 0;
                }
                return false;
            }
        }.setIntegerMask(-65536);
        container = this.tlk().addComposite(container, null, 2, this.tlk().ld(this.cols() - 1, true, true, true, false), 1, label, null);
        ComboController valueFormatsController = this.tlk().addCombo(container, new ComboController(this.editor(), field instanceof String ? this.clazz().getField((String)field) : field, PlotConfiguration.formatValueLabels, PlotConfiguration.formatValueOptions){}.setIntegerMask(65535), this.tlk().ld(1, true, true, true, false), 8192, null);
        this.tlk().addCombo(container, collectionFormatsController, 1, 8192, null);
        return valueFormatsController;
    }

    protected IController addValueFormatCombo(Object container, Object field, String label) throws NoSuchFieldException, SecurityException {
        ComboController valueFormatsController = this.tlk().addCombo(container, new ComboController(this.editor(), field instanceof String ? this.clazz().getField((String)field) : field, PlotConfiguration.formatValueLabels, PlotConfiguration.formatValueOptions){}.setIntegerMask(65535), this.cols(), 8193, label);
        return valueFormatsController;
    }

    static interface IPropertyModelProvider {
        public IPropertyModel getPropertyModel(String var1);
    }
}

