/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer.vcd;

import de.toem.impulse.ImpulseLicense;
import de.toem.impulse.cells.ports.IPortProgress;
import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.record.SignalProxy;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IEventSamplesWriter;
import de.toem.impulse.samples.IFloatSamplesWriter;
import de.toem.impulse.samples.ILogicSamplesWriter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.ITextSamplesWriter;
import de.toem.impulse.samples.writer.SamplesWriter;
import de.toem.impulse.serializer.AbstractRecordReader;
import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.impulse.serializer.vcd.VcdVariable;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Cover;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.serializer.Message;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.pageable.PageTable;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class VcdReader
extends AbstractRecordReader {
    static byte[][] commands = new byte[Command.values().length][];
    private static Pattern PATTERN_TIMESCALE;
    private static Pattern PATTERN_VAR;
    static final int TOKEN_COMMAND = 16;
    static final int TOKEN_TIME = 32;
    static final int TOKEN_VECTOR_CHANGE = 48;
    static final int TOKEN_REAL_CHANGE = 64;
    static final int TOKEN_WS = 80;
    static final int TOKEN_CHANGE2 = 96;
    static final int TOKEN_CHANGE4 = 112;
    static final int TOKEN_CHANGE16 = 128;
    static final int TOKEN_STRING_CHANGE = 144;
    static final int TOKEN_NONE = 240;
    static int[] token;
    IProgress progress;
    DumpState state = DumpState.Normal;
    TimeBase timeBase = TimeBase.ns;
    long current = 0L;
    long offset = 0L;
    boolean hierarchyResolution;
    private static final int MAX_STATES = 65536;
    private byte[] statesBuffer = new byte[65536];
    Record record;
    ICell base;
    ICell scope;
    ICover cover;
    Map<String, VcdVariable<String>> ids = new HashMap<String, VcdVariable<String>>();
    Map<ICell, List<VcdVariable<String>>> vars = new HashMap<ICell, List<VcdVariable<String>>>();
    Map<String, ISamplesWriter> samplesById = new HashMap<String, ISamplesWriter>();
    ISamplesWriter[] samplesIndex;
    boolean initialized;
    int samplesIndexBase;
    long started;
    int changed;

    static {
        int n = 0;
        while (n < Command.values().length) {
            VcdReader.commands[n] = Command.values()[n].toString().toLowerCase().getBytes();
            ++n;
        }
        PATTERN_TIMESCALE = Pattern.compile("\\s*(1|10|100)\\s*(fs|ps|ns|us|ms|s)\\s*");
        PATTERN_VAR = Pattern.compile("\\s*(\\w+)\\s+(\\d+)\\s+([!-~]+)\\s+(.*)");
        token = new int[256];
        int i = 0;
        while (i < 256) {
            VcdReader.token[i] = 240;
            ++i;
        }
        VcdReader.token[36] = 16;
        VcdReader.token[35] = 32;
        VcdReader.token[98] = 48;
        VcdReader.token[66] = 48;
        VcdReader.token[114] = 64;
        VcdReader.token[82] = 64;
        VcdReader.token[115] = 144;
        VcdReader.token[83] = 144;
        VcdReader.token[32] = 80;
        VcdReader.token[10] = 80;
        VcdReader.token[9] = 80;
        VcdReader.token[13] = 80;
        VcdReader.token[48] = 96;
        VcdReader.token[49] = 97;
        VcdReader.token[90] = 114;
        VcdReader.token[122] = 114;
        VcdReader.token[88] = 115;
        VcdReader.token[120] = 115;
        VcdReader.token[76] = 132;
        VcdReader.token[108] = 132;
        VcdReader.token[72] = 133;
        VcdReader.token[104] = 133;
        VcdReader.token[85] = 134;
        VcdReader.token[117] = 134;
        VcdReader.token[87] = 135;
        VcdReader.token[119] = 135;
        VcdReader.token[45] = 136;
    }

    public VcdReader(String id, InputStream in) {
        super(id, in);
    }

    public VcdReader() {
    }

    public static IPropertyModel getPropertyModel() {
        return new PropertyModel().add("systemCHierarchy", "true", "SystemC Hierarchy Resolver", null, null);
    }

    public static IPropertyModel getPropertyModel(Class sz) {
        return VcdReader.getPropertyModel();
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        return 32;
    }

    @Override
    protected int isApplicable(byte[] buffer) {
        if (new String(buffer).trim().startsWith("$")) {
            return 1;
        }
        return -1;
    }

    /*
     * Unable to fully structure code
     */
    @Override
    public ICover read(IProgress progress, String config, ICell base, int insert) {
        block14: {
            block13: {
                this.progress = progress;
                this.started = System.currentTimeMillis();
                this.record = new Record();
                this.cover = new Cover(this.record);
                this.cover.setSerializer(this.id);
                if (insert == 0) {
                    this.base = this.record;
                } else if (insert == 2) {
                    this.base = base;
                } else if (insert == 2) {
                    return null;
                }
                this.hierarchyResolution = this.properties != null && Boolean.TRUE.equals(this.properties.get("systemCHierarchy")) != false;
                this.scope = this.base;
                if (ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.serializer", "de.toem.impulse.feature.default", this.id)) {
                    base.insertChild(new Message(I18n.Serializer_LockedSerializer, 3, I18n.Serializer_TheSelectedSerializerLocked), 0);
                    return this.cover;
                }
                try {
                    try {
                        PageTable.setBoost(true);
                        this.parse(progress);
                        break block13;
                    }
                    catch (Throwable e) {
                        AbstractSingleDomainRecordReader.addParseErrorMessage(this.id, e, this.base);
                        PageTable.setBoost(false);
                        ** for (cell : this.base.getTribe((boolean)true))
                    }
                }
                catch (Throwable var6_12) {
                    PageTable.setBoost(false);
                    ** for (cell : this.base.getTribe((boolean)true))
                }
lbl-1000:
                // 1 sources

                {
                    cell.setData("SERIALIZER", null);
                    cell.setData("WRITER", null);
                    continue;
lbl31:
                    // 1 sources

                    break block14;
                }
lbl-1000:
                // 1 sources

                {
                    cell.setData("SERIALIZER", null);
                    cell.setData("WRITER", null);
                    continue;
                }
lbl38:
                // 1 sources

                throw var6_12;
            }
            PageTable.setBoost(false);
            for (ICell cell : this.base.getTribe(true)) {
                cell.setData("SERIALIZER", null);
                cell.setData("WRITER", null);
            }
        }
        return this.cover;
    }

    @Override
    public synchronized int hasChanged() {
        return this.changed;
    }

    @Override
    public synchronized ICover flush() {
        this.writeSignals(false);
        this.changed = 0;
        return this.cover;
    }

    @Override
    public boolean supportsStreaming() {
        return true;
    }

    @Override
    public boolean supportsPortIntro() {
        return true;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     * WARNING - Removed back jump from a try to a catch block - possible behaviour change.
     * Enabled aggressive block sorting
     * Enabled unnecessary exception pruning
     * Enabled aggressive exception aggregation
     */
    void parse(IProgress progress) throws ParseException {
        if (this.in == null) {
            return;
        }
        byte[] buffer = new byte[65536];
        int read = 0;
        int readTotal = 0;
        int available = 0;
        int availableTotal = 0;
        int used = 0;
        int wrapped = 0;
        boolean insertedFinalWs = false;
        try {
            availableTotal = this.in.available();
        }
        catch (IOException iOException) {
            try {
            }
            catch (Throwable e) {
                if (!(e instanceof ParseException)) return;
                ((ParseException)e).position += readTotal - wrapped;
                throw (ParseException)e;
            }
            finally {
                this.writeSignals(true);
            }
        }
        {
            while (read != -1 || available > 0) {
                read = this.in.read(buffer, wrapped, buffer.length - wrapped);
                if (read == 0) {
                    this.flush();
                }
                readTotal += read > 0 ? read : 0;
                if (read == -1 && !insertedFinalWs) {
                    read = 1;
                    insertedFinalWs = true;
                    buffer[wrapped] = 32;
                }
                if (progress != null) {
                    progress.done(1.0 * (double)(read > 0 ? read : 0) / (double)availableTotal);
                    if (progress.isCanceled()) {
                        return;
                    }
                }
                if (this.initialized && progress instanceof IPortProgress && !((IPortProgress)progress).isStreaming()) {
                    IProgress iProgress = progress;
                    synchronized (iProgress) {
                        while (!((IPortProgress)progress).isStreaming() && !progress.isCanceled()) {
                            try {
                                progress.wait(250L);
                            }
                            catch (InterruptedException interruptedException) {}
                        }
                    }
                }
                available = wrapped + (read > 0 ? read : 0);
                byte[] parseBuffer = buffer;
                if (available < buffer.length) {
                    parseBuffer = new byte[available];
                    System.arraycopy(buffer, 0, parseBuffer, 0, available);
                }
                if ((used = this.parse(parseBuffer)) > 0) {
                    int n = this.changed = 3 > this.changed ? 3 : this.changed;
                }
                if (read == -1 && available > 0 && used == 0) {
                    throw new ParseException(0, I18n.Serializer_ParseError);
                }
                wrapped = available - used;
                System.arraycopy(buffer, used, buffer, 0, wrapped);
            }
            return;
        }
    }

    private boolean writeSignals(boolean close) {
        boolean changed = false;
        for (ICell cell : this.base.getTribe(false, Signal.class)) {
            if (cell.getData("SERIALIZER") != this) continue;
            Signal signal = (Signal)cell;
            ISamplesWriter writer = (ISamplesWriter)cell.getData("WRITER");
            if (writer == null || !writer.isOpen()) continue;
            if (close) {
                writer.close(this.current + 1L);
            } else {
                writer.flush(this.current);
            }
            changed |= writer.apply(signal);
        }
        return changed;
    }

    /*
     * Unable to fully structure code
     */
    private synchronized int parse(byte[] buffer) throws ParseException {
        n = 0;
        used = 0;
        try {
            while (n < buffer.length) {
                b = buffer[n];
                sel = VcdReader.token[b];
                switch (sel & 4080) {
                    case 80: {
                        break;
                    }
                    case 32: {
                        used = this.parseTime(buffer, n);
                        if (!this.initialized) {
                            this.initialize();
                            return n + used;
                        }
                        ** GOTO lbl53
                    }
                    case 48: {
                        if (!this.initialized) {
                            throw new ParseException(0, I18n.Serializer_NotInitialized);
                        }
                        used = this.parseVectorChange(buffer, n);
                        ** GOTO lbl53
                    }
                    case 96: {
                        if (!this.initialized) {
                            throw new ParseException(0, I18n.Serializer_NotInitialized);
                        }
                        used = this.parseL2Change(buffer, n, (byte)(sel & 15));
                        ** GOTO lbl53
                    }
                    case 112: {
                        if (!this.initialized) {
                            throw new ParseException(0, I18n.Serializer_NotInitialized);
                        }
                        used = this.parseL4Change(buffer, n, (byte)(sel & 15));
                        ** GOTO lbl53
                    }
                    case 128: {
                        if (!this.initialized) {
                            throw new ParseException(0, I18n.Serializer_NotInitialized);
                        }
                        used = this.parseL16Change(buffer, n, (byte)(sel & 15));
                        ** GOTO lbl53
                    }
                    case 64: {
                        if (!this.initialized) {
                            throw new ParseException(0, I18n.Serializer_NotInitialized);
                        }
                        used = this.parseRealChange(buffer, n);
                        ** GOTO lbl53
                    }
                    case 144: {
                        if (!this.initialized) {
                            throw new ParseException(0, I18n.Serializer_NotInitialized);
                        }
                        used = this.parseStringChange(buffer, n);
                        ** GOTO lbl53
                    }
                    case 16: {
                        used = this.parseCommand(buffer, n);
                        if (this.initialized) {
                            return n + used + 1;
                        }
                        ** GOTO lbl53
                    }
                    case 240: {
                        throw new ParseException(0, I18n.Serializer_InvalidCharacter);
                    }
lbl53:
                    // 9 sources

                    default: {
                        if (used == 0) {
                            return n;
                        }
                        n += used;
                    }
                }
                ++n;
            }
            return n;
        }
        catch (Throwable e) {
            if (!(e instanceof ParseException)) {
                SystemLog.log(e);
                e = new ParseException(0, I18n.Serializer_SyntaxError, e);
            }
            ((ParseException)e).position = n - used;
            i = len = used;
            ** while (i < buffer.length)
        }
lbl-1000:
        // 1 sources

        {
            if ((buffer[n - used + i] == 10 || buffer[n - used + i] == 13) && i > 10) {
                len = i;
                break;
            }
            ++i;
            continue;
        }
lbl72:
        // 2 sources

        e.text = new String(buffer, n - used, len).trim();
        throw e;
    }

    private int parseTime(byte[] buffer, int n) throws ParseException {
        long time = 0L;
        int i = n + 1;
        while (i < buffer.length) {
            if (buffer[i] > 57 || buffer[i] < 48) {
                if (time > this.current) {
                    this.current = time + this.offset;
                }
                return i - n;
            }
            time = time * 10L + (long)(buffer[i] - 48);
            ++i;
        }
        return 0;
    }

    private int parseL2Change(byte[] buffer, int n, byte state) throws ParseException {
        int index = 0;
        int i = n + 1;
        while (i < buffer.length) {
            if (buffer[i] > 126 || buffer[i] < 33) {
                ISamplesWriter writer = this.getWriter(index, buffer, n + 1, i - n - 1);
                if (writer instanceof ILogicSamplesWriter) {
                    ILogicSamplesWriter logicWriter;
                    logicWriter.write(this.current, false, 1, (logicWriter = (ILogicSamplesWriter)writer).getBitWidth() > 1 ? (byte)0 : state, state);
                } else if (writer instanceof IEventSamplesWriter) {
                    ((IEventSamplesWriter)writer).write(this.current, false);
                }
                return i - n;
            }
            index = index * 100 + (buffer[i] - 32);
            ++i;
        }
        return 0;
    }

    private int parseL4Change(byte[] buffer, int n, byte state) throws ParseException {
        int index = 0;
        int i = n + 1;
        while (i < buffer.length) {
            if (buffer[i] > 126 || buffer[i] < 33) {
                ISamplesWriter writer = this.getWriter(index, buffer, n + 1, i - n - 1);
                if (writer instanceof ILogicSamplesWriter) {
                    ILogicSamplesWriter logicWriter = (ILogicSamplesWriter)writer;
                    logicWriter.write(this.current, state == 3, 2, logicWriter.getBitWidth() > 1 ? (byte)0 : state, state);
                } else if (writer instanceof IEventSamplesWriter) {
                    ((IEventSamplesWriter)writer).write(this.current, state == 3);
                }
                return i - n;
            }
            index = index * 100 + (buffer[i] - 32);
            ++i;
        }
        return 0;
    }

    private int parseL16Change(byte[] buffer, int n, byte state) throws ParseException {
        int index = 0;
        int i = n + 1;
        while (i < buffer.length) {
            if (buffer[i] > 126 || buffer[i] < 33) {
                ISamplesWriter writer = this.getWriter(index, buffer, n + 1, i - n - 1);
                if (writer instanceof ILogicSamplesWriter) {
                    ILogicSamplesWriter logicWriter;
                    logicWriter.write(this.current, false, 3, (logicWriter = (ILogicSamplesWriter)writer).getBitWidth() > 1 ? (byte)0 : state, state);
                } else if (writer instanceof IEventSamplesWriter) {
                    ((IEventSamplesWriter)writer).write(this.current, false);
                }
                return i - n;
            }
            index = index * 100 + (buffer[i] - 32);
            ++i;
        }
        return 0;
    }

    private int parseVectorChange(byte[] buffer, int n) throws ParseException {
        byte b;
        int i = n + 1;
        int states = 0;
        int first = 0;
        int level = 1;
        boolean tag = false;
        block6: while (i < buffer.length && states < 65536) {
            int sel = token[buffer[i]];
            switch (sel & 0xF0) {
                case 96: {
                    this.statesBuffer[states++] = (byte)(sel & 0xF);
                    break;
                }
                case 112: {
                    this.statesBuffer[states++] = (byte)(sel & 0xF);
                    if (level < 2) {
                        level = 2;
                    }
                    if ((sel & 0xF) != 3) break;
                    tag = true;
                    break;
                }
                case 128: {
                    this.statesBuffer[states++] = (byte)(sel & 0xF);
                    level = 3;
                    break;
                }
                case 80: {
                    ++i;
                    break block6;
                }
                default: {
                    throw new ParseException(0, I18n.Serializer_InvalidLogicVector);
                }
            }
            ++i;
        }
        while (i < buffer.length) {
            b = buffer[i];
            if (b != 32 && b != 9) break;
            ++i;
        }
        int index = 0;
        int m = i;
        while (i < buffer.length) {
            b = buffer[i];
            if (b > 126 || b < 33) {
                ISamplesWriter writer = this.getWriter(index, buffer, m, i - m);
                if (writer instanceof ILogicSamplesWriter) {
                    ILogicSamplesWriter logicWriter = (ILogicSamplesWriter)writer;
                    if (states > logicWriter.getBitWidth()) {
                        first += states - logicWriter.getBitWidth();
                    }
                    byte preceding = 0;
                    if (states < logicWriter.getBitWidth() && this.statesBuffer[first] == 1) {
                        preceding = 0;
                    } else {
                        preceding = this.statesBuffer[first];
                        ++first;
                    }
                    while (first < states) {
                        if (this.statesBuffer[first] != preceding) break;
                        ++first;
                    }
                    if (states - first == 0) {
                        logicWriter.write(this.current, tag, level, preceding);
                    } else {
                        logicWriter.write(this.current, tag, level, preceding, this.statesBuffer, first, states - first);
                    }
                } else if (writer instanceof IEventSamplesWriter) {
                    ((IEventSamplesWriter)writer).write(this.current, tag);
                }
                return i - n;
            }
            index = index * 100 + (buffer[i] - 32);
            ++i;
        }
        return 0;
    }

    private int parseRealChange(byte[] buffer, int n) throws ParseException {
        byte b;
        int i = n + 1;
        double value = 0.0;
        boolean tag = false;
        while (i < buffer.length) {
            b = buffer[i];
            if (b == 32 || b == 9) {
                try {
                    value = Double.parseDouble(new String(buffer, n + 1, i - n));
                }
                catch (Throwable throwable) {}
                break;
            }
            ++i;
        }
        while (i < buffer.length) {
            b = buffer[i];
            if (b != 32 && b != 9) break;
            ++i;
        }
        int index = 0;
        int m = i;
        while (i < buffer.length) {
            b = buffer[i];
            if (b > 126 || b < 33) {
                ISamplesWriter writer = this.getWriter(index, buffer, m, i - m);
                if (writer instanceof IFloatSamplesWriter) {
                    ((IFloatSamplesWriter)writer).write(this.current, tag, value);
                }
                return i - n;
            }
            index = index * 100 + (buffer[i] - 32);
            ++i;
        }
        return 0;
    }

    private int parseStringChange(byte[] buffer, int n) throws ParseException {
        byte b;
        int i = n + 1;
        String value = "";
        boolean tag = false;
        while (i < buffer.length) {
            b = buffer[i];
            if (b == 32 || b == 9) {
                try {
                    value = new String(buffer, n + 1, i - n);
                }
                catch (Throwable throwable) {}
                break;
            }
            ++i;
        }
        while (i < buffer.length) {
            b = buffer[i];
            if (b != 32 && b != 9) break;
            ++i;
        }
        int index = 0;
        int m = i;
        while (i < buffer.length) {
            b = buffer[i];
            if (b > 126 || b < 33) {
                ISamplesWriter writer = this.getWriter(index, buffer, m, i - m);
                if (writer instanceof ITextSamplesWriter) {
                    ((ITextSamplesWriter)writer).write(this.current, tag, value);
                }
                return i - n;
            }
            index = index * 100 + (buffer[i] - 32);
            ++i;
        }
        return 0;
    }

    private int parseCommand(byte[] buffer, int n) throws ParseException {
        boolean more = false;
        Command[] commandArray = Command.values();
        int n2 = commandArray.length;
        int n3 = 0;
        while (n3 < n2) {
            byte[] bytes;
            Command command = commandArray[n3];
            int i = n;
            boolean skip = false;
            byte[] byArray = bytes = commands[command.ordinal()];
            int n4 = bytes.length;
            int n5 = 0;
            while (n5 < n4) {
                byte b = byArray[n5];
                if (++i >= buffer.length) {
                    more = true;
                    skip = true;
                    break;
                }
                if (b != buffer[i]) {
                    skip = true;
                    break;
                }
                ++n5;
            }
            if (!skip) {
                ++i;
                int used = 0;
                switch (command) {
                    case VAR: {
                        String[] parameters = new String[6];
                        used = this.parseVarParameters(buffer, i, parameters, PATTERN_VAR);
                        if (used == 0) {
                            return 0;
                        }
                        if (parameters[0] == null || parameters[1] == null || parameters[2] == null || parameters[3] == null || parameters[3].isEmpty()) {
                            throw new ParseException(0, I18n.Serializer_InvalidParameterCount);
                        }
                        VcdVariable var = new VcdVariable();
                        var.name = parameters[3];
                        var.name = var.name.replaceAll("\\s+\\[", "[");
                        var.handle = parameters[2];
                        var.signalType = "event".equals(parameters[0]) ? ISamples.SignalType.Event : ("real".equals(parameters[0]) ? ISamples.SignalType.Float : ("string".equals(parameters[0]) ? ISamples.SignalType.Text : ISamples.SignalType.Logic));
                        var.description = parameters[0];
                        var.scale = Utils.parseInt(parameters[1], 1);
                        var.idx0 = Utils.parseInt(parameters[4], -1);
                        var.idx1 = Utils.parseInt(parameters[5], -1);
                        if (var.idx1 > var.idx0) {
                            int swap = var.idx0;
                            var.idx0 = var.idx1;
                            var.idx1 = swap;
                        }
                        var.scope = this.scope;
                        if (this.ids.containsKey(var.handle)) {
                            var.sharedHandle = true;
                            this.ids.get(var.handle).sharedHandle = true;
                        }
                        if (var.signalType == ISamples.SignalType.Float && var.idx0 >= 0) {
                            throw new ParseException(0, I18n.Serializer_RealTypeHasVector);
                        }
                        if (var.signalType == ISamples.SignalType.Text && var.idx0 >= 0) {
                            throw new ParseException(0, I18n.Serializer_StringTypeHasVector);
                        }
                        if (this.ids.containsKey(var.handle)) {
                            if (var.scale != this.ids.get(var.handle).scale) {
                                throw new ParseException(0, I18n.Serializer_SharedIdsDifferentScale);
                            }
                        } else {
                            this.ids.put((String)var.handle, var);
                        }
                        if (!this.vars.containsKey(this.scope)) {
                            this.vars.put(this.scope, new ArrayList());
                        }
                        if (!this.vars.get(this.scope).contains(var)) {
                            this.vars.get(this.scope).add(var);
                        }
                        this.logCommand(command, parameters);
                        break;
                    }
                    case ENDDEFINITIONS: {
                        String[] parameters = new String[1];
                        used = this.parseParameterBlock(buffer, i, parameters);
                        if (used == 0) {
                            return 0;
                        }
                        this.logCommand(command, parameters);
                    }
                    case END: {
                        break;
                    }
                    case SCOPE: {
                        String[] parameters;
                        if (this.scope != this.base) {
                            this.hierarchyResolution = false;
                        }
                        if ((used = this.parseParameters(buffer, i, parameters = new String[2])) == 0) {
                            return 0;
                        }
                        if (parameters[0] == null || parameters[1] == null) {
                            throw new ParseException(0, I18n.Serializer_InvalidParameterCount);
                        }
                        ICell scope = this.scope.getChildByName(parameters[1]);
                        if (!(scope instanceof Scope)) {
                            scope = new Scope();
                            scope.setName(parameters[1]);
                            ((Scope)scope).domainType = parameters[0];
                            this.scope.addChild(scope);
                        }
                        this.scope = scope;
                        this.logCommand(command, parameters);
                        break;
                    }
                    case UPSCOPE: {
                        String[] parameters = new String[1];
                        used = this.parseParameterBlock(buffer, i, parameters);
                        if (used == 0) {
                            return 0;
                        }
                        if (this.scope.getParent() != null) {
                            this.scope = this.scope.getParent();
                        }
                        this.logCommand(command, parameters);
                        break;
                    }
                    case COMMENT: {
                        String[] parameters = new String[1];
                        used = this.parseParameterBlock(buffer, i, parameters);
                        if (used == 0) {
                            return 0;
                        }
                        this.logCommand(command, parameters);
                        break;
                    }
                    case DATE: {
                        String[] parameters = new String[1];
                        used = this.parseParameterBlock(buffer, i, parameters);
                        if (used == 0) {
                            return 0;
                        }
                        this.logCommand(command, parameters);
                        break;
                    }
                    case DUMPALL: {
                        break;
                    }
                    case DUMPOFF: {
                        break;
                    }
                    case DUMPON: {
                        break;
                    }
                    case DUMPVARS: {
                        if (this.initialized) break;
                        this.initialize();
                        break;
                    }
                    case VERSION: {
                        String[] parameters = new String[1];
                        used = this.parseParameterBlock(buffer, i, parameters);
                        if (used == 0) {
                            return 0;
                        }
                        this.hierarchyResolution &= parameters[0].contains("SystemC");
                        this.logCommand(command, parameters);
                        break;
                    }
                    case TIMESCALE: {
                        String[] parameters = new String[2];
                        used = this.parseParameters(buffer, i, parameters, PATTERN_TIMESCALE);
                        if (used == 0) {
                            return 0;
                        }
                        if (parameters[0] == null || parameters[1] == null) {
                            throw new ParseException(0, I18n.Serializer_InvalidParameterCount);
                        }
                        int factor = Utils.parseInt(parameters[0], -1);
                        if (factor == 10) {
                            parameters[1] = String.valueOf(parameters[1]) + "10";
                        } else if (factor == 100) {
                            parameters[1] = String.valueOf(parameters[1]) + "100";
                        }
                        this.timeBase = TimeBase.parse(parameters[1]);
                        if (this.timeBase == null) {
                            throw new ParseException(0, I18n.Serializer_InvalidParameterValues);
                        }
                        if (!this.samplesById.isEmpty()) {
                            for (ISamplesWriter writer : this.samplesById.values()) {
                                writer.setDomainBase(this.timeBase);
                            }
                        }
                        this.logCommand(command, parameters);
                        break;
                    }
                    case TIMEZERO: {
                        String[] parameters = new String[1];
                        used = this.parseParameterBlock(buffer, i, parameters);
                        if (used == 0) {
                            return 0;
                        }
                        this.current = this.offset = Utils.parseLong(parameters[0], 0L);
                        this.logCommand(command, parameters);
                    }
                }
                return i - n + used;
            }
            ++n3;
        }
        if (!more) {
            throw new ParseException(0, I18n.Serializer_NoCommandFound);
        }
        return 0;
    }

    private int parseParameters(byte[] buffer, int n, String[] parameters) throws ParseException {
        int i = n;
        while (i < buffer.length) {
            if (buffer[i] == 36 && buffer.length > i + 3 && buffer[i + 1] == 101 && buffer[i + 2] == 110 && buffer[i + 3] == 100) {
                String string = new String(buffer, n, i - n);
                if (parameters != null) {
                    String[] splitted = string.trim().split("\\s+");
                    int j = 0;
                    while (j < splitted.length && j < parameters.length) {
                        parameters[j] = splitted[j];
                        ++j;
                    }
                }
                return i + 3 - n;
            }
            ++i;
        }
        return 0;
    }

    private int parseParameters(byte[] buffer, int n, String[] parameters, Pattern pattern) throws ParseException {
        int i = n;
        while (i < buffer.length) {
            if (buffer[i] == 36 && buffer.length > i + 3 && buffer[i + 1] == 101 && buffer[i + 2] == 110 && buffer[i + 3] == 100) {
                Matcher m;
                String string = new String(buffer, n, i - n);
                if (parameters != null && (m = pattern.matcher(string)).find()) {
                    int j = 0;
                    while (j < m.groupCount() && j < parameters.length) {
                        parameters[j] = m.group(j + 1);
                        ++j;
                    }
                }
                return i + 3 - n;
            }
            ++i;
        }
        return 0;
    }

    private int parseVarParameters(byte[] buffer, int n, String[] parameters, Pattern pattern) throws ParseException {
        int i = n;
        while (i < buffer.length) {
            if (buffer[i] == 36 && buffer.length > i + 3 && buffer[i + 1] == 101 && buffer[i + 2] == 110 && buffer[i + 3] == 100) {
                String string;
                Matcher m;
                if (parameters != null && parameters.length == 6 && (m = pattern.matcher(string = new String(buffer, n, i - n))).find()) {
                    parameters[0] = m.group(1);
                    parameters[1] = m.group(2);
                    parameters[2] = m.group(3);
                    String rem = m.group(4).trim();
                    int vec0Idx = rem.lastIndexOf(91);
                    if (vec0Idx > 0) {
                        parameters[3] = rem.substring(0, vec0Idx).trim();
                        int dimIdx = rem.indexOf(58, vec0Idx);
                        int vec1Idx = rem.indexOf(93, vec0Idx);
                        if (vec1Idx > 0) {
                            if (dimIdx > 0) {
                                parameters[4] = rem.substring(vec0Idx + 1, dimIdx).trim();
                                parameters[5] = rem.substring(dimIdx + 1, vec1Idx).trim();
                            } else {
                                parameters[4] = rem.substring(vec0Idx + 1, vec1Idx).trim();
                            }
                        }
                    } else {
                        parameters[3] = rem;
                    }
                }
                return i + 3 - n;
            }
            ++i;
        }
        return 0;
    }

    private int parseParameterBlock(byte[] buffer, int n, String[] parameters) throws ParseException {
        int i = n;
        while (i < buffer.length) {
            if (buffer[i] == 36 && buffer.length > i + 3 && buffer[i + 1] == 101 && buffer[i + 2] == 110 && buffer[i + 3] == 100) {
                String string = new String(buffer, n, i - n);
                if (parameters != null && parameters.length == 1) {
                    parameters[0] = string.trim();
                }
                return i + 3 - n;
            }
            ++i;
        }
        return 0;
    }

    private void logCommand(Command command, String[] parameters) {
    }

    private void initialize() {
        VcdVariable.identifyGroups(this.vars);
        VcdVariable.createSignals(this.vars, this.record, this.timeBase, null);
        this.samplesById = VcdVariable.createWriters(this.vars, this.timeBase, this);
        if (this.samplesById.isEmpty()) {
            return;
        }
        int max = 0;
        int min = Integer.MAX_VALUE;
        for (String id : this.samplesById.keySet()) {
            if (this.index(id) > max) {
                max = this.index(id);
            }
            if (this.index(id) >= min) continue;
            min = this.index(id);
        }
        try {
            long count = (long)max + 1L - (long)min;
            if (count > 0L && count < 0x1000000L) {
                this.samplesIndex = new ISamplesWriter[(int)count];
                this.samplesIndexBase = min;
                SamplesWriter.adjustBufferGeometry();
                for (String string : this.samplesById.keySet()) {
                    this.samplesIndex[this.index((String)string) - this.samplesIndexBase] = this.samplesById.get(string);
                }
            }
        }
        catch (Throwable throwable) {}
        boolean open = true;
        for (String id : this.samplesById.keySet()) {
            if (!open) break;
            open = this.samplesById.get(id).open(this.current);
        }
        if (this.hierarchyResolution) {
            try {
                Scope hierarchy = new Scope();
                hierarchy.setName("Hierarchy");
                for (ICell iCell : this.base.getTribe(false, AbstractSignal.class)) {
                    if (iCell.getData("SERIALIZER") != this) continue;
                    Signal signal = null;
                    if (iCell instanceof Signal) {
                        signal = (Signal)iCell;
                    } else if (iCell instanceof SignalProxy) {
                        signal = (Signal)this.base.getRoot().getCellByLink(((SignalProxy)iCell).signal);
                    }
                    String name = signal.getName();
                    String description = signal.description;
                    String[] splitted = name.split("\\.");
                    Scope scope = hierarchy;
                    int n = 0;
                    while (n < splitted.length - 1) {
                        if (scope.getChildByName(splitted[n]) != null) {
                            scope = (Scope)scope.getChildByName(splitted[n]);
                        } else {
                            Scope next = new Scope();
                            next.setName(splitted[n]);
                            scope.addChild(next);
                            scope = next;
                        }
                        ++n;
                    }
                    SignalProxy proxy = new SignalProxy();
                    proxy.setName(splitted[splitted.length - 1]);
                    proxy.description = description;
                    proxy.signal = signal.getLink(this.base.getRoot());
                    scope.addChild(proxy);
                }
                this.base.insertChild(hierarchy, 0);
            }
            catch (Throwable throwable) {}
        }
        this.changed = 4;
        this.initialized = true;
    }

    private ISamplesWriter getWriter(int index, byte[] buffer, int pos, int length) throws ParseException {
        if (this.samplesIndex != null) {
            return this.samplesIndex[index - this.samplesIndexBase];
        }
        String key = new String(buffer, pos, length);
        ISamplesWriter writer = this.samplesById.get(key);
        if (writer == null) {
            throw new ParseException(0, I18n.Serializer_WriterNotFound);
        }
        return writer;
    }

    private int index(String id) {
        int index = 0;
        byte[] byArray = id.getBytes();
        int n = byArray.length;
        int n2 = 0;
        while (n2 < n) {
            byte b = byArray[n2];
            index = index * 100 + (b - 32);
            ++n2;
        }
        return index;
    }

    @Override
    public String getConfigurationName() {
        return null;
    }

    static enum Command {
        VAR,
        ENDDEFINITIONS,
        END,
        SCOPE,
        UPSCOPE,
        COMMENT,
        DATE,
        DUMPALL,
        DUMPOFF,
        DUMPON,
        DUMPVARS,
        VERSION,
        TIMESCALE,
        TIMEZERO;

    }

    static enum DumpState {
        Normal,
        DumpVars,
        DumpOn,
        DumpOff,
        DumpAll;

    }
}

