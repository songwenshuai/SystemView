/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.svdat;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.view.FolderConfiguration;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.cells.view.ViewConfiguration;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IndexBase;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.extension.embedded.svdat.SvDatConfiguration;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISingleDomainRecordGenerator;
import de.toem.impulse.samples.IStructSamplesWriter;
import de.toem.impulse.serializer.BinaryParseBuffer;
import de.toem.impulse.values.StructMember;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.text.MultilineText;
import de.toem.toolkits.ui.tlk.TLK;
import java.io.BufferedReader;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.Stack;

public final class SvDatParser {
    private static final int SYSVIEW_EVENT_ID_NOP = 0;
    private static final int SYSVIEW_EVENT_ID_OVERFLOW = 1;
    private static final int SYSVIEW_EVENT_ID_ISR_ENTER = 2;
    private static final int SYSVIEW_EVENT_ID_ISR_EXIT = 3;
    private static final int SYSVIEW_EVENT_ID_TASK_START_EXEC = 4;
    private static final int SYSVIEW_EVENT_ID_TASK_STOP_EXEC = 5;
    private static final int SYSVIEW_EVENT_ID_TASK_START_READY = 6;
    private static final int SYSVIEW_EVENT_ID_TASK_STOP_READY = 7;
    private static final int SYSVIEW_EVENT_ID_TASK_CREATE = 8;
    private static final int SYSVIEW_EVENT_ID_TASK_INFO = 9;
    private static final int SYSVIEW_EVENT_ID_TRACE_START = 10;
    private static final int SYSVIEW_EVENT_ID_TRACE_STOP = 11;
    private static final int SYSVIEW_EVENT_ID_SYSTIME_CYCLES = 12;
    private static final int SYSVIEW_EVENT_ID_SYSTIME_US = 13;
    private static final int SYSVIEW_EVENT_ID_SYSDESC = 14;
    private static final int SYSVIEW_EVENT_ID_USER_START = 15;
    private static final int SYSVIEW_EVENT_ID_USER_STOP = 16;
    private static final int SYSVIEW_EVENT_ID_IDLE = 17;
    private static final int SYSVIEW_EVENT_ID_ISR_TO_SCHEDULER = 18;
    private static final int SYSVIEW_EVENT_ID_TIMER_ENTER = 19;
    private static final int SYSVIEW_EVENT_ID_TIMER_EXIT = 20;
    private static final int SYSVIEW_EVENT_ID_STACK_INFO = 21;
    private static final int SYSVIEW_EVENT_ID_MODULEDESC = 22;
    private static final int SYSVIEW_EVENT_ID_INIT = 24;
    private static final int SYSVIEW_EVENT_ID_NAME_RESOURCE = 25;
    private static final int SYSVIEW_EVENT_ID_PRINT_FORMATTED = 26;
    private static final int SYSVIEW_EVENT_ID_NUMMODULES = 27;
    private static final int SYSVIEW_EVENT_ID_END_CALL = 28;
    private static final int SYSVIEW_EVENT_ID_TASK_TERMINATE = 29;
    private static final int SYSVIEW_EVENT_ID_UNKNOWN = 30;
    private static final int SYSVIEW_EVENT_ID_EX = 31;
    private final SvDatConfiguration configuration;
    private final ISingleDomainRecordGenerator generator;
    private final TimeBase timeBase;
    private int version = -1;
    private int changed;
    private boolean removeHeader = true;
    private long timeStampFreq = 0L;
    private long timeStamp = 0L;
    private long current = 0L;
    private Map<String, String> sysDescription = new HashMap<String, String>();
    private Map<Integer, String> resources = new HashMap<Integer, String>();
    private Map<Integer, String[]> userEvents = new HashMap<Integer, String[]>();
    private Map<Integer, IsrContext> isrs = new HashMap<Integer, IsrContext>();
    private Map<Integer, TaskContext> tasks = new HashMap<Integer, TaskContext>();
    private Map<Integer, UserContext> userCalls = new HashMap<Integer, UserContext>();
    private Map<Integer, TimerContext> timers = new HashMap<Integer, TimerContext>();
    private SchedContext schedContext;
    private IdleContext idleContext;
    private Context noneContext;
    private Context terminal;
    private Context rawLog;
    private Stack<IsrContext> currentIsrs;
    private Context currentContext;

