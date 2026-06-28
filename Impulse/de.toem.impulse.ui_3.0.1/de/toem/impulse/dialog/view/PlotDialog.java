/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.view;

import de.toem.impulse.ImpulseLicense;
import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.charts.AbstractChartCell;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.cells.view.PlotConfigurationTemplate;
import de.toem.impulse.cells.view.SourceReference;
import de.toem.impulse.dialog.view.AbstractSignalControlProvider;
import de.toem.impulse.dialog.view.DerivedSamplesContext;
import de.toem.impulse.dialog.view.SourceReferenceDialog;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.UnknownBase;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.convert.ConvertedSamples;
import de.toem.impulse.samples.producer.SamplesProducerDescriptor;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.Element;
import de.toem.toolkits.pattern.element.ElementCellModifier;
import de.toem.toolkits.pattern.element.ElementModifierEvent;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.IElementModifier;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.information.GroupedInformation;
import de.toem.toolkits.pattern.information.GroupedInformations;
import de.toem.toolkits.pattern.information.IGroupedInformation;
import de.toem.toolkits.pattern.information.IGroupedInformations;
import de.toem.toolkits.pattern.information.IInformationGroup;
import de.toem.toolkits.pattern.preferences.AbstractPreferenceCell;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.provider.IContext;
import de.toem.toolkits.pattern.registry.Registration;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.ButtonController;
import de.toem.toolkits.ui.controller.base.CellTableController;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.ComboController;
import de.toem.toolkits.ui.controller.base.ExpandableController;
import de.toem.toolkits.ui.controller.base.GroupController;
import de.toem.toolkits.ui.controller.base.ImageController;
import de.toem.toolkits.ui.controller.base.RadioSetController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.controller.commands.CommandButtonController;
import de.toem.toolkits.ui.controller.source.PropertySource;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.part.dialog.Dialogs;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.controls.ITlkControl;
import de.toem.toolkits.ui.tlk.controls.ITlkExpandable;
import de.toem.toolkits.ui.tlk.controls.ITlkGroup;
import de.toem.toolkits.ui.tlk.controls.ITlkTabFolder;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

