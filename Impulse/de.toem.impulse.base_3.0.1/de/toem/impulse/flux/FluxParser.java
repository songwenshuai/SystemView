/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.flux;

import com.occultusterra.compression.FastLZ;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.flux.IFluxHandler;
import de.toem.impulse.flux.IFluxInitialisation;
import de.toem.impulse.samples.IRecordGenerator;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.writer.IConvergingSamplesWriter;
import de.toem.impulse.samples.writer.IDivergingSamplesWriter;
import de.toem.impulse.serializer.BinaryParseBuffer;
import de.toem.impulse.values.StructMember;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.pageable.Pageable;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.ByteArrayInputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Vector;
import java.util.zip.GZIPInputStream;
import java.util.zip.Inflater;
import kanzi.IndexedByteArray;
import kanzi.function.LZ4Codec;

public class FluxParser {
    static final int VERSION = 6;
    static final int MAX_TRACE = 2048;
    static final int MAX_ITEMS = 0x2000000;
    static final int MAX_ENTRYSIZE = 0x200000;
    static final String DEFINITION = "DEFINITION";
    static final int ENTRY_HEAD = 1;
    static final int MODE_HEAD_NORMAL = 0;
    static final int MODE_HEAD_SYNC = 1;
    static final int ENTRY_SWTH = 4;
    static final int ENTRY_PBLK = 5;
    static final int ENTRY_PBLK_MODE_LZ4 = 0;
    static final int ENTRY_PBLK_MODE_FLZ = 1;
    static final int ENTRY_PBLK_MODE_ZLIB = 2;
    static final int ENTRY_PBLK_MODE_GZIP = 3;
    static final int ENTRY_SECT = 6;
    static final int ENTRY_SCPD = 16;
    static final int ENTRY_SIGD = 17;
    static final int ENTRY_MSGD = 18;
    static final int ENTRY_SIRD = 19;
    static final int ENTRY_SSGD = 20;
    static final int ENTRY_SSRD = 21;
    static final int ENTRY_OPEN = 32;
    static final int ENTRY_CLOS = 33;
    static final int ENTRY_DOMD = 34;
    static final int ENTRY_CURR = 35;
    static final int ENTRY_ENMD = 48;
    static final int ENTRY_MEMD = 49;
    static final int ENTRY_ATRE = 64;
    static final int ENTRY_ATLA = 65;
    static final int ENTRY_CREQ = 128;
    static final int ENTRY_CRES = 129;
    private static final String HEAD = "flux";
    private static final Trace.Content.ScatteredDefinition[] UNGROUPED = new Trace.Content.ScatteredDefinition[0];
    private final Vector<Trace> traces;
    private final IRecordGenerator generator;
    private int version = -1;
    private Trace currentTrace;
    private final IFluxHandler handler;
    private final boolean handleData;

    public FluxParser(IRecordGenerator generator) {
        this(generator, null);
    }

    public FluxParser(IRecordGenerator generator, IFluxHandler handler) {
        this.generator = generator;
        this.traces = new Vector(2048);
        this.traces.setSize(2048);
        this.handler = handler;
        this.handleData = handler != null ? handler.requiresData() : false;
    }

    public Trace getCurrentTrace() {
        return this.currentTrace;
    }

    public Vector<Trace> getAllTraces() {
        return this.traces;
    }

    public boolean synchronize(BinaryParseBuffer b) {
        boolean fristRound = true;
        do {
            if (!fristRound) {
                b.skipBytes(1);
                if (!b.isOk()) {
                    return false;
                }
            }
            fristRound = false;
        } while (b.peekByte() != 0 || b.peekByte(1) != 1 || b.peekByte(2) != HEAD.charAt(0) || b.peekByte(3) != HEAD.charAt(1) || b.peekByte(4) != HEAD.charAt(2) || b.peekByte(5) != HEAD.charAt(3));
        return true;
    }

    public boolean isFinished() {
        return this.handler != null ? this.handler.isFinished() : false;
    }

    public void flush(Signal signal) {
        OpenState state;
        Trace.Content.SignalDefinition definition;
        if ((this.handler == null || this.handler.handleFlush(this.currentTrace, signal)) && (definition = (Trace.Content.SignalDefinition)signal.getData(DEFINITION, Trace.Content.SignalDefinition.class)) != null && definition.writer != null && definition.getContent() != null && definition.writer.isOpen() && (state = definition.getContent().state) != null) {
            definition.writer.flush(state.current);
            definition.writer.apply(signal);
        }
    }

