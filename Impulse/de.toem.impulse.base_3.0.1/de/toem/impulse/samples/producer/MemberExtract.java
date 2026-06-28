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
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.samples.producer.AbstractUpdatableMasterSamplesProducer;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Struct;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.controller.source.PropertySource;
import de.toem.toolkits.ui.proposal.ContentProposal;
import de.toem.toolkits.ui.proposal.ContentProposalExtension;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MemberExtract
extends AbstractUpdatableMasterSamplesProducer {
    private Object member;
    private boolean keepTags;
    private boolean ignoreNone;
    private boolean hideIdentical;
    private IReadableSamples source;
    private Map<Integer, MemberExtractSlaveProduction> memberMap;
    private boolean previousTag;
    private Object previousValue;

    public MemberExtract() {
    }

    public MemberExtract(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, 0, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public MemberExtract(IReadableSamples source) {
        super(source.getId(), source.getName(), 2, source, ISamples.ProcessType.Unknown, ISamples.SignalType.Unknown, ISamples.SignalDescriptor.DEFAULT, source.getDomainBase(), null, null, null, null, null, null, source.getDomainBase());
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
                        return true;
                    }
                };
            }
        }.add("member", "", I18n.Samples_Member, null, null).add("keepTags", true, I18n.Producer_KeepTags, I18n.Producer_KeepTagsComment).add("ignoreNone", true, I18n.Producer_IgnoreNone, I18n.Producer_IgnoreNoneComment).add("hideIdentical", true, I18n.Producer_HideIdentical, I18n.Producer_HideIdenticalComment);
    }

    @Override
    protected IReadableSamples loopThroughSource() {
        return this.source;
    }

    @Override
    public void init(String sstart, String send, String srate, IDomainBaseProvider readerBaseProvider) {
        int noOfSources = 0;
        int n = 0;
        while (n < this.sources.size()) {
            IReadableSamples source = (IReadableSamples)this.sources.get(n);
            if (source != null) {
                ++noOfSources;
                this.source = source;
            }
            ++n;
        }
        if (noOfSources == 0) {
            this.setError(I18n.General_NoInput);
        }
        if (noOfSources != 1) {
            this.setError(I18n.General_InvalidNoOfInput);
        }
        if (this.parameters != null) {
            String pmember = this.parameters.get("member");
            if (pmember == null || Utils.isEmpty(pmember)) {
                if (this.mode == 0) {
                    this.setError(I18n.Producer_MemberUndefined);
                }
            } else {
                int memberId = Utils.parseInt(pmember, -1);
                this.member = memberId != -1 ? Integer.valueOf(memberId) : pmember;
            }
            this.keepTags = this.parameters.getTyped("keepTags", Boolean.class);
            this.ignoreNone = this.parameters.getTyped("ignoreNone", Boolean.class);
            this.hideIdentical = this.parameters.getTyped("hideIdentical", Boolean.class);
        } else {
            this.keepTags = false;
            this.ignoreNone = false;
            this.hideIdentical = false;
        }
        if (this.mode == 2) {
            this.memberMap = new HashMap<Integer, MemberExtractSlaveProduction>();
            if (this.source != null) {
                ISamples.SignalType sourceSignalType = this.source.getSignalType();
                List<IMemberDescriptor> descriptors = this.source.getMemberDescriptors();
                int scale = this.source.getScale();
                if (!descriptors.isEmpty()) {
                    for (IMemberDescriptor d : descriptors) {
                        if (d.isHidden()) continue;
                        new MemberExtractSlaveProduction(d);
                    }
                } else if (sourceSignalType == ISamples.SignalType.FloatArray || sourceSignalType == ISamples.SignalType.IntegerArray || sourceSignalType == ISamples.SignalType.TextArray || sourceSignalType == ISamples.SignalType.EventArray) {
                    int n2 = 0;
                    while (n2 < scale) {
                        new MemberExtractSlaveProduction(n2);
                        ++n2;
                    }
                }
            }
        }
        super.init(sstart, send, srate, readerBaseProvider);
    }

    @Override
    public void modifyInit() {
        super.modifyInit();
        if (this.mode == 0) {
            IMemberDescriptor descriptor;
            if ((this.flags & 2) == 0) {
                descriptor = this.source.getMemberDescriptor(this.member);
                this.signalType = this.detSignalType(this.signalType, descriptor);
            }
            if ((this.flags & 4) == 0 && (descriptor = this.source.getMemberDescriptor(this.member)) != null) {
                this.signalDescriptor = ISamples.SignalDescriptor.fromMember(descriptor);
            }
        }
    }

    private ISamples.SignalType detSignalType(ISamples.SignalType sourceSignalType, IMemberDescriptor descriptor) {
        ISamples.SignalType signalType = sourceSignalType;
        switch (signalType) {
            case Struct: {
                signalType = ISamples.SignalType.Text;
                if (descriptor == null || ISamples.SignalType.fromMember(descriptor) == ISamples.SignalType.Unknown) break;
                signalType = ISamples.SignalType.fromMember(descriptor);
                break;
            }
            case EventArray: {
                signalType = ISamples.SignalType.Event;
                break;
            }
            case IntegerArray: {
                signalType = ISamples.SignalType.Integer;
                break;
            }
            case FloatArray: {
                signalType = ISamples.SignalType.Float;
                break;
            }
            case TextArray: {
                signalType = ISamples.SignalType.Text;
            }
        }
        return signalType;
    }

    @Override
    protected void updateInit() {
        for (IMemberDescriptor d : this.source.getMemberDescriptors()) {
            if (d.isHidden() || this.memberMap.containsKey(d.getId())) continue;
            new MemberExtractSlaveProduction(d);
        }
    }

    @Override
    public List<String> getChildSlaveIds(String slaveId) {
        ArrayList<String> list = new ArrayList<String>();
        if (this.mode != 0 && this.slaveProductions != null && this.idMap != null) {
            MemberExtractSlaveProduction slave = slaveId != null ? (MemberExtractSlaveProduction)this.idMap.get(slaveId) : null;
            for (AbstractUpdatableMasterSamplesProducer.AbstractSlaveProduction c : this.slaveProductions) {
                if ((slave != null ? slave.memberId : -1) != ((MemberExtractSlaveProduction)c).parentId) continue;
                list.add(c.slaveId);
            }
        }
        return list;
    }

    @Override
    protected boolean instatiate(IProgress p) {
        ISamplePointer[] pointer = new ISamplePointer[]{new SamplePointer(this.source)};
        if (this.mode == 0) {
            this.targetWriter = PackedSamples.createWriter(this.processType, this.signalType, this.signalDescriptor, this.productionBase);
            if (this.targetWriter == null) {
                return false;
            }
            this.targetWriter.open(this.start, (this.flags & 0x20) != 0 ? this.end : this.start, this.rate, 0, 0, null);
            this.iter = new SamplesIterator(this.targetWriter, pointer);
            this.setReference(PackedSamples.createReader(this.targetWriter, this.readerBase));
            return this.reference != null;
        }
        if (this.mode == 2) {
            for (AbstractUpdatableMasterSamplesProducer.AbstractSlaveProduction slave : this.slaveProductions) {
                slave.instantiate();
            }
            this.iter = new SamplesIterator(pointer);
            return true;
        }
        return false;
    }

    @Override
    protected boolean reInstatiate(IProgress p) {
        boolean result = super.reInstatiate(p);
        for (AbstractUpdatableMasterSamplesProducer.AbstractSlaveProduction slave : this.slaveProductions) {
            if (slave.isInstantiated()) continue;
            slave.instantiate();
        }
        return result;
    }

    protected Object detMemberId(Object member) {
        IMemberDescriptor m;
        if (member instanceof String && (m = this.source.getMemberDescriptor(member)) != null) {
            return m.getId();
        }
        return member;
    }

    @Override
    protected boolean execute(IProgress p) {
        block8: {
            ISamplePointer[] input;
            block7: {
                input = this.iter.pointers();
                if (this.mode != 0) break block7;
                this.member = this.detMemberId(this.member);
                ISamplesWriter targetWriter = this.targetWriter;
                while (this.iter.hasNext() && !p.isCanceled()) {
                    boolean tag;
                    Object value;
                    boolean isNone;
                    Long current = this.iter.next();
                    CompoundValue cvalue = input[0].compound();
                    if (cvalue == null) continue;
                    boolean hasMember = cvalue.hasMember(this.member);
                    boolean bl = isNone = cvalue.isNone() || !hasMember;
                    if (this.ignoreNone && isNone) continue;
                    Object object = value = !isNone ? this.value(cvalue, this.member, this.signalType, false) : null;
                    if (!isNone && value == null) continue;
                    boolean bl2 = tag = this.keepTags && cvalue != null && cvalue.isTagged();
                    if (this.hideIdentical && tag == this.previousTag && Utils.equals(value, this.previousValue)) continue;
                    if (isNone) {
                        targetWriter.writeNone((long)current, tag);
                    } else {
                        targetWriter.writeSample((long)current, tag, value);
                    }
                    this.previousTag = tag;
                    this.previousValue = value;
                }
                break block8;
            }
            if (this.mode != 2) break block8;
            while (this.iter.hasNext()) {
                Long current = this.iter.next();
                CompoundValue cvalue = input[0].compound();
                for (AbstractUpdatableMasterSamplesProducer.AbstractSlaveProduction slave : this.slaveProductions) {
                    boolean tag;
                    Object value;
                    boolean isNone;
                    ISamplesWriter targetWriter = ((AbstractUpdatableMasterSamplesProducer.AbstractWritableSlaveProduction)slave).targetWriter;
                    if (targetWriter == null) continue;
                    int memberId = ((MemberExtractSlaveProduction)slave).memberId;
                    boolean hasMember = cvalue != null && cvalue.hasMember(memberId);
                    boolean bl = isNone = cvalue == null || cvalue.isNone() || !hasMember;
                    if (this.ignoreNone && isNone) continue;
                    Object object = value = !isNone ? this.value(cvalue, memberId, slave.signalType, true) : null;
                    if (!isNone && value == null) continue;
                    boolean bl3 = tag = this.keepTags && cvalue != null && cvalue.isTagged();
                    if (this.hideIdentical && tag == slave.previousTag && Utils.equals(value, slave.previousValue)) continue;
                    if (isNone) {
                        targetWriter.writeNone((long)current, tag);
                    } else {
                        targetWriter.writeSample((long)current, tag, value);
                    }
                    slave.previousTag = tag;
                    slave.previousValue = value;
                }
            }
        }
        return true;
    }

    private Object value(CompoundValue value, Object member, ISamples.SignalType signalType, boolean cloneStruct) {
        switch (signalType) {
            case Unknown: {
                return null;
            }
            case Text: {
                return value.stringValueOf(member);
            }
            case Event: {
                return value.enumValueOf(member);
            }
            case Integer: {
                return value.numberValueOf(member);
            }
            case Float: {
                return value.numberValueOf(member);
            }
            case Logic: {
                return value.logicValueOf(member);
            }
            case Struct: {
                Struct s = value.structValueOf(member);
                return s != null && cloneStruct ? s.clone() : s;
            }
            case Binary: {
                return value.bytesValueOf(member);
            }
            case EventArray: {
                return value.valueOf(member);
            }
            case IntegerArray: {
                return value.valueOf(member);
            }
            case FloatArray: {
                return value.valueOf(member);
            }
            case TextArray: {
                return value.valueOf(member);
            }
        }
        return null;
    }

    class MemberExtractSlaveProduction
    extends AbstractUpdatableMasterSamplesProducer.AbstractWritableSlaveProduction {
        int memberId;
        int parentId;

        public MemberExtractSlaveProduction(IMemberDescriptor descriptor) {
            super(descriptor.getName(), null, "." + descriptor.getName(), MemberExtract.this.detSignalType(MemberExtract.this.source.getSignalType(), descriptor), ISamples.SignalDescriptor.fromMember(descriptor));
            this.memberId = descriptor.getId();
            this.parentId = descriptor.getParentId();
            MemberExtract.this.memberMap.put(this.memberId, this);
        }

        public MemberExtractSlaveProduction(int memberId) {
            super(String.valueOf(memberId), null, "[" + String.valueOf(memberId) + "]", MemberExtract.this.detSignalType(MemberExtract.this.source.getSignalType(), null), ISamples.SignalDescriptor.DEFAULT);
            this.memberId = memberId;
            this.parentId = -1;
            MemberExtract.this.memberMap.put(this.memberId, this);
        }
    }
}

