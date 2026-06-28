/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.samples.producer.AbstractUpdatableSamplesProducer;
import de.toem.impulse.values.CompoundValue;
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

public class ExpressionFilter
extends AbstractUpdatableSamplesProducer {
    private Object member;
    private FilterExpression filter;
    private boolean keepTags;
    private boolean ignoreNone;
    private boolean keepGroups;
    private boolean keepAttachments;

    public ExpressionFilter() {
    }

    public ExpressionFilter(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
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
                        }), this.cols(), 0x100001, I18n.Samples_Member_);
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
                        return true;
                    }
                };
            }
        }.add("member", "", I18n.Samples_Member, null, null).add("filter", "", I18n.General_Filter, null, null).add("keepTags", true, I18n.Producer_KeepTags, I18n.Producer_KeepTagsComment).add("ignoreNone", true, I18n.Producer_IgnoreNone, I18n.Producer_IgnoreNoneComment).add("keepAttachments", false, I18n.Producer_KeepAttachments, I18n.Producer_KeepAttachmentsComment).add("keepGroups", false, I18n.Producer_KeepGroups, I18n.Producer_KeepGroupsComment);
    }

    @Override
    public void init(String sstart, String send, String srate, IDomainBaseProvider readerBaseProvider) {
        int memberId;
        String pmember = this.parameters.get("member");
        this.member = pmember == null || Utils.isEmpty(pmember) ? null : ((memberId = Utils.parseInt(pmember, -1)) != -1 ? Integer.valueOf(memberId) : pmember);
        this.filter = new FilterExpression(this.parameters.get("filter"), 7);
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
        this.keepTags = this.parameters.getTyped("keepTags", Boolean.class);
        this.ignoreNone = this.parameters.getTyped("ignoreNone", Boolean.class);
        this.keepGroups = this.parameters.getTyped("keepGroups", Boolean.class);
        this.keepAttachments = this.parameters.getTyped("keepAttachment", Boolean.class);
        super.init(sstart, send, srate, readerBaseProvider);
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
        return this.reference != null;
    }

    @Override
    protected boolean execute(IProgress p) {
        ISamplePointer[] input = this.iter.pointers();
        while (this.iter.hasNext() && !p.isCanceled()) {
            boolean hit;
            String fval;
            boolean isNone;
            long current = this.iter.next(this.targetWriter);
            CompoundValue cvalue = input[0].compound();
            if (cvalue == null) continue;
            cvalue.setUnits(current);
            boolean hasMember = this.member != null && cvalue.hasMember(this.member);
            boolean bl = isNone = cvalue.isNone() || this.member != null && !hasMember;
            if (this.ignoreNone && isNone) continue;
            String string = isNone || this.filter == null ? null : (fval = this.member == null ? cvalue.stringValue() : cvalue.stringValueOf(this.member));
            boolean bl2 = this.filter != null ? this.filter.matches(fval != null ? fval : "") : (hit = true);
            if (!hit) continue;
            if (!this.keepTags) {
                cvalue.setTag(false);
            }
            if (!this.keepGroups) {
                cvalue.setOrder(0);
            }
            if (!this.keepAttachments) {
                cvalue.setAttachments(null);
            }
            this.targetWriter.writeSample(cvalue);
        }
        return true;
    }
}

