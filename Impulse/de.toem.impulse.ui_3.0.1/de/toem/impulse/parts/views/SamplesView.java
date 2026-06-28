/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.parts.views;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cell.parts.SamplesViewMemberPreferences;
import de.toem.impulse.cell.parts.SamplesViewPreferences;
import de.toem.impulse.cells.record.PortScope;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.RecordContent;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.dialog.parts.SamplesViewPreferenceDialog;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.parts.views.SampleViewDialog;
import de.toem.impulse.parts.views.SubView;
import de.toem.impulse.provider.ISimpleSamplesProvider;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IPointer;
import de.toem.impulse.samples.IReadableGroup;
import de.toem.impulse.samples.IReadableMembers;
import de.toem.impulse.samples.IReadableSample;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.IReadableValue;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesDisplayInformation;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.iterator.AbstractPointer;
import de.toem.impulse.samples.iterator.GroupPointer;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.producer.AbstractSamplesFilter;
import de.toem.impulse.samples.producer.SamplesMerger;
import de.toem.impulse.ui.DomainPosition;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.impulse.values.AttachedLabel;
import de.toem.impulse.values.AttachedRelation;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.ElementModifierEvent;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.element.exploits.Markers;
import de.toem.toolkits.pattern.filter.FilterExpression;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.pattern.threading.Progress;
import de.toem.toolkits.ui.controller.base.TableController;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SamplesView
extends SubView {
    public static final String POSITION_COLUMN = I18n.SamplesView_Position;
    public static final String DOMAIN_END_COLUMN = I18n.Samples_DomainEnd;
    public static final String VALUE_COLUMN = I18n.SamplesView_Value;
    public static final String SIGNAL_COLUMN = I18n.SamplesView_Signal;
    public static final String GROUP_COLUMN = I18n.SamplesView_Group;
    public static final String ORDER_COLUMN = I18n.SamplesView_Order;
    public static final String SAMPLES_COLUMN = I18n.SamplesView_Samples;
    public static final String LABEL_COLUMN = I18n.SamplesView_Label;
    public static final String RELATION_COLUMN = I18n.SamplesView_Relation;
    public static final String TAG_COLUMN = I18n.SamplesView_Tag;
    private TableController tableController;
    private List<String> columns = Arrays.asList(POSITION_COLUMN, VALUE_COLUMN);
    private List<String> memberColumns;
    private int rows;
    private boolean reverse = false;
    private List<String> filteredColumns;
    private Map<String, FilterExpression> filterDefinitions;
    private SamplesViewPreferences defaultPreferences = new SamplesViewPreferences();
    protected IExecutable delayedApplyPresentation = p -> this.applyPresentation();
    private int applyPresentationCounter;

    public SamplesView(ITlkPartContainer partContainer, int style) {
        super(partContainer, style);
    }

    public SamplesView(ITlkPartContainer.ITlkEditorSession session, int style) {
        super(session, style);
    }

    public SamplesView() {
    }

    @Override
    protected void createControls(Object container) {
        super.createControls(container);
        this.tableController = new TableController(this, null){
            private SampleDialogInput sampleDialogInput;

            @Override
            protected void setContextMenus() {
                this.tlk().addMenu(this.control, this, "MENU", "de.toem.eclipse.toolkits.popupmenu.table", "de.toem.impulse.menu.samples.context", new String[]{"add", "insert", "clone", "cut", "paste", "delete", "rename", "updown"});
            }

            @Override
            public boolean needsUpdate() {
                return true;
            }

            @Override
            protected boolean contentToBeChanged(Object value, Markers markers, boolean enabled) {
                return true;
            }

            @Override
            public void doUpdateControl() {
                this.access(table -> {
                    SamplesView.this.rows = SamplesView.rowCount(SamplesView.this.readable, SamplesView.this.showGroups);
                    String[] columns = SamplesView.this.columns != null ? SamplesView.this.columns.toArray(new String[SamplesView.this.columns.size()]) : null;
                    boolean showN = SamplesView.this.readable instanceof AbstractSamplesFilter || SamplesView.this.readable instanceof SamplesMerger;
                    table.setColumns(columns, String.valueOf(showN ? "N/" : "") + (SamplesView.this.showGroups ? "Group(#)" : "Index(@)"));
                    super.doUpdateControl();
                    SamplesView.this.index = SamplesView.this.positionToIndex(SamplesView.this.position);
                    SamplesView.this.tableController.selectRow(SamplesView.this.sortIdx(SamplesView.this.index), false);
                    Utils.log("after", SamplesView.this.position, SamplesView.this.index);
                    if (this.sampleDialogInput != null) {
                        this.sampleDialogInput.dataChanged();
                    }
                });
            }

            @Override
            public int getRowCount() {
                return SamplesView.this.rows;
            }

            @Override
            protected String getLead(int rowIndex) {
                return SamplesView.this.getLead(rowIndex, false);
            }

            @Override
            protected String getRowHeaderTooltip(int rowIndex, int headerColumnIndex) {
                if (SamplesView.this.inputBySamples == null || SamplesView.this.readable == null || SamplesView.this.readable.getDomainBase() == null) {
                    return null;
                }
                rowIndex = SamplesView.this.sortIdx(rowIndex);
                if (headerColumnIndex == 0) {
                    IReadableValue value;
                    boolean showN;
                    ISamples iSamples = SamplesView.this.readable instanceof AbstractSamplesFilter ? ((AbstractSamplesFilter)SamplesView.this.readable).getReference() : SamplesView.this.readable;
                    boolean bl = showN = SamplesView.this.readable instanceof AbstractSamplesFilter || SamplesView.this.readable instanceof SamplesMerger;
                    if (showN && (value = SamplesView.dataValue(SamplesView.this.readable, SamplesView.this.showGroups, rowIndex, false)) != null) {
                        return String.valueOf(I18n.General_Index_) + " " + String.valueOf(rowIndex) + "\n" + I18n.General_SourceIndex_ + " " + String.valueOf(value.getIndex());
                    }
                    return String.valueOf(I18n.General_Index_) + " " + String.valueOf(rowIndex);
                }
                return null;
            }

            @Override
            public Object getDataValue(int rowIndex, int columnIndex, String columnName) {
                return SamplesView.this.getDataValue(SamplesView.this.readable, SamplesView.this.inputBySamples, columnName, rowIndex);
            }

            @Override
            protected int getColumnFormat(int columnIndex, String columnName) {
                ICell memberPreferences;
                int format = 0;
                if (columnName.equals(POSITION_COLUMN)) {
                    format = (int)((long)format | SamplesViewPreferences.alignment2TableFormat(SamplesView.this.getPreferences().domainAlignment));
                    format = (int)((long)format | SamplesViewPreferences.fixedFont2TableFormat(SamplesView.this.getPreferences().domainFixedFont));
                } else if (columnName.equals(DOMAIN_END_COLUMN)) {
                    format = (int)((long)format | SamplesViewPreferences.alignment2TableFormat(SamplesView.this.getPreferences().domainAlignment));
                    format = (int)((long)format | SamplesViewPreferences.fixedFont2TableFormat(SamplesView.this.getPreferences().domainFixedFont));
                } else if (columnName.equals(VALUE_COLUMN)) {
                    format = (int)((long)format | SamplesViewPreferences.alignment2TableFormat(SamplesView.this.getPreferences().valueAlignment));
                    format = (int)((long)format | SamplesViewPreferences.fixedFont2TableFormat(SamplesView.this.getPreferences().valueFixedFont));
                    format = (int)((long)format | SamplesViewPreferences.wrap2TableFormat(SamplesView.this.getPreferences().valueWrap));
                } else if (columnName.equals(SIGNAL_COLUMN)) {
                    format = (int)((long)format | SamplesViewPreferences.alignment2TableFormat(SamplesView.this.getPreferences().signalAlignment));
                    format = (int)((long)format | SamplesViewPreferences.fixedFont2TableFormat(SamplesView.this.getPreferences().signalFixedFont));
                } else if (columnName.equals(GROUP_COLUMN)) {
                    format = (int)((long)format | SamplesViewPreferences.alignment2TableFormat(SamplesView.this.getPreferences().groupAlignment));
                    format = (int)((long)format | SamplesViewPreferences.fixedFont2TableFormat(SamplesView.this.getPreferences().groupFixedFont));
                } else if (columnName.equals(ORDER_COLUMN)) {
                    format = (int)((long)format | SamplesViewPreferences.alignment2TableFormat(SamplesView.this.getPreferences().orderAlignment));
                    format = (int)((long)format | SamplesViewPreferences.fixedFont2TableFormat(SamplesView.this.getPreferences().orderFixedFont));
                } else if (columnName.equals(SAMPLES_COLUMN)) {
                    format = (int)((long)format | SamplesViewPreferences.alignment2TableFormat(SamplesView.this.getPreferences().samplesAlignment));
                    format = (int)((long)format | SamplesViewPreferences.fixedFont2TableFormat(SamplesView.this.getPreferences().samplesFixedFont));
                } else if (columnName.equals(LABEL_COLUMN)) {
                    format = (int)((long)format | SamplesViewPreferences.alignment2TableFormat(SamplesView.this.getPreferences().labelAlignment));
                    format = (int)((long)format | SamplesViewPreferences.fixedFont2TableFormat(SamplesView.this.getPreferences().labelFixedFont));
                } else if (columnName.equals(RELATION_COLUMN)) {
                    format = (int)((long)format | SamplesViewPreferences.alignment2TableFormat(SamplesView.this.getPreferences().relationAlignment));
                    format = (int)((long)format | SamplesViewPreferences.fixedFont2TableFormat(SamplesView.this.getPreferences().relationFixedFont));
                } else if (columnName.equals(TAG_COLUMN)) {
                    format = (int)((long)format | SamplesViewPreferences.alignment2TableFormat(SamplesView.this.getPreferences().tagAlignment));
                    format = (int)((long)format | SamplesViewPreferences.fixedFont2TableFormat(SamplesView.this.getPreferences().tagFixedFont));
                } else if (columnName.startsWith("[") && (memberPreferences = SamplesView.this.getPreferences().getChildByName(columnName)) instanceof SamplesViewMemberPreferences) {
                    format = (int)((long)format | SamplesViewPreferences.alignment2TableFormat(((SamplesViewMemberPreferences)memberPreferences).memberAlignment));
                    format = (int)((long)format | SamplesViewPreferences.fixedFont2TableFormat(((SamplesViewMemberPreferences)memberPreferences).memberFixedFont));
                    format = (int)((long)format | SamplesViewPreferences.wrap2TableFormat(((SamplesViewMemberPreferences)memberPreferences).memberWrap));
                }
                return format;
            }

            @Override
            protected int getRowFormat(int rowIndex) {
                if (SamplesView.this.inputBySamples == null || SamplesView.this.readable == null || SamplesView.this.readable.getDomainBase() == null) {
                    return 0;
                }
                int format = 0;
                if (SamplesView.this.getPreferences().tagBackground && SamplesView.this.readable != null) {
                    int tag;
                    IReadableValue value = SamplesView.this.getReadableValue(SamplesView.this.readable, rowIndex, false);
                    int n = tag = value != null ? value.getTag() : 0;
                    if (tag >= 8) {
                        format |= 0x3C0;
                    } else {
                        switch (tag) {
                            case 1: {
                                format |= 0x200;
                                break;
                            }
                            case 2: {
                                format |= 0x240;
                                break;
                            }
                            case 3: {
                                format |= 0x280;
                                break;
                            }
                            case 4: {
                                format |= 0x2C0;
                                break;
                            }
                            case 5: {
                                format |= 0x300;
                                break;
                            }
                            case 6: {
                                format |= 0x340;
                                break;
                            }
                            case 7: {
                                format |= 0x380;
                            }
                        }
                    }
                }
                return format;
            }

            @Override
            protected String getIconId(int rowIndex) {
                if (SamplesView.this.inputBySamples == null || SamplesView.this.readable == null || SamplesView.this.readable.getDomainBase() == null) {
                    return null;
                }
                rowIndex = SamplesView.this.sortIdx(rowIndex);
                return null;
            }

            @Override
            protected void sort(String[] sortedColumns, Map<String, Integer> sortDirection) {
                super.sort(sortedColumns, sortDirection);
                SamplesView.this.reverse = Arrays.asList(sortedColumns).contains(POSITION_COLUMN) && sortDirection.containsKey(POSITION_COLUMN) && sortDirection.get(POSITION_COLUMN) == -1;
                Actives.runFinally(SamplesView.this.delayedApplyPresentation, 500);
            }

            @Override
            protected String getFilterTooltip(int columnIndex) {
                String columnName = this.columnIndex2Name(columnIndex);
                if (POSITION_COLUMN.equals(columnName)) {
                    return String.valueOf(I18n.Filter_Info) + I18n.SamplesView_DomainFilterInfo;
                }
                if (DOMAIN_END_COLUMN.equals(columnName)) {
                    return String.valueOf(I18n.Filter_Info) + I18n.SamplesView_DomainFilterInfo;
                }
                if (VALUE_COLUMN.equals(columnName) || columnName != null && columnName.startsWith("[")) {
                    return String.valueOf(I18n.Filter_Info) + I18n.SamplesView_ValueFilterInfo;
                }
                if (GROUP_COLUMN.equals(columnName)) {
                    return String.valueOf(I18n.Filter_Info) + I18n.SamplesView_GroupFilterInfo;
                }
                return I18n.Filter_Info;
            }

            @Override
            protected void filter(String[] filteredColumns, Map<String, String> filterDefinitions) {
                super.filter(filteredColumns, filterDefinitions);
                SamplesView.this.filteredColumns = this.filteredColumns;
                SamplesView.this.filterDefinitions = this.filterDefinitions;
                Actives.runFinally(SamplesView.this.delayedApplyPresentation, 500);
            }

            @Override
            protected FilterExpression createFilterExpression(String columnName, String definition) {
                if (POSITION_COLUMN.equals(columnName) || DOMAIN_END_COLUMN.equals(columnName)) {
                    return new FilterExpression(definition, 7){

                        @Override
                        protected Number parseNumber(String text, Number def) {
                            return (this).SamplesView.this.readable.getDomainBase().parseUnits(text);
                        }
                    };
                }
                return new FilterExpression(definition, 7);
            }

            @Override
            protected void selectionChanged() {
                super.selectionChanged();
                int selectedRow = this.getLastSelectedRow();
                SamplesView.this.index = SamplesView.this.sortIdx(selectedRow);
                SamplesView.this.position = SamplesView.this.index2Position(SamplesView.this.index);
                if (SamplesView.this.getSelectionSync()) {
                    Actives.runFinally(SamplesView.this.delayedSelectionChanged, 100);
                }
                if (this.sampleDialogInput != null) {
                    this.sampleDialogInput.selectionChanged(SamplesView.this.index);
                }
            }

            @Override
            public Object command(String id, Object data, int doIt, Object sender) {
                if (!this.isControlValid()) {
                    return null;
                }
                if (id.equals("de.toem.impulse.commands.goto")) {
                    if (doIt != 5) {
                        return this.doGoto(data, doIt, sender);
                    }
                } else if (id.equals("de.toem.impulse.commands.showIn.textEditor")) {
                    if (doIt == 5) {
                        return true;
                    }
                    if (doIt == 0 || doIt == 1) {
                        IReadableValue value = null;
                        if (SamplesView.this.readable != null && SamplesView.this.readable.isSettled()) {
                            ISamplesDisplayInformation descriptor = SamplesView.inputDescriptor(SamplesView.this.readable, SamplesView.this.inputBySamples, SamplesView.this.showGroups, SamplesView.this.index);
                            value = SamplesView.dataValue(SamplesView.this.readable, SamplesView.this.showGroups, SamplesView.this.index, false);
                            if (value instanceof IReadableMembers && ((IReadableMembers)((Object)value)).hasMember("RecPos")) {
                                ((IReadableMembers)((Object)value)).intValueOf("RecPos");
                                RecordContent recordContent = null;
                                if (descriptor != null && descriptor instanceof IPlotItem && ((IPlotItem)((Object)descriptor)).getData() instanceof PlotConfiguration && SamplesView.this.recordViewer != null) {
                                    recordContent = ((PlotConfiguration)((IPlotItem)((Object)descriptor)).getData()).getSignal(SamplesView.this.recordViewer);
                                } else if (descriptor != null && descriptor instanceof Signal) {
                                    recordContent = (Signal)descriptor;
                                }
                                while (recordContent != null) {
                                    if (recordContent instanceof PortScope) {
                                        IElement iElement = ((PortScope)recordContent).source != null ? ((PortScope)recordContent).source.resolveElement() : null;
                                        break;
                                    }
                                    if (recordContent instanceof Record) {
                                        recordContent.getElement();
                                        break;
                                    }
                                    recordContent = (RecordContent)(recordContent.getParent() instanceof RecordContent ? recordContent.getParent() : null);
                                }
                            }
                        }
                        return true;
                    }
                } else {
                    return super.command(id, data, doIt, sender);
                }
                return null;
            }

            private Object doGoto(Object data, int doIt, Object sender) {
                if (doIt == 0 || doIt == 1) {
                    int rows = SamplesView.this.tableController.getRowCount();
                    if ("prev".equals(data) && SamplesView.this.index == 0) {
                        return false;
                    }
                    if ("pos1".equals(data) && SamplesView.this.index == 0) {
                        return false;
                    }
                    if ("next".equals(data) && SamplesView.this.index == rows - 1) {
                        return false;
                    }
                    if ("end".equals(data) && SamplesView.this.index == rows - 1) {
                        return false;
                    }
                    if ("back".equals(data)) {
                        return false;
                    }
                    if (data instanceof Link && !SamplesView.this.getInputSync()) {
                        return false;
                    }
                    if (doIt == 0) {
                        int nextIndex = SamplesView.this.index;
                        if ("prev".equals(data)) {
                            --nextIndex;
                        } else if ("pos1".equals(data)) {
                            nextIndex = 0;
                        } else if ("next".equals(data)) {
                            ++nextIndex;
                        } else if ("end".equals(data)) {
                            nextIndex = rows - 1;
                        } else if (!"back".equals(data) && data instanceof Link && SamplesView.this.getInputSync()) {
                            SamplesView.this.recordViewer.gotoTarget((Link)data);
                        }
                        if (nextIndex != SamplesView.this.index) {
                            SamplesView.this.tableController.selectRow(SamplesView.this.sortIdx(nextIndex), false);
                            SamplesView.this.index = nextIndex;
                            SamplesView.this.position = SamplesView.this.index2Position(SamplesView.this.index);
                            if (SamplesView.this.getSelectionSync()) {
                                Actives.runFinally(SamplesView.this.delayedSelectionChanged, 100);
                            }
                            if (this.sampleDialogInput != null) {
                                this.sampleDialogInput.selectionChanged(SamplesView.this.index);
                            }
                        }
                    }
                    return true;
                }
                return null;
            }

            @Override
            protected Object doEdit(Object data, int doIt, Object sender) {
                if (doIt == 0 || doIt == 1) {
                    if (doIt == 0) {
                        this.openSampleViewDialog();
                    }
                    return true;
                }
                return null;
            }

            @Override
            public void execute(String id, Object data) {
                if (id != null && data instanceof String) {
                    Link link = Link.parse((String)data);
                    this.doGoto(link, 0, this);
                } else {
                    this.doEdit(data, 0, this);
                }
            }

            protected void openSampleViewDialog() {
            }

            final class SampleDialogInput
            extends SampleViewDialog.AbstractSampleDialogInput {
                public SampleDialogInput() {
                    this.init();
                }

                @Override
                public void init() {
                    int index = this.getLastSelectedRow();
                    this.pointer = (this).SamplesView.this.showGroups ? new GroupPointer((this).SamplesView.this.readable, index) : new SamplePointer((this).SamplesView.this.readable, index);
                }

                @Override
                public String getSourceIconId() {
                    return this.getEditor().getIconId();
                }

                @Override
                public String getSourceLabel() {
                    return this.getEditor().getLabel();
                }

                @Override
                public String getLabel() {
                    return SamplesView.this.getLead(this.pointer.getIndex(), true);
                }

                @Override
                public Object getColor() {
                    return SamplesView.this.getColor(this.pointer.getIndex());
                }

                public void selectionChanged(int index) {
                    this.pointer.setIndex(index);
                    if (this.inputListener != null) {
                        this.inputListener.indexChanged();
                    }
                }

                public void dataChanged() {
                    if (this.pointer instanceof AbstractPointer) {
                        ((AbstractPointer)this.pointer).setReference((this).SamplesView.this.readable);
                    } else {
                        this.init();
                    }
                    if (this.inputListener != null) {
                        this.inputListener.pointerChanged();
                    }
                }

                @Override
                protected void syncBack(IPointer pointer) {
                    if ((this).SamplesView.this.readable != null) {
                        (this).SamplesView.this.index = pointer.getIndex();
                        if ((this).SamplesView.this.index > SamplesView.this.tableController.getRowCount()) {
                            (this).SamplesView.this.index = SamplesView.this.tableController.getRowCount() - 1;
                        }
                        SamplesView.this.tableController.selectRow(SamplesView.this.sortIdx((this).SamplesView.this.index), false);
                        (this).SamplesView.this.position = SamplesView.this.index2Position((this).SamplesView.this.index);
                        if (SamplesView.this.getSelectionSync()) {
                            Actives.runFinally((this).SamplesView.this.delayedSelectionChanged, 100);
                        }
                    }
                }

                @Override
                public boolean canGoTarget(Link link) {
                    return true;
                }

                @Override
                public boolean canGoBack() {
                    return SamplesView.this.canGoBack();
                }

                @Override
                public void goTarget(Link link) {
                    if ((this).SamplesView.this.recordViewer != null && !(this).SamplesView.this.recordViewer.isDisposed()) {
                        if (SamplesView.this.getInputSync()) {
                            (this).SamplesView.this.recordViewer.gotoTarget(link);
                        } else {
                            (this).SamplesView.this.recordViewer.requestTarget(link, (this).SamplesView.this.recordViewerChildListener);
                        }
                    }
                }

                @Override
                public void goBack() {
                    SamplesView.this.goBack();
                }

                @Override
                public void highlightAttachment(IAttachment attachment) {
                }
            }
        };
        this.tlk().addTable(container, this.tableController, this.mainLayoutData, 67170, null, new String[]{POSITION_COLUMN, VALUE_COLUMN});
        this.tableController.access(table -> table.setSort(new String[]{POSITION_COLUMN}, null, null));
    }

    public void setFocus() {
        this.tlk().setFocusIfNotContained(this.tableController);
    }

    @Override
    protected boolean setInputObjects(int sourceType, Object source, Object sourceData, List<Object> inputObjects) {
        return super.setInputObjects(sourceType, source, sourceData, inputObjects);
    }

    @Override
    protected void updateControls(ElementModifierEvent event) {
        if (event != null && event.getField() != null && event.getField().getName().toLowerCase().contains("show")) {
            this.presentationChanged = true;
        }
        super.updateControls(event);
    }

    @Override
    public SamplesViewPreferences getPreferences() {
        SamplesViewPreferences preferences;
        if (this.getPersistence() != null && (preferences = (SamplesViewPreferences)ImpulsePreferences.partsPreferences.getCellByLink(this.getPersistence().partPreferences, SamplesViewPreferences.class)) != null) {
            return preferences;
        }
        return this.defaultPreferences;
    }

    @Override
    public IControlProvider getPreferencesControls(Object owner) {
        IControlProvider preferenceControls = SamplesViewPreferenceDialog.getControls(owner, new SamplesViewPreferenceDialog.IMemberColumnFilter(){

            @Override
            public boolean filter(ICell column) {
                return SamplesView.this.memberColumns == null || !SamplesView.this.memberColumns.contains(column.getName());
            }
        }, false);
        return preferenceControls;
    }

    private static final int rowCount(IReadableSamples readable, boolean grouped) {
        int rowCount = readable != null ? (grouped ? readable.getGroups() : readable.getCount()) : 0;
        return rowCount;
    }

    private static final IReadableValue dataValue(IReadableSamples readable, boolean grouped, int index, boolean attchments) {
        return grouped ? readable.valuesAtGroup(index, attchments ? 12 : 0) : readable.compoundAt(index, 12);
    }

    private static final ISamplesDisplayInformation inputDescriptor(IReadableSamples readable, Map<IReadableSamples, ISimpleSamplesProvider> inputBySamples, boolean grouped, int index) {
        if (grouped) {
            index = readable.indexAtGroup(index);
        }
        IReadableSamples unfiltered = readable instanceof AbstractSamplesFilter ? ((AbstractSamplesFilter)readable).getReference() : readable;
        int unfilteredIndex = readable instanceof AbstractSamplesFilter ? ((AbstractSamplesFilter)readable).fil2SrcIdx(index) : index;
        IReadableSamples providerSamples = unfiltered instanceof SamplesMerger ? ((SamplesMerger)unfiltered).getSourceAt(unfilteredIndex) : unfiltered;
        ISimpleSamplesProvider input = inputBySamples.get(providerSamples);
        return input instanceof ISamplesDisplayInformation ? (ISamplesDisplayInformation)((Object)input) : null;
    }

    public final int sortIdx(int idx) {
        return this.reverse ? this.rows - idx - 1 : idx;
    }

    protected String getLead(int rowIndex, boolean prefix) {
        IReadableValue value;
        boolean showN;
        if (this.inputBySamples == null || this.readable == null || this.readable.getDomainBase() == null) {
            return null;
        }
        int dataIndex = this.sortIdx(rowIndex);
        ISamples iSamples = this.readable instanceof AbstractSamplesFilter ? ((AbstractSamplesFilter)this.readable).getReference() : this.readable;
        boolean bl = showN = this.readable instanceof AbstractSamplesFilter || this.readable instanceof SamplesMerger;
        if (showN && (value = SamplesView.dataValue(this.readable, this.showGroups, dataIndex, false)) != null) {
            return String.valueOf(dataIndex) + " / " + (this.showGroups ? "#" : "@") + value.getIndex();
        }
        return String.valueOf(prefix ? (this.showGroups ? "#" : "@") : "") + dataIndex;
    }

    protected Object getColor(int rowIndex) {
        if (this.inputBySamples == null || this.readable == null || this.readable.getDomainBase() == null) {
            return -1;
        }
        int dataIndex = this.sortIdx(rowIndex);
        ISamplesDisplayInformation descriptor = SamplesView.inputDescriptor(this.readable, this.inputBySamples, this.showGroups, dataIndex);
        return descriptor != null ? descriptor.getColor() : null;
    }

    public IReadableValue getReadableValue(IReadableSamples readable, int rowIndex, boolean attchments) {
        if (this.inputBySamples == null || readable == null || readable.getDomainBase() == null) {
            return null;
        }
        int dataIndex = this.sortIdx(rowIndex);
        return SamplesView.dataValue(readable, this.showGroups, dataIndex, attchments);
    }

    public String getDataValue(IReadableSamples readable, Map<IReadableSamples, ISimpleSamplesProvider> inputBySamples, String columnName, int rowIndex) {
        if (inputBySamples == null || readable == null || readable.getDomainBase() == null) {
            return null;
        }
        int dataIndex = this.sortIdx(rowIndex);
        boolean attchments = RELATION_COLUMN.equals(columnName) || LABEL_COLUMN.equals(columnName);
        IReadableValue value = SamplesView.dataValue(readable, this.showGroups, dataIndex, attchments);
        ISamplesDisplayInformation descriptor = VALUE_COLUMN.equals(columnName) || SIGNAL_COLUMN.equals(columnName) ? SamplesView.inputDescriptor(readable, inputBySamples, this.showGroups, dataIndex) : null;
        return this.getDataValue(value, descriptor, columnName);
    }

    public String getDataValue(IReadableValue value, ISamplesDisplayInformation descriptor, String columnName) {
        if (value == null) {
            return null;
        }
        String text = null;
        if (POSITION_COLUMN.equals(columnName)) {
            int n = SamplesViewPreferences.unitMode2Style(this.getPreferences().domainUnitMode);
            text = this.readable.getDomainBase().toString(value.getUnits(), n);
        } else if (DOMAIN_END_COLUMN.equals(columnName)) {
            int n = SamplesViewPreferences.unitMode2Style(this.getPreferences().domainUnitMode);
            text = value.isGroup() ? this.readable.getDomainBase().toString(((IReadableGroup)value).getEndUnits(), n) : "";
        } else if (VALUE_COLUMN.equals(columnName)) {
            int n = this.getPreferences().valueFormat;
            text = value != null && (descriptor != null || n != -1) ? (n != -1 ? value.format(n) : value.format(descriptor.getValueColumnFormat())) : "";
        } else if (SIGNAL_COLUMN.equals(columnName)) {
            text = descriptor != null ? descriptor.getLabel() : "";
        } else if (GROUP_COLUMN.equals(columnName)) {
            text = String.valueOf(value != null ? value.getGroup() : -1);
        } else if (ORDER_COLUMN.equals(columnName)) {
            text = value.isSample() ? ISample.GROUP_ORDER_LABELS[((IReadableSample)value).getOrder() & 7] : "";
        } else if (SAMPLES_COLUMN.equals(columnName)) {
            if (value instanceof IReadableGroup) {
                for (CompoundValue compoundValue : ((IReadableGroup)value).compounds()) {
                    text = String.valueOf(text == null ? "" : String.valueOf(text) + ";") + compoundValue.getIndex();
                }
            } else if (value instanceof IReadableValue) {
                text = String.valueOf(value.getIndex());
            }
        } else if (LABEL_COLUMN.equals(columnName)) {
            text = null;
            List<IAttachment> list = value.attachments(-1);
            if (list != null) {
                for (IAttachment a : list) {
                    if (!(a instanceof AttachedLabel)) continue;
                    text = String.valueOf(text == null ? "" : String.valueOf(text) + ";") + ((AttachedLabel)a).getMessage();
                }
            }
        } else if (RELATION_COLUMN.equals(columnName)) {
            text = null;
            List<IAttachment> list = value.attachments(-1);
            if (list != null) {
                for (IAttachment a : list) {
                    if (!(a instanceof AttachedRelation)) continue;
                    AttachedRelation assoc = (AttachedRelation)a;
                    text = String.valueOf(text == null ? "" : String.valueOf(text) + "<br/>") + "<a href='" + assoc.getLink().toString() + "'>" + assoc.format(2) + "</a>";
                }
            }
        } else if (TAG_COLUMN.equals(columnName)) {
            text = value != null && value.isTagged() ? value.getTagDomain().getLabel(value.getTag()) : null;
        } else if (columnName != null && columnName.startsWith("[")) {
            int n;
            int n2 = -1;
            ICell memberPreferences = this.getPreferences().getChildByName(columnName);
            if (memberPreferences instanceof SamplesViewMemberPreferences) {
                n = ((SamplesViewMemberPreferences)memberPreferences).memberFormat;
            }
            String member = columnName.substring(1, columnName.length() - 1);
            text = value instanceof IReadableMembers ? ((IReadableMembers)((Object)value)).formatOf(member, n) : null;
        }
        return text;
    }

    @Override
    protected void recordViewerPositionChanged(DomainPosition position) {
        super.recordViewerPositionChanged(position);
        this.index = this.positionToIndex(position);
        if (this.index > this.tableController.getRowCount()) {
            this.index = this.tableController.getRowCount() - 1;
        }
        this.tableController.selectRow(this.sortIdx(this.index), false);
    }

    @Override
    public Object command(String id, Object data, int doIt, Object sender) {
        Object ctrlResult;
        if (!this.isValid()) {
            return null;
        }
        Object object = ctrlResult = this.tableController != null ? this.tableController.command(id, data, doIt, sender) : null;
        if (ctrlResult != null) {
            return ctrlResult;
        }
        return super.command(id, data, doIt, sender);
    }

    @Override
    protected void applyPresentation() {
        super.applyPresentation();
        final int nextApplyPresentationCounter = ++this.applyPresentationCounter;
        final List<Object> inputObjects = this.getInputObjects();
        if (inputObjects == null) {
            return;
        }
        final ArrayList<IReadableSamples> nextReaders = new ArrayList<IReadableSamples>();
        final HashMap<IReadableSamples, ISimpleSamplesProvider> nextInputBySamples = new HashMap<IReadableSamples, ISimpleSamplesProvider>();
        final HashMap<ISimpleSamplesProvider, IReadableSamples> nextSamplesByInput = new HashMap<ISimpleSamplesProvider, IReadableSamples>();
        this.getSource();
        this.extractReaders(inputObjects, nextReaders, nextInputBySamples, nextSamplesByInput);
        final Progress progress = new Progress("Processing table filter");
        progress.start();
        Actives.run(new IExecutable(){

            @Override
            public void execute(final IProgress p) {
                AbstractSamplesFilter filtered;
                IReadableSamples unfiltered;
                final ArrayList<String> memberColumns = new ArrayList<String>();
                boolean showGroupColumns = false;
                boolean showSignalColumn = nextReaders.size() > 1;
                boolean showValueColumn = false;
                boolean showGroups = SamplesView.this.getEnableGroups();
                for (IReadableSamples reader : nextReaders) {
                    List<IMemberDescriptor> descriptors;
                    if (reader == null) continue;
                    if (reader.getGroups() > 0) {
                        showGroupColumns = true;
                    } else {
                        showGroups = false;
                    }
                    boolean hasMemberColumn = false;
                    if (reader != null && (reader.getSignalType() == ISamples.SignalType.Struct || reader.getSignalDescriptor().getScale() > 1) && (descriptors = reader.getMemberDescriptors()) != null) {
                        for (IMemberDescriptor member : descriptors) {
                            if (member.isHidden() || member.getParentId() >= 0) continue;
                            hasMemberColumn = true;
                            String columnName = "[" + member.getName() + "]";
                            if (memberColumns.contains(columnName)) continue;
                            memberColumns.add(columnName);
                        }
                    }
                    if (hasMemberColumn) continue;
                    showValueColumn = true;
                }
                int valueShowMode = SamplesView.this.getPreferences().valueShowMode;
                int signalShowMode = SamplesView.this.getPreferences().signalShowMode;
                int groupShowMode = SamplesView.this.getPreferences().groupShowMode;
                int orderShowMode = SamplesView.this.getPreferences().orderShowMode;
                int samplesShowMode = SamplesView.this.getPreferences().samplesShowMode;
                int labelShowMode = SamplesView.this.getPreferences().labelShowMode;
                int assocShowMode = SamplesView.this.getPreferences().relationShowMode;
                int flagShowMode = SamplesView.this.getPreferences().tagShowMode;
                final ArrayList<String> nextColumns = new ArrayList<String>();
                nextColumns.add(POSITION_COLUMN);
                if (showGroups) {
                    nextColumns.add(DOMAIN_END_COLUMN);
                }
                if (showValueColumn && valueShowMode == 2 || valueShowMode == 1) {
                    nextColumns.add(VALUE_COLUMN);
                }
                if (showSignalColumn && signalShowMode == 2 || signalShowMode == 1) {
                    nextColumns.add(SIGNAL_COLUMN);
                }
                if (showGroupColumns && groupShowMode == 2 && !showGroups || groupShowMode == 1) {
                    nextColumns.add(GROUP_COLUMN);
                }
                if (!showGroups && (showGroupColumns && orderShowMode == 2 || orderShowMode == 1)) {
                    nextColumns.add(ORDER_COLUMN);
                }
                if (showGroups && samplesShowMode == 2 || samplesShowMode == 1) {
                    nextColumns.add(SAMPLES_COLUMN);
                }
                if (labelShowMode == 1) {
                    nextColumns.add(LABEL_COLUMN);
                }
                if (assocShowMode == 1) {
                    nextColumns.add(RELATION_COLUMN);
                }
                if (flagShowMode == 1) {
                    nextColumns.add(TAG_COLUMN);
                }
                for (String columnName : memberColumns) {
                    boolean show = true;
                    ICell memberPreferences = SamplesView.this.getPreferences().getChildByName(columnName);
                    if (memberPreferences instanceof SamplesViewMemberPreferences) {
                        boolean bl = show = ((SamplesViewMemberPreferences)memberPreferences).memberShowMode == 1;
                    }
                    if (!show) continue;
                    nextColumns.add(columnName);
                }
                if (nextApplyPresentationCounter != SamplesView.this.applyPresentationCounter || p.isCanceled()) {
                    return;
                }
                IReadableSamples iReadableSamples = unfiltered = nextReaders.size() == 1 ? (IReadableSamples)nextReaders.get(0) : new SamplesMerger(null, null, nextReaders){

                    @Override
                    protected boolean continueExecution() {
                        return nextApplyPresentationCounter == SamplesView.this.applyPresentationCounter || p.isCanceled();
                    }
                };
                if (nextApplyPresentationCounter != SamplesView.this.applyPresentationCounter || p.isCanceled()) {
                    return;
                }
                if (SamplesView.this.filterDefinitions != null) {
                    for (FilterExpression filter : SamplesView.this.filterDefinitions.values()) {
                        filter.reset();
                    }
                }
                ArrayList<String> filteredColumns = new ArrayList<String>();
                if (SamplesView.this.filteredColumns != null) {
                    for (String fc : SamplesView.this.filteredColumns) {
                        if (!nextColumns.contains(fc)) continue;
                        filteredColumns.add(fc);
                    }
                }
                AbstractSamplesFilter abstractSamplesFilter = filtered = !Utils.isEmpty(filteredColumns) ? new AbstractSamplesFilter(null, null, unfiltered, showGroups){

                    @Override
                    protected boolean filter(long current, int index, int group, IReadableValue value) {
                        if (value == null) {
                            return false;
                        }
                        boolean toBeAdded = true;
                        IReadableSamples readable = (IReadableSamples)this.getReference();
                        if (readable instanceof SamplesMerger) {
                            ((SamplesMerger)readable).getSourceAt(index);
                        }
                        ISamplesDisplayInformation descriptor = nextInputBySamples.get(readable) instanceof ISamplesDisplayInformation ? (ISamplesDisplayInformation)nextInputBySamples.get(readable) : null;
                        for (String columnName : SamplesView.this.filteredColumns) {
                            String text;
                            FilterExpression definition;
                            if (!nextColumns.contains(columnName) || (definition = (FilterExpression)SamplesView.this.filterDefinitions.get(columnName)) == null) continue;
                            if ((POSITION_COLUMN.equals(columnName) || DOMAIN_END_COLUMN.equals(columnName)) && definition.isNumerical()) {
                                long units;
                                long l = units = DOMAIN_END_COLUMN.equals(columnName) && value.isGroup() ? ((IReadableGroup)value).getEndUnits() : value.getUnits();
                                if (definition.matches(units)) continue;
                                toBeAdded = false;
                                break;
                            }
                            if (VALUE_COLUMN.equals(columnName) && definition.isNumerical()) {
                                if (definition.matches(((IReadableSample)value).doubleValue())) continue;
                                toBeAdded = false;
                                break;
                            }
                            if (columnName != null && columnName.startsWith("[") && definition.isNumerical()) {
                                String member = columnName.substring(1, columnName.length() - 1);
                                if (definition.matches(((IReadableMembers)((Object)value)).doubleValueOf(member))) continue;
                                toBeAdded = false;
                                break;
                            }
                            if ((GROUP_COLUMN.equals(columnName) || columnName != null && columnName.startsWith("[")) && definition.isNumerical()) {
                                if (definition.matches(value.getGroup())) continue;
                                toBeAdded = false;
                                break;
                            }
                            if (!definition.isTextual() || definition.matches(text = SamplesView.this.getDataValue(value, descriptor, columnName))) continue;
                            toBeAdded = false;
                            break;
                        }
                        return !toBeAdded;
                    }

                    @Override
                    protected boolean continueExecution() {
                        return nextApplyPresentationCounter == SamplesView.this.applyPresentationCounter || p.isCanceled();
                    }
                } : null;
                if (nextApplyPresentationCounter != SamplesView.this.applyPresentationCounter || p.isCanceled()) {
                    return;
                }
                final IReadableSamples nextReadable = filtered != null ? filtered : unfiltered;
                nextReadable.ensureSettled(new Progress(){

                    @Override
                    public void done(double work) {
                        super.done(work);
                        Actives.runInMain(new IExecutable(){

                            @Override
                            public void execute(IProgress p) {
                                if (SamplesView.this.inputController != null) {
                                    SamplesView.this.inputController.updateControl(true);
                                }
                            }
                        });
                    }
                });
                if (nextApplyPresentationCounter != SamplesView.this.applyPresentationCounter || p.isCanceled()) {
                    return;
                }
                final boolean nextShowGroups = showGroups;
                Actives.runInMain(new IExecutable(){

                    @Override
                    public void execute(IProgress p) {
                        SamplesViewPreferences preferences = SamplesView.this.getPreferences();
                        for (String column : nextColumns) {
                            if (!column.startsWith("[") || preferences.getChildByName(column) != null) continue;
                            SamplesViewMemberPreferences cell = new SamplesViewMemberPreferences(column);
                            preferences.addChild(cell);
                        }
                        (this).SamplesView.this.readable = nextReadable;
                        (this).SamplesView.this.showGroups = nextShowGroups;
                        (this).SamplesView.this.inputBySamples = nextInputBySamples;
                        (this).SamplesView.this.samplesByInput = nextSamplesByInput;
                        SamplesView.this.columns = nextColumns;
                        SamplesView.this.memberColumns = memberColumns;
                        SamplesView.this.tableController.update(true);
                        if (SamplesView.this.inputController != null) {
                            ArrayList<String> text = new ArrayList<String>();
                            for (ISimpleSamplesProvider input : nextInputBySamples.values()) {
                                if (!(input instanceof ISamplesDisplayInformation)) continue;
                                text.add(((ISamplesDisplayInformation)((Object)input)).getLabel());
                            }
                            SamplesView.this.inputController.setValue(text.toArray(new String[text.size()]));
                        }
                    }
                });
                progress.done(1.0);
                Actives.run(new IExecutable(){

                    @Override
                    public void execute(IProgress p) {
                        while (nextApplyPresentationCounter == SamplesView.this.applyPresentationCounter) {
                            Actives.sleep(1000);
                            if (!SamplesView.this.getInputRefresh() || nextApplyPresentationCounter != SamplesView.this.applyPresentationCounter) continue;
                            Actives.runInMain(new IExecutable(){

                                @Override
                                public void execute(IProgress p) {
                                    int insertedRows;
                                    int update;
                                    ArrayList<IReadableSamples> updatedReaders = new ArrayList<IReadableSamples>();
                                    SamplesView.this.extractReaders(inputObjects, updatedReaders, nextInputBySamples, nextSamplesByInput);
                                    if (!nextReaders.equals(updatedReaders)) {
                                        SamplesView.this.applyPresentation();
                                        return;
                                    }
                                    if (unfiltered instanceof ISamplesProducer) {
                                        update = ((ISamplesProducer)unfiltered).update();
                                        if (update < 0) {
                                            SamplesView.this.applyPresentation();
                                            return;
                                        }
                                        if (update > 0) {
                                            unfiltered.ensureSettled(new Progress());
                                        }
                                    }
                                    if (filtered instanceof ISamplesProducer) {
                                        update = ((ISamplesProducer)filtered).update();
                                        if (update < 0) {
                                            SamplesView.this.applyPresentation();
                                            return;
                                        }
                                        if (update > 0) {
                                            filtered.ensureSettled(new Progress());
                                        }
                                    }
                                    if ((insertedRows = SamplesView.rowCount(nextReadable, ((this).this).SamplesView.this.showGroups) - SamplesView.this.rows) > 0) {
                                        SamplesView.this.tableController.update(true);
                                    }
                                }
                            });
                        }
                    }
                });
            }
        }, progress, true);
    }

    @Override
    protected void applyInputText() {
    }
}