    public SvDatParser(ISingleDomainRecordGenerator generator, SvDatConfiguration configuration) {
        this.generator = generator;
        this.configuration = configuration;
        this.timeBase = (TimeBase)DomainBase.parse(configuration.domainBase);
        generator.initRecord("SystemViewer", this.timeBase);
        this.terminal = new Context("Terminal", false);
        this.rawLog = new Context("SV Events", false);
        this.noneContext = new Context("None", false);
        this.idleContext = new IdleContext("Idle");
        this.schedContext = new SchedContext("Scheduler");
        this.idleContext.enter("Running", "Inital");
        this.currentIsrs = new Stack();
        this.currentContext = this.idleContext;
        this.currentContext.enter("Inital");
        generator.open(0L);
        String userEvents = MultilineText.toAscii(configuration.userEvents);
        if (!Utils.isEmpty(userEvents)) {
            BufferedReader r = new BufferedReader(new StringReader(userEvents));
            String line = null;
            try {
                while ((line = r.readLine()) != null) {
                    String[] splitted = line.trim().split("\\s+");
                    if (splitted.length < 2) continue;
                    int id = Utils.parseInt(splitted[0], -1);
                    int n = 2;
                    while (n < splitted.length) {
                        int len = splitted[n].length();
                        int eq = splitted[n].indexOf("=%");
                        if (eq == -1 || eq == 0 || eq != len - 3) {
                            id = -1;
                        } else {
                            splitted[n] = String.valueOf(splitted[n].substring(len - 1, len)) + splitted[n].substring(0, eq).toLowerCase();
                        }
                        ++n;
                    }
                    if (id <= 0) continue;
                    this.userEvents.put(id, splitted);
                }
            }
            catch (Throwable e) {
                SystemLog.log(e);
            }
        }
        this.changed = 4;
    }

    public void finalizeParse() {
        PlotConfiguration plotConfiguration;
        this.generator.close(this.current + 1L);
        ViewConfiguration view = this.generator.addViewConfiguration("System Viewer Default View", null);
        int idx = 0;
        FolderConfiguration folder = this.generator.addFolderConfiguration(view, "Isrs", null);
        for (IsrContext isrContext : this.isrs.values()) {
            this.addGanttPlot(folder, isrContext.signal, this.calcPlotColor(16761856, idx));
            ++idx;
        }
        idx = 0;
        folder = this.generator.addFolderConfiguration(view, "Tasks", null);
        for (TaskContext taskContext : this.tasks.values()) {
            this.addGanttPlot(folder, taskContext.signal, this.calcPlotColor(1810914, idx));
            ++idx;
        }
        idx = 0;
        folder = this.generator.addFolderConfiguration(view, "Timer", null);
        for (TimerContext timerContext : this.timers.values()) {
            this.addGanttPlot(folder, timerContext.signal, this.calcPlotColor(8926671, idx));
            ++idx;
        }
        if (!folder.hasChildren()) {
            view.removeChild(folder);
        }
        idx = 0;
        folder = this.generator.addFolderConfiguration(view, "User calls", null);
        for (UserContext userContext : this.userCalls.values()) {
            this.addGanttPlot(folder, userContext.signal, this.calcPlotColor(691511, idx));
            ++idx;
        }
        if (!folder.hasChildren()) {
            view.removeChild(folder);
        }
        folder = this.generator.addFolderConfiguration(view, "Scheduler", null);
        this.addGanttPlot(folder, this.idleContext.signal, 0xAAAAAA);
        this.addGanttPlot(folder, this.schedContext.signal, 9136404);
        folder = this.generator.addFolderConfiguration(view, "Console", null);
        PlotConfiguration plotConfiguration2 = this.generator.addPlotConfiguration(folder, this.terminal.signal);
        if (plotConfiguration2 != null) {
            plotConfiguration2.columnValueFormat = 262150;
        }
        if ((plotConfiguration = this.generator.addPlotConfiguration(folder, this.rawLog.signal)) != null) {
            plotConfiguration.columnValueFormat = 262150;
        }
        this.generator.initRecord("SystemViewer", IndexBase.n);
        Signal statistics = this.generator.addSignal(null, "Basic Statistics", null, ISamples.ProcessType.Discrete, ISamples.SignalType.Struct, ISamples.SignalDescriptor.DEFAULT);
        IStructSamplesWriter structWriter = (IStructSamplesWriter)this.generator.getWriter(statistics);
        StructMember[] members = new StructMember[]{new StructMember("Context", 7, "LABEL", 6), new StructMember("Calls", 3, null, -1), new StructMember("Frequency [Hz]", 3, null, -1), new StructMember("Time Min [ms]", 4, null, -1), new StructMember("Time Max [ms]", 4, null, -1), new StructMember("Time Total [ms]", 4, null, -1), new StructMember("Time Total [%]", 4, null, -1)};
        ArrayList<Context> all = new ArrayList<Context>();
        all.addAll(this.isrs.values());
        all.addAll(this.tasks.values());
        all.addAll(this.timers.values());
        all.addAll(this.userCalls.values());
        all.add(this.idleContext);
        all.add(this.schedContext);
        idx = 0;
        structWriter.open(idx);
        for (Context c : all) {
            members[0].setValue(c.name);
            members[1].setValue(c.calls);
            members[2].setValue(Math.round((double)c.calls / ((double)this.current * this.timeBase.toSeconds())));
            members[3].setValue(SvDatParser.round((double)c.durationMin / TimeBase.ms.toSeconds() * this.timeBase.toSeconds(), 4));
            members[4].setValue(SvDatParser.round((double)c.durationMax / TimeBase.ms.toSeconds() * this.timeBase.toSeconds(), 4));
            members[5].setValue(SvDatParser.round((double)c.total / TimeBase.ms.toSeconds() * this.timeBase.toSeconds(), 4));
            members[6].setValue(SvDatParser.round((double)c.total * 100.0 / (double)this.current, 4));
            structWriter.write((long)idx++, false, members);
        }
        structWriter.close(idx);
        folder = this.generator.addFolderConfiguration(view, "Statistics", null);
        PlotConfiguration plotConfiguration3 = this.generator.addPlotConfiguration(folder, statistics);
        if (plotConfiguration3 != null) {
            plotConfiguration3.color = 6583943;
            plotConfiguration3.preferedHeight = true;
            plotConfiguration3.preferedHeightValue = 200;
        }
    }