public class PlotDialog
extends ControlProviderElementDialog {
    public PlotDialog(ITlkPartContainer parent, int style) {
        super(parent, PlotDialog.getControls(), style);
    }

    public PlotDialog() {
    }

    @Override
    protected void updateControls(ElementModifierEvent event) {
        super.updateControls(event);
        Element base = this.partContainer instanceof ITlkPart ? ((ITlkPart)((Object)this.partContainer)).getElement() : IElement.NONE;
        this.provider.update(new Object[]{this.dialogElements, base});
    }

    public static IControlProvider getControls() {
        AbstractSignalControlProvider provider = new AbstractSignalControlProvider(){
            RadioSetController styleController;
            IController primarySourceImageController;
            IController linkTableController;
            IController linkTableComboController;
            ITlkComposite productionConfig;
            ITlkComposite definition;
            ITlkComposite domainClassBase;
            ITlkComposite domainRange;
            ITlkTabFolder parameters;
            ITlkComposite logicInterpretation;
            ITlkComposite transformLinear;
            ITlkComposite interpretationExpandable;
            ITlkComposite logicFormat;
            ITlkComposite vectorFormat;
            ITlkComposite lineFormat;
            ITlkComposite eventFormat;
            ITlkComposite transactionFormat;
            ITlkComposite logFormat;
            ITlkComposite chartFormat;
            ITlkComposite areaFormat;
            ITlkComposite gantFormat;
            private List<IElement> updateElements;
            private IElement updateBase;

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.plot_dialog";
            }

            public IGroupedInformations<? extends IGroupedInformation, IInformationGroup> getProductionInformation() {
                GroupedInformations info = new GroupedInformations();
                info.add(new GroupedInformation(IGroupedInformations.DEFAULT_GROUP, null, "No production", null));
                for (IGroupedInformation i : ISamplesProducer.all) {
                    if (ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.productions", "de.toem.impulse.feature.default", i.getId())) {
                        info.add(new GroupedInformation(i.getGroups(), i.getId(), "\ud83d\udd12 " + i.getLabel(), i.getDescription(), i.getIconId()));
                        continue;
                    }
                    info.add(i);
                }
                return info;
            }

            @Override
            public boolean fillThis() {
                try {
                    ITlkComposite left = this.container();
                    this.tlk().addText(left, new TextController(this.editor(), PlotConfiguration.class.getField("name")), this.cols(), 0x100001, I18n.General_Name_);
                    this.tlk().addText(left, new TextController(this.editor(), PlotConfiguration.class.getField("description")), this.cols(), 0x100001, I18n.General_Description_);
                    ITlkExpandable input = this.tlk().addExpandable(left, new ExpandableController(this.editor(), "dialog.plot.input", false), this.cols(), this.cols(), 0, I18n.General_Source, null);
                    final IGroupedInformations<? extends IGroupedInformation, IInformationGroup> productionInformation = this.getProductionInformation();
                    this.addProduction(input, PlotConfiguration.class.getField("production"), this.tlk().ld(this.cols() - 1, 524288, 1), 1, productionInformation, p -> {
                        SamplesProducerDescriptor descr = (SamplesProducerDescriptor)ISamplesProducer.all.get(this.production.getValueAsString());
                        this.showControl(descr != null && descr.showMultipleInputs(), this.linkTableController, this.linkTableComboController);
                        boolean showProcessType = descr != null && descr.showProcess();
                        this.showControl(showProcessType, (IController)this.processType);
                        boolean showSignalType = descr != null && descr.showType();
                        this.showControl(showSignalType, (IController)this.signalType);
                        boolean showSignalDescriptor = descr != null && descr.showDescriptor();
                        this.showControl(showSignalDescriptor, (IController)this.signalDescriptor);
                        boolean showDomainBase = descr != null && descr.showProductionBase();
                        this.showControl(showDomainBase, (ITlkControl)this.domainClassBase);
                        showDomainBase = descr != null && descr.showProductionBase();
                        this.showControl(showDomainBase, (ITlkControl)this.domainRange);
                        this.showControl(showDomainBase, (IController)this.domainRate);
                        boolean showParameters = descr != null && descr.showParameters();
                        this.showControl(showParameters, (ITlkControl)this.parameters);
                        this.showControl(showProcessType || showSignalType || showSignalDescriptor || showDomainBase || showParameters, (ITlkControl)this.productionConfig);
                        this.showControl(descr != null && descr.showDefinition(), (ITlkControl)this.definition);
                    });
                    ITlkComposite primarySourceMain = this.tlk().addComposite(input, null, 3, this.cols(), 0x100001, I18n.PlotDialog_PrimarySource, null);
                    this.primarySourceImageController = this.tlk().addImage(primarySourceMain, new ImageController(this.editor(), null){

                        @Override
                        public Object value() {
                            ICell source = null;
                            if (this.hasCell() && updateBase != null && updateBase.isBound() && updateBase.hasCell(Record.class)) {
                                source = ((PlotConfiguration)this.getCell()).getSourceCell((Record)updateBase.getCell(), ((PlotConfiguration)this.getCell()).samples);
                            }
                            return source;
                        }
                    }, this.tlk().ld(1, 0x1000000, 20), 0, null);
                    this.tlk().addText(primarySourceMain, new TextController(this.editor(), PlotConfiguration.class.getField("samples")){

                        @Override
                        protected Object convert(Object value) {
                            if (value instanceof Link) {
                                return ((Link)value).toString();
                            }
                            return value;
                        }

                        @Override
                        protected Object revert(Object value) {
                            if (value instanceof String) {
                                return Link.parse((String)value);
                            }
                            return value;
                        }
                    }, this.tlk().ld(1, 524288, 1), 0, null);
                    this.tlk().addInPlaceDialog(input, primarySourceMain, SourceReferenceDialog.getControls(null, null, PlotConfiguration.class.getField("samples")), this.cols(), this.cols(), I18n.PlotDialog_PrimarySource);
                    this.linkTableController = (CellTableController)this.tlk().addTable(input, new CellTableController(this.editor(), SourceReference.class.getField("reference")){

                        @Override
                        protected String getIconId(int rowIndex, ICell cell) {
                            ICell source;
                            if (this.hasCell() && updateBase != null && updateBase.isBound() && updateBase.hasCell(Record.class) && cell != null && (source = ((PlotConfiguration)this.getCell()).getSourceCell((Record)updateBase.getCell(), ((SourceReference)cell).reference)) != null) {
                                return source.getIconId();
                            }
                            return super.getIconId(rowIndex, cell);
                        }
                    }.initCells(null, SourceReference.class).initCheckSource(SourceReference.class.getField("enabled")).initColumnDataSources(false, new Object[]{SourceReference.class.getField("reference"), SourceReference.class.getField("description")}).initLead("in", 40, 1), this.tlk().ld(this.cols() - 1, 524288, 1, 4, 100), 1116259, I18n.PlotDialog_AdditionalSources, new String[]{I18n.PlotDialog_SignalPlot, I18n.General_Description});
                    this.linkTableController.addDropSupport(7, null);
                    this.productionConfig = this.tlk().addGroup(input, new GroupController(this.editor(), PlotConfiguration.class.getField("production")){

                        @Override
                        protected Object convert(Object value) {
                            String label = value instanceof String && productionInformation.get((String)value) != null ? ((IGroupedInformation)productionInformation.get((String)value)).getLabel() : "";
                            return label;
                        }
                    }, this.cols(), this.cols(), 0, I18n.Samples_Production, null);
                    this.addProcessType(this.productionConfig, PlotConfiguration.class.getField("processType"), this.tlk().ld(this.cols() - 1, true, true, true, false), 1);
                    this.addSignalType(this.productionConfig, PlotConfiguration.class.getField("signalType"), this.tlk().ld(this.cols() - 1, true, true, true, false), 1);
                    this.addSignalDescriptor(this.productionConfig, PlotConfiguration.class.getField("signalDescriptor"), this.tlk().ld(this.cols() - 1, true, true, true, false), 0x100001);
                    this.domainClassBase = this.tlk().addComposite(this.productionConfig, null, 2, this.cols(), 1, I18n.Samples_DomainBase_, null);
                    this.addDomainClass(this.domainClassBase, this.tlk().ld(1, 524288, -1), 0);
                    this.addDomainBase(this.domainClassBase, PlotConfiguration.class.getField("domainBase"), this.tlk().ldc(1, 4, 25), 0);
                    this.domainRange = this.tlk().addComposite(this.productionConfig, null, 2, this.tlk().ld(this.cols() - 1, true, true, true, false), 1, I18n.Samples_DomainRange, null);
                    this.addDomainStart(this.domainRange, PlotConfiguration.class.getField("start"), this.tlk().ld(1, 524288, 1), 0x100001);
                    this.addDomainEnd(this.domainRange, PlotConfiguration.class.getField("end"), 2, 0x100001);
                    this.addDomainRate(this.domainRange, PlotConfiguration.class.getField("rate"), 2, 0x100001);
                    ITlkComposite composite = this.tlk().addComposite(this.productionConfig, null, this.cols(), this.tlk().ld(this.cols(), 4, -1, 4, 3), 0x100000, null, null);
                    composite.setBackground("--de-toem-toolkits-tlk-css-color-grey");
                    this.parameters = this.addParameterTabFolder(this.productionConfig, "dialog.samples.production.parameters", "parameters", ISamplesProducer.all, "production");
                    this.definition = this.addDefinition(this.productionConfig, PlotConfiguration.class.getField("definition"), PlotConfiguration.class.getField("language"));
                    this.interpretationExpandable = this.tlk().addExpandable(left, new ExpandableController(this.editor(), "dialog.plot.interpretation", false), this.cols(), this.cols(), 0, I18n.Samples_Interpretation, null);
                    this.logicInterpretation = this.tlk().addGroup(this.interpretationExpandable, null, this.cols(), this.cols(), 0, I18n.Samples_Logic, null);
                    this.tlk().addRadioSet(this.logicInterpretation, new RadioSetController(this.editor(), PlotConfiguration.class.getField("dataInterpretation")), 3, this.cols(), 0, null, new String[]{I18n.Samples_Unsigned, I18n.Samples_Signed, I18n.Samples_IEEE754}, null);
                    this.transformLinear = this.tlk().addGroup(this.interpretationExpandable, null, this.cols(), this.cols(), 0, I18n.Samples_LinearTransform, null);
                    this.tlk().addButton(this.transformLinear, new CheckController(this.editor(), PlotConfiguration.class.getField("transformLinear")), 1, 2048, I18n.Samples_LinearFormula, null);
                    ITlkComposite sub = this.tlk().addComposite(this.transformLinear, null, 4, this.cols() - 1, 0, null, null);
                    this.tlk().addText(sub, new TextController(this.editor(), PlotConfiguration.class.getField("transformLinearM")), 1, 0x100001, I18n.PlotDialog_M);
                    this.tlk().addText(sub, new TextController(this.editor(), PlotConfiguration.class.getField("transformLinearB")), 1, 0x100001, I18n.PlotDialog_B);
                    ITlkExpandable visualisation = this.tlk().addExpandable(left, new ExpandableController(this.editor(), "dialog.plot.visualisation", true), this.cols(), this.cols(), 0, I18n.General_Visualisation, null);
                    this.styleController = new RadioSetController(this.editor(), PlotConfiguration.class.getField("style")){

                        @Override
                        protected boolean locked(Object option) {
                            return ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.diagrams", "de.toem.impulse.feature.default", String.valueOf(option));
                        }
                    };
                    this.tlk().addRadioSet(visualisation, this.styleController, this.tlk().lt(3, 0), this.cols(), 4096, I18n.Plot_PlotType_, PlotConfiguration.STYLE_LABELS, PlotConfiguration.STYLE_IMAGES);
                    String[] options = new String[10];
                    final Object[] colors = new Object[10];
                    int n = 0;
                    while (n < colors.length) {
                        colors[n] = Registration.colors.getRgbInt("de.toem.impulse.color.sample." + n);
                        ++n;
                    }
                    this.tlk().addRadioSet(visualisation, new RadioSetController(this.editor(), PlotConfiguration.class.getField("color")){

                        @Override
                        protected Object convert(Object value) {
                            int n = 0;
                            while (n < colors.length) {
                                if (Utils.equals(value, colors[n])) {
                                    return n;
                                }
                                ++n;
                            }
                            return -1;
                        }

                        @Override
                        protected Object revert(Object value) {
                            int n;
                            if (value instanceof Integer && (n = ((Integer)value).intValue()) >= 0 && n < colors.length) {
                                return colors[n];
                            }
                            return 0;
                        }
                    }, this.tlk().lt(5, 0), this.cols(), 4224, null, options, colors);
                    this.fillStyle(visualisation);
                }
                catch (Throwable e) {
                    SystemLog.log(e);
                }
                return true;
            }

            public boolean fillStyle(Object container) {
                try {
                    this.logicFormat = this.tlk().addGroup(container, null, this.cols(), this.cols(), 0, I18n.PlotDialog_LogicStyle, null);
                    this.addFormatCombos(this.logicFormat, "aValueFormat", I18n.General_Format_);
                    this.tlk().addButton(this.logicFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("relation")){

                        @Override
                        protected boolean locked(Object option) {
                            return ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.diagrams", "de.toem.impulse.feature.diagram.relation", null);
                        }
                    }, this.cols(), 2048, I18n.Plot_Relation, null);
                    this.vectorFormat = this.tlk().addGroup(container, null, this.cols(), this.cols(), 0, I18n.PlotDialog_VectorStyle, null);
                    this.addFormatCombos(this.vectorFormat, "bValueFormat", I18n.General_Format_);
                    this.tlk().addButton(this.vectorFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("relation")){

                        @Override
                        protected boolean locked(Object option) {
                            return ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.diagrams", "de.toem.impulse.feature.diagram.relation", null);
                        }
                    }, this.cols(), 2048, I18n.Plot_Relation, null);
                    this.lineFormat = this.tlk().addGroup(container, null, this.cols(), this.cols(), 0, I18n.PlotDialog_LineStyle, null);
                    this.tlk().addButton(this.lineFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("combine")), this.cols() - 1, 2048, I18n.Plot_Combine, null);
                    this.tlk().addButton(this.lineFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("interpolation")), 1, 2048, "Interpolated", null);
                    this.tlk().addButton(this.lineFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("annotation")), this.cols() - 1, 2048, I18n.Plot_Annotated, null);
                    this.tlk().addButton(this.lineFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("axis")), 1, 2048, I18n.Plot_DrawAxis, null);
                    this.tlk().addButton(this.lineFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("scale")), 1, 2048, I18n.General_Range, null);
                    ITlkComposite sub = this.tlk().addComposite(this.lineFormat, null, 4, this.cols() - 1, 0, null, null);
                    this.tlk().addText(sub, new TextController(this.editor(), PlotConfiguration.class.getField("scaleFrom")), this.tlk().ld(1, 4, 40), 0x100001, I18n.General_From_);
                    this.tlk().addText(sub, new TextController(this.editor(), PlotConfiguration.class.getField("scaleTo")), this.tlk().ld(1, 4, 40), 0x100001, I18n.General_To_);
                    this.tlk().addRadioSet(this.lineFormat, new RadioSetController(this.editor(), PlotConfiguration.class.getField("scaleType")), 3, this.cols(), 1, I18n.Axis_AxisType_, new String[]{I18n.Axis_Linear, I18n.Axis_Log10}, null);
                    this.eventFormat = this.tlk().addGroup(container, null, this.cols(), this.cols(), 0, I18n.PlotDialog_EventStyle, null);
                    this.addFormatCombos(this.eventFormat, "cValueFormat", I18n.General_Format_);
                    this.tlk().addButton(this.eventFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("aModifier")), this.cols() - 1, 2048, I18n.Plot_MultiColor, null);
                    this.tlk().addButton(this.eventFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("relation")){

                        @Override
                        protected boolean locked(Object option) {
                            return ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.diagrams", "de.toem.impulse.feature.diagram.relation", null);
                        }
                    }, 1, 2048, I18n.Plot_Relation, null);
                    this.transactionFormat = this.tlk().addGroup(container, null, this.cols(), this.cols(), 0, I18n.PlotDialog_TransactionStyle, null);
                    this.tlk().addButton(this.transactionFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("relation")){

                        @Override
                        protected boolean locked(Object option) {
                            return ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.diagrams", "de.toem.impulse.feature.diagram.relation", null);
                        }
                    }, this.cols(), 2048, I18n.Plot_Relation, null);
                    this.addParameterTabFolder(this.transactionFormat, "dialog.samples.t.parameters", "styleParameters", p -> new PropertyModel(){

                        @Override
                        public IControlProvider getControls() {
                            return new AbstractControlProvider(){

                                @Override
                                protected boolean fillThis() {
                                    this.tlk().addText(this.container(), new TextController(this.editor(), new PropertySource("filter")), this.tlk().ld(this.cols(), 4, 1), 1, I18n.General_Filter_);
                                    return true;
                                }
                            };
                        }
                    }.add("filter", "", "Filter", "Filter"), "");
                    this.logFormat = this.tlk().addGroup(container, null, this.cols(), this.cols(), 0, I18n.PlotDialog_LogStyle, null);
                    this.tlk().addButton(this.logFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("relation")){

                        @Override
                        protected boolean locked(Object option) {
                            return ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.diagrams", "de.toem.impulse.feature.diagram.relation", null);
                        }
                    }, this.cols(), 2048, I18n.Plot_Relation, null);
                    this.addParameterTabFolder(this.logFormat, "dialog.samples.log.parameters", "styleParameters", p -> new PropertyModel(){

                        @Override
                        public IControlProvider getControls() {
                            return new AbstractControlProvider(){

                                @Override
                                protected boolean fillThis() {
                                    this.tlk().addText(this.container(), new TextController(this.editor(), new PropertySource("filter")), this.tlk().ld(this.cols(), 4, 1), 1, I18n.General_Filter_);
                                    return true;
                                }
                            };
                        }
                    }.add("filter", "", "Filter", "Filter"), "");
                    this.chartFormat = this.tlk().addGroup(container, null, this.cols(), this.cols(), 0, I18n.PlotDialog_ChartStyle, null);
                    ITlkComposite chartComposite = this.tlk().addComposite(this.chartFormat, null, 4, this.cols(), 1, I18n.General_Chart_, null);
                    this.tlk().addImage(chartComposite, new ImageController(this.editor(), PlotConfiguration.class.getField("styleDescriptor")){

                        @Override
                        protected Object convert(Object value) {
                            return value instanceof Link ? ((Link)value).resolveCell(ImpulsePreferences.chartPreferences) : null;
                        }
                    }, this.tlk().ld(1, 0x1000000, 20), 0, null);
                    final 15 primarySourceController = this.tlk().addText(chartComposite, new TextController(this.editor(), PlotConfiguration.class.getField("styleDescriptor")){

                        @Override
                        protected Object convert(Object value) {
                            if (value instanceof Link) {
                                return ((Link)value).toString();
                            }
                            return value;
                        }

                        @Override
                        protected Object revert(Object value) {
                            if (value instanceof String) {
                                return Link.parse((String)value);
                            }
                            return value;
                        }
                    }, this.tlk().ld(1, 524288, 1), 0, null);
                    this.tlk().addButton(chartComposite, new ButtonController(this.editor(), null){

                        @Override
                        public void execute(String id, Object data) {
                            ICell cell = ImpulsePreferences.getChart(primarySourceController.getValueAsString());
                            String dialog = Dialogs.getForElement(Elements.getElement(cell));
                            if (dialog != null) {
                                this.tlk().openDialog(dialog, this.editor(), Elements.getElements(cell), false, null);
                            }
                        }
                    }, 1, 4096, "", "de-toem-toolkits-tlk-css-general-edit-image");
                    AbstractControlProvider chartControls = new AbstractControlProvider(){
                        IController cellController;

                        @Override
                        protected boolean fillThis() {
                            Class<AbstractChartCell> childClass = AbstractChartCell.class;
                            try {
                                this.cellController = new CellTableController(this.editor(), PlotConfiguration.class.getField("styleDescriptor")){

                                    @Override
                                    public void selectionChanged() {
                                        this.tlk().updateEnable();
                                        super.selectionChanged();
                                    }
                                }.initCells(ImpulsePreferences.chartPreferences, childClass).initColumnDataSources(true, new Object[]{childClass.getField("description")});
                                this.tlk().addSpace(this.container(), this.tlk().ld(this.cols(), 4, this.tlk().wc(60)));
                                this.tlk().addTable(this.container(), this.cellController, this.tlk().ld(this.cols() - 1, 524288, 1, 4, 250), 67138, null, new String[]{I18n.General_Name, I18n.General_Description});
                                ITlkComposite buttonComp = this.tlk().addComposite(this.container(), null, 1, this.tlk().ld(1, 4, -1, 128, -1), 0, null, null);
                                CommandButtonController.addEditDefaultButtons(this.tlk(), buttonComp, this.editor(), this.cellController, true, true, true, false);
                            }
                            catch (NoSuchFieldException | SecurityException exception) {}
                            return false;
                        }

                        @Override
                        public void setFocus(boolean force) {
                            this.tlk().setFocus(this.cellController, force);
                        }
                    };
                    this.tlk().addInPlaceDialog(this.chartFormat, chartComposite, chartControls, this.cols(), this.cols(), I18n.General_Chart);
                    this.addParameterTabFolder(this.chartFormat, "dialog.samples.chart.parameters", "styleParameters", ImpulsePreferences.chartPreferences, "styleDescriptor");
                    this.tlk().addButton(this.chartFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("combine")), 1, 2048, I18n.Plot_Combine, null);
                    this.areaFormat = this.tlk().addGroup(container, null, this.cols(), this.cols(), 0, I18n.PlotDialog_AreaStyle, null);
                    this.tlk().addButton(this.areaFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("combine")), 1, 2048, I18n.Plot_Combine, null);
                    this.tlk().addButton(this.areaFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("interpolation")), 1, 2048, I18n.Plot_Interpolated, null);
                    this.tlk().addButton(this.areaFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("annotation")){

                        @Override
                        protected boolean locked(Object option) {
                            return ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.diagrams", "de.toem.impulse.feature.diagram.label", null);
                        }
                    }, 1, 2048, I18n.Plot_Annotated, null);
                    this.tlk().addButton(this.areaFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("axis")), this.cols() - 2, 2048, I18n.Plot_DrawAxis, null);
                    this.tlk().addButton(this.areaFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("bModifier")), 1, 2048, I18n.Plot_Transparent, null);
                    this.tlk().addButton(this.areaFormat, new CheckController(this.editor(), PlotConfiguration.class.getField("scale")), 1, 2048, I18n.General_Range, null);
                    sub = this.tlk().addComposite(this.areaFormat, null, 4, this.cols() - 1, 0, null, null);
                    this.tlk().addText(sub, new TextController(this.editor(), PlotConfiguration.class.getField("scaleFrom")), this.tlk().ld(1, 4, 40), 0x100001, I18n.General_From_);
                    this.tlk().addText(sub, new TextController(this.editor(), PlotConfiguration.class.getField("scaleTo")), this.tlk().ld(1, 4, 40), 0x100001, I18n.General_To_);
                    this.tlk().addRadioSet(this.areaFormat, new RadioSetController(this.editor(), PlotConfiguration.class.getField("scaleType")), 3, this.cols(), 1, I18n.Axis_AxisType_, new String[]{I18n.Axis_Linear, I18n.Axis_Log10}, null);
                    this.gantFormat = this.tlk().addGroup(container, null, this.cols(), this.cols(), 0, I18n.PloDialogt_GantStyle, null);
                    this.addFormatCombos(this.gantFormat, "bValueFormat", I18n.General_Format_);
                    ITlkComposite flags = this.tlk().addComposite(this.gantFormat, null, this.cols(), this.cols(), 0, null, null);
                    this.tlk().addButton(flags, new CheckController(this.editor(), PlotConfiguration.class.getField("aModifier")), this.cols() - 2, 2048, I18n.Plot_MultiColor, null);
                    this.tlk().addButton(flags, new CheckController(this.editor(), PlotConfiguration.class.getField("relation")), 1, 2048, I18n.Plot_Relation, null);
                    this.tlk().addButton(flags, new CheckController(this.editor(), PlotConfiguration.class.getField("annotation")), 1, 2048, I18n.Plot_Annotated, null);
                    ITlkGroup defaultValueColumn = this.tlk().addGroup(container, null, this.cols(), this.cols(), 0, I18n.PlotDialog_DefaultValueColumnStyle, null);
                    this.addFormatCombos(defaultValueColumn, "columnValueFormat", null);
                    ITlkGroup markerFormat = this.tlk().addGroup(container, null, this.cols(), this.cols(), 0, I18n.Marker_Markers, null);
                    this.tlk().addRadioSet(markerFormat, new RadioSetController(this.editor(), PlotConfiguration.class.getField("markerPresentation")), 3, this.cols(), 1, I18n.General_Show_, PlotConfiguration.markerOptions, null);
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
                return true;
            }

            public boolean fillTemplate(Object container) throws NoSuchFieldException {
                try {
                    final CellTableController templateTableController = this.tlk().addTable(container, new CellTableController(this.editor(), null, PlotConfigurationTemplate.class){

                        @Override
                        protected void setContextMenus() {
                        }

                        @Override
                        public void selectionChanged() {
                            this.tlk().updateEnable();
                            super.selectionChanged();
                        }

                        @Override
                        public boolean needsUpdate() {
                            return true;
                        }

                        @Override
                        public boolean isAffected(ElementModifierEvent event) {
                            return true;
                        }

                        @Override
                        protected List<ICell> getRawTableCells() {
                            if (ImpulsePreferences.templatePreferences.isBound() && ImpulsePreferences.templatePreferences.hasCell()) {
                                return ImpulsePreferences.templatePreferences.getCell().getChildren();
                            }
                            return ICell.EMPTY_LIST;
                        }

                        @Override
                        protected boolean filterCells(ICell cell) {
                            return !((PlotConfigurationTemplate)cell).enabled || !((PlotConfigurationTemplate)cell).hasChildren() || ((PlotConfigurationTemplate)cell).getChildren().size() != 1 || !(((PlotConfigurationTemplate)cell).getChildren().get(0) instanceof PlotConfiguration) || ((PlotConfigurationTemplate)cell).getChildren().get(0).hasChildren();
                        }

                        @Override
                        public void execute(String id, Object data) {
                            ICell template;
                            List<ICell> selection = this.getSelectedCells();
                            if (selection != null && selection.size() == 1 && (template = selection.get(0)) != null && template.hasChildren()) {
                                Field[] f;
                                ICell child = template.getChild(0).clone();
                                ArrayList<Field[]> fields = new ArrayList<Field[]>();
                                Field[] fieldArray = PlotConfiguration.class.getFields();
                                int n = fieldArray.length;
                                int n2 = 0;
                                while (n2 < n) {
                                    f = fieldArray[n2];
                                    if (!(f.getName().equals("name") || f.getName().equals("description") || f.getName().equals("samples"))) {
                                        fields.add(f);
                                    }
                                    ++n2;
                                }
                                f = fields.toArray(new Field[fields.size()]);
                                ArrayList<ElementCellModifier> modifiers = new ArrayList<ElementCellModifier>();
                                for (ICell c : this.getCells()) {
                                    if (c == null || !c.isBound()) continue;
                                    modifiers.add(new ElementCellModifier(c.getElement(), f, child));
                                }
                                this.editor().apply(null, null, modifiers.toArray(new IElementModifier[modifiers.size()]), true);
                            }
                        }

                        @Override
                        protected boolean allowModification(IController.Commands command, Object data, Object context) {
                            return false;
                        }
                    }.initColumnDataSources(true, new Object[]{AbstractPreferenceCell.class.getField("description")}), this.tlk().ldc(this.cols() - 1, 524288, 1, 4, 8), 67138, null, new String[]{I18n.General_Name, I18n.General_Description});
                    ITlkComposite buttons = this.tlk().addComposite(container, null, this.cols(), this.tlk().ld(1, 4, -1, 1024, -1), 0, null, null);
                    ButtonController button = this.tlk().addButton(buttons, new ButtonController(this.editor(), null){

                        @Override
                        public void execute(String id, Object data) {
                            templateTableController.execute(id, data);
                        }

                        @Override
                        public boolean enabled() {
                            List<ICell> selection = templateTableController.getSelectedCells();
                            return selection != null && selection.size() == 1 && super.enabled();
                        }
                    }, this.tlk().ld(this.cols(), 4, this.tlk().bh(), 1024, this.tlk().bh()), 0, ">", null);
                    button.setTooltip(I18n.PlotDialog_TakeTemplate);
                    button = this.tlk().addButton(buttons, new ButtonController(this.editor(), null){

                        @Override
                        public boolean enabled() {
                            return super.enabled() && !this.hasCells();
                        }
                    }, this.tlk().ld(this.cols(), 4, this.tlk().bh(), 1024, this.tlk().bh()), 0, "", "de.toem.eclipse.toolkits.images.general.add");
                    button.setTooltip(I18n.PlotDialog_AddTemplate);
                }
                catch (SecurityException securityException) {}
                return true;
            }

            @Override
            public void updateThis(Object user) {
                IContext context;
                if (user instanceof Object[]) {
                    this.updateElements = (List)((Object[])user)[0];
                    this.updateBase = (IElement)((Object[])user)[1];
                }
                IContext iContext = context = this.updateBase != null && this.updateBase.isBound() && this.updateBase.hasCell(Record.class) ? (Record)this.updateBase.getCell() : null;
                if (this.editor().getContainerPart() instanceof IContext) {
                    context = (IContext)((Object)this.editor().getContainerPart());
                }
                context = new DerivedSamplesContext(context, 1);
                boolean error = false;
                boolean showLogicInterpretation = true;
                boolean showTransformLinear = true;
                boolean doShowAll = false;
                boolean offerLogic = true;
                boolean offerVector = true;
                boolean offerLine = true;
                boolean offerEvent = true;
                boolean offerTransaction = true;
                boolean offerLog = true;
                boolean offerImage = true;
                boolean offerChart = true;
                boolean offerArea = true;
                boolean offerGant = true;
                String processTypeNullText = null;
                String signalTypeNullText = null;
                String signalDescriptorNullText = null;
                String domainClassNullText = null;
                String domainBaseNullText = null;
                String domainStartNullText = null;
                String domainEndNullText = null;
                String domainRateNullText = null;
                IDomainBase derivedDomainBase = null;
                ISamples.SignalType signalType = null;
                ISamples.SignalDescriptor signalDescriptor = null;
                boolean showCollectionFormats = true;
                ArrayList<IMemberDescriptor> sourceMembers = new ArrayList<IMemberDescriptor>();
                this.editor().setData("members", sourceMembers);
                if (this.updateElements != null) {
                    for (IElement element : this.updateElements) {
                        if (element.isBound() && element.hasCell(PlotConfiguration.class)) {
                            ISamples.SignalType type = ISamples.SignalType.Unknown;
                            ISamples.SignalDescriptor descriptor = ISamples.SignalDescriptor.DEFAULT;
                            PlotConfiguration config = (PlotConfiguration)element.getCell();
                            IReadableSamples readable = null;
                            IReadableSamples interpreted = null;
                            interpreted = config.getSamples(context);
                            IReadableSamples iReadableSamples = readable = interpreted != null ? interpreted.getProducer() : null;
                            if (readable == null) {
                                IReadableSamples iReadableSamples2 = readable = interpreted != null ? interpreted.getReader() : null;
                            }
                            if (readable != null) {
                                type = readable.getSignalType();
                                descriptor = readable.getSignalDescriptor();
                                processTypeNullText = this.qualOrEmpty(processTypeNullText, Utils.isEmpty(this.processType.getValueAsString()) ? String.valueOf(I18n.General_Derived) + " (" + (Object)((Object)readable.getProcessType()) + ")" : I18n.General_Derived);
                                signalTypeNullText = this.qualOrEmpty(signalTypeNullText, Utils.isEmpty(this.signalType.getValueAsString()) && type != null ? String.valueOf(I18n.General_Derived) + " (" + (Object)((Object)type) + ")" : I18n.General_Derived);
                                signalDescriptorNullText = this.qualOrEmpty(signalDescriptorNullText, Utils.isEmpty(this.signalDescriptor.getValueAsString()) && type != null && descriptor != null ? String.valueOf(I18n.General_Derived) + " (" + descriptor.toUserString(type) + ")" : I18n.General_Derived);
                                domainClassNullText = this.qualOrEmpty(domainClassNullText, String.valueOf(I18n.General_Derived) + " (" + readable.getDomainClass() + ")");
                                domainBaseNullText = this.qualOrEmpty(domainBaseNullText, String.valueOf(I18n.General_Derived) + " (" + readable.getDomainBase() + ")");
                                domainStartNullText = this.qualOrEmpty(domainStartNullText, String.valueOf(I18n.General_Derived) + " (" + readable.getStart() + ")");
                                domainEndNullText = this.qualOrEmpty(domainEndNullText, String.valueOf(I18n.General_Derived) + " (" + readable.getEnd() + ")");
                                domainRateNullText = this.qualOrEmpty(domainRateNullText, String.valueOf(I18n.General_Derived) + " (" + readable.getRate() + ")");
                                derivedDomainBase = this.qualOrEmpty(derivedDomainBase, readable.getDomainBase());
                                if (readable instanceof ISamplesProducer) {
                                    for (IReadableSamples s : ((ISamplesProducer)readable).getSources()) {
                                        List<IMemberDescriptor> members;
                                        List<IMemberDescriptor> list = members = s != null ? s.getMemberDescriptors() : null;
                                        if (members == null) continue;
                                        sourceMembers.addAll(members);
                                    }
                                } else {
                                    List<IMemberDescriptor> members = readable.getMemberDescriptors();
                                    if (members != null) {
                                        sourceMembers.addAll(members);
                                    }
                                }
                            }
                            if (signalType == null) {
                                signalType = type;
                            } else if (!signalType.equals((Object)type)) {
                                signalType = ISamples.SignalType.Unknown;
                            }
                            if (signalDescriptor == null) {
                                signalDescriptor = descriptor;
                            } else if (!signalDescriptor.equals(descriptor)) {
                                signalDescriptor = ISamples.SignalDescriptor.DEFAULT;
                            }
                            if (type != ISamples.SignalType.Logic) {
                                showLogicInterpretation = false;
                            }
                            if (type != ISamples.SignalType.Logic && (type != ISamples.SignalType.Integer && type != ISamples.SignalType.Float || descriptor.getScale() != 1)) {
                                showTransformLinear = false;
                            }
                            if (type == ISamples.SignalType.Logic) {
                                showCollectionFormats = false;
                                offerGant = false;
                                offerImage = false;
                                offerLog = false;
                                offerTransaction = false;
                                continue;
                            }
                            if (type == ISamples.SignalType.Integer) {
                                showCollectionFormats = false;
                                offerGant = false;
                                offerImage = false;
                                offerLog = false;
                                offerTransaction = false;
                                offerLogic = false;
                                continue;
                            }
                            if (type == ISamples.SignalType.IntegerArray) {
                                offerArea = false;
                                offerLine = false;
                                offerGant = false;
                                offerImage = false;
                                offerTransaction = false;
                                offerLogic = false;
                                continue;
                            }
                            if (type == ISamples.SignalType.Float) {
                                showCollectionFormats = false;
                                offerGant = false;
                                offerImage = false;
                                offerLog = false;
                                offerTransaction = false;
                                offerLogic = false;
                                continue;
                            }
                            if (type == ISamples.SignalType.FloatArray) {
                                offerArea = false;
                                offerLine = false;
                                offerGant = false;
                                offerImage = false;
                                offerTransaction = false;
                                offerLogic = false;
                                continue;
                            }
                            if (type == ISamples.SignalType.Event) {
                                showCollectionFormats = false;
                                offerImage = false;
                                offerLog = false;
                                offerTransaction = false;
                                offerLogic = false;
                                continue;
                            }
                            if (type == ISamples.SignalType.EventArray) {
                                offerArea = false;
                                offerLine = false;
                                offerImage = false;
                                offerTransaction = false;
                                offerLogic = false;
                                continue;
                            }
                            if (type == ISamples.SignalType.Struct && !descriptor.isTransaction()) {
                                offerImage = false;
                                offerTransaction = false;
                                offerArea = false;
                                offerLine = false;
                                offerLogic = false;
                                continue;
                            }
                            if (type == ISamples.SignalType.Struct && descriptor.isTransaction()) {
                                offerGant = false;
                                offerChart = false;
                                offerImage = false;
                                offerLogic = false;
                                offerArea = false;
                                offerLine = false;
                                offerLogic = false;
                                continue;
                            }
                            if (type == ISamples.SignalType.Text) {
                                offerGant = false;
                                offerImage = false;
                                offerArea = false;
                                offerLine = false;
                                offerLog = false;
                                offerTransaction = false;
                                offerLogic = false;
                                showCollectionFormats = false;
                                continue;
                            }
                            if (type == ISamples.SignalType.TextArray) {
                                offerGant = false;
                                offerImage = false;
                                offerArea = false;
                                offerLine = false;
                                offerTransaction = false;
                                offerLogic = false;
                                continue;
                            }
                            if (type == ISamples.SignalType.Binary && descriptor.isImage()) {
                                showCollectionFormats = false;
                                offerGant = false;
                                offerLogic = false;
                                offerArea = false;
                                offerLine = false;
                                offerTransaction = false;
                                offerLog = false;
                                offerLogic = false;
                                continue;
                            }
                            if (type == ISamples.SignalType.Binary) {
                                showCollectionFormats = false;
                                offerGant = false;
                                offerLogic = false;
                                offerImage = false;
                                offerArea = false;
                                offerLine = false;
                                offerTransaction = false;
                                offerLog = false;
                                offerLogic = false;
                                continue;
                            }
                            offerGant = false;
                            offerChart = false;
                            offerImage = false;
                            offerLogic = false;
                            offerArea = false;
                            offerLine = false;
                            offerLog = false;
                            offerTransaction = false;
                            offerEvent = false;
                            offerVector = false;
                            offerLogic = false;
                            doShowAll = true;
                            showCollectionFormats = true;
                            continue;
                        }
                        error = true;
                    }
                }
                boolean bl = !error;
                boolean changed = false;
                changed |= this.showControl((showLogicInterpretation &= !error) | doShowAll, (ITlkControl)this.logicInterpretation);
                changed |= this.showControl((showTransformLinear &= bl) | doShowAll, (ITlkControl)this.transformLinear);
                changed |= this.showControl(showLogicInterpretation | showTransformLinear | doShowAll, (ITlkControl)this.interpretationExpandable);
                Object style = this.styleController.getValue();
                changed |= this.showControl(style != null && style.equals(1), (ITlkControl)this.logicFormat);
                changed |= this.showControl(style != null && style.equals(2), (ITlkControl)this.vectorFormat);
                changed |= this.showControl(style != null && style.equals(3), (ITlkControl)this.eventFormat);
                changed |= this.showControl(style != null && style.equals(4), (ITlkControl)this.lineFormat);
                changed |= this.showControl(style != null && style.equals(5), (ITlkControl)this.transactionFormat);
                changed |= this.showControl(style != null && style.equals(6), (ITlkControl)this.logFormat);
                changed |= this.showControl(style != null && style.equals(8), (ITlkControl)this.chartFormat);
                changed |= this.showControl(style != null && style.equals(9), (ITlkControl)this.areaFormat);
                changed |= this.showControl(style != null && style.equals(10), (ITlkControl)this.gantFormat);
                changed |= this.offerStyle(1, offerLogic | doShowAll);
                changed |= this.offerStyle(2, offerVector | doShowAll);
                changed |= this.offerStyle(3, offerEvent | doShowAll);
                changed |= this.offerStyle(4, offerLine | doShowAll);
                changed |= this.offerStyle(5, offerTransaction | doShowAll);
                changed |= this.offerStyle(6, offerLog | doShowAll);
                changed |= this.offerStyle(7, offerImage | doShowAll);
                changed |= this.offerStyle(8, offerChart | doShowAll);
                changed |= this.offerStyle(9, offerArea | doShowAll);
                changed |= this.offerStyle(10, offerGant | doShowAll);
                Object[][] options = ConvertedSamples.formatValueExampleOptions(ISamples.SignalType.Unknown, ISamples.SignalDescriptor.DEFAULT);
                for (IController c : this.tlk().getControllers()) {
                    if (!(c instanceof ComboController) || !c.getName().contains("ValueFormat")) continue;
                    if (((ComboController)c).getIntegerMask() == -65536) {
                        if (c.isEnabled() == showCollectionFormats) continue;
                        c.setEnabled(showCollectionFormats, false);
                        c.updateControl(true);
                        changed = true;
                        continue;
                    }
                    if (((ComboController)c).getIntegerMask() != 65535) continue;
                    ((ComboController)c).setOptions((String[])options[0], options[1], (String[])options[2]);
                    c.updateControl(true);
                }
                this.processType.setNullItem(processTypeNullText != null ? processTypeNullText : "");
                this.signalType.setNullItem(signalTypeNullText != null ? signalTypeNullText : "");
                this.signalDescriptor.setNullText(signalDescriptorNullText != null ? signalDescriptorNullText : "");
                this.domainClass.setNullItem(domainClassNullText != null ? domainClassNullText : "");
                this.domainBase.setNullItem(domainBaseNullText != null ? domainBaseNullText : "");
                this.domainStart.setNullText(domainStartNullText != null ? domainStartNullText : "");
                this.domainEnd.setNullText(domainEndNullText != null ? domainEndNullText : "");
                this.domainRate.setNullText(domainRateNullText != null ? domainRateNullText : "");
                this.processType.updateControl(true);
                this.signalType.updateControl(true);
                this.signalDescriptor.updateControl(true);
                this.domainClass.updateControl(true);
                this.domainBase.updateControl(true);
                this.domainStart.updateControl(true);
                this.domainEnd.updateControl(true);
                this.domainRate.updateControl(true);
                this.derivedDomainBase = derivedDomainBase != null && derivedDomainBase != UnknownBase.Unknown ? derivedDomainBase : null;
                this.primarySourceImageController.updateControl(true);
                this.linkTableController.updateControl(true);
                if (changed) {
                    this.tlk().reflow(true, true);
                }
            }

            boolean offerStyle(int style, boolean offer) {
                if (this.styleController.isOptionEnabled(style) == offer) {
                    return false;
                }
                this.styleController.setOptionEnabled(style, offer, false);
                return true;
            }

            private void prepareAvailableFormats(Set<Integer> availableFormatSet, int[] avalilableFormats) {
                Iterator<Integer> iter = availableFormatSet.iterator();
                while (iter.hasNext()) {
                    int val = iter.next();
                    boolean found = false;
                    int[] nArray = avalilableFormats;
                    int n = avalilableFormats.length;
                    int n2 = 0;
                    while (n2 < n) {
                        int n3 = nArray[n2];
                        if (n3 == val) {
                            found = true;
                            break;
                        }
                        ++n2;
                    }
                    if (found) continue;
                    iter.remove();
                }
            }

            String qualOrEmpty(String ref, String obj) {
                if (ref == null) {
                    return obj;
                }
                if (ref.isEmpty()) {
                    return "";
                }
                if (ref.equals(obj)) {
                    return ref;
                }
                return "";
            }

            IDomainBase qualOrEmpty(IDomainBase ref, IDomainBase obj) {
                if (ref == null) {
                    return obj;
                }
                if (ref.equals(UnknownBase.Unknown)) {
                    return UnknownBase.Unknown;
                }
                if (ref.equals(obj)) {
                    return ref;
                }
                return UnknownBase.Unknown;
            }
        };
        provider.setCellClass(PlotConfiguration.class);
        return provider;
    }
}

