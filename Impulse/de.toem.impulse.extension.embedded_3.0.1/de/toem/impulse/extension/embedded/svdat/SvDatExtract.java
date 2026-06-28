/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.svdat;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.extension.embedded.svdat.ISvDat;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.producer.AbstractContextExtract;
import de.toem.impulse.samples.producer.AbstractUpdatableMasterSamplesProducer;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Enumeration;
import de.toem.toolkits.core.Assert;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.tlk.TLK;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SvDatExtract
extends AbstractContextExtract
implements ISvDat {
    private Object taskMemberId;
    private Object isrMemberId;
    private Object prioMemberId;
    private Object nameMemberId;
    private AbstractContextExtract.Folder taskFolder;
    private AbstractContextExtract.Folder isrFolder;
    private List<IsrContext> isrs;
    private Map<Integer, IsrContext> isrMap;
    private List<TaskContext> tasks;
    private Map<Integer, TaskContext> taskMap;
    private SchedContext schedContext;
    private IdleContext idleContext;
    private AbstractContextExtract.IContext currentContext;

    public SvDatExtract() {
    }

    public SvDatExtract(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public SvDatExtract(IReadableSamples source) {
        super(source);
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
        this.taskFolder = new AbstractContextExtract.Folder(this, "tasks", "Tasks");
        this.tasks = new ArrayList<TaskContext>();
        this.taskMap = new HashMap<Integer, TaskContext>();
        this.isrFolder = new AbstractContextExtract.Folder(this, "isrs", "ISR");
        this.isrs = new ArrayList<IsrContext>();
        this.isrMap = new HashMap<Integer, IsrContext>();
        this.updateInit();
        this.idleContext = new IdleContext();
        this.schedContext = new SchedContext();
        super.init(sstart, send, srate, readerBaseProvider);
    }

    @Override
    public void modifyInit() {
        super.modifyInit();
    }

    @Override
    protected void updateInit() {
        if (this.source != null) {
            List<Enumeration> enums;
            this.detMemberIds();
            if (this.taskMemberId instanceof Integer) {
                enums = this.source.getEnums(8 + (Integer)this.taskMemberId);
                for (Enumeration e : enums) {
                    if (this.taskMap.containsKey(e.value)) continue;
                    TaskContext task = new TaskContext(e.value, false);
                    task.setLabel(e.label);
                }
            }
            if (this.isrMemberId instanceof Integer) {
                enums = this.source.getEnums(8 + (Integer)this.isrMemberId);
                for (Enumeration e : enums) {
                    if (this.isrMap.containsKey(e.value)) continue;
                    IsrContext isr = new IsrContext(e.value, false);
                    isr.setLabel(e.label);
                }
            }
        }
    }

    protected void detMemberIds() {
        this.eventMemberId = this.detMemberId(this.eventMemberId, "Event");
        this.taskMemberId = this.detMemberId(this.taskMemberId, "taskId");
        this.isrMemberId = this.detMemberId(this.isrMemberId, "isrId");
        this.prioMemberId = this.detMemberId(this.prioMemberId, "prio");
        this.nameMemberId = this.detMemberId(this.nameMemberId, "name");
    }

    @Override
    public boolean hasChildSlaves(String slaveId) {
        return this.mode != 0 && (slaveId == null || Utils.equals(slaveId, this.taskFolder.getSlaveId()) || Utils.equals(slaveId, this.isrFolder.getSlaveId()));
    }

    @Override
    public List<String> getChildSlaveIds(String slaveId) {
        ArrayList<String> list;
        block3: {
            block4: {
                block2: {
                    list = new ArrayList<String>();
                    if (slaveId != null) break block2;
                    list.add(this.taskFolder.getSlaveId());
                    list.add(this.isrFolder.getSlaveId());
                    list.add(this.schedContext.getSlaveId());
                    list.add(this.idleContext.getSlaveId());
                    break block3;
                }
                if (!Utils.equals(slaveId, this.taskFolder.getSlaveId())) break block4;
                for (AbstractUpdatableMasterSamplesProducer.AbstractWritableSlaveProduction abstractWritableSlaveProduction : this.tasks) {
                    list.add(abstractWritableSlaveProduction.slaveId);
                }
                break block3;
            }
            if (!Utils.equals(slaveId, this.isrFolder.getSlaveId())) break block3;
            for (AbstractUpdatableMasterSamplesProducer.AbstractWritableSlaveProduction abstractWritableSlaveProduction : this.isrs) {
                list.add(abstractWritableSlaveProduction.slaveId);
            }
        }
        return list;
    }

    @Override
    protected boolean execute(IProgress p) {
        ISamplePointer[] input = this.iter.pointers();
        if (this.mode == 2) {
            this.detMemberIds();
            block33: while (this.iter.hasNext()) {
                this.current = this.iter.next();
                this.index = input[0].getIndex();
                CompoundValue cvalue = input[0].compound();
                int id = cvalue.intValueOf(this.eventMemberId);
                switch (id) {
                    case 0: {
                        break;
                    }
                    case 1: {
                        break;
                    }
                    case 2: {
                        int isrId = cvalue.intValueOf(this.isrMemberId);
                        IsrContext isr = this.isrMap.containsKey(isrId) ? this.isrMap.get(isrId) : new IsrContext(isrId, true);
                        isr.isrEnter(id);
                        break;
                    }
                    case 3: {
                        if (!(this.currentContext instanceof IsrContext)) continue block33;
                        ((IsrContext)this.currentContext).isrExit(id);
                        break;
                    }
                    case 4: {
                        int taskId = cvalue.intValueOf(this.taskMemberId);
                        TaskContext task = this.taskMap.containsKey(taskId) ? this.taskMap.get(taskId) : new TaskContext(taskId, true);
                        task.taskStartExec(id);
                        break;
                    }
                    case 5: {
                        if (!(this.currentContext instanceof TaskContext)) continue block33;
                        ((TaskContext)this.currentContext).taskStopExec(id);
                        break;
                    }
                    case 6: {
                        int taskId = cvalue.intValueOf(this.taskMemberId);
                        TaskContext task = this.taskMap.containsKey(taskId) ? this.taskMap.get(taskId) : new TaskContext(taskId, true);
                        task.taskStartReady(id);
                        break;
                    }
                    case 7: {
                        int taskId = cvalue.intValueOf(this.taskMemberId);
                        TaskContext task = this.taskMap.containsKey(taskId) ? this.taskMap.get(taskId) : new TaskContext(taskId, true);
                        task.taskStopReady(id);
                        break;
                    }
                    case 8: {
                        int taskId = cvalue.intValueOf(this.taskMemberId);
                        TaskContext task = this.taskMap.containsKey(taskId) ? this.taskMap.get(taskId) : new TaskContext(taskId, true);
                        task.taskCreate(id);
                        break;
                    }
                    case 9: {
                        int taskId = cvalue.intValueOf(this.taskMemberId);
                        int prio = cvalue.intValueOf(this.prioMemberId);
                        String name = cvalue.stringValueOf(this.nameMemberId);
                        TaskContext task = this.taskMap.containsKey(taskId) ? this.taskMap.get(taskId) : new TaskContext(taskId, true);
                        task.setLabel(name);
                        task.setPrio(prio);
                        break;
                    }
                    case 10: {
                        break;
                    }
                    case 11: {
                        break;
                    }
                    case 12: {
                        break;
                    }
                    case 13: {
                        break;
                    }
                    case 14: {
                        break;
                    }
                    case 15: {
                        break;
                    }
                    case 16: {
                        break;
                    }
                    case 17: {
                        this.idleContext.idle(id);
                        break;
                    }
                    case 18: {
                        if (!(this.currentContext instanceof IsrContext)) continue block33;
                        ((IsrContext)this.currentContext).isrExitToScheduler(id);
                        break;
                    }
                    case 19: {
                        break;
                    }
                    case 20: {
                        break;
                    }
                    case 21: {
                        break;
                    }
                    case 22: {
                        break;
                    }
                    case 24: {
                        break;
                    }
                    case 25: {
                        break;
                    }
                    case 26: {
                        break;
                    }
                    case 27: {
                        break;
                    }
                    case 28: {
                        break;
                    }
                    case 29: {
                        break;
                    }
                    case 30: {
                        break;
                    }
                    case 31: {
                        break;
                    }
                    default: {
                        if (id < 32) break;
                    }
                }
            }
        }
        return true;
    }

    class IdleContext
    extends SvDatContext {
        public IdleContext() {
            super("idle", "idle", false);
            SvDatExtract.this.idleContext = this;
        }

        public void idle(int eventId) {
            if (SvDatExtract.this.currentContext != null && SvDatExtract.this.currentContext != this) {
                SvDatExtract.this.currentContext.leave(eventId, this);
            }
            this.enter(1, eventId, SvDatExtract.this.currentContext);
            SvDatExtract.this.currentContext = this;
        }
    }

    final class IsrContext
    extends SvDatContext
    implements AbstractContextExtract.IInterruptingContext {
        private AbstractContextExtract.IInterruptibleContext interrupted;

        public IsrContext(int isrId, boolean instatiate) {
            super(String.valueOf(SvDatExtract.this.isrFolder.getSlaveId()) + "." + isrId, "isr" + isrId, instatiate);
            SvDatExtract.this.isrs.add(this);
            SvDatExtract.this.isrMap.put(isrId, this);
            this.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample." + SvDatExtract.this.isrMap.size() % 10, 0);
        }

        @Override
        public void instantiate() {
            super.instantiate();
            if (this.targetWriter != null) {
                this.targetWriter.setEnum(8, "Running", 1);
                this.targetWriter.setEnum(8, null, 0);
            }
        }

        @Override
        public final void leave(int eventId) {
            super.leave(eventId);
            this.interrupted.leave(eventId);
        }

        @Override
        public AbstractContextExtract.IInterruptibleContext getInterrupted() {
            return this.interrupted;
        }

        void isrEnter(int eventId) {
            if (SvDatExtract.this.currentContext instanceof AbstractContextExtract.IInterruptibleContext) {
                ((AbstractContextExtract.IInterruptibleContext)SvDatExtract.this.currentContext).interruptTo(eventId, this);
                this.interrupted = (AbstractContextExtract.InterruptibleContext)SvDatExtract.this.currentContext;
            }
            this.enter(1, eventId, SvDatExtract.this.currentContext);
            SvDatExtract.this.currentContext = this;
        }

        void isrExit(int eventId) {
            if (SvDatExtract.this.currentContext == this) {
                this.leave(0, eventId, this.interrupted);
                SvDatExtract.this.currentContext = this.interrupted;
                if (this.interrupted != null) {
                    this.interrupted.resume(eventId);
                }
                this.interrupted = null;
            } else {
                Assert.notGettingHere("Can not exit isr");
            }
        }

        public void isrExitToScheduler(int eventId) {
            if (SvDatExtract.this.currentContext == this) {
                this.leave(0, eventId, SvDatExtract.this.schedContext);
                SvDatExtract.this.schedContext.enter(eventId, SvDatExtract.this.currentContext);
                SvDatExtract.this.currentContext = SvDatExtract.this.schedContext;
                if (this.interrupted != null) {
                    this.interrupted.leave(eventId);
                }
                this.interrupted = null;
            } else {
                Assert.notGettingHere("Can not exit isr");
            }
        }
    }

    class SchedContext
    extends SvDatContext {
        public SchedContext() {
            super("scheduler", "scheduler", false);
            SvDatExtract.this.schedContext = this;
        }
    }

    abstract class SvDatContext
    extends AbstractContextExtract.InterruptibleContext {
        static final int STATE_RUNNING = 1;
        static final int STATE_WAITING = 0;

        public SvDatContext(String slaveId, String name, boolean instatiate) {
            super(SvDatExtract.this, slaveId, name, instatiate);
        }

        @Override
        public void instantiate() {
            super.instantiate();
            if (this.targetWriter != null) {
                this.targetWriter.setEnum(8, "Running", 1);
                this.targetWriter.setEnum(8, "Waiting", 0);
            }
        }

        @Override
        public final void enter(int eventId, AbstractContextExtract.IContext from) {
            this.enter(1, eventId, from);
        }

        @Override
        public final void leave(int eventId, AbstractContextExtract.IContext to) {
            this.leave(0, eventId, to);
        }

        @Override
        public final void interruptTo(int event, AbstractContextExtract.IInterruptingContext context) {
            this.interrupt(0, event, context);
        }
    }

    final class TaskContext
    extends SvDatContext {
        static final int STATE_READY = 65282;
        int prio;

        public TaskContext(int taskId, boolean instatiate) {
            super(String.valueOf(SvDatExtract.this.taskFolder.getSlaveId()) + "." + taskId, "task" + taskId, instatiate);
            this.prio = 0;
            SvDatExtract.this.tasks.add(this);
            SvDatExtract.this.taskMap.put(taskId, this);
            this.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.sample." + SvDatExtract.this.taskMap.size() % 10, 0);
        }

        public void setPrio(int prio) {
            this.prio = prio;
        }

        @Override
        public void instantiate() {
            super.instantiate();
            if (this.targetWriter != null) {
                this.targetWriter.setEnum(8, "Ready", 65282);
            }
        }

        public void taskCreate(int eventId) {
        }

        public void taskStartReady(int eventId) {
            this.enter(65282, eventId, null);
        }

        public void taskStartExec(int eventId) {
            if (SvDatExtract.this.currentContext != null && SvDatExtract.this.currentContext != this) {
                SvDatExtract.this.currentContext.leave(eventId, this);
            }
            this.enter(1, eventId, SvDatExtract.this.currentContext);
            SvDatExtract.this.currentContext = this;
        }

        public void taskStopReady(int eventId) {
            if (SvDatExtract.this.currentContext == this) {
                this.leave(0, eventId, SvDatExtract.this.schedContext);
                SvDatExtract.this.schedContext.enter(eventId, SvDatExtract.this.currentContext);
                SvDatExtract.this.currentContext = SvDatExtract.this.schedContext;
            } else {
                this.leave(0, eventId, SvDatExtract.this.schedContext);
            }
        }

        public void taskStopExec(int eventId) {
        }
    }
}