    /*
     * Unable to fully structure code
     */
    public int parseEntry(IProgress p, BinaryParseBuffer b, int changed) {
        block239: {
            block238: {
                first = (int)b.parsePlus();
                if (!b.isOk()) {
                    return changed;
                }
                if (first != 0) break block238;
                entry = b.getByte() & 255;
                if (!b.isOk()) {
                    return changed;
                }
                switch (entry) {
                    case 1: {
                        head = b.parseString("flux".length());
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (!"flux".equals(head)) {
                            b.setError("Head:Invalid head bytes");
                            return changed;
                        }
                        version = b.getByte();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (this.version == -1) {
                            this.version = version;
                        } else if (version != this.version) {
                            b.setError("Head:Multiple versions");
                            return changed;
                        }
                        if (this.version > 6) {
                            b.setError("Unsupported versions");
                            return changed;
                        }
                        id = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (id < 0 || id > 2048) {
                            b.setError("Head:Invalid trace id");
                            return changed;
                        }
                        name = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        description = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        mode = b.getByte();
                        if (!b.isOk()) {
                            return changed;
                        }
                        maxItemId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (maxItemId < 0 || maxItemId > 0x2000000) {
                            b.setError("Head:Invalid maxItemId ");
                            return changed;
                        }
                        maxEntrySize = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (maxEntrySize < 0 || maxEntrySize > 0x200000) {
                            b.setError("Head:Invalid maxEntrySize ");
                            return changed;
                        }
                        if (this.traces.get(id) == null) {
                            b.resize(maxEntrySize);
                            this.currentTrace = new Trace(name, description, mode, maxItemId, maxEntrySize, this.generator.getBase());
                            this.traces.set(id, this.currentTrace);
                            if (this.handler != null) {
                                this.handler.handleCreated(p, this.currentTrace, b);
                                break;
                            }
                        } else {
                            this.currentTrace = this.traces.get(id);
                            if (mode == 1) {
                                this.traces.get(id).setIgnoreRedefinitions(true);
                                break;
                            }
                        }
                        break block239;
                    }
                    case 4: {
                        id = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (id < 0 || id > 2048) {
                            b.setError("Swth:Invalid trace id");
                            return changed;
                        }
                        if (this.traces.get(id) == null) {
                            b.setError("Swth:No trace available");
                            return changed;
                        }
                        this.currentTrace = this.traces.get(id);
                        break;
                    }
                    case 5: {
                        if (this.currentTrace == null) {
                            b.setError("Pblk:No trace available");
                            return changed;
                        }
                        mode = b.getByte();
                        if (!b.isOk()) {
                            return changed;
                        }
                        oSize = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (oSize < 0 || oSize > this.currentTrace.maxEntrySize) {
                            b.setError("Pblk:No pack original size");
                            return changed;
                        }
                        pSize = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (oSize < 0 || oSize > this.currentTrace.maxEntrySize) {
                            b.setError("Pblk:No pack packed size");
                            return changed;
                        }
                        compressed = b.getBytes(pSize);
                        decompressed = new byte[oSize];
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (mode != 0) ** GOTO lbl109
                        lz4 = new LZ4Codec();
                        if (!lz4.inverse(new IndexedByteArray(compressed, 0), new IndexedByteArray(decompressed, 0))) {
                            b.setError("Pblk:LZ4 decompression failed");
                        }
                        ** GOTO lbl145
lbl109:
                        // 1 sources

                        if (mode != 1) ** GOTO lbl117
                        try {
                            FastLZ.decompress(compressed, decompressed);
                        }
                        catch (Exception v0) {
                            b.setError("Pblk:FastLZ decompression failed");
                        }
                        ** GOTO lbl145
lbl117:
                        // 1 sources

                        if (mode != 2) ** GOTO lbl130
                        try {
                            decompresser = new Inflater();
                            decompresser.setInput(compressed);
                            result = decompresser.inflate(decompressed);
                            decompresser.end();
                            if (result != oSize) {
                                b.setError("Pblk:GZip decompression failed");
                            }
                            ** GOTO lbl145
                        }
                        catch (Exception v1) {
                            b.setError("Pblk:ZLib decompression failed");
                        }
                        ** GOTO lbl145
lbl130:
                        // 1 sources

                        if (mode == 3) {
                            try {
                                bin = new ByteArrayInputStream(compressed);
                                gzipper = new GZIPInputStream(bin);
                                result = 0;
                                chunk = 0;
                                while ((chunk = gzipper.read(decompressed, result, decompressed.length - result)) > 0) {
                                    result += chunk;
                                }
                                if (result != oSize) {
                                    b.setError("Pblk:GZip decompression failed");
                                }
                                gzipper.close();
                            }
                            catch (Exception v2) {
                                b.setError("Pblk:GZip decompression failed");
                            }
                        }
lbl145:
                        // 10 sources

                        b.insertBytes(decompressed);
                        break;
                    }
                    case 6: {
                        if (this.currentTrace == null) {
                            b.setError("Pblk:No trace available");
                            return changed;
                        }
                        counter = b.getByte();
                        if (!b.isOk()) {
                            return changed;
                        }
                        size = b.getWord();
                        if (!b.isOk()) {
                            return changed;
                        }
                        used = b.getWord();
                        if (!b.isOk()) {
                            return changed;
                        }
                        bytes = b.getBytes(used);
                        if (!b.isOk()) {
                            return changed;
                        }
                        b.skipBytes(size - used);
                        if (!b.isOk()) {
                            return changed;
                        }
                        this.currentTrace.addSection(counter, bytes);
                        if ((counter & 128) != 0) {
                            ordered = this.currentTrace.reorderSections();
                            b.insertBytes(ordered);
                            break;
                        }
                        break block239;
                    }
                    case 16: {
                        itemId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (itemId <= 0 || itemId > this.currentTrace.maxItemId) {
                            b.setError("Scpd:Invalid itemId");
                            return changed;
                        }
                        parentId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (parentId < 0 || parentId > this.currentTrace.maxItemId) {
                            b.setError("Scpd:Invalid parentId");
                            return changed;
                        }
                        name = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        description = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Scpd:No trace available");
                            return changed;
                        }
                        if (this.currentTrace.ignoreDefinitions) {
                            return changed;
                        }
                        if (!this.currentTrace.contents[parentId = this.currentTrace.getSharedId(parentId)].isScope()) {
                            b.setError("Scpd:No valid scope");
                            return changed;
                        }
                        if (this.currentTrace.contents[itemId] != null) {
                            b.setError("Scpd:Id allready used");
                            return changed;
                        }
                        targetId = this.currentTrace.findScope(parentId, name, description);
                        if (targetId < 0) {
                            content = this.currentTrace.createContent(itemId);
                            content.setScope(parentId, name, description);
                            break;
                        }
                        this.currentTrace.setSharedId(itemId, targetId);
                        break;
                    }
                    case 17: {
                        itemId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (itemId <= 0 || itemId > this.currentTrace.maxItemId) {
                            b.setError("Sigd:Invalid itemId");
                            return changed;
                        }
                        parentId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (parentId < 0 || parentId > this.currentTrace.maxItemId) {
                            b.setError("Sigd:Invalid parentId");
                            return changed;
                        }
                        name = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        description = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        type = b.getByte();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (type < 0 || type >= ISamples.SignalType.values().length) {
                            b.setError("Sigd:Invalid signal type");
                            return changed;
                        }
                        descriptor = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Sigd:No trace available");
                            return changed;
                        }
                        if (this.currentTrace.ignoreDefinitions) {
                            return changed;
                        }
                        if (!this.currentTrace.contents[parentId = this.currentTrace.getSharedId(parentId)].isScope()) {
                            b.setError("Sigd:No valid scope");
                            return changed;
                        }
                        if (this.currentTrace.contents[itemId] != null) {
                            b.setError("Sigd:Id allready used");
                            return changed;
                        }
                        content = this.currentTrace.createContent(itemId);
                        sType = ISamples.SignalType.values()[type];
                        sDescriptor = ISamples.SignalDescriptor.parseUser(sType, descriptor);
                        content.setSignal(parentId, name, description, sType, sDescriptor);
                        break;
                    }
                    case 18: {
                        itemIdFrom = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (itemIdFrom <= 0 || itemIdFrom > this.currentTrace.maxItemId) {
                            b.setError("Msgd:Invalid itemIdFrom");
                            return changed;
                        }
                        itemIdTo = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (itemIdTo <= 0 || itemIdTo > this.currentTrace.maxItemId || itemIdTo < itemIdFrom) {
                            b.setError("Msgd:Invalid itemIdTo");
                            return changed;
                        }
                        parentId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (parentId < 0 || parentId > this.currentTrace.maxItemId) {
                            b.setError("Msgd:Invalid parentId");
                            return changed;
                        }
                        name = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        description = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        type = b.getByte();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (type < 0 || type >= ISamples.SignalType.values().length) {
                            b.setError("Msgd:Invalid signal type");
                            return changed;
                        }
                        descriptor = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Msgd:No trace available");
                            return changed;
                        }
                        if (this.currentTrace.ignoreDefinitions) {
                            return changed;
                        }
                        if (!this.currentTrace.contents[parentId = this.currentTrace.getSharedId(parentId)].isScope()) {
                            b.setError("Msgd:No valid scope");
                            return changed;
                        }
                        sType = ISamples.SignalType.values()[type];
                        sDescriptor = ISamples.SignalDescriptor.parseUser(sType, descriptor);
                        itemId = itemIdFrom;
                        while (itemId <= itemIdTo) {
                            if (this.currentTrace.contents[itemId] != null) {
                                b.setError("Msgd:Id allready used");
                                return changed;
                            }
                            content = this.currentTrace.createContent(itemId);
                            content.setSignal(parentId, name, description, sType, sDescriptor);
                            ++itemId;
                        }
                        break block239;
                    }
                    case 20: {
                        itemId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (itemId <= 0 || itemId > this.currentTrace.maxItemId) {
                            b.setError("Ssgd:Invalid itemIdFrom");
                            return changed;
                        }
                        parentId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (parentId < 0 || parentId > this.currentTrace.maxItemId) {
                            b.setError("Ssgd:Invalid parentId");
                            return changed;
                        }
                        name = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        description = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        type = b.getByte();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (type < 0 || type >= ISamples.SignalType.values().length) {
                            b.setError("Ssgd:Invalid signal type");
                            return changed;
                        }
                        descriptor = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        from = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        to = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (to < from) {
                            b.setError("Ssgd:Invalid range");
                            return changed;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Ssgd:No trace available");
                            return changed;
                        }
                        if (this.currentTrace.ignoreDefinitions) {
                            return changed;
                        }
                        if (!this.currentTrace.contents[parentId = this.currentTrace.getSharedId(parentId)].isScope()) {
                            b.setError("Ssgd:Id valid scope");
                            return changed;
                        }
                        if (this.currentTrace.contents[itemId] != null) {
                            b.setError("Ssgd:Id allready used");
                            return changed;
                        }
                        content = this.currentTrace.createContent(itemId);
                        sType = ISamples.SignalType.values()[type];
                        sDescriptor = ISamples.SignalDescriptor.parseUser(sType, descriptor);
                        content.setSignal(parentId, name, description, sType, sDescriptor, from, to);
                        break;
                    }
                    case 19: {
                        referenceId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (referenceId <= 0 || referenceId > this.currentTrace.maxItemId) {
                            b.setError("Sird:Invalid itemIdFrom");
                            return changed;
                        }
                        parentId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (parentId < 0 || parentId > this.currentTrace.maxItemId) {
                            b.setError("Sird:Invalid parentId");
                            return changed;
                        }
                        name = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        description = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Sird:No trace available");
                            return changed;
                        }
                        if (this.currentTrace.ignoreDefinitions) {
                            return changed;
                        }
                        if (!this.currentTrace.contents[parentId = this.currentTrace.getSharedId(parentId)].isScope()) {
                            b.setError("Sird:No valid scope");
                            return changed;
                        }
                        refContent = this.currentTrace.contents[referenceId];
                        if (refContent == null || !refContent.isSignal()) {
                            b.setError("Sird:Reference id not defined");
                            return changed;
                        }
                        refContent.addReference(parentId, name, description);
                        break;
                    }
                    case 21: {
                        referenceId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (referenceId <= 0 || referenceId > this.currentTrace.maxItemId) {
                            b.setError("Ssrd:Invalid itemIdFrom");
                            return changed;
                        }
                        parentId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (parentId < 0 || parentId > this.currentTrace.maxItemId) {
                            b.setError("Ssrd:Invalid parentId");
                            return changed;
                        }
                        name = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        description = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        from = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        to = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (to < from) {
                            b.setError("Ssrd:Invalid range");
                            return changed;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Ssrd:No trace available");
                            return changed;
                        }
                        if (this.currentTrace.ignoreDefinitions) {
                            return changed;
                        }
                        if (!this.currentTrace.contents[parentId = this.currentTrace.getSharedId(parentId)].isScope()) {
                            b.setError("Ssrd:No valid scope");
                            return changed;
                        }
                        refContent = this.currentTrace.contents[referenceId];
                        if (refContent == null || !refContent.isSignal()) {
                            b.setError("Ssrd:Reference id not defined");
                            return changed;
                        }
                        refContent.addReference(parentId, name, description, from, to);
                        break;
                    }
                    case 32: {
                        itemId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (itemId < 0 || itemId > this.currentTrace.maxItemId) {
                            b.setError("Open:Invalid itemId");
                            return changed;
                        }
                        domain = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        start = b.parseLong();
                        if (!b.isOk()) {
                            return changed;
                        }
                        rate = b.parseLong();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Open:No trace available");
                            return changed;
                        }
                        if (this.currentTrace.ignoreDefinitions) {
                            return changed;
                        }
                        if (this.handler == null || this.handler.handleOpen(p, this.currentTrace, itemId, domain, start, rate, b)) {
                            content = this.currentTrace.contents[itemId];
                            if (content == null) {
                                b.setError("Open:No valid scope");
                                return changed;
                            }
                            this.currentTrace.open(itemId, domain, start, rate, b);
                            if (this.handler != null) {
                                this.handler.handleOpened(p, this.currentTrace, itemId, b);
                            }
                            changed = changed < 4 ? 4 : changed;
                            break;
                        }
                        break block239;
                    }
                    case 33: {
                        itemId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (itemId < 0 || itemId > this.currentTrace.maxItemId) {
                            b.setError("Close:Invalid itemId");
                            return changed;
                        }
                        end = b.parseLong();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Close:No trace available");
                            return changed;
                        }
                        if (this.handler == null || this.handler.handleClose(p, this.currentTrace, itemId, end, b)) {
                            content = this.currentTrace.contents[itemId];
                            if (content == null) {
                                b.setError("Close:No valid scope");
                                return changed;
                            }
                            this.currentTrace.close(itemId, end, b);
                            if (this.handler != null) {
                                this.handler.handleClosed(p, this.currentTrace, itemId, b);
                            }
                            changed = changed < 4 ? 4 : changed;
                            break;
                        }
                        break block239;
                    }
                    case 34: {
                        domain = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Open:No trace available");
                            return changed;
                        }
                        base = DomainBase.parse(domain);
                        if (base == DomainBase.Unknown) {
                            b.setError("Domd:Invalid domain base");
                            return changed;
                        }
                        this.currentTrace.defaultDomainBase = base;
                        break;
                    }
                    case 35: {
                        itemId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (itemId < 0 || itemId > this.currentTrace.maxItemId) {
                            b.setError("Curr:Invalid itemId");
                            return changed;
                        }
                        current = b.parseLong();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Curr:No trace available");
                            return changed;
                        }
                        content = this.currentTrace.contents[itemId];
                        if (content == null) {
                            b.setError("Curr:Invalid content");
                            return changed;
                        }
                        state = this.currentTrace.getOpenState(itemId);
                        if (state == null) {
                            b.setError("Curr:Not opened");
                            return changed;
                        }
                        if (current > state.current) {
                            state.current = current;
                            break;
                        }
                        break block239;
                    }
                    case 48: {
                        itemId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (itemId <= 0 || itemId > this.currentTrace.maxItemId) {
                            b.setError("Enmd:Invalid itemId");
                            return changed;
                        }
                        enumeration = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        label = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        value = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Enmd:No trace available");
                            return changed;
                        }
                        content = this.currentTrace.contents[itemId];
                        if (content == null) {
                            b.setError("Enmd:No content available");
                            return changed;
                        }
                        writer = content.writer;
                        if (writer == null || !writer.isOpen()) {
                            b.setError("Enmd:No open writer available");
                            return changed;
                        }
                        writer.setEnum(enumeration, label, value);
                        break;
                    }
                    case 49: {
                        itemId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (itemId <= 0 || itemId > this.currentTrace.maxItemId) {
                            b.setError("Memd:Invalid itemId");
                            return changed;
                        }
                        memberId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        parentId = -1;
                        if (this.version >= 5) {
                            parentId = (int)b.parsePlus();
                            parentId = parentId > 0 ? --parentId : -1;
                            if (!b.isOk()) {
                                return changed;
                            }
                        }
                        label = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        type = b.getByte();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (type < 0) {
                            b.setError("Memd:Invalid struct type");
                            return changed;
                        }
                        desciptor = b.parseString();
                        if (!b.isOk()) {
                            return changed;
                        }
                        sDescriptor = ISamples.SignalDescriptor.parseUser(ISamples.SignalType.Unknown, desciptor);
                        if (this.currentTrace == null) {
                            b.setError("Memd:No trace available");
                            return changed;
                        }
                        content = this.currentTrace.contents[itemId];
                        if (content == null) {
                            b.setError("Memd:No content available");
                            return changed;
                        }
                        writer = content.writer;
                        if (writer == null || !writer.isOpen()) {
                            b.setError("Memd:No open writer available");
                            return changed;
                        }
                        writer.setMember(memberId, parentId, label, type, sDescriptor.getContent(), sDescriptor.getFormat());
                        break;
                    }
                    case 64: {
                        itemId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (itemId <= 0 || itemId > this.currentTrace.maxItemId) {
                            b.setError("Atre:Invalid itemId");
                            return changed;
                        }
                        type = 0;
                        if (this.version >= 5) {
                            b.getByte();
                            if (!b.isOk()) {
                                return changed;
                            }
                            if (type < 0) {
                                b.setError("Atre:Invalid struct type");
                                return changed;
                            }
                        }
                        target = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        style = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        deltaOrPosition = b.parseLong();
                        if (!b.isOk()) {
                            return changed;
                        }
                        domainBase = -1;
                        if (this.version >= 5) {
                            domainBase = (int)b.parsePlus();
                            if (!b.isOk()) {
                                return changed;
                            }
                        }
                        if (this.currentTrace == null) {
                            b.setError("Atre:No trace available");
                            return changed;
                        }
                        content = this.currentTrace.contents[itemId];
                        if (content == null) {
                            b.setError("Atre:No content available");
                            return changed;
                        }
                        writer = content.writer;
                        if (writer == null || !writer.isOpen()) {
                            b.setError("Atre:No open writer available");
                            return changed;
                        }
                        writer.attachRelation(type, target, style, deltaOrPosition, domainBase);
                        break;
                    }
                    case 65: {
                        itemId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (itemId <= 0 || itemId > this.currentTrace.maxItemId) {
                            b.setError("Atla:Invalid itemId");
                            return changed;
                        }
                        style = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        b.parseLong();
                        if (!b.isOk()) {
                            return changed;
                        }
                        b.parseLong();
                        if (!b.isOk()) {
                            return changed;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Atla:No trace available");
                            return changed;
                        }
                        content = this.currentTrace.contents[itemId];
                        if (content == null) {
                            b.setError("Atla:No content available");
                            return changed;
                        }
                        writer = content.writer;
                        if (writer == null || !writer.isOpen()) {
                            b.setError("Atla:No open writer available");
                            return changed;
                        }
                        writer.attachLabel(style);
                        break;
                    }
                    case 128: 
                    case 129: {
                        controlId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        messageId = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        count = (int)b.parsePlus();
                        if (!b.isOk()) {
                            return changed;
                        }
                        members = new StructMember[count];
                        n = 0;
                        while (n < count) {
                            member = new StructMember(null, b.buffer, b.pos, 0);
                            b.skipBytes(member.getPackLength());
                            if (!b.isOk()) {
                                return changed;
                            }
                            members[n] = member;
                            ++n;
                        }
                        if (this.currentTrace == null) {
                            b.setError("Creq:No trace available");
                            return changed;
                        }
                        if (this.handler != null) {
                            this.handler.handleControl(p, this.currentTrace, entry == 128, controlId, messageId, members, b);
                            break;
                        }
                        break block239;
                    }
                    default: {
                        b.setError("Unknown entry");
                        break;
                    }
                }
                break block239;
            }
            if (this.currentTrace == null) {
                b.setError("Data:No trace available");
                return changed;
            }
            itemId = first >>> 3;
            tag = (first & 1) != 0;
            v3 = hasDelta = (first & 2) != 0;
            if (itemId <= 0 || itemId > this.currentTrace.maxItemId) {
                b.setError("Data:Invalid itemId");
                return changed;
            }
            content = this.currentTrace.contents[itemId];
            if (content == null) {
                b.setError("Data:Invalid content");
                return changed;
            }
            delta = 0L;
            if (hasDelta) {
                delta = b.parsePlus();
                if (!b.isOk()) {
                    return changed;
                }
                if (delta < 0L) {
                    b.setError("Data:Negative delta");
                }
            }
            size = (int)b.parsePlus();
            if (!b.isOk()) {
                return changed;
            }
            df = size & 3;
            xdf = size >>> 2 & 3;
            if (b.available() < (size >>>= 4)) {
                b.setNotEnoughData();
                return changed;
            }
            if (!this.handleData || this.handler.handleData(p, this.currentTrace, itemId, delta, size, b)) {
                state = content.state;
                if (state == null) {
                    b.setError("Data:Not opened");
                    return changed;
                }
                if (delta > 0L) {
                    state.current += delta;
                }
                if ((writer = content.writer) == null || !writer.isOpen()) {
                    b.setError("Data:No open writer available");
                    return changed;
                }
                format0 = (byte)(df << 6 | xdf << 3 | (tag != false ? 1 : 0));
                writer.writeSample(state.current, format0, b.buffer, b.pos, size);
            }
            b.skipBytes(size);
            if (!b.isOk()) {
                return changed;
            }
            changed = changed < 3 ? 3 : changed;
        }
        return changed;
    }

    public static void writeControlEntry(OutputStream out, boolean request, int controlId, int messageId, StructMember[] members) {
        try {
            int dlength = 0;
            int valid = 0;
            if (members != null) {
                StructMember[] structMemberArray = members;
                int n = members.length;
                int n2 = 0;
                while (n2 < n) {
                    StructMember member = structMemberArray[n2];
                    if (member != null && member.isValid()) {
                        member.pack();
                        dlength += member.getPackLength();
                        ++valid;
                    }
                    ++n2;
                }
            }
            byte[] buffer = new byte[dlength += 2 + PackedSamples.plusLength(controlId) + PackedSamples.plusLength(messageId) + PackedSamples.plusLength(valid)];
            int written = 0;
            buffer[written++] = 0;
            buffer[written++] = (byte)(request ? 128 : 129);
            written += PackedSamples.plusWrite(buffer, written, controlId);
            written += PackedSamples.plusWrite(buffer, written, messageId);
            written += PackedSamples.plusWrite(buffer, written, valid);
            if (members != null) {
                StructMember[] structMemberArray = members;
                int n = members.length;
                int n3 = 0;
                while (n3 < n) {
                    StructMember member = structMemberArray[n3];
                    if (member != null && member.isValid()) {
                        byte[] pack = member.getPack();
                        System.arraycopy(pack, 0, buffer, written, pack.length);
                        written += pack.length;
                    }
                    ++n3;
                }
            }
            int transmitted = 0;
            while (transmitted < written) {
                int toBeTransmitted;
                out.write(toBeTransmitted | (transmitted + (toBeTransmitted = Math.min(written - transmitted, 127)) == written ? 128 : 0));
                out.write(buffer, transmitted, toBeTransmitted);
                transmitted += toBeTransmitted;
            }
            out.flush();
        }
        catch (Throwable throwable) {}
    }

    public final class OpenState {
        int itemId;
        List<ISamplesWriter> writer = new ArrayList<ISamplesWriter>();
        public long current;

        public OpenState(int itemId) {
            this.itemId = itemId;
        }
    }

    public final class Trace
    implements IFluxInitialisation {
        final String name;
        final String description;
        final int mode;
        final int maxItemId;
        final int maxEntrySize;
        final Content[] contents;
        final int[] sharedIds;
        final ICell base;
        IDomainBase defaultDomainBase = DomainBase.Unknown;
        boolean applied;
        List<OpenState> openStates = new ArrayList<OpenState>();
        List<Section> sections = new ArrayList<Section>();
        boolean ignoreDefinitions;

        public Trace(String name, String description, int mode, int maxItemId, int maxEntrySize, ICell base) {
            this.name = name;
            this.description = description;
            this.mode = mode;
            this.maxItemId = maxItemId;
            this.maxEntrySize = maxEntrySize;
            this.contents = new Content[maxItemId + 1];
            this.sharedIds = new int[maxItemId + 1];
            this.base = base;
            this.createContent(0);
        }

        public Content createContent(int itemId) {
            Content content;
            this.contents[itemId] = content = new Content(itemId);
            if (itemId == 0) {
                content.setScope(-1, null, null);
            }
            this.applied = false;
            return content;
        }

        public void applyDefinitions() {
            this.group(0);
            this.apply(0, true, false);
            this.apply(0, false, false);
            this.applied = true;
        }

        public void apply(int scopeId, boolean signals, boolean createWriter) {
            int id;
            int[] childIds = ((Content.ScopeDefinition)this.contents[scopeId].definition).childIds;
            int count = ((Content.ScopeDefinition)this.contents[scopeId].definition).count;
            ICell scope = this.contents[scopeId].definition.cell;
            int n = 0;
            while (n < count) {
                id = childIds[n];
                if (this.contents[id].isSignal()) {
                    Content.SignalDefinition signalDefinition = (Content.SignalDefinition)this.contents[id].definition;
                    Content.ScatteredDefinition definition = !signals && signalDefinition != null ? signalDefinition.next : signalDefinition;
                    while (!(signals ? !(definition instanceof Content.SignalDefinition) : !(definition instanceof Content.ReferenceDefinition))) {
                        if (definition.parentId == scopeId) {
                            if (!definition.isScatteredAndGrouped()) {
                                if (definition instanceof Content.SignalDefinition) {
                                    ISamples.SignalDescriptor descriptor = ((Content.SignalDefinition)definition).descriptor;
                                    if (definition.isScattered()) {
                                        descriptor.setScale(definition.to - definition.from + 1);
                                    }
                                    definition.cell = FluxParser.this.generator.addSignal(scope, definition.getName(), definition.description, ISamples.ProcessType.Unknown, ((Content.SignalDefinition)definition).signalType, descriptor, DomainBase.Unknown, createWriter);
                                } else if (!signalDefinition.isScatteredAndGrouped()) {
                                    definition.cell = FluxParser.this.generator.addSignalProxy(scope, definition.getName(), definition.description, (Signal)signalDefinition.cell);
                                } else {
                                    ISamples.SignalDescriptor descriptor = signalDefinition.descriptor;
                                    if (definition.isScattered()) {
                                        descriptor.setScale(definition.to - definition.from + 1);
                                    }
                                    definition.cell = FluxParser.this.generator.addSignal(scope, definition.getName(), definition.description, ISamples.ProcessType.Unknown, signalDefinition.signalType, descriptor, DomainBase.Unknown, createWriter);
                                }
                            } else if (definition.group[0] == definition) {
                                int scale = definition.group[definition.group.length - 1].to + 1;
                                if (definition instanceof Content.SignalDefinition) {
                                    ISamples.SignalDescriptor descriptor = new ISamples.SignalDescriptor(null, scale, 0, -1);
                                    definition.cell = FluxParser.this.generator.addSignal(scope, definition.getName(), definition.description, ISamples.ProcessType.Unknown, ((Content.SignalDefinition)definition).signalType, descriptor, DomainBase.Unknown, createWriter);
                                } else {
                                    boolean useReference = signalDefinition.isScatteredAndGrouped();
                                    if (useReference &= signalDefinition.group.length == definition.group.length) {
                                        int m = 0;
                                        while (m < definition.group.length) {
                                            useReference &= definition.group[m].from == signalDefinition.group[m].from;
                                            useReference &= definition.group[m].getItemId() == signalDefinition.group[m].getItemId();
                                            ++m;
                                        }
                                    }
                                    if (useReference) {
                                        definition.cell = FluxParser.this.generator.addSignalProxy(scope, definition.getName(), definition.description, (Signal)signalDefinition.cell);
                                    } else {
                                        ISamples.SignalDescriptor descriptor = new ISamples.SignalDescriptor(null, scale, 0, -1);
                                        definition.cell = FluxParser.this.generator.addSignal(scope, definition.getName(), definition.description, ISamples.ProcessType.Unknown, signalDefinition.signalType, descriptor, DomainBase.Unknown, createWriter);
                                    }
                                }
                            }
                        }
                        if (definition.cell != null) {
                            definition.cell.setData(FluxParser.DEFINITION, definition);
                        }
                        definition = definition.next;
                    }
                }
                ++n;
            }
            n = 0;
            while (n < count) {
                id = childIds[n];
                if (this.contents[id].isScope()) {
                    this.apply(id, signals, createWriter);
                }
                ++n;
            }
        }

        public void group(int scopeId) {
            int n;
            int[] childIds = ((Content.ScopeDefinition)this.contents[scopeId].definition).childIds;
            int count = ((Content.ScopeDefinition)this.contents[scopeId].definition).count;
            Content.ScatteredDefinition[] scattered = this.getAllScatteredDefinitions(scopeId);
            if (scattered != null) {
                n = 0;
                while (n < scattered.length) {
                    if (scattered[n].group == null) {
                        ArrayList<Content.ScatteredDefinition> group = null;
                        int m = n + 1;
                        while (m < scattered.length) {
                            if (scattered[m].group == null && Utils.equals(scattered[n].name, scattered[m].name)) {
                                if (group == null) {
                                    group = new ArrayList<Content.ScatteredDefinition>();
                                    group.add(scattered[n]);
                                }
                                group.add(scattered[m]);
                            }
                            ++m;
                        }
                        if (group != null) {
                            Collections.sort(group, new Comparator<Content.ScatteredDefinition>(){

                                @Override
                                public int compare(Content.ScatteredDefinition o1, Content.ScatteredDefinition o2) {
                                    return Utils.compare(o1.from, o2.from, 2);
                                }
                            });
                            boolean continuousGroup = true;
                            int idx = 0;
                            for (Content.ScatteredDefinition d : group) {
                                if (d.from != idx || d.to < d.from) {
                                    continuousGroup = false;
                                    break;
                                }
                                idx = d.to + 1;
                            }
                            Content.ScatteredDefinition[] groupArray = group.toArray(new Content.ScatteredDefinition[group.size()]);
                            for (Content.ScatteredDefinition d : group) {
                                Content.ScatteredDefinition[] scatteredDefinitionArray = d.group = continuousGroup ? groupArray : UNGROUPED;
                            }
                        }
                    }
                    ++n;
                }
            }
            n = 0;
            while (n < count) {
                int id = childIds[n];
                if (this.contents[id].isScope()) {
                    this.group(id);
                }
                ++n;
            }
        }

        private Content.ScatteredDefinition[] getAllScatteredDefinitions(int scopeId) {
            ArrayList<Content.SignalDefinition> list = null;
            int[] childIds = ((Content.ScopeDefinition)this.contents[scopeId].definition).childIds;
            int count = ((Content.ScopeDefinition)this.contents[scopeId].definition).count;
            int n = 0;
            while (n < count) {
                int id = childIds[n];
                if (this.contents[id].isSignal()) {
                    Content.ScatteredDefinition definition = (Content.SignalDefinition)this.contents[id].definition;
                    while (definition != null) {
                        if (definition.parentId == scopeId && definition.from >= 0) {
                            if (list == null) {
                                list = new ArrayList<Content.SignalDefinition>();
                            }
                            list.add((Content.SignalDefinition)definition);
                        }
                        definition = definition.next;
                    }
                }
                ++n;
            }
            return list != null ? list.toArray(new Content.ScatteredDefinition[list.size()]) : null;
        }

        public boolean open(int itemId, String domain, long start, long rate, BinaryParseBuffer b) {
            if (this.contents[itemId] == null || this.contents[itemId].isSignal() && ((Content.SignalDefinition)this.contents[itemId].definition).isScatteredAndGrouped()) {
                if (b != null) {
                    b.setError("Open:No valid scope");
                }
                return false;
            }
            if (!this.applied) {
                this.applyDefinitions();
            }
            ICell scope = this.contents[itemId].definition.cell;
            List<ICell> signals = scope.getTribe(true, Signal.class);
            return this.open(itemId, signals, domain, start, rate, null, b);
        }

        public boolean open(int itemId, List<ICell> signals, String domain, long start, long rate, List<Integer> itemIds, BinaryParseBuffer b) {
            OpenState state = new OpenState(itemId);
            state.current = start;
            IDomainBase base = this.defaultDomainBase;
            if (!Utils.isEmpty(domain)) {
                base = DomainBase.parse(domain);
            }
            if (base == DomainBase.Unknown) {
                if (b != null) {
                    b.setError("Open:Invalid domain base");
                }
                return false;
            }
            ISamples.ProcessType pt = rate > 0L ? ISamples.ProcessType.Continuous : ISamples.ProcessType.Discrete;
            for (ICell signal : signals) {
                Pageable<byte[]> samples = ((Signal)signal).samples;
                Content.ScatteredDefinition definition = (Content.ScatteredDefinition)signal.getData(FluxParser.DEFINITION, Content.ScatteredDefinition.class);
                if (definition == null || definition.getContent().getTrace() != this) continue;
                Content content = definition.getContent();
                ISamples.SignalType st = ISamples.SignalType.valueOf((Signal)signal);
                ISamples.SignalDescriptor sd = ISamples.SignalDescriptor.valueOf((Signal)signal);
                if (definition.writer != null) {
                    if (b != null) {
                        b.setError("Open:Allready open:" + definition.getName());
                    }
                    return false;
                }
                if (definition.isScatteredAndGrouped()) {
                    definition.writer = FluxParser.this.generator.createConvergingWriter((Signal)signal, pt, st, sd, base);
                    if (definition.writer == null) {
                        if (b != null) {
                            b.setError("Open:Could not create writer:" + definition.getName());
                        }
                        return false;
                    }
                    Content.ScatteredDefinition[] scatteredDefinitionArray = definition.group;
                    int n = definition.group.length;
                    int n2 = 0;
                    while (n2 < n) {
                        Content.ScatteredDefinition d = scatteredDefinitionArray[n2];
                        Content gcontent = d.getContent();
                        if (itemIds != null) {
                            itemIds.add(gcontent.itemId);
                        }
                        if (gcontent.writer != null && gcontent.state != state) {
                            if (b != null) {
                                b.setError("Open:Allready open:" + definition.getName());
                            }
                            return false;
                        }
                        ISamplesWriter writer = ((IConvergingSamplesWriter)definition.writer).createSource(d.from, d.to - d.from + 1);
                        if (gcontent.writer == null) {
                            gcontent.writer = writer;
                        } else if (gcontent.writer instanceof IDivergingSamplesWriter) {
                            ((IDivergingSamplesWriter)gcontent.writer).addDestination(writer);
                        } else {
                            gcontent.writer = FluxParser.this.generator.createDivergingWriter(writer);
                        }
                        gcontent.state = state;
                        ++n2;
                    }
                } else {
                    definition.writer = FluxParser.this.generator.createWriter((Signal)signal, pt, st, sd, base);
                    if (definition.writer == null) {
                        if (b != null) {
                            b.setError("Open:Could not create writer:" + definition.getName());
                        }
                        return false;
                    }
                    if (itemIds != null) {
                        itemIds.add(content.itemId);
                    }
                    if (content.writer != null && content.state != state) {
                        if (b != null) {
                            b.setError("Open:Allready open:" + definition.getName());
                        }
                        return false;
                    }
                    if (content.writer == null) {
                        content.writer = definition.writer;
                    } else if (content.writer instanceof IDivergingSamplesWriter) {
                        ((IDivergingSamplesWriter)content.writer).addDestination(definition.writer);
                    } else {
                        content.writer = FluxParser.this.generator.createDivergingWriter(content.writer);
                    }
                    content.state = state;
                }
                if (definition.writer == null) {
                    if (b != null) {
                        b.setError("Open:Could not create writer");
                    }
                    return false;
                }
                definition.writer.open(start, rate, 0, 0, samples);
                state.writer.add(definition.writer);
                if (this.openStates.contains(state)) continue;
                this.openStates.add(state);
            }
            return true;
        }

        public boolean close(int itemId, long end, BinaryParseBuffer b) {
            if (this.contents[itemId] == null || this.contents[itemId].isSignal() && ((Content.SignalDefinition)this.contents[itemId].definition).isScatteredAndGrouped()) {
                if (b != null) {
                    b.setError("Open:No valid scope");
                }
                return false;
            }
            if (!this.applied) {
                this.applyDefinitions();
            }
            ICell scope = this.contents[itemId].definition.cell;
            List<ICell> signals = scope.getTribe(true, Signal.class);
            return this.close(itemId, signals, end, b);
        }

        public boolean close(int itemId, List<ICell> signals, long end, BinaryParseBuffer b) {
            for (ICell signal : signals) {
                Content.SignalDefinition definition = (Content.SignalDefinition)signal.getData(FluxParser.DEFINITION, Content.SignalDefinition.class);
                if (definition == null || definition.getContent().getTrace() != this) continue;
                Content content = definition.getContent();
                if (definition.writer != null && definition.writer.isOpen()) {
                    definition.writer.close(end);
                    definition.writer = null;
                    content.state.writer.remove(definition.writer);
                    if (content.state.writer.isEmpty()) {
                        this.openStates.remove(content.state);
                    }
                }
                if (definition.isScatteredAndGrouped()) {
                    Content.ScatteredDefinition[] scatteredDefinitionArray = definition.group;
                    int n = definition.group.length;
                    int n2 = 0;
                    while (n2 < n) {
                        Content.ScatteredDefinition d = scatteredDefinitionArray[n2];
                        Content gcontent = d.getContent();
                        gcontent.state = null;
                        gcontent.writer = null;
                        ++n2;
                    }
                    continue;
                }
                content.state = null;
                content.writer = null;
            }
            return true;
        }

        public int getNoOfItems(ICell signal) {
            Content.SignalDefinition definition = (Content.SignalDefinition)signal.getData(FluxParser.DEFINITION, Content.SignalDefinition.class);
            if (definition == null) {
                return 0;
            }
            if (definition.isScatteredAndGrouped()) {
                return definition.group.length;
            }
            return 1;
        }

        public IDomainBase getDefaultDomainBase() {
            return this.defaultDomainBase;
        }

        public ICell getBase() {
            return this.base;
        }

        public IRecordGenerator getGenerator() {
            return FluxParser.this.generator;
        }

        public List<OpenState> getOpenStates() {
            return this.openStates;
        }

        public OpenState getOpenState() {
            return this.openStates.isEmpty() ? null : this.openStates.get(0);
        }

        public OpenState getOpenState(int itemId) {
            for (OpenState state : this.openStates) {
                if (state.itemId != itemId) continue;
                return state;
            }
            return null;
        }

        public int findScope(int parentId, String name, String description) {
            if (this.contents[parentId] != null && this.contents[parentId].isScope()) {
                return ((Content.ScopeDefinition)this.contents[parentId].definition).find(name, description);
            }
            return -1;
        }

        public void setSharedId(int itemId, int targetId) {
            this.sharedIds[itemId] = targetId;
        }

        public int getSharedId(int itemId) {
            return this.sharedIds[itemId] != 0 ? this.sharedIds[itemId] : itemId;
        }

        public void addSection(int counter, byte[] bytes) {
            this.sections.add(new Section(counter, bytes));
        }

        public byte[] reorderSections() {
            int size = 0;
            int n = 0;
            while (n < this.sections.size()) {
                size += this.sections.get((int)n).bytes.length;
                ++n;
            }
            byte[] ordered = new byte[size];
            int pos = 0;
            int count = this.sections.get((int)0).counter == 0 ? 0 : this.sections.size();
            int n2 = 1;
            while (n2 < this.sections.size()) {
                if (this.sections.get((int)0).counter != this.sections.get((int)n2).counter) {
                    if (this.sections.get((int)1).counter == 0) {
                        count = n2;
                        break;
                    }
                    pos = n2;
                    break;
                }
                ++n2;
            }
            int inserted = 0;
            int n3 = 0;
            while (n3 < count) {
                System.arraycopy(this.sections.get((int)pos).bytes, 0, ordered, inserted, this.sections.get((int)pos).bytes.length);
                inserted += this.sections.get((int)pos).bytes.length;
                if (++pos >= this.sections.size()) {
                    pos = 0;
                }
                ++n3;
            }
            return ordered;
        }

        @Override
        public int flxAddSignal(int itemId, int parentId, String name, String description, byte type, String descriptor) {
            if (itemId <= 0 || itemId > this.maxItemId) {
                return -1;
            }
            if (parentId < 0 || parentId > this.maxItemId) {
                return -1;
            }
            if (type < 0 || type >= ISamples.SignalType.values().length) {
                return -1;
            }
            if (!this.contents[parentId = this.getSharedId(parentId)].isScope()) {
                return -1;
            }
            if (this.contents[itemId] != null) {
                return -1;
            }
            Content content = this.createContent(itemId);
            ISamples.SignalType sType = ISamples.SignalType.values()[type];
            ISamples.SignalDescriptor sDescriptor = ISamples.SignalDescriptor.parseUser(sType, descriptor);
            content.setSignal(parentId, name, description, sType, sDescriptor);
            return 0;
        }

        @Override
        public int flxAddSignals(int itemIdFrom, int itemIdTo, int parentId, String name, String description, byte type, String descriptor) {
            if (itemIdFrom <= 0 || itemIdFrom > this.maxItemId) {
                return -1;
            }
            if (itemIdTo <= 0 || itemIdTo > this.maxItemId || itemIdTo < itemIdFrom) {
                return -1;
            }
            if (parentId < 0 || parentId > this.maxItemId) {
                return -1;
            }
            if (type < 0 || type >= ISamples.SignalType.values().length) {
                return -1;
            }
            if (!this.contents[parentId = this.getSharedId(parentId)].isScope()) {
                return -1;
            }
            ISamples.SignalType sType = ISamples.SignalType.values()[type];
            ISamples.SignalDescriptor sDescriptor = ISamples.SignalDescriptor.parseUser(sType, descriptor);
            int itemId = itemIdFrom;
            while (itemId <= itemIdTo) {
                if (this.contents[itemId] != null) {
                    return -1;
                }
                Content content = this.createContent(itemId);
                content.setSignal(parentId, name, description, sType, sDescriptor);
                ++itemId;
            }
            return 0;
        }

        @Override
        public int flxAddScatteredSignal(int itemId, int parentId, String name, String description, byte type, String descriptor, int scatteredFrom, int scatteredTo) {
            if (itemId <= 0 || itemId > this.maxItemId) {
                return -1;
            }
            if (parentId < 0 || parentId > this.maxItemId) {
                return -1;
            }
            if (type < 0 || type >= ISamples.SignalType.values().length) {
                return -1;
            }
            if (scatteredTo < scatteredFrom) {
                return -1;
            }
            if (!this.contents[parentId = this.getSharedId(parentId)].isScope()) {
                return -1;
            }
            if (this.contents[itemId] != null) {
                return -1;
            }
            Content content = this.createContent(itemId);
            ISamples.SignalType sType = ISamples.SignalType.values()[type];
            ISamples.SignalDescriptor sDescriptor = ISamples.SignalDescriptor.parseUser(sType, descriptor);
            content.setSignal(parentId, name, description, sType, sDescriptor, scatteredFrom, scatteredTo);
            return 0;
        }

        @Override
        public int flxAddSignalReference(int referenceId, int parentId, String name, String description) {
            if (referenceId <= 0 || referenceId > this.maxItemId) {
                return -1;
            }
            if (parentId < 0 || parentId > this.maxItemId) {
                return -1;
            }
            if (!this.contents[parentId = this.getSharedId(parentId)].isScope()) {
                return -1;
            }
            Content refContent = this.contents[referenceId];
            if (refContent == null || !refContent.isSignal()) {
                return -1;
            }
            refContent.addReference(parentId, name, description);
            return 0;
        }

        @Override
        public int flxAddScatteredSignalReference(int referenceId, int parentId, String name, String description, int scatteredFrom, int scatteredTo) {
            if (referenceId <= 0 || referenceId > this.maxItemId) {
                return -1;
            }
            if (parentId < 0 || parentId > this.maxItemId) {
                return -1;
            }
            if (scatteredTo < scatteredFrom) {
                return -1;
            }
            if (!this.contents[parentId = this.getSharedId(parentId)].isScope()) {
                return -1;
            }
            Content refContent = this.contents[referenceId];
            if (refContent == null || !refContent.isSignal()) {
                return -1;
            }
            refContent.addReference(parentId, name, description, scatteredFrom, scatteredTo);
            return 0;
        }

        @Override
        public int flxAddScope(int itemId, int parentId, String name, String description) {
            if (itemId <= 0 || itemId > this.maxItemId) {
                return -1;
            }
            if (parentId < 0 || parentId > this.maxItemId) {
                return -1;
            }
            if (!this.contents[parentId = this.getSharedId(parentId)].isScope()) {
                return -1;
            }
            if (this.contents[itemId] != null) {
                return -1;
            }
            int targetId = this.findScope(parentId, name, description);
            if (targetId < 0) {
                Content content = this.createContent(itemId);
                content.setScope(parentId, name, description);
            } else {
                this.setSharedId(itemId, targetId);
            }
            return 0;
        }

        @Override
        public int flxOpen(int itemId, String domainBase, long start, long rate) {
            if (itemId < 0 || itemId > this.maxItemId) {
                return -1;
            }
            Content content = ((FluxParser)FluxParser.this).currentTrace.contents[itemId];
            if (content == null) {
                return -1;
            }
            this.open(itemId, domainBase, start, rate, null);
            return 0;
        }

        @Override
        public int flxClose(int itemId, long end) {
            if (itemId < 0 || itemId > this.maxItemId) {
                return -1;
            }
            Content content = ((FluxParser)FluxParser.this).currentTrace.contents[itemId];
            if (content == null) {
                return -1;
            }
            this.close(itemId, end, null);
            return 0;
        }

        @Override
        public int flxWriteEnumDef(int itemId, int enumeration, String label, int value) {
            if (itemId <= 0 || itemId > ((FluxParser)FluxParser.this).currentTrace.maxItemId) {
                return -1;
            }
            Content content = ((FluxParser)FluxParser.this).currentTrace.contents[itemId];
            if (content == null) {
                return -1;
            }
            ISamplesWriter writer = content.writer;
            if (writer == null || !writer.isOpen()) {
                return -1;
            }
            writer.setEnum(enumeration, label, value);
            return 0;
        }

        @Override
        public int flxWriteArrayDef(int itemId, int index, String label) {
            if (itemId <= 0 || itemId > this.maxItemId) {
                return -1;
            }
            Content content = this.contents[itemId];
            if (content == null) {
                return -1;
            }
            ISamplesWriter writer = content.writer;
            if (writer == null || !writer.isOpen()) {
                return -1;
            }
            writer.setMember(index, label, 0, null, -1);
            return 0;
        }

        @Override
        public int flxWriteMemberDef(int itemId, int memberId, String label, byte type, String descriptor) {
            if (itemId <= 0 || itemId > this.maxItemId) {
                return -1;
            }
            if (type < 0) {
                return -1;
            }
            ISamples.SignalDescriptor sDescriptor = ISamples.SignalDescriptor.parseUser(ISamples.SignalType.Unknown, descriptor);
            Content content = this.contents[itemId];
            if (content == null) {
                return -1;
            }
            ISamplesWriter writer = content.writer;
            if (writer == null || !writer.isOpen()) {
                return -1;
            }
            writer.setMember(memberId, label, type, sDescriptor.getContent(), sDescriptor.getFormat());
            return 0;
        }

        public void setIgnoreRedefinitions(boolean ignore) {
            this.ignoreDefinitions = ignore;
        }

        public boolean ignoreRedefinitions() {
            return this.ignoreDefinitions;
        }

        final class Content {
            int itemId;
            Definition definition;
            OpenState state;
            ISamplesWriter writer;

            Content(int itemId) {
                this.itemId = itemId;
            }

            void setScope(int parentId, String name, String description) {
                this.definition = new ScopeDefinition(parentId, name, description);
                if (parentId >= 0) {
                    ICell scope = Trace.this.contents[parentId].definition.cell;
                    this.definition.cell = FluxParser.this.generator.addScope(scope, name, description);
                    ((ScopeDefinition)Trace.this.contents[parentId].definition).add(this.itemId, false);
                } else {
                    this.definition.cell = Trace.this.base;
                }
            }

            public void setSignal(int parentId, String name, String description, ISamples.SignalType signalType, ISamples.SignalDescriptor descriptor) {
                this.setSignal(parentId, name, description, signalType, descriptor, -1, -1);
            }

            public void setSignal(int parentId, String name, String description, ISamples.SignalType signalType, ISamples.SignalDescriptor descriptor, int from, int to) {
                this.definition = new SignalDefinition(parentId, name, description, signalType, descriptor, from, to);
                ((ScopeDefinition)Trace.this.contents[parentId].definition).add(this.itemId, false);
            }

            public boolean isSignal() {
                return this.definition instanceof SignalDefinition;
            }

            public boolean isScope() {
                return this.definition instanceof ScopeDefinition;
            }

            public void addReference(int parentId, String name, String description) {
                this.addReference(parentId, name, description, -1, -1);
            }

            public boolean addReference(int parentId, String name, String description, int from, int to) {
                if (this.definition instanceof ScatteredDefinition) {
                    if (this.definition.parentId == parentId && Utils.equals(name, this.definition.name) && Utils.equals(description, this.definition.description)) {
                        return false;
                    }
                    ScatteredDefinition def = (ScatteredDefinition)this.definition;
                    while (def.next != null) {
                        def = def.next;
                        if (def.parentId != parentId || !Utils.equals(name, def.name) || !Utils.equals(description, def.description)) continue;
                        return false;
                    }
                    def.next = new ReferenceDefinition(parentId, name, description, from, to);
                    ((ScopeDefinition)Trace.this.contents[parentId].definition).add(this.itemId, true);
                    return true;
                }
                return false;
            }

            public Trace getTrace() {
                return Trace.this;
            }

            class Definition {
                int parentId;
                String name;
                String description;
                ICell cell;

                public Definition(int parentId, String name, String description) {
                    this.parentId = parentId;
                    this.name = name != null ? name.intern() : name;
                    this.description = description != null ? description.intern() : description;
                }

                public Content getContent() {
                    return Content.this;
                }

                public int getItemId() {
                    return Content.this.itemId;
                }
            }

            final class ReferenceDefinition
            extends ScatteredDefinition {
                public ReferenceDefinition(int parentId, String name, String description, int from, int to) {
                    super(parentId, name, description, from, to);
                }
            }

            abstract class ScatteredDefinition
            extends Definition {
                ReferenceDefinition next;
                int from;
                int to;
                ScatteredDefinition[] group;
                ISamplesWriter writer;

                public ScatteredDefinition(int parentId, String name, String description, int from, int to) {
                    super(parentId, name, description);
                    this.from = from;
                    this.to = to;
                }

                String getName() {
                    String result = this.name;
                    if (this.group == UNGROUPED) {
                        result = this.to != this.from ? String.valueOf(this.name) + (this.name.endsWith("]") ? "" : " ") + "[" + this.to + ":" + this.from + "]" : String.valueOf(this.name) + (this.name.endsWith("]") ? "" : " ") + "[" + this.from + "]";
                    }
                    if (FluxParser.this.handler != null) {
                        result = FluxParser.this.handler.adjustItemName(false, result);
                    }
                    return result.intern();
                }

                boolean isScatteredAndGrouped() {
                    return this.from >= 0 && this.group != null && this.group != UNGROUPED;
                }

                boolean isScattered() {
                    return this.from >= 0;
                }

                boolean isGrouped() {
                    return this.group != null && this.group != UNGROUPED;
                }
            }

            final class ScopeDefinition
            extends Definition {
                int count;
                int[] childIds;

                public ScopeDefinition(int parentId, String name, String description) {
                    super(parentId, name, description);
                    this.childIds = new int[10];
                }

                void add(int id, boolean reference) {
                    if (reference) {
                        int n = 0;
                        while (n < this.count) {
                            if (this.childIds[n] == id) {
                                return;
                            }
                            ++n;
                        }
                    }
                    if (this.count == this.childIds.length) {
                        int[] newIds = new int[this.childIds.length * 2];
                        System.arraycopy(this.childIds, 0, newIds, 0, this.childIds.length);
                        this.childIds = newIds;
                    }
                    this.childIds[this.count++] = id;
                }

                int find(String name, String description) {
                    int n = 0;
                    while (n < this.count) {
                        Definition definition;
                        if (this.childIds[n] > 0 && this.childIds[n] < ((Content)Content.this).Trace.this.contents.length && (definition = ((Content)Content.this).Trace.this.contents[this.childIds[n]].definition) != null && Utils.equals(definition.name, name) && Utils.equals(definition.description, description)) {
                            return this.childIds[n];
                        }
                        ++n;
                    }
                    return -1;
                }
            }

            final class SignalDefinition
            extends ScatteredDefinition {
                ISamples.ProcessType processType;
                ISamples.SignalType signalType;
                ISamples.SignalDescriptor descriptor;

                public SignalDefinition(int parentId, String name, String description, ISamples.SignalType signalType, ISamples.SignalDescriptor descriptor, int from, int to) {
                    super(parentId, name, description, from, to);
                    this.signalType = signalType;
                    this.descriptor = descriptor;
                }
            }
        }

        final class Section {
            int counter;
            byte[] bytes;

            public Section(int counter, byte[] bytes) {
                this.counter = counter;
                this.bytes = bytes;
            }
        }
    }
}

