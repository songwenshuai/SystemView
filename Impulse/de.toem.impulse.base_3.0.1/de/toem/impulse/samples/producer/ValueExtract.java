/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IBinarySamplesWriter;
import de.toem.impulse.samples.IEventSamplesWriter;
import de.toem.impulse.samples.IFloatSamplesWriter;
import de.toem.impulse.samples.IIntegerSamplesWriter;
import de.toem.impulse.samples.ILogicSamplesWriter;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.ITextSamplesWriter;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.samples.producer.AbstractUpdatableSamplesProducer;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Logic;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.filter.FilterExpression;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.controller.source.PropertySource;
import de.toem.toolkits.ui.proposal.ContentProposal;
import de.toem.toolkits.ui.proposal.ContentProposalExtension;
import de.toem.toolkits.ui.proposal.PatternContentProposal;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class ValueExtract
extends AbstractUpdatableSamplesProducer {
    private Object member;
    private FilterExpression filter;
    private Object emember;
    private Object extract;
    private boolean keepTags;
    private boolean ignoreNone;
    private boolean hideIdentical;
    private boolean previousTag;
    private String previousValue;

    public ValueExtract() {
    }

    public ValueExtract(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public static IPropertyModel getPropertyModel() {
        return new PropertyModel(){

            @Override
            public IControlProvider getControls() {
                return new AbstractControlProvider(){

                    @Override
                    protected boolean fillThis() {
                        this.tlk().addText(this.container(), new TextController(this.editor(), new PropertySource("member")).add(new ContentProposalExtension(true){

                            @Override
                            public ContentProposal[] getProposals(String contents, int position) {
                                this.clear();
                                Object sourceMembers = this.editor().getData("members");
                                if (sourceMembers instanceof List && !((List)sourceMembers).isEmpty()) {
                                    for (Object m : (List)sourceMembers) {
                                        if (!(m instanceof IMemberDescriptor)) continue;
                                        this.add(((IMemberDescriptor)m).getPath(), null, null);
                                    }
                                } else {
                                    this.add("Message", null, null);
                                    this.add("Level", null, null);
                                    this.add("0", null, null);
                                    this.add("1", null, null);
                                    this.add("2", null, null);
                                }
                                return super.getProposals(contents, position);
                            }
                        }), this.cols(), 0x100001, I18n.Samples_FilterMember_);
                        this.tlk().addText(this.container(), new TextController(this.editor(), new PropertySource("filter")).add(new PatternContentProposal(){

                            @Override
                            protected void init() {
                                this.add("abc", "Show sample if text contains 'abc'", null);
                                this.add("0.4 < 2.0", "Show sample if value matches range:", null);
                                this.add("0.4 < 2.0", null, null);
                                this.add("< 0x400", null, null);
                                this.add("<= -4.2", null, null);
                                this.add("> 34", null, null);
                                this.add(">= 0.04", null, null);
                                this.add("", "Filter samples using a regular expression:", null);
                                super.init();
                            }
                        }), this.cols(), 0x100001, I18n.General_Filter_);
                        this.tlk().addText(this.container(), new TextController(this.editor(), new PropertySource("emember")).add(new ContentProposalExtension(true){

                            @Override
                            public ContentProposal[] getProposals(String contents, int position) {
                                this.clear();
                                Object sourceMembers = this.editor().getData("members");
                                if (sourceMembers instanceof List && !((List)sourceMembers).isEmpty()) {
                                    for (Object m : (List)sourceMembers) {
                                        if (!(m instanceof IMemberDescriptor)) continue;
                                        this.add(((IMemberDescriptor)m).getPath(), null, null);
                                    }
                                } else {
                                    this.add("Message", null, null);
                                    this.add("Level", null, null);
                                    this.add("0", null, null);
                                    this.add("1", null, null);
                                    this.add("2", null, null);
                                }
                                return super.getProposals(contents, position);
                            }
                        }), this.cols(), 0x100001, I18n.Samples_Member_);
                        this.tlk().addText(this.container(), new TextController(this.editor(), new PropertySource("extract")).add(new PatternContentProposal(){

                            @Override
                            protected void init() {
                                this.add("xxx:", "Extract value after 'xxx:'", null);
                                this.add("xxx:([^\\s]+)", "Extract value using a regular expression", null);
                                super.init();
                            }
                        }), this.cols(), 0x100001, I18n.Producer_Extract_);
                        return true;
                    }
                };
            }
        }.add("member", "", I18n.Samples_FilterMember, null, null).add("filter", "", I18n.General_Filter, null, null).add("emember", "", I18n.Samples_ExtractMember, null, null).add("extract", "", I18n.Producer_Extract, null, null).add("keepTags", true, I18n.Producer_KeepTags, I18n.Producer_KeepTagsComment).add("ignoreNone", true, I18n.Producer_IgnoreNone, I18n.Producer_IgnoreNoneComment).add("hideIdentical", true, I18n.Producer_HideIdentical, I18n.Producer_HideIdenticalComment);
    }

    @Override
    public void init(String sstart, String send, String srate, IDomainBaseProvider readerBaseProvider) {
        int memberId;
        String pmember = this.parameters.get("member");
        this.member = pmember == null || Utils.isEmpty(pmember) ? null : ((memberId = Utils.parseInt(pmember, -1)) != -1 ? Integer.valueOf(memberId) : pmember);
        this.filter = Utils.isEmpty(this.parameters.get("filter")) ? null : new FilterExpression(this.parameters.get("filter"), 7);
        pmember = this.parameters.get("emember");
        this.emember = pmember == null || Utils.isEmpty(pmember) ? null : ((memberId = Utils.parseInt(pmember, -1)) != -1 ? Integer.valueOf(memberId) : pmember);
        this.extract = this.parameters.get("extract");
        if (!Utils.isEmpty((String)this.extract)) {
            if (this.extract != null && FilterExpression.isRegularExpression((String)this.extract)) {
                try {
                    this.extract = Pattern.compile((String)this.extract);
                }
                catch (Throwable throwable) {}
            }
        } else {
            this.extract = null;
        }
        this.keepTags = this.parameters.getTyped("keepTags", Boolean.class);
        this.ignoreNone = this.parameters.getTyped("ignoreNone", Boolean.class);
        this.hideIdentical = this.parameters.getTyped("hideIdentical", Boolean.class);
        int noOfInputs = 0;
        int n = 0;
        while (n < this.sources.size()) {
            IReadableSamples source = (IReadableSamples)this.sources.get(n);
            if (source != null) {
                ++noOfInputs;
            }
            ++n;
        }
        if (noOfInputs == 0) {
            this.setError(I18n.General_NoInput);
        }
        super.init(sstart, send, srate, readerBaseProvider);
    }

    @Override
    public void modifyInit() {
        super.modifyInit();
        if (ISamples.SignalType.Struct.equals((Object)this.signalType)) {
            if ((this.flags & 2) != 0) {
                this.setError(I18n.Producer_StructNotSupported);
            } else {
                this.signalType = ISamples.SignalType.Text;
            }
        }
        boolean isArray = false;
        switch (this.signalType) {
            case EventArray: {
                this.signalType = ISamples.SignalType.Event;
                isArray = true;
                break;
            }
            case IntegerArray: {
                this.signalType = ISamples.SignalType.Integer;
                isArray = true;
                break;
            }
            case FloatArray: {
                this.signalType = ISamples.SignalType.Float;
                isArray = true;
                break;
            }
            case TextArray: {
                this.signalType = ISamples.SignalType.Text;
                isArray = true;
            }
        }
        if (isArray && (this.flags & 2) != 0) {
            this.setError(I18n.Producer_ArraysNotSupported);
        }
    }

    @Override
    protected boolean instatiate(IProgress p) {
        this.targetWriter = PackedSamples.createWriter(this.processType, this.signalType, this.signalDescriptor, this.productionBase);
        if (this.targetWriter == null) {
            return false;
        }
        this.targetWriter.open(this.start, (this.flags & 0x20) != 0 ? this.end : this.start, this.rate, 0, 0, null);
        ISamplePointer[] input = new ISamplePointer[1];
        if (this.sources == null) {
            return false;
        }
        int n = 0;
        while (n < this.sources.size()) {
            IReadableSamples source = (IReadableSamples)this.sources.get(n);
            if (source != null) {
                input[0] = new SamplePointer(source);
            }
            ++n;
        }
        if (input[0] == null) {
            return false;
        }
        this.iter = new SamplesIterator(this.targetWriter, input);
        this.setReference(PackedSamples.createReader(this.targetWriter, this.readerBase));
        IMemberDescriptor m = input[0].getMemberDescriptor(this.member);
        if (m != null) {
            this.member = m.getId();
        }
        if ((m = input[0].getMemberDescriptor(this.emember)) != null) {
            this.emember = m.getId();
        }
        return this.reference != null;
    }

    @Override
    protected boolean execute(IProgress p) {
        ISamplePointer[] input = this.iter.pointers();
        while (this.iter.hasNext() && !p.isCanceled()) {
            boolean tag;
            boolean hit;
            String fval;
            boolean isNone;
            Long current = this.iter.next(this.targetWriter);
            CompoundValue cvalue = input[0].compound();
            if (cvalue == null) continue;
            boolean hasMember = this.member != null && cvalue.hasMember(this.member);
            boolean bl = isNone = cvalue.isNone() || this.member != null && !hasMember;
            if (this.ignoreNone && isNone) continue;
            String string = isNone || this.filter == null ? null : (fval = this.member == null ? cvalue.stringValue() : cvalue.stringValueOf(this.member));
            boolean bl2 = this.filter != null ? this.filter.matches(fval != null ? fval : "") : (hit = true);
            if (!hit) continue;
            hasMember = this.emember != null && cvalue.hasMember(this.emember);
            boolean bl3 = isNone = cvalue.isNone() || this.emember != null && !hasMember;
            if (this.ignoreNone && isNone) continue;
            String value = null;
            if (!isNone) {
                String val;
                String string2 = val = this.emember == null ? cvalue.stringValue() : cvalue.stringValueOf(this.emember);
                if (val == null) continue;
                if (this.extract instanceof String) {
                    int pos = val.indexOf((String)this.extract);
                    if (pos >= 0) {
                        pos += ((String)this.extract).length();
                        while (pos < val.length() && Character.isWhitespace(val.charAt(pos))) {
                            ++pos;
                        }
                        int start = pos;
                        while (pos < val.length() && !Character.isWhitespace(val.charAt(pos))) {
                            ++pos;
                        }
                        int end = pos;
                        if (end > start) {
                            value = val.substring(start, end);
                        }
                    }
                } else if (this.extract instanceof Pattern) {
                    Matcher m = ((Pattern)this.extract).matcher(val);
                    if (m.find() && m.groupCount() == 1) {
                        value = m.group(1);
                    }
                } else {
                    value = val;
                }
            }
            if (!isNone && value == null) continue;
            boolean bl4 = tag = this.keepTags && cvalue != null && cvalue.isTagged();
            if (this.hideIdentical && tag == this.previousTag && Utils.equals(value, this.previousValue)) continue;
            if (isNone) {
                this.targetWriter.writeNone((long)current, tag);
            } else {
                this.write(this.targetWriter, current, tag, value);
            }
            this.previousTag = tag;
            this.previousValue = value;
        }
        return true;
    }

    private boolean write(ISamplesWriter writer, long current, boolean tag, String value) {
        if (writer instanceof IFloatSamplesWriter) {
            ((IFloatSamplesWriter)writer).write(current, tag, Utils.parseDouble(value, 0.0));
        } else if (writer instanceof IIntegerSamplesWriter) {
            ((IIntegerSamplesWriter)writer).write(current, tag, Utils.parseLong(value, 0L));
        } else if (writer instanceof ITextSamplesWriter) {
            ((ITextSamplesWriter)writer).write(current, tag, value);
        } else if (writer instanceof IBinarySamplesWriter) {
            ((IBinarySamplesWriter)writer).write(current, tag, value.getBytes());
        } else if (writer instanceof IEventSamplesWriter) {
            ((IEventSamplesWriter)writer).write(current, tag, value);
        } else if (writer instanceof ILogicSamplesWriter) {
            ((ILogicSamplesWriter)writer).write(current, tag, Logic.valueOf(value));
        } else {
            return false;
        }
        return true;
    }
}