    public int hasChanged() {
        return this.changed;
    }

    public void resetChanged() {
        this.changed = 0;
    }

    public long getCurrent() {
        return this.current;
    }

    public void parseEntry(BinaryParseBuffer b) {
        int id = (int)b.parsePlus();
        if (!b.isOk()) {
            return;
        }
        if (this.removeHeader) {
            if (id == 59) {
                b.parseLine();
                return;
            }
            this.removeHeader = false;
        }
        long l = -1L;
        long t = -1L;
        switch (id) {
            case 0: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_NOP", "udt", t);
                break;
            }
            case 1: {
                int packets = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_OVERFLOW", "udt,upackets", t, packets);
                break;
            }
            case 2: {
                int isrId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_ISR_ENTER", "udt,uisrId", t, isrId);
                if (!this.isrs.containsKey(isrId)) break;
                this.isrs.get(isrId).startIsr();
                break;
            }
            case 3: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_ISR_EXIT", "udt", t);
                if (this.currentIsrs.isEmpty()) break;
                this.currentIsrs.peek().stopIsr();
                break;
            }
            case 4: {
                int taskId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_TASK_START_EXEC", "udt,utaskId", t, taskId);
                if (!this.tasks.containsKey(taskId)) break;
                this.tasks.get(taskId).taskStartExec();
                break;
            }
            case 5: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_TASK_STOP_EXEC", "udt", t);
                if (!(this.currentContext instanceof TaskContext)) break;
                ((TaskContext)this.currentContext).taskStopExec();
                break;
            }
            case 6: {
                int taskId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_TASK_START_READY", "udt,utaskId", t, taskId);
                if (!this.tasks.containsKey(taskId)) break;
                this.tasks.get(taskId).taskStartReady();
                break;
            }
            case 7: {
                int taskId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int cause = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_TASK_STOP_READY", "udt,utaskId,ucause", t, taskId, cause);
                if (!this.tasks.containsKey(taskId)) break;
                this.tasks.get(taskId).taskStopReady();
                break;
            }
            case 8: {
                int taskId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_TASK_CREATE", "udt,utaskId", t, taskId);
                if (!this.tasks.containsKey(taskId)) break;
                this.tasks.get(taskId).taskCreate();
                break;
            }
            case 9: {
                int taskId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int prio = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                String name = b.parseString();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_TASK_INFO", "udt,utaskId,uprio,sname", t, taskId, prio, name);
                if (this.tasks.containsKey(taskId)) break;
                this.tasks.put(taskId, new TaskContext(name));
                break;
            }
            case 10: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.timeStamp = 0L;
                this.log("EV_TRACE_START", "udt", t);
                break;
            }
            case 11: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_TRACE_STOP", "udt", t);
                break;
            }
            case 12: {
                long cycles = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_SYSTIME_CYCLES", "udt,ucycles", t, cycles);
                break;
            }
            case 13: {
                long time = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                time |= b.parsePlus() << 32;
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                this.adjustCurrent(t);
                this.log("EV_SYSTIME_US", "udt,utime", t, time);
                break;
            }
            case 14: {
                String description = b.parseString();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_SYSDESC", "udt,sdescription", t, description);
                this.parseSysDesc(description);
                break;
            }
            case 15: {
                int userId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_USER_START", "udt,uuserId", t, userId);
                if (!this.userCalls.containsKey(userId)) {
                    this.userCalls.put(userId, new UserContext("User<" + userId + ">"));
                }
                this.userCalls.get(userId).userEnter();
                break;
            }
            case 16: {
                int userId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_USER_STOP", "udt,uuserId", t, userId);
                if (this.getActiveContext() != this.userCalls.get(userId)) break;
                ((UserContext)this.getActiveContext()).userExit();
                break;
            }
            case 17: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_IDLE", "udt", t);
                this.idleContext.goIdle(new Object[0]);
                break;
            }
            case 18: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_ISR_TO_SCHEDULER", "udt", t);
                if (this.currentIsrs.isEmpty()) break;
                this.currentIsrs.peek().stopIsrToSched();
                break;
            }
            case 19: {
                int timerId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_TIMER_ENTER", "udt,utimerId", t, timerId);
                if (!this.timers.containsKey(timerId)) {
                    this.timers.put(timerId, new TimerContext("Timer<" + timerId + ">"));
                }
                this.timers.get(timerId).timerEnter();
                break;
            }
            case 20: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_TIMER_EXIT", "udt", t);
                if (!(this.getActiveContext() instanceof TimerContext)) break;
                ((TimerContext)this.getActiveContext()).timerExit();
                break;
            }
            case 21: {
                int stackId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int stackBase = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int stackSize = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_STACK_INFO", "udt,ustackId,ustackBase,ustackSize", t, stackId, stackBase, stackSize);
                break;
            }
            case 22: {
                int moduleId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int events = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                String description = b.parseString();
                if (!b.isOk()) {
                    return;
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_MODULEDESC", "udt,umoduleId,uevents,sdescription", t, moduleId, events, description);
                break;
            }
            case 24: {
                l = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int pos = b.pos;
                long f0 = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                long f1 = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                long start = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int shift = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                if ((long)(b.pos - pos) < l) {
                    b.skipBytes((int)(l - (long)(b.pos - pos)));
                    if (!b.isOk()) {
                        return;
                    }
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.timeStampFreq = f0;
                this.adjustCurrent(t);
                this.log("EV_INIT", "udt,uf0,uf1,ustart,ushift", t, f0, f1, start, shift);
                break;
            }
            case 25: {
                l = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int pos = b.pos;
                int resourceId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                String name = b.parseString();
                if (!b.isOk()) {
                    return;
                }
                if ((long)(b.pos - pos) < l) {
                    b.skipBytes((int)(l - (long)(b.pos - pos)));
                    if (!b.isOk()) {
                        return;
                    }
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_NAME_RESOURCE", "udt,uresourceId,sname", t, resourceId, name);
                this.resources.put(resourceId, name);
                break;
            }
            case 26: {
                l = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int pos = b.pos;
                String message = b.parseString().trim();
                if (!b.isOk()) {
                    return;
                }
                if ((long)(b.pos - pos) < l) {
                    b.skipBytes((int)(l - (long)(b.pos - pos)));
                    if (!b.isOk()) {
                        return;
                    }
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_PRINT_FORMATTED", "udt,smessage", t, message);
                String context = this.getActiveContext().name;
                this.terminal.write(message, "cContext", new Object[]{context});
                break;
            }
            case 27: {
                l = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int pos = b.pos;
                int numModules = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                if ((long)(b.pos - pos) < l) {
                    b.skipBytes((int)(l - (long)(b.pos - pos)));
                    if (!b.isOk()) {
                        return;
                    }
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_NUMMODULES", "udt,unumModules", t, numModules);
                break;
            }
            case 28: {
                l = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int pos = b.pos;
                int eventId = (int)b.parsePlus();
                int para0 = 0;
                if (!b.isOk()) {
                    return;
                }
                if ((long)(b.pos - pos) < l) {
                    para0 = (int)b.parsePlus();
                    if (!b.isOk()) {
                        return;
                    }
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_END_CALL", "udt,ueventId,upara0", t, eventId, para0);
                break;
            }
            case 29: {
                l = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int pos = b.pos;
                int taskId = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                if ((long)(b.pos - pos) < l) {
                    b.skipBytes((int)(l - (long)(b.pos - pos)));
                    if (!b.isOk()) {
                        return;
                    }
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_TASK_TERMINATE", "udt,utaskId", t, taskId);
                break;
            }
            case 30: {
                l = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int pos = b.pos;
                if ((long)(b.pos - pos) < l) {
                    b.skipBytes((int)(l - (long)(b.pos - pos)));
                    if (!b.isOk()) {
                        return;
                    }
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_UNKNOWN", "udt", t);
                break;
            }
            case 31: {
                l = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                int pos = b.pos;
                int numModules = (int)b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                if ((long)(b.pos - pos) < l) {
                    b.skipBytes((int)(l - (long)(b.pos - pos)));
                    if (!b.isOk()) {
                        return;
                    }
                }
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.log("EV_NUMMODULES", "udt,unumModules", t, numModules);
                break;
            }
            default: {
                if (id >= 32 && id <= 2047) {
                    l = b.parsePlus();
                    if (!b.isOk()) {
                        return;
                    }
                    int pos = b.pos;
                    byte[] bytes = b.peekBytes((int)l);
                    if (!b.isOk()) {
                        return;
                    }
                    String event = "Unknown";
                    ArrayList<Object> values = new ArrayList<Object>();
                    String labels = "";
                    String[] userEvent = this.userEvents.get(id);
                    if (userEvent == null) {
                        labels = "pid,xbytes";
                        if (id >= 32 && id <= 1023) {
                            event = "OS Event";
                        } else if (id >= 1024 && id <= 2048) {
                            event = "User Event";
                        }
                        values.add(id);
                        values.add(b.getBytes((int)l));
                        if (!b.isOk()) {
                            return;
                        }
                    } else {
                        event = userEvent[1];
                        int n = 2;
                        while (n < userEvent.length) {
                            labels = String.valueOf(labels) + (!labels.isEmpty() ? "," : "") + userEvent[n];
                            int val = (int)b.parsePlus();
                            values.add(val);
                            ++n;
                        }
                    }
                    b.pos = pos;
                    b.skipBytes((int)l);
                    t = b.parsePlus();
                    if (!b.isOk()) {
                        return;
                    }
                    this.adjustCurrent(t);
                    this.log(id < 1024 ? "EV_OS" : "EV_USER", "udt,uid,xbytes", t, id, bytes);
                    this.getActiveContext().write(event, labels, values.toArray());
                    break;
                }
                b.setError("Invalid id");
            }
        }
    }

    private void parseSysDesc(String sysDesc) {
        String[] splitted;
        String[] stringArray = splitted = sysDesc.split("\\,");
        int n = splitted.length;
        int n2 = 0;
        while (n2 < n) {
            String def = stringArray[n2];
            int idx = def.indexOf(61);
            if (idx > 0) {
                int isr;
                String key = def.substring(0, idx);
                String value = def.substring(idx + 1);
                this.sysDescription.put(key, value);
                if (key.startsWith("I#") && !this.isrs.containsKey(isr = Utils.parseInt(key.substring(2), -1))) {
                    this.isrs.put(isr, new IsrContext(value));
                }
            }
            ++n2;
        }
    }

    private void adjustCurrent(long t) {
        this.timeStamp += t;
        if (this.timeStampFreq > 0L) {
            this.current = (long)((double)this.timeStamp / this.timeBase.toSeconds() / (double)this.timeStampFreq);
            this.changed = this.changed < 2 ? 2 : this.changed;
        }
    }

    private void log(String event, String labels, Object ... p) {
        this.rawLog.write(event, labels, p);
    }

    private Context getActiveContext() {
        if (!this.currentIsrs.isEmpty()) {
            return this.currentIsrs.peek().getActiveContext();
        }
        if (this.currentContext != null) {
            return this.currentContext.getActiveContext();
        }
        return this.noneContext;
    }

    private static double round(double wert, int stellen) {
        return (double)Math.round(wert * Math.pow(10.0, stellen)) / Math.pow(10.0, stellen);
    }

    private int calcPlotColor(int base, int idx) {
        float[] hsb = TLK.rgbInt2Hsb(base);
        float c = (hsb[0] + (float)((idx + 1) / 2 * 10 * (idx % 2 == 0 ? 1 : -1))) % 360.0f;
        if (c < 0.0f) {
            c += 360.0f;
        }
        return TLK.hsb2RgbInt(c, hsb[1], hsb[2]);
    }

    private void addGanttPlot(ICell cell, Signal signal, int color) {
        if (signal == null) {
            return;
        }
        PlotConfiguration plot = this.generator.addPlotConfiguration(cell, signal);
        plot.style = 10;
        plot.color = color;
        plot.columnValueFormat = 262150;
    }

    class Context {
        String name;
        Signal signal;
        boolean hasState;
        String state;
        String interruptedState;
        Context innerContext;
        StructMember[] defaultmembers;
        Map<String, StructMember[]> eventmembers = new HashMap<String, StructMember[]>();
        long total;
        long calls;
        boolean entered;
        long enteredAt;
        long durationMin;
        long durationMax;

        public Context(String name, boolean hasState) {
            this.name = name;
            this.hasState = hasState;
        }

        protected String getDestination() {
            this.createSignal();
            return this.signal.getPath();
        }

        protected Context getActiveContext() {
            if (this.innerContext != null) {
                return this.innerContext.getActiveContext();
            }
            return this;
        }

        protected void createSignal() {
            if (this.signal == null) {
                this.signal = SvDatParser.this.generator.addSignal(null, this.name, null, ISamples.ProcessType.Discrete, ISamples.SignalType.Struct, new ISamples.SignalDescriptor(this.hasState ? "gantt" : "log", 6));
            }
            if (this.defaultmembers == null) {
                IStructSamplesWriter structWriter = (IStructSamplesWriter)SvDatParser.this.generator.getWriter(this.signal);
                this.defaultmembers = new StructMember[2];
                this.defaultmembers[0] = new StructMember("State", 7, "STATE", 6);
                this.defaultmembers[1] = new StructMember("Event", 8, "EVENT", 6);
                structWriter.getLegend().addEnum(8, "Running", 1);
                structWriter.getLegend().addEnum(8, "Waiting", 0);
                structWriter.getLegend().addEnum(8, "Ready", 65282);
                this.defaultmembers[0].setValid(this.hasState);
            }
            SvDatParser.this.changed = SvDatParser.this.changed < 4 ? 4 : SvDatParser.this.changed;
        }

        void write(String event) {
            this.createSignal();
            IStructSamplesWriter structWriter = (IStructSamplesWriter)SvDatParser.this.generator.getWriter(this.signal);
            this.defaultmembers[0].setValue(this.state);
            this.defaultmembers[1].setValue(event);
            structWriter.write(SvDatParser.this.current, false, this.defaultmembers);
            SvDatParser.this.changed = SvDatParser.this.changed < 3 ? 3 : SvDatParser.this.changed;
        }

        void write(String event, String parameters, Object[] values) {
            this.write(event, parameters.isEmpty() ? new String[]{} : parameters.split(","), values);
        }

        void write(String event, String[] parameters, Object[] values) {
            String p;
            String[] stringArray;
            char type;
            int n;
            int midx;
            boolean tag = false;
            if (event != null && (event.toLowerCase().contains("overflow") || event.toLowerCase().contains("error"))) {
                tag = true;
            }
            this.createSignal();
            String signature = "";
            String[] stringArray2 = parameters;
            int n2 = parameters.length;
            int n3 = 0;
            while (n3 < n2) {
                String p2 = stringArray2[n3];
                signature = String.valueOf(signature) + p2;
                ++n3;
            }
            StructMember[] sigMembers = null;
            if (!this.eventmembers.containsKey(signature)) {
                sigMembers = new StructMember[parameters.length + this.defaultmembers.length];
                int n4 = 0;
                while (n4 < this.defaultmembers.length) {
                    sigMembers[n4] = this.defaultmembers[n4];
                    ++n4;
                }
                midx = this.defaultmembers.length;
                String[] stringArray3 = parameters;
                n = parameters.length;
                int n5 = 0;
                while (n5 < n) {
                    String p3 = stringArray3[n5];
                    String label = p3.substring(1);
                    type = p3.charAt(0);
                    sigMembers[midx] = type == 'b' ? new StructMember(label, 3, null, 1) : (type == 'd' || type == 'u' || type == 'D' ? new StructMember(label, 3, null, 5) : (type == 'p' ? new StructMember(label, 3, null, 3) : (type == 't' || type == 'I' || type == 'e' ? new StructMember(label, 7, null, -1) : (type == 'c' ? new StructMember(label, 7, null, -1) : (type == 's' ? new StructMember(label, 1, null, -1) : (type == 'x' ? new StructMember(label, 6, null, -1) : new StructMember(label, 1, null, -1)))))));
                    ++midx;
                    ++n5;
                }
                this.eventmembers.put(signature, sigMembers);
            }
            int n6 = midx = this.hasState ? 2 : 1;
            if (parameters.length > 0) {
                String merge = "";
                stringArray = parameters;
                int n7 = parameters.length;
                n = 0;
                while (n < n7) {
                    p = stringArray[n];
                    type = p.charAt(0);
                    if (type != 'c') {
                        String label = p.substring(1);
                        merge = String.valueOf(merge) + (merge.isEmpty() ? "" : ", ") + label + "=%" + midx + "%";
                    }
                    ++midx;
                    ++n;
                }
                if (!merge.isEmpty()) {
                    event = String.valueOf(event) + " (" + merge + ")";
                }
            }
            sigMembers = this.eventmembers.get(signature);
            sigMembers[0].setValue(this.state);
            sigMembers[1].setValue(event);
            midx = this.defaultmembers.length;
            int idx = 0;
            stringArray = parameters;
            int n8 = parameters.length;
            n = 0;
            while (n < n8) {
                p = stringArray[n];
                type = p.charAt(0);
                Object value = null;
                if (values != null && values.length > idx) {
                    value = values[idx];
                }
                sigMembers[midx].setValue(null);
                if (type == 'b' || type == 'd' || type == 'u' || type == 'D' || type == 'p') {
                    if (value instanceof Integer || value instanceof Long) {
                        sigMembers[midx].setValue(value);
                    }
                } else if (type == 't' || type == 'I' || type == 'e' || type == 'c') {
                    if (type == 't' && value instanceof Integer) {
                        TaskContext task = (TaskContext)SvDatParser.this.tasks.get(value);
                        if (task != null) {
                            sigMembers[midx].setValue(String.valueOf(task.name) + "<" + value + ">");
                        } else {
                            sigMembers[midx].setValue("<" + value + ">");
                        }
                    } else if (type == 'I' && value instanceof Integer) {
                        String resource = (String)SvDatParser.this.resources.get(value);
                        if (resource != null) {
                            sigMembers[midx].setValue(String.valueOf(resource) + "<" + value + ">");
                        } else {
                            sigMembers[midx].setValue(value);
                        }
                    } else if ((type == 'e' || type == 'c') && (value instanceof String || value instanceof Integer)) {
                        sigMembers[midx].setValue(value);
                    }
                } else if (type == 's') {
                    if (value instanceof String) {
                        sigMembers[midx].setValue(value);
                    }
                } else if (type == 'x' && value instanceof byte[]) {
                    sigMembers[midx].setValue(value);
                }
                ++idx;
                ++midx;
                ++n;
            }
            IStructSamplesWriter structWriter = (IStructSamplesWriter)SvDatParser.this.generator.getWriter(this.signal);
            structWriter.write(SvDatParser.this.current, tag, sigMembers);
            SvDatParser.this.changed = SvDatParser.this.changed < 3 ? 3 : SvDatParser.this.changed;
        }

        private void writeAssoc(Context context, String event) {
            IStructSamplesWriter structWriter = (IStructSamplesWriter)SvDatParser.this.generator.getWriter(this.signal);
            structWriter.attachRelation(context.getDestination(), "/666666/line/none", 0L);
            SvDatParser.this.changed = SvDatParser.this.changed < 3 ? 3 : SvDatParser.this.changed;
        }

        final void enter(String event) {
            this.enter("Running", event);
        }

        final void enter(String state, String event) {
            this.state = state;
            this.write(event);
            if ("Running".equals(state)) {
                this.enterRunning();
            }
        }

        final void interruptTo(Context context, String event) {
            this.interruptedState = this.state;
            this.state = null;
            this.write(event);
            this.writeAssoc(context, event);
            this.leaveRunning();
        }

        final void resume(String event) {
            this.state = this.interruptedState;
            this.interruptedState = null;
            this.write(event);
            if ("Running".equals(this.state)) {
                this.enterRunning();
            }
        }

        final void leaveTo(Context context, String event) {
            if ("Running".equals(this.state)) {
                this.leaveRunning();
            }
            this.state = null;
            this.write(event);
            if (context != null) {
                this.writeAssoc(context, event);
            }
        }

        void enterRunning() {
            if (!this.entered) {
                this.enteredAt = SvDatParser.this.current;
                ++this.calls;
                this.entered = true;
            }
        }

        void leaveRunning() {
            if (this.entered) {
                long duration = SvDatParser.this.current - this.enteredAt;
                this.total += duration;
                if (this.durationMax < duration) {
                    this.durationMax = duration;
                }
                if (this.durationMin == 0L || this.durationMin > duration) {
                    this.durationMin = duration;
                }
                this.entered = false;
            }
        }
    }

    class IdleContext
    extends Context {
        public IdleContext(String name) {
            super(name, true);
        }

        void goIdle(Object ... p) {
            if (SvDatParser.this.currentContext != null && SvDatParser.this.currentContext != this) {
                SvDatParser.this.currentContext.leaveTo(this, "Go Idle");
            }
            SvDatParser.this.currentContext = this;
            this.enter("Go Idle");
        }
    }

    class IsrContext
    extends Context {
        public IsrContext(String name) {
            super(name, true);
        }

        void startIsr() {
            if (SvDatParser.this.currentIsrs.isEmpty() && SvDatParser.this.currentContext != null) {
                SvDatParser.this.currentContext.interruptTo(this, "Interrupted by Isr");
            } else if (!SvDatParser.this.currentIsrs.isEmpty()) {
                ((IsrContext)SvDatParser.this.currentIsrs.peek()).interruptTo(this, "Interrupted by inner Isr");
            }
            SvDatParser.this.currentIsrs.push(this);
            this.enter("Start Isr");
        }

        void stopIsr() {
            if (SvDatParser.this.currentIsrs.peek() == this) {
                SvDatParser.this.currentIsrs.pop();
            }
            if (SvDatParser.this.currentContext != null & SvDatParser.this.currentIsrs.isEmpty()) {
                this.leaveTo(SvDatParser.this.currentContext, "Isr Finished");
                SvDatParser.this.currentContext.resume("Isr Finished");
            } else if (!SvDatParser.this.currentIsrs.isEmpty()) {
                this.leaveTo((Context)SvDatParser.this.currentIsrs.peek(), "Isr Finished");
                ((IsrContext)SvDatParser.this.currentIsrs.peek()).resume("Inner Isr Finished");
            }
        }

        void stopIsrToSched() {
            SvDatParser.this.currentIsrs.clear();
            this.leaveTo(SvDatParser.this.schedContext, "Finish Isr to Scheduler");
            SvDatParser.this.currentContext = SvDatParser.this.schedContext;
            SvDatParser.this.schedContext.enter("Finish Isr to Scheduler");
        }
    }

    class SchedContext
    extends Context {
        public SchedContext(String name) {
            super(name, true);
        }
    }

    class TaskContext
    extends Context {
        public TaskContext(String name) {
            super(name, true);
        }

        public void taskCreate() {
        }

        void taskStartReady() {
            this.enter("Ready", "Start Ready");
        }

        void taskStartExec() {
            if (SvDatParser.this.currentContext != null && SvDatParser.this.currentContext != this) {
                SvDatParser.this.currentContext.leaveTo(this, "Start Exec");
            }
            SvDatParser.this.currentContext = this;
            this.enter("Start Exec");
        }

        void taskStopReady() {
            if (SvDatParser.this.currentContext == this) {
                this.leaveTo(SvDatParser.this.schedContext, "Stop Ready");
                SvDatParser.this.currentContext = SvDatParser.this.schedContext;
                SvDatParser.this.schedContext.enter("Stop Ready");
            } else {
                this.leaveTo(null, "Stop Ready");
            }
        }

        void taskStopExec() {
        }
    }

    class TimerContext
    extends Context {
        Context parent;

        public TimerContext(String name) {
            super(name, true);
        }

        public void timerEnter() {
            if (SvDatParser.this.getActiveContext() != this) {
                this.parent = SvDatParser.this.getActiveContext();
                this.parent.leaveTo(this, "Start Timer");
                this.parent.innerContext = this;
                this.enter("Start Timer");
            }
        }

        public void timerExit() {
            this.leaveTo(this.parent, "Exit Timer");
            this.parent.innerContext = null;
            this.parent.enter("Exit Timer");
        }
    }

    class UserContext
    extends Context {
        Context parent;

        public UserContext(String name) {
            super(name, true);
        }

        public void userEnter() {
            if (SvDatParser.this.getActiveContext() != this) {
                this.parent = SvDatParser.this.getActiveContext();
                this.parent.leaveTo(this, "Start User");
                this.getActiveContext().innerContext = this;
                this.enter("Start User");
            }
        }

        public void userExit() {
            this.leaveTo(this.parent, "Exit User");
            this.parent.innerContext = null;
            this.parent.enter("Exit User");
        }
    }
}

