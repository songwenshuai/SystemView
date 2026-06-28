/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.paint.IPaintStyle;
import de.toem.impulse.paint.plan.PaintStyle;
import de.toem.impulse.samples.IEventSamplesWriter;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.samples.producer.AbstractUpdatableMasterSamplesProducer;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.Struct;
import de.toem.toolkits.core.Assert;
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
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public abstract class AbstractContextExtract
extends AbstractUpdatableMasterSamplesProducer {
    protected IReadableSamples source;
    protected static final String MEMBER_EVENT = "Event";
    protected Object eventMemberId;
    protected long current;
    protected int index;

    public AbstractContextExtract() {
    }

    public AbstractContextExtract(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, 2, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public AbstractContextExtract(IReadableSamples source) {
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

    protected Object detMemberId(Object currentId, String name) {
        IMemberDescriptor m;
        if ((currentId instanceof String || currentId == null) && (m = this.source.getMemberDescriptor(name)) != null) {
            return m.getId();
        }
        return currentId != null ? currentId : name;
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

    public abstract class Context
    extends AbstractUpdatableMasterSamplesProducer.AbstractWritableSlaveProduction
    implements IContext {
        protected int core;
        protected int state;
        protected boolean running;
        protected long total;
        protected long calls;
        protected long enteredAt;
        protected long durationMin;
        protected long durationMax;
        protected Object color;
        protected PaintStyle paintStyle;
        Map<String, Integer> targets;

        public Context(String slaveId, String name, boolean instatiate) {
            super(AbstractContextExtract.this, slaveId, null, name, ISamples.SignalType.EventArray, new ISamples.SignalDescriptor("default", 2, 0, -1));
            this.state = 0;
            this.paintStyle = new PaintStyle();
            this.targets = new HashMap<String, Integer>();
            if (instatiate) {
                this.instantiate();
            }
            this.paintStyle.diagramType = 10;
            this.paintStyle.valueColumnFormat = -1;
            this.paintStyle.diagramMods = 22528;
        }

        public void setLabel(String label) {
            if (label != null && !Utils.equals(label, this.name)) {
                this.name = label;
            }
        }

        @Override
        public Object getColor() {
            return this.color;
        }

        @Override
        public IPaintStyle getPaintStyle() {
            return this.paintStyle;
        }

        @Override
        public CompoundValue compoundAt(int idx) {
            Object value;
            CompoundValue compound = super.compoundAt(idx);
            Object object = value = compound != null ? compound.val() : null;
            if (value instanceof Enumeration[] && ((Enumeration[])value).length == 2) {
                this.labelFromSource(((Enumeration[])value)[1]);
            }
            return compound;
        }

        @Override
        public CompoundValue compoundAt(int idx, int attachments) {
            Object value;
            CompoundValue compound = super.compoundAt(idx, attachments);
            Object object = value = compound != null ? compound.val() : null;
            if (value instanceof Enumeration[] && ((Enumeration[])value).length == 2) {
                this.labelFromSource(((Enumeration[])value)[1]);
            }
            return compound;
        }

        @Override
        public Object valueAt(int idx) {
            Object value = super.valueAt(idx);
            if (value instanceof Enumeration[] && ((Enumeration[])value).length == 2) {
                this.labelFromSource(((Enumeration[])value)[1]);
            }
            return value;
        }

        private void labelFromSource(Enumeration tenum) {
            Object senum;
            Struct struct = AbstractContextExtract.this.source.structValueAt(tenum.value);
            Object object = senum = struct != null ? struct.valueOf(AbstractContextExtract.this.eventMemberId) : null;
            if (senum instanceof Enumeration) {
                tenum.label = ((Enumeration)senum).label;
                tenum.value = ((Enumeration)senum).value;
            }
        }

        @Override
        public void instantiate() {
            super.instantiate();
            if (this.targetWriter != null) {
                this.targetWriter.setMember(0, "State", "state", -1);
                this.targetWriter.setMember(1, AbstractContextExtract.MEMBER_EVENT, "event", -1);
            }
        }

        @Override
        public int getCore() {
            return this.core;
        }

        public final void enter(int state, int event, IContext from) {
            this.run(true, false);
            this.change(state, event);
            if (from != null && from != this) {
                this.writeRelation(from, event, false);
            }
        }

        public final void leave(int state, int event, IContext to) {
            this.run(false, false);
            this.change(state, event);
            if (to != null && to != this) {
                this.writeRelation(to, event, true);
            }
        }

        @Override
        public boolean isRunning() {
            return this.running;
        }

        @Override
        public void change(int state, int event) {
            this.state = state;
            ((IEventSamplesWriter)this.targetWriter).write(AbstractContextExtract.this.current, false, new int[]{this.state, AbstractContextExtract.this.index});
        }

        protected void writeRelation(IContext to, int event, boolean reverse) {
            int targetStyleId = 0;
            if (!this.targets.containsKey(to.getId())) {
                targetStyleId = this.targets.size() + 1;
                this.targets.put(to.getId(), targetStyleId);
                this.targetWriter.setEnum(1, to.getId(), targetStyleId);
                this.targetWriter.setEnum(2, "/666666/line/none//" + this.getName().replace("/", "") + "/" + to.getName().replace("/", ""), targetStyleId);
            } else {
                targetStyleId = this.targets.get(to.getId());
            }
            ((IEventSamplesWriter)this.targetWriter).attachRelation(!reverse ? 1 : 0, targetStyleId, targetStyleId, 0L);
        }

        protected final void run(boolean enter, boolean interruption) {
            if (enter && !this.running) {
                if (!interruption) {
                    this.enteredAt = AbstractContextExtract.this.current;
                    ++this.calls;
                }
                this.running = true;
            } else if (!enter && this.running) {
                if (!interruption) {
                    long duration = AbstractContextExtract.this.current - this.enteredAt;
                    this.total += duration;
                    if (this.durationMax < duration) {
                        this.durationMax = duration;
                    }
                    if (this.durationMin == 0L || this.durationMin > duration) {
                        this.durationMin = duration;
                    }
                }
                this.running = false;
            }
        }
    }

    public class Folder
    extends AbstractUpdatableMasterSamplesProducer.AbstractWritableSlaveProduction {
        public Folder(String slaveId, String name) {
            super(AbstractContextExtract.this, slaveId, null, name, ISamples.SignalType.Event, ISamples.SignalDescriptor.EventGantt);
        }

        @Override
        public String getIconId() {
            return "configuration.folder";
        }
    }

    public static interface IContext
    extends ISamplesProducer.ISlaveProduction {
        @Override
        public String getId();

        @Override
        public String getName();

        public int getCore();

        public void enter(int var1, IContext var2);

        public void leave(int var1, IContext var2);

        public boolean isRunning();

        public void change(int var1, int var2);
    }

    public static interface IInterruptibleContext
    extends IContext {
        public void interruptTo(int var1, IInterruptingContext var2);

        public boolean isInterrupted();

        public IInterruptingContext getInterrupt();

        public IInterruptingContext resume(int var1);

        public void leave(int var1);
    }

    public static interface IInterruptingContext
    extends IContext {
        public IInterruptibleContext getInterrupted();
    }

    public abstract class InterruptibleContext
    extends Context
    implements IInterruptibleContext {
        protected IInterruptingContext interruptedBy;
        protected int interruptedState;

        public InterruptibleContext(String slaveId, String name, boolean instatiate) {
            super(slaveId, name, instatiate);
        }

        @Override
        public final boolean isInterrupted() {
            return this.interruptedBy != null;
        }

        @Override
        public final IInterruptingContext getInterrupt() {
            return this.interruptedBy != null && this.interruptedBy instanceof InterruptibleContext && ((InterruptibleContext)((Object)this.interruptedBy)).isInterrupted() ? ((InterruptibleContext)((Object)this.interruptedBy)).getInterrupt() : this.interruptedBy;
        }

        public final void interrupt(int state, int event, IInterruptingContext context) {
            if (this.interruptedBy == null && context != this) {
                this.interruptedState = this.state;
                this.interruptedBy = context;
                this.run(false, true);
                this.change(state, event);
                this.writeRelation(context, event, true);
            } else {
                Assert.notGettingHere("Can not interrupt");
            }
        }

        @Override
        public final IInterruptingContext resume(int event) {
            if (this.interruptedBy != null) {
                this.run(true, true);
                this.change(this.interruptedState, event);
                this.writeRelation(this.interruptedBy, event, false);
                IInterruptingContext result = this.interruptedBy;
                this.interruptedBy = null;
                this.interruptedState = 0;
                return result;
            }
            Assert.notGettingHere("Can not resume");
            return null;
        }

        @Override
        public void leave(int event) {
            if (this.interruptedBy == null) {
                Assert.notGettingHere("Can not leave");
            } else {
                this.interruptedBy = null;
                this.interruptedState = 0;
            }
        }
    }
}

