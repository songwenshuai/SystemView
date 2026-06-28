/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.svdat;

public interface ISvDat {
    public static final int SYSVIEW_EVENT_ID_NOP = 0;
    public static final int SYSVIEW_EVENT_ID_OVERFLOW = 1;
    public static final int SYSVIEW_EVENT_ID_ISR_ENTER = 2;
    public static final int SYSVIEW_EVENT_ID_ISR_EXIT = 3;
    public static final int SYSVIEW_EVENT_ID_TASK_START_EXEC = 4;
    public static final int SYSVIEW_EVENT_ID_TASK_STOP_EXEC = 5;
    public static final int SYSVIEW_EVENT_ID_TASK_START_READY = 6;
    public static final int SYSVIEW_EVENT_ID_TASK_STOP_READY = 7;
    public static final int SYSVIEW_EVENT_ID_TASK_CREATE = 8;
    public static final int SYSVIEW_EVENT_ID_TASK_INFO = 9;
    public static final int SYSVIEW_EVENT_ID_TRACE_START = 10;
    public static final int SYSVIEW_EVENT_ID_TRACE_STOP = 11;
    public static final int SYSVIEW_EVENT_ID_SYSTIME_CYCLES = 12;
    public static final int SYSVIEW_EVENT_ID_SYSTIME_US = 13;
    public static final int SYSVIEW_EVENT_ID_SYSDESC = 14;
    public static final int SYSVIEW_EVENT_ID_USER_START = 15;
    public static final int SYSVIEW_EVENT_ID_USER_STOP = 16;
    public static final int SYSVIEW_EVENT_ID_IDLE = 17;
    public static final int SYSVIEW_EVENT_ID_ISR_TO_SCHEDULER = 18;
    public static final int SYSVIEW_EVENT_ID_TIMER_ENTER = 19;
    public static final int SYSVIEW_EVENT_ID_TIMER_EXIT = 20;
    public static final int SYSVIEW_EVENT_ID_STACK_INFO = 21;
    public static final int SYSVIEW_EVENT_ID_MODULEDESC = 22;
    public static final int SYSVIEW_EVENT_ID_INIT = 24;
    public static final int SYSVIEW_EVENT_ID_NAME_RESOURCE = 25;
    public static final int SYSVIEW_EVENT_ID_PRINT_FORMATTED = 26;
    public static final int SYSVIEW_EVENT_ID_NUMMODULES = 27;
    public static final int SYSVIEW_EVENT_ID_END_CALL = 28;
    public static final int SYSVIEW_EVENT_ID_TASK_TERMINATE = 29;
    public static final int SYSVIEW_EVENT_ID_UNKNOWN = 30;
    public static final int SYSVIEW_EVENT_ID_EX = 31;
    public static final String SYSVIEW_EVENT_NOP = "NOP";
    public static final String SYSVIEW_EVENT_OVERFLOW = "OVERFLOW";
    public static final String SYSVIEW_EVENT_ISR_ENTER = "ISR_ENTER";
    public static final String SYSVIEW_EVENT_ISR_EXIT = "ISR_EXIT";
    public static final String SYSVIEW_EVENT_TASK_START_EXEC = "TASK_START_EXEC";
    public static final String SYSVIEW_EVENT_TASK_STOP_EXEC = "TASK_STOP_EXEC";
    public static final String SYSVIEW_EVENT_TASK_START_READY = "TASK_START_READY";
    public static final String SYSVIEW_EVENT_TASK_STOP_READY = "TASK_STOP_READY";
    public static final String SYSVIEW_EVENT_TASK_CREATE = "TASK_CREATE";
    public static final String SYSVIEW_EVENT_TASK_INFO = "TASK_INFO";
    public static final String SYSVIEW_EVENT_TRACE_START = "TRACE_START";
    public static final String SYSVIEW_EVENT_TRACE_STOP = "TRACE_STOP";
    public static final String SYSVIEW_EVENT_SYSTIME_CYCLES = "SYSTIME_CYCLES";
    public static final String SYSVIEW_EVENT_SYSTIME_US = "SYSTIME_US";
    public static final String SYSVIEW_EVENT_SYSDESC = "SYSDESC";
    public static final String SYSVIEW_EVENT_USER_START = "USER_START";
    public static final String SYSVIEW_EVENT_USER_STOP = "USER_STOP";
    public static final String SYSVIEW_EVENT_IDLE = "IDLE";
    public static final String SYSVIEW_EVENT_ISR_TO_SCHEDULER = "ISR_TO_SCHEDULER";
    public static final String SYSVIEW_EVENT_TIMER_ENTER = "TIMER_ENTER";
    public static final String SYSVIEW_EVENT_TIMER_EXIT = "TIMER_EXIT";
    public static final String SYSVIEW_EVENT_STACK_INFO = "STACK_INFO";
    public static final String SYSVIEW_EVENT_MODULEDESC = "MODULEDESC";
    public static final String SYSVIEW_EVENT_INIT = "INIT";
    public static final String SYSVIEW_EVENT_NAME_RESOURCE = "NAME_RESOURCE";
    public static final String SYSVIEW_EVENT_PRINT_FORMATTED = "PRINT_FORMATTED";
    public static final String SYSVIEW_EVENT_NUMMODULES = "NUMMODULES";
    public static final String SYSVIEW_EVENT_END_CALL = "END_CALL";
    public static final String SYSVIEW_EVENT_TASK_TERMINATE = "TASK_TERMINATE";
    public static final String SYSVIEW_EVENT_UNKNOWN = "UNKNOWN";
    public static final String SYSVIEW_EVENT_EX = "EX";
    public static final String SYSVIEW_EVENT_MEMBER_TASKID = "taskId";
    public static final String SYSVIEW_EVENT_MEMBER_ISRID = "isrId";
    public static final String SYSVIEW_EVENT_MEMBER_PRIO = "prio";
    public static final String SYSVIEW_EVENT_MEMBER_NAME = "name";
}

