/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.sample;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IGroupPointer;
import de.toem.impulse.samples.IPointer;
import de.toem.impulse.samples.IReadableGroup;
import de.toem.impulse.samples.IReadableMembers;
import de.toem.impulse.samples.IReadableSample;
import de.toem.impulse.samples.IReadableValue;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.convert.ConvertedMembers;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.filter.FilterExpression;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.ImageController;
import de.toem.toolkits.ui.controller.base.LabelController;
import de.toem.toolkits.ui.controller.base.TabFolderController;
import de.toem.toolkits.ui.controller.base.TableController;
import de.toem.toolkits.ui.controller.base.TextBoxController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.ControlProviderDialog;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.controls.ITlkTable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;

public class SampleDialog
extends ControlProviderDialog {
    public SampleDialog(ITlkPartContainer parent, int style) {
        super(parent, SampleDialog.getControls(null, false), style);
    }

    public SampleDialog() {
    }

    public static IControlProvider getControls(ISampleDialogInput sampleDialogInput, boolean showAssocButtons) {
        return new AbstractControlProvider(sampleDialogInput){
            ISampleDialogInput input;
            private ISampleDialogInput.IInputListener inputListener;
            private IController sourceController;
            private IController indexController;
            private ITlkComposite colorLine;
            private IController domainController;
            private IController startEndController;
            private TableController valueController;
            private IController bytesDetails;
            private IController textDetails;
            private IController imageDetails;
            private List<String> selectionLabels;
            private IAttachment.IAttachedRelation selectedRelation;
            static final int VALUE_ASSOC = 1;
            static final int VALUE_REVASSOC = 2;
            static final int VALUE_LABEL = 3;
            static final int VALUE_SAMPLE = 4;
            static final int VALUE_INFO = 17;
            static final int VALUE_FORMAT = 18;
            static final int VALUE_MEMBER = 19;
            static final int MASK_FILTERABLE = 16;
            {
                this.input = iSampleDialogInput;
                this.selectionLabels = new ArrayList<String>();
            }

            @Override
            protected boolean fillThis() {
                ITlkComposite main = (ITlkComposite)this.container().getData("inspection.main");
                ITlkComposite details = (ITlkComposite)this.container().getData("inspection.details");
                ITlkComposite textParent = null;
                ITlkComposite hexParent = null;
                ITlkComposite imageParent = null;
                if (main == null) {
                    main = this.container();
                }
                if (details == null) {
                    imageParent = details = this.container();
                    hexParent = details;
                    textParent = details;
                } else {
                    hexParent = imageParent = details;
                    textParent = imageParent;
                    details = this.tlk().addTabFolder(details, new TabFolderController(this.editor(), "inspection.folder"), this.cols(), 1024, null);
                    textParent = this.tlk().addComposite(details, null, this.cols(), this.cols(), 0, "[]", null);
                    hexParent = this.tlk().addComposite(details, null, this.cols(), this.cols(), 0, I18n.Samples_Binary, null);
                    imageParent = this.tlk().addComposite(details, null, this.cols(), this.cols(), 0, I18n.Samples_Image, null);
                }
                this.inputListener = new ISampleDialogInput.IInputListener(){

                    @Override
                    public void pointerChanged() {
                        if (this.editor() instanceof ISampleDialogEditor) {
                            ((ISampleDialogEditor)((Object)this.editor())).update(input, 1);
                        } else {
                            this.updateThis(null);
                        }
                    }

                    @Override
                    public void indexChanged() {
                        if (this.editor() instanceof ISampleDialogEditor) {
                            ((ISampleDialogEditor)((Object)this.editor())).update(input, 2);
                        } else {
                            this.updateThis(null);
                        }
                    }
                };
                if (this.input != null) {
                    this.input.onInputChange(this.inputListener);
                }
                ITlkComposite composite = this.tlk().addComposite(main, null, this.cols(), this.cols(), 0, null, null);
                this.sourceController = this.tlk().addImage(composite, new ImageController(this.editor(), null), this.tlk().ld(1, 0x1000000, 20), 0, null);
                this.indexController = this.tlk().addText(composite, new TextController(this.editor(), null), this.tlk().ld(1, 524288, 1), 8192, null);
                this.colorLine = this.tlk().addComposite(main, null, this.cols(), this.tlk().ld(this.cols(), 4, 1, 4, 3), 0x100000, null, null);
                this.domainController = this.tlk().addLabel(main, new LabelController(this.editor(), null), 1, 0, null, null);
                this.startEndController = this.tlk().addText(main, new TextController(this.editor(), null), this.tlk().ld(this.cols() - 1, 524288, 1), 139264, null);
                this.valueController = this.tlk().addTable(main, new TableController(this.editor(), null){
                    List<Object[]> filterAndSorted;

                    @Override
                    protected void setContextMenus() {
                        this.tlk().addMenu(this.control, this, "MENU", "de.toem.toolkits.popupmenu.table", "de.toem.impulse.menu.sample.context", new String[]{"add", "insert", "clone", "edit", "cut", "paste", "delete", "rename", "updown"});
                    }

                    @Override
                    public void doUpdateControl() {
                        this.access(table -> {
                            this.filterAndSorted = this.filterAndSort(this.converted);
                            super.doUpdateControl();
                            ArrayList<Integer> selectedRows = new ArrayList<Integer>();
                            int r = 0;
                            while (r < this.getRowCount()) {
                                String lb = (String)this.getDataValue(r, -1, null);
                                if (selectionLabels.contains(lb)) {
                                    selectedRows.add(r);
                                }
                                ++r;
                            }
                            int[] rows = new int[selectedRows.size()];
                            int n = 0;
                            while (n < selectedRows.size()) {
                                rows[n] = (Integer)selectedRows.get(n);
                                ++n;
                            }
                            table.selectRows(rows, false);
                        });
                    }

                    protected List<Object[]> filterAndSort(Object value) {
                        ArrayList<Object[]> converted = new ArrayList<Object[]>();
                        if (value instanceof List) {
                            int index;
                            converted.addAll((Collection)value);
                            for (final String columnName : this.filteredColumns) {
                                index = this.columnName2Index(columnName);
                                FilterExpression definition = (FilterExpression)this.filterDefinitions.get(columnName);
                                if (index < 0 || definition == null) continue;
                                Iterator iter = converted.iterator();
                                while (iter.hasNext()) {
                                    Object val;
                                    boolean matches;
                                    Object[] row = (Object[])iter.next();
                                    if (((Integer)row[0] & 0x10) == 0 || (matches = definition.matches(val = this.getDataValue(row, index, columnName)))) continue;
                                    iter.remove();
                                }
                            }
                            for (final String columnName : this.sortedColumns) {
                                index = this.columnName2Index(columnName);
                                if (index < 0 || (Integer)this.sortDirection.get(columnName) == 0) continue;
                                final int sIndex = index;
                                converted.sort(new Comparator<Object[]>(){

                                    @Override
                                    public int compare(Object[] row0, Object[] row1) {
                                        if (row0[0] == row1[0]) {
                                            Object v0 = this.getDataValue(row0, sIndex, columnName);
                                            Object v1 = this.getDataValue(row1, sIndex, columnName);
                                            return Utils.compare(v0, v1, (int)((Integer)sortDirection.get(columnName)));
                                        }
                                        return Utils.compare(row0[0], row1[0], (int)((Integer)sortDirection.get(columnName)));
                                    }
                                });
                            }
                        }
                        return converted;
                    }

                    @Override
                    public int getRowCount() {
                        if (this.filterAndSorted != null) {
                            return this.filterAndSorted.size();
                        }
                        return 0;
                    }

                    @Override
                    public String getIconId(int rowIndex) {
                        if (this.filterAndSorted != null && rowIndex >= 0 && rowIndex < this.filterAndSorted.size()) {
                            Object[] value = this.filterAndSorted.get(rowIndex);
                            switch ((Integer)value[0]) {
                                case 18: {
                                    return "de.toem.impulse.images.info.format";
                                }
                                case 17: {
                                    return "de.toem.impulse.images.info.info";
                                }
                                case 19: {
                                    return "de.toem.impulse.images.info.member";
                                }
                                case 3: {
                                    return "de.toem.impulse.images.info.label";
                                }
                                case 1: {
                                    return "de.toem.impulse.images.info.assoc";
                                }
                                case 2: {
                                    return "de.toem.impulse.images.info.revassoc";
                                }
                                case 4: {
                                    return "de.toem.impulse.images.info.sample";
                                }
                            }
                        }
                        return null;
                    }

                    @Override
                    protected String getLead(int rowIndex) {
                        if (this.filterAndSorted != null && rowIndex >= 0 && rowIndex < this.filterAndSorted.size()) {
                            Object[] value = this.filterAndSorted.get(rowIndex);
                            switch ((Integer)value[0]) {
                                case 18: {
                                    return "F";
                                }
                                case 17: {
                                    return "I";
                                }
                                case 19: {
                                    return "M";
                                }
                                case 3: {
                                    return "L";
                                }
                                case 1: 
                                case 2: {
                                    return "A";
                                }
                                case 4: {
                                    return "S";
                                }
                            }
                        }
                        return null;
                    }

                    @Override
                    protected int getExpanded(int rowIndex) {
                        if (rowIndex >= 0 && rowIndex < this.filterAndSorted.size()) {
                            this.filterAndSorted.get(rowIndex);
                        }
                        return 0;
                    }

                    @Override
                    protected String getRowHeaderTooltip(int headerColumnIndex, int rowIndex) {
                        if (this.filterAndSorted != null && rowIndex >= 0 && rowIndex < this.filterAndSorted.size()) {
                            Object[] value = this.filterAndSorted.get(rowIndex);
                            switch ((Integer)value[0]) {
                                case 18: {
                                    return "Format";
                                }
                                case 17: {
                                    return "Info";
                                }
                                case 19: {
                                    return "Member";
                                }
                                case 3: {
                                    return I18n.Attachment_Label;
                                }
                                case 1: 
                                case 2: {
                                    return I18n.Attachment_Relation;
                                }
                                case 4: {
                                    return "Sample";
                                }
                            }
                        }
                        return null;
                    }

                    @Override
                    public Object getDataValue(int rowIndex, int columnIndex, String columnName) {
                        if (this.filterAndSorted != null && rowIndex >= 0 && rowIndex < this.filterAndSorted.size()) {
                            Object[] value = this.filterAndSorted.get(rowIndex);
                            return this.getDataValue(value, columnIndex, columnName);
                        }
                        return null;
                    }

                    private Object getDataValue(Object[] value, int columnIndex, String columnName) {
                        if (value != null) {
                            if (columnIndex == 0) {
                                return value[1];
                            }
                            if (columnIndex == 1) {
                                return value[2];
                            }
                            if (columnIndex == -1) {
                                return String.valueOf(String.valueOf(value[0])) + value[1];
                            }
                            if (columnIndex == -2) {
                                return value.length >= 5 ? value[4] : null;
                            }
                        }
                        return null;
                    }

                    @Override
                    public void execute(String id, Object data) {
                        this.doGoto("target", 0, this);
                    }

                    @Override
                    public Object command(String id, Object data, int doIt, Object sender) {
                        if (id.equals("de.toem.impulse.commands.goto")) {
                            if (doIt != 5) {
                                return this.doGoto(data, doIt, sender);
                            }
                        } else {
                            return super.command(id, data, doIt, sender);
                        }
                        if (doIt == 5) {
                            return true;
                        }
                        return null;
                    }

                    @Override
                    protected void selectionChanged() {
                        if (this.updating) {
                            return;
                        }
                        super.selectionChanged();
                        this.access(table -> {
                            ITlkTable.TableSelection selection = table.getSelection();
                            if (selection != null) {
                                Object relation;
                                selectionLabels.clear();
                                selection.forEachRow((n, selectedRow) -> {
                                    if (selectedRow >= 0 && selectedRow < this.getRowCount()) {
                                        String label = (String)this.getDataValue(selectedRow, -1, null);
                                        selectionLabels.add(label);
                                    }
                                });
                                if (selection.totalRows == 1 && (relation = this.getDataValue(selection.getTopRow(), -2, null)) instanceof IAttachment.IAttachedRelation) {
                                    input.highlightAttachment((IAttachment.IAttachedRelation)relation);
                                }
                            }
                            this.updateValues();
                        });
                    }
                }, this.tlk().ld(this.cols(), 4, 1, 524288, 1), 1115714, null, new String[]{I18n.General_Name, I18n.Samples_Value});
                this.textDetails = this.tlk().addTextBox(textParent, new TextBoxController(this.editor(), null), this.tlk().ld(this.cols(), 4, 1, 524288, 1), 0x100008, null);
                this.bytesDetails = this.tlk().addTextBox(hexParent, new TextBoxController(this.editor(), null){

                    @Override
                    protected Object revert(Object value) {
                        return value;
                    }

                    @Override
                    protected Object convert(Object value) {
                        if (value instanceof byte[]) {
                            byte[] bytes = (byte[])value;
                            char[] hexArray = "0123456789ABCDEF".toCharArray();
                            StringBuilder builder = new StringBuilder();
                            StringBuilder ascii = new StringBuilder();
                            int j = 0;
                            while (j < bytes.length) {
                                if (j % 16 == 0) {
                                    builder.append(hexArray[j >>> 20 & 0xF]);
                                    builder.append(hexArray[j >>> 16 & 0xF]);
                                    builder.append(hexArray[j >>> 12 & 0xF]);
                                    builder.append(hexArray[j >>> 8 & 0xF]);
                                    builder.append(hexArray[j >>> 4 & 0xF]);
                                    builder.append(hexArray[j & 0xF]);
                                    builder.append(' ');
                                }
                                int v = bytes[j] & 0xFF;
                                builder.append(hexArray[v >>> 4]);
                                builder.append(hexArray[v & 0xF]);
                                builder.append(' ');
                                if (Character.isLetterOrDigit((char)v)) {
                                    ascii.append(String.valueOf((char)v));
                                } else {
                                    ascii.append('.');
                                }
                                if (j % 16 == 15) {
                                    builder.append(' ');
                                    builder.append((CharSequence)ascii);
                                    builder.append('\n');
                                    ascii = new StringBuilder();
                                }
                                ++j;
                            }
                            if (ascii.length() > 0) {
                                int n = ascii.length();
                                while (n < 16) {
                                    builder.append(" ");
                                    ++n;
                                }
                                builder.append(' ');
                                builder.append((CharSequence)ascii);
                            }
                            return builder.toString();
                        }
                        return null;
                    }
                }, this.tlk().ld(this.cols(), 4, 1, 524288, 1), 1056792, null);
                this.bytesDetails.setFont("org.eclipse.jface.textfont");
                this.imageDetails = new ImageController(this.editor(), null){};
                this.tlk().addImage(imageParent, this.imageDetails, this.tlk().ld(this.cols(), 4, 1, 524288, 1), 0x100000, null);
                this.updateValues();
                return true;
            }

            boolean getBooleanPreferences(ICell preferences, String name, boolean def) {
                if (preferences == null || !preferences.hasValue(name)) {
                    return def;
                }
                return preferences.getValueAsBoolean(name);
            }

            long getLongPreferences(ICell preferences, String name, long def) {
                if (preferences == null || !preferences.hasValue(name)) {
                    return def;
                }
                return preferences.getValueAsLong(name);
            }

            private String addToModel(IReadableValue value, ISamples.SignalType signalType, int format, List<Object[]> values, boolean force) {
                String text = value.format(format | 0xFFFF0000);
                String string = text != null ? text : (text = force ? "" : null);
                if (text != null) {
                    String label = SampleConverter.getFormatLabel(format);
                    boolean makeTuples = !(signalType != ISamples.SignalType.Logic && signalType != ISamples.SignalType.Integer || format != 1 && format != 2 && format != 3);
                    String columnText = makeTuples ? this.makeTuples(Utils.limit(text, 256)) : Utils.limit(text, 256);
                    text = makeTuples ? this.makeTuples(text) : text;
                    values.add(new Object[]{format >= 16 && format <= 20 ? 17 : 18, label, columnText, text});
                }
                return text;
            }

            private String addToModel(IReadableMembers members, int format, int memberId, String path, List<Object[]> values, boolean force) {
                String text = String.valueOf(members.formatOf(memberId, format));
                String string = text != null ? text : (text = force ? "" : null);
                if (text != null) {
                    String label = "[" + path + "]";
                    String columnText = Utils.limit(text, 256);
                    values.add(new Object[]{19, label, columnText, text});
                }
                return text;
            }

            private void addToModel(IAttachment attachment, List<Object[]> values, boolean showLabels, boolean showAssocs) {
                if (attachment instanceof IAttachment.IAttachedRelation && showAssocs) {
                    IAttachment.IAttachedRelation assoc = (IAttachment.IAttachedRelation)attachment;
                    String target = assoc.format(1);
                    String all = assoc.format(2);
                    values.add(new Object[]{assoc.isReverse() ? 2 : 1, assoc.getMessage(), target, all, assoc});
                } else if (attachment instanceof IAttachment.IAttachedLabel && showLabels) {
                    IAttachment.IAttachedLabel label = (IAttachment.IAttachedLabel)attachment;
                    values.add(new Object[]{3, label.getMessage(), "", label.getMessage(), label});
                }
            }

            private void addToModel(IReadableSample sample, List<Object[]> values) {
                String order = ISample.GROUP_ORDER_LABELS[sample.getOrder() & 7];
                String index = "@" + sample.getIndex();
                String domain = sample.getPosition().toString();
                String value = sample.format(-1);
                values.add(new Object[]{4, order, String.valueOf(index) + "(" + domain + ")", String.valueOf(order) + index + "(" + domain + ")" + "=" + value});
            }

            void updateValues() {
                IPointer pointer;
                ArrayList<Object[]> tableData = new ArrayList<Object[]>();
                if (this.tlk().isDisposed()) {
                    return;
                }
                ICell preferences = this.input != null ? this.input.getPreferences(0, null) : null;
                ICell typePreferences = this.input != null ? this.input.getPreferences(1, null) : null;
                boolean showDefaultFormat = this.getBooleanPreferences(preferences, "showDefaultFormat", true);
                boolean showNonAvailableFormats = this.getBooleanPreferences(preferences, "showNonAvailableFormats", false);
                boolean showLabels = this.getBooleanPreferences(preferences, "showLabels", true);
                boolean showAssocs = this.getBooleanPreferences(preferences, "showAssocs", true);
                long showInfos = this.getLongPreferences(preferences, "showInfos", -1L);
                boolean showGroupSamples = this.getBooleanPreferences(preferences, "showGroupSamples", true);
                long showFormats = this.getLongPreferences(typePreferences, "showFormats", -1L);
                boolean showBytes = this.getBooleanPreferences(typePreferences, "showBytes", true);
                boolean showImage = this.getBooleanPreferences(typePreferences, "showImage", true);
                String label = this.input != null ? this.input.getLabel() : null;
                Object color = this.input != null ? this.input.getColor() : null;
                IPointer iPointer = pointer = this.input != null ? this.input.getPointer() : null;
                if (pointer != null) {
                    ISamples.SignalType signalType;
                    int f;
                    int n;
                    Object[] objectArray;
                    List<IAttachment> attachments;
                    Iterator tagInfo;
                    if (this.sourceController != null) {
                        this.sourceController.setValue(this.input.getSourceIconId());
                        this.sourceController.setTooltip(this.input.getSourceLabel());
                    }
                    if (pointer != null) {
                        pointer.getIndex();
                    }
                    ConvertedMembers value = null;
                    if (pointer instanceof ISamplePointer) {
                        value = ((ISamplePointer)pointer).compound();
                    } else if (pointer instanceof IGroupPointer) {
                        value = ((IGroupPointer)pointer).val();
                    }
                    int idx = value != null ? value.getIndex() : -1;
                    int group = value != null ? value.getGroup() : -1;
                    int tag = value != null ? value.getTag() : 0;
                    Iterator iterator = tagInfo = value != null && tag > 0 && value.getTagDomain() != null ? pointer.getTagDomain().getLabel(tag) : null;
                    String taggedLabel = String.valueOf(label != null ? label : (pointer instanceof IGroupPointer ? "Group #" + group : "Index @" + idx)) + (tagInfo != null ? " " + tagInfo : "");
                    this.indexController.setValue(taggedLabel);
                    this.indexController.setTooltip(taggedLabel);
                    this.indexController.setBackground(tag > 0 ? "de.toem.impulse.color.sample.tag" + Math.min(tag, 8) : null);
                    this.colorLine.setBackground(color != null ? color : "--de-toem-toolkits-tlk-css-color-grey");
                    IDomainBase base = pointer.getDomainBase();
                    String ltxt = base != null ? base.getDomainLabel() : "";
                    String txt = "";
                    if (value instanceof IReadableGroup) {
                        IReadableGroup grouped = (IReadableGroup)((Object)value);
                        txt = String.valueOf(grouped.getStart() != null ? grouped.getStart().toString() : "") + "/" + (grouped.getEnd() != null ? grouped.getEnd().toString() : "");
                        ltxt = String.valueOf(ltxt) + "/" + I18n.Samples_DomainEnd;
                    } else if (pointer != null) {
                        txt = base != null ? base.toString(pointer.getUnits(true)) : "";
                    }
                    this.domainController.setValue(String.valueOf(ltxt) + ":");
                    this.startEndController.setValue(txt);
                    List<IAttachment> list = attachments = value != null ? pointer.attachments(30) : null;
                    if (attachments != null && !attachments.isEmpty()) {
                        for (IAttachment iAttachment : attachments) {
                            this.addToModel(iAttachment, tableData, showLabels, showAssocs);
                        }
                    }
                    if (showGroupSamples && value instanceof IReadableGroup) {
                        for (IReadableSample iReadableSample : ((IReadableGroup)((Object)value)).compounds()) {
                            this.addToModel(iReadableSample, tableData);
                        }
                    }
                    if (showDefaultFormat && value != null) {
                        this.addToModel((IReadableValue)((Object)value), pointer.getSignalType(), -1, tableData, false);
                    }
                    if (value != null) {
                        objectArray = SampleConverter.formatValueOptions();
                        n = objectArray.length;
                        int n2 = 0;
                        while (n2 < n) {
                            Object object = objectArray[n2];
                            f = (Integer)object;
                            if (f >= 16 && f <= 20 && (showInfos & (long)(1 << f)) != 0L) {
                                this.addToModel((IReadableValue)((Object)value), pointer.getSignalType(), f, tableData, showNonAvailableFormats);
                            }
                            ++n2;
                        }
                    }
                    if (value != null) {
                        objectArray = SampleConverter.formatValueOptions();
                        n = objectArray.length;
                        int n3 = 0;
                        while (n3 < n) {
                            Object object = objectArray[n3];
                            f = (Integer)object;
                            if (f >= 1 && (f < 16 || f > 20) && f <= 39 && (showFormats & 1L << f) != 0L) {
                                this.addToModel((IReadableValue)((Object)value), pointer.getSignalType(), f, tableData, showNonAvailableFormats);
                            }
                            ++n3;
                        }
                    }
                    if ((signalType = pointer.getSignalType()).isArrayOrStruct() && pointer instanceof IReadableMembers) {
                        IReadableMembers members = (IReadableMembers)((Object)pointer);
                        this.updateMemberValues(members, tableData);
                    }
                    if (showBytes && pointer instanceof ISamplePointer && pointer.getSignalType() == ISamples.SignalType.Binary) {
                        if (!Arrays.equals((byte[])this.bytesDetails.getValue(), ((ISamplePointer)pointer).bytesValue())) {
                            this.bytesDetails.setValue(((ISamplePointer)pointer).bytesValue(), true, false, false, true);
                        }
                    } else {
                        this.bytesDetails.setValue(null, true, false, false, true);
                    }
                    if (showImage && pointer instanceof ISamplePointer && pointer.getSignalType() == ISamples.SignalType.Binary && pointer.getSignalDescriptor().isImage()) {
                        if (!Arrays.equals((byte[])this.imageDetails.getValue(), ((ISamplePointer)pointer).bytesValue())) {
                            this.imageDetails.setValue(pointer.val(), true, false, false, true);
                        }
                    } else {
                        this.imageDetails.setValue(null, true, false, false, true);
                    }
                }
                if (this.valueController != null) {
                    this.valueController.setValue(tableData, true, false, false, true);
                }
                if (this.textDetails != null) {
                    ArrayList<Object[]> selectedData = new ArrayList<Object[]>();
                    ArrayList<Integer> selectedRows = new ArrayList<Integer>();
                    int row = 0;
                    for (Object[] v : tableData) {
                        if (this.selectionLabels.contains(String.valueOf(String.valueOf(v[0])) + v[1])) {
                            selectedData.add(v);
                            selectedRows.add(row);
                        }
                        ++row;
                    }
                    int[] rows = new int[selectedRows.size()];
                    int n = 0;
                    while (n < selectedRows.size()) {
                        rows[n] = (Integer)selectedRows.get(n);
                        ++n;
                    }
                    if (!selectedData.isEmpty()) {
                        String text = "";
                        for (Object[] s : selectedData) {
                            text = String.valueOf(text) + String.valueOf(s[3]) + Utils.lineSeparator;
                        }
                        this.textDetails.setValue(text, true, false, false, true);
                    } else {
                        this.textDetails.setValue("", true, false, false, true);
                    }
                }
                this.selectedRelation = null;
                for (Object[] v : tableData) {
                    if (!this.selectionLabels.contains(String.valueOf(String.valueOf(v[0])) + v[1]) || v.length < 5 || !(v[4] instanceof IAttachment.IAttachedRelation)) continue;
                    IAttachment.IAttachedRelation relation = (IAttachment.IAttachedRelation)v[4];
                    if (this.selectedRelation == null) {
                        this.selectedRelation = relation;
                        continue;
                    }
                    this.selectedRelation = null;
                    break;
                }
            }

            void updateMemberValues(IReadableMembers members, List<Object[]> values) {
                int n = 0;
                while (n < members.noOfMembers()) {
                    int memberId = members.idOf(n);
                    String memberPath = members.pathOf(n);
                    ICell memberPreferences = this.input != null ? this.input.getPreferences(2, memberPath) : null;
                    boolean memberShowDefaultFormat = this.getBooleanPreferences(memberPreferences, "showDefaultFormat", true);
                    boolean memberShowNonAvailableFormats = this.getBooleanPreferences(memberPreferences, "showNonAvailableFormats", false);
                    long memberShowFormats = this.getLongPreferences(memberPreferences, "showFormats", 0L);
                    if (memberShowDefaultFormat) {
                        this.addToModel(members, -1, memberId, memberPath, values, memberShowNonAvailableFormats);
                    }
                    Object[] objectArray = SampleConverter.formatValueOptions();
                    int n2 = objectArray.length;
                    int n3 = 0;
                    while (n3 < n2) {
                        Object o = objectArray[n3];
                        int f = (Integer)o;
                        if (f >= 1 && (f < 16 || f > 20) && f <= 39 && (memberShowFormats & 1L << f) != 0L) {
                            this.addToModel(members, f, memberId, memberPath, values, memberShowNonAvailableFormats);
                        }
                        ++n3;
                    }
                    Object value = members.valueOf(memberId);
                    if (value instanceof IReadableMembers) {
                        this.updateMemberValues((IReadableMembers)value, values);
                    }
                    ++n;
                }
            }

            @Override
            protected void updateThis(Object user) {
                if (user instanceof ISampleDialogInput && user != this.input) {
                    if (this.input != null) {
                        this.input.onInputChange(null);
                    }
                    this.input = (ISampleDialogInput)user;
                    ((ISampleDialogInput)user).onInputChange(this.inputListener);
                }
                this.updateValues();
            }

            private String makeTuples(String value) {
                String line = "";
                while (!value.isEmpty()) {
                    String tuple;
                    if (value.length() > 5) {
                        tuple = value.substring(value.length() - 4);
                        value = value.substring(0, value.length() - 4);
                    } else {
                        tuple = value;
                        value = "";
                    }
                    if (line.length() > 1) {
                        line = " " + line;
                    }
                    line = String.valueOf(tuple) + line;
                }
                return line;
            }

            @Override
            public Object command(String id, Object data, int doIt, Object sender) {
                if (id.equals("de.toem.impulse.commands.goto")) {
                    if (doIt != 5) {
                        return this.doGoto(data, doIt, sender);
                    }
                } else {
                    return super.command(id, data, doIt, sender);
                }
                if (doIt == 5) {
                    return true;
                }
                return null;
            }

            private Object doGoto(Object data, int doIt, Object sender) {
                if (this.input == null) {
                    return null;
                }
                if (doIt == 0 || doIt == 1) {
                    IPointer pointer = this.input.getPointer();
                    if (pointer == null) {
                        return false;
                    }
                    if ("prev".equals(data) && !this.input.canGoPrev()) {
                        return false;
                    }
                    if ("pos1".equals(data) && !this.input.canGoPos1()) {
                        return false;
                    }
                    if ("next".equals(data) && !this.input.canGoNext()) {
                        return false;
                    }
                    if ("end".equals(data) && !this.input.canGoEnd()) {
                        return false;
                    }
                    if ("back".equals(data) && !this.input.canGoBack()) {
                        return false;
                    }
                    if ("target".equals(data) && !this.input.canGoTarget(this.selectedRelation != null ? this.selectedRelation.getLink() : null)) {
                        return false;
                    }
                    if (doIt == 0) {
                        if ("prev".equals(data)) {
                            this.input.goPrev();
                        } else if ("pos1".equals(data)) {
                            this.input.goPos1();
                        } else if ("next".equals(data)) {
                            this.input.goNext();
                        } else if ("end".equals(data)) {
                            this.input.goEnd();
                        } else if ("back".equals(data)) {
                            this.input.goBack();
                        } else if ("target".equals(data) && this.selectedRelation != null) {
                            Link link = this.selectedRelation.getLink();
                            this.input.goTarget(link);
                        }
                        this.updateValues();
                    }
                    return true;
                }
                return null;
            }
        };
    }

    public static abstract class AbstractSampleDialogInput
    implements ISampleDialogInput {
        protected IPointer pointer;
        protected ISampleDialogInput.IInputListener inputListener;

        public AbstractSampleDialogInput() {
            this.init();
        }

        public void init() {
        }

        @Override
        public String getSourceIconId() {
            return null;
        }

        @Override
        public String getSourceLabel() {
            return null;
        }

        @Override
        public void onInputChange(ISampleDialogInput.IInputListener handler) {
            this.inputListener = handler;
        }

        @Override
        public IPointer getPointer() {
            return this.pointer;
        }

        protected abstract void syncBack(IPointer var1);

        @Override
        public boolean canGoPrev() {
            return this.pointer != null && this.pointer.hasPrev();
        }

        @Override
        public boolean canGoNext() {
            return this.pointer != null && this.pointer.hasNext();
        }

        @Override
        public boolean canGoPos1() {
            return this.pointer != null && this.pointer.getIndex() > this.pointer.getMinIndex();
        }

        @Override
        public boolean canGoEnd() {
            return this.pointer != null && this.pointer.getIndex() < this.pointer.getMaxIndex();
        }

        @Override
        public boolean canGoTarget(Link link) {
            return false;
        }

        @Override
        public boolean canGoBack() {
            return false;
        }

        @Override
        public void goPrev() {
            if (this.pointer != null) {
                this.pointer.goPrev();
                this.syncBack(this.pointer);
            }
        }

        @Override
        public void goNext() {
            if (this.pointer != null) {
                this.pointer.goNext();
                this.syncBack(this.pointer);
            }
        }

        @Override
        public void goPos1() {
            if (this.pointer != null) {
                this.pointer.goPos1();
                this.syncBack(this.pointer);
            }
        }

        @Override
        public void goEnd() {
            if (this.pointer != null) {
                this.pointer.goEnd();
                this.syncBack(this.pointer);
            }
        }
    }

    public static interface ISampleDialogEditor {
        public static final int CHANGED_INPUT = 0;
        public static final int CHANGED_POINTER = 1;
        public static final int CHANGED_INDEX = 2;

        public void update(ISampleDialogInput var1, int var2);
    }

    public static interface ISampleDialogInput {
        public static final int PREFERENCES_STANDARD = 0;
        public static final int PREFERENCES_TYPE = 1;
        public static final int PREFERENCES_MEMBER = 2;

        public void onInputChange(IInputListener var1);

        public String getSourceIconId();

        public String getSourceLabel();

        public String getLabel();

        public IPointer getPointer();

        public ICell getPreferences(int var1, Object var2);

        public boolean filterPreferences(ICell var1);

        public Object getColor();

        public boolean canGoPrev();

        public boolean canGoNext();

        public boolean canGoPos1();

        public boolean canGoEnd();

        public boolean canGoTarget(Link var1);

        public boolean canGoBack();

        public void goPrev();

        public void goNext();

        public void goPos1();

        public void goEnd();

        public void goTarget(Link var1);

        public void goBack();

        public void highlightAttachment(IAttachment var1);

        public static interface IInputListener {
            public void pointerChanged();

            public void indexChanged();
        }
    }
}

