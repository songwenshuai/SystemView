/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.svdat;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.extension.embedded.svdat.ISvDat;
import de.toem.impulse.extension.embedded.svdat.SvDatConfiguration;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISingleDomainRecordGenerator;
import de.toem.impulse.samples.IStructSamplesWriter;
import de.toem.impulse.serializer.BinaryParseBuffer;
import de.toem.impulse.values.StructMember;
import de.toem.toolkits.core.Assert;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.text.MultilineText;
import java.io.BufferedReader;
import java.io.StringReader;
import java.util.HashMap;
import java.util.Map;

public final class SvDatParser2
implements ISvDat {
    public static final String[] SYSVIEW_EVENT_LABELS = new String[32];
    private final SvDatConfiguration configuration;
    private final ISingleDomainRecordGenerator generator;
    private final TimeBase timeBase;
    private int version = -1;
    private boolean removeHeader = true;
    private long timeStampFreq = 0L;
    private long timeStamp = 0L;
    private int changed;
    private long current = 0L;
    private Map<String, String> sysDescription = new HashMap<String, String>();
    private Map<Integer, String> resources = new HashMap<Integer, String>();
    private Map<Integer, String[]> userEvents = new HashMap<Integer, String[]>();
    Signal signal;
    IStructSamplesWriter writer;
    final StructMember eventMember = new StructMember("Event", 8, "EVENT", -1);
    final Map<String, StructMember> members = new HashMap<String, StructMember>();
    final StructMember taskIdMember = this.assertMember("ttaskId");
    final StructMember isrIdMember = this.assertMember("tisrId");
    Map<Integer, Event> events = new HashMap<Integer, Event>();

    static {
        SvDatParser2.SYSVIEW_EVENT_LABELS[0] = "NOP";
        SvDatParser2.SYSVIEW_EVENT_LABELS[1] = "OVERFLOW";
        SvDatParser2.SYSVIEW_EVENT_LABELS[2] = "ISR_ENTER";
        SvDatParser2.SYSVIEW_EVENT_LABELS[3] = "ISR_EXIT";
        SvDatParser2.SYSVIEW_EVENT_LABELS[4] = "TASK_START_EXEC";
        SvDatParser2.SYSVIEW_EVENT_LABELS[5] = "TASK_STOP_EXEC";
        SvDatParser2.SYSVIEW_EVENT_LABELS[6] = "TASK_START_READY";
        SvDatParser2.SYSVIEW_EVENT_LABELS[7] = "TASK_STOP_READY";
        SvDatParser2.SYSVIEW_EVENT_LABELS[8] = "TASK_CREATE";
        SvDatParser2.SYSVIEW_EVENT_LABELS[9] = "TASK_INFO";
        SvDatParser2.SYSVIEW_EVENT_LABELS[10] = "TRACE_START";
        SvDatParser2.SYSVIEW_EVENT_LABELS[11] = "TRACE_STOP";
        SvDatParser2.SYSVIEW_EVENT_LABELS[12] = "SYSTIME_CYCLES";
        SvDatParser2.SYSVIEW_EVENT_LABELS[13] = "SYSTIME_US";
        SvDatParser2.SYSVIEW_EVENT_LABELS[14] = "SYSDESC";
        SvDatParser2.SYSVIEW_EVENT_LABELS[15] = "USER_START";
        SvDatParser2.SYSVIEW_EVENT_LABELS[16] = "USER_STOP";
        SvDatParser2.SYSVIEW_EVENT_LABELS[17] = "IDLE";
        SvDatParser2.SYSVIEW_EVENT_LABELS[18] = "ISR_TO_SCHEDULER";
        SvDatParser2.SYSVIEW_EVENT_LABELS[19] = "TIMER_ENTER";
        SvDatParser2.SYSVIEW_EVENT_LABELS[20] = "TIMER_EXIT";
        SvDatParser2.SYSVIEW_EVENT_LABELS[21] = "STACK_INFO";
        SvDatParser2.SYSVIEW_EVENT_LABELS[22] = "MODULEDESC";
        SvDatParser2.SYSVIEW_EVENT_LABELS[24] = "INIT";
        SvDatParser2.SYSVIEW_EVENT_LABELS[25] = "NAME_RESOURCE";
        SvDatParser2.SYSVIEW_EVENT_LABELS[26] = "PRINT_FORMATTED";
        SvDatParser2.SYSVIEW_EVENT_LABELS[27] = "NUMMODULES";
        SvDatParser2.SYSVIEW_EVENT_LABELS[28] = "END_CALL";
        SvDatParser2.SYSVIEW_EVENT_LABELS[29] = "TASK_TERMINATE";
        SvDatParser2.SYSVIEW_EVENT_LABELS[30] = "UNKNOWN";
        SvDatParser2.SYSVIEW_EVENT_LABELS[31] = "EX";
    }

    public SvDatParser2(ISingleDomainRecordGenerator generator, SvDatConfiguration configuration) {
        this.generator = generator;
        this.configuration = configuration;
        this.timeBase = (TimeBase)DomainBase.parse(configuration.domainBase);
        generator.initRecord("SystemViewer", this.timeBase);
        this.signal = generator.addSignal(null, "0", null, ISamples.ProcessType.Discrete, ISamples.SignalType.Struct, ISamples.SignalDescriptor.DEFAULT, this.timeBase);
        this.writer = (IStructSamplesWriter)generator.getWriter(this.signal);
        generator.open(0L);
        this.eventMember.assignLegend(this.writer.getLegend());
        this.taskIdMember.assignLegend(this.writer.getLegend());
        this.isrIdMember.assignLegend(this.writer.getLegend());
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
                this.write(0, "udt", t);
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
                this.write(1, "udt,upackets", t, packets);
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
                this.write(2, "udt,uisrId", t, isrId);
                break;
            }
            case 3: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.write(3, "udt", t);
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
                this.write(4, "udt,ttaskId", t, taskId);
                break;
            }
            case 5: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.write(5, "udt", t);
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
                this.write(6, "udt,ttaskId", t, taskId);
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
                this.write(7, "udt,ttaskId,ucause", t, taskId, cause);
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
                this.write(8, "udt,ttaskId", t, taskId);
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
                this.write(9, "udt,ttaskId,uprio,sname", t, taskId, prio, name);
                this.writer.setEnum(8 + this.taskIdMember.getId(), name, taskId);
                break;
            }
            case 10: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.timeStamp = 0L;
                this.write(10, "udt", t);
                break;
            }
            case 11: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.write(11, "udt", t);
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
                this.write(12, "udt,ucycles", t, cycles);
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
                this.write(13, "udt,utime", t, time);
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
                this.write(14, "udt,sdescription", t, description);
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
                this.write(15, "udt,uuserId", t, userId);
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
                this.write(16, "udt,uuserId", t, userId);
                break;
            }
            case 17: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.write(17, "udt", t);
                break;
            }
            case 18: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.write(18, "udt", t);
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
                this.write(19, "udt,utimerId", t, timerId);
                break;
            }
            case 20: {
                t = b.parsePlus();
                if (!b.isOk()) {
                    return;
                }
                this.adjustCurrent(t);
                this.write(20, "udt", t);
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
                this.write(21, "udt,ustackId,ustackBase,ustackSize", t, stackId, stackBase, stackSize);
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
                this.write(22, "udt,umoduleId,uevents,sdescription", t, moduleId, events, description);
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
                this.write(24, "udt,uf0,uf1,ustart,ushift", t, f0, f1, start, shift);
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
                this.write(25, "udt,uresourceId,sname", t, resourceId, name);
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
                this.write(26, "udt,smessage", t, message);
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
                this.write(27, "udt,unumModules", t, numModules);
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
                this.write(28, "udt,ueventId,upara0", t, eventId, para0);
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
                this.write(29, "udt,ttaskId", t, taskId);
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
                this.write(30, "udt", t);
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
                this.write(27, "udt,unumModules", t, numModules);
                break;
            }
            default: {
                if (id >= 32 && id <= 2047) {
                    l = b.parsePlus();
                    if (!b.isOk()) {
                        return;
                    }
                    int pos = b.pos;
                    b.peekBytes((int)l);
                    if (!b.isOk()) {
                        return;
                    }
                    b.pos = pos;
                    b.skipBytes((int)l);
                    t = b.parsePlus();
                    if (!b.isOk()) {
                        return;
                    }
                    this.adjustCurrent(t);
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
                String key = def.substring(0, idx);
                String value = def.substring(idx + 1);
                this.sysDescription.put(key, value);
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

    public void finalizeParse() {
        this.generator.close(this.current + 1L);
    }

    void write(int eventId, String signature, Object ... values) {
        int n;
        int n2;
        Object[] objectArray;
        int n3;
        Event event = null;
        if (!this.events.containsKey(eventId)) {
            event = new Event();
            event.signature = signature;
            event.parameters = signature.split(",");
            event.members = new StructMember[event.parameters.length + 1];
            event.members[0] = this.eventMember;
            n3 = 1;
            objectArray = event.parameters;
            n2 = event.parameters.length;
            n = 0;
            while (n < n2) {
                Object p = objectArray[n];
                event.members[n3++] = this.assertMember((String)p);
                ++n;
            }
            this.events.put(eventId, event);
            if (eventId == 1) {
                event.tag = true;
            }
            String eventLabel = null;
            if (eventId >= 0 && eventId < SYSVIEW_EVENT_LABELS.length) {
                eventLabel = SYSVIEW_EVENT_LABELS[eventId];
            }
            if (eventLabel != null) {
                this.writer.setEnum(8, String.valueOf(eventLabel) + " (%%)", eventId);
            }
        } else {
            event = this.events.get(eventId);
        }
        Assert.isTrue(event != null && Utils.equals(event.signature, signature) && event.members.length == values.length + 1);
        event.members[0].setValue(eventId);
        n3 = 1;
        objectArray = values;
        n2 = values.length;
        n = 0;
        while (n < n2) {
            Object value = objectArray[n];
            event.members[n3].setValue(value);
            ++n3;
            ++n;
        }
        this.writer.write(this.current, event.tag, event.members);
        this.changed = this.changed < 3 ? 3 : this.changed;
    }

    private StructMember assertMember(String parameter) {
        String label = parameter.substring(1);
        char type = parameter.charAt(0);
        if (!this.members.containsKey(label)) {
            StructMember member = null;
            member = "dt".equals(label) ? new StructMember(label, 3, null, 5) : (type == 'b' ? new StructMember(label, 131, null, 1) : (type == 'd' || type == 'u' || type == 'D' ? new StructMember(label, 131, null, 5) : (type == 'p' ? new StructMember(label, 131, null, 3) : (type == 't' || type == 'I' || type == 'e' ? new StructMember(label, 135, null, -1) : (type == 'c' ? new StructMember(label, 7, null, -1) : (type == 's' ? new StructMember(label, 129, null, -1) : (type == 'x' ? new StructMember(label, 134, null, -1) : new StructMember(label, 129, null, -1))))))));
            if (member != null) {
                this.members.put(label, member);
                if (this.writer != null) {
                    member.assignLegend(this.writer.getLegend());
                }
            }
            return member;
        }
        return this.members.get(label);
    }

    class Event {
        boolean tag = false;
        StructMember[] members;
        String signature;
        String[] parameters;

        Event() {
        }
    }
}

