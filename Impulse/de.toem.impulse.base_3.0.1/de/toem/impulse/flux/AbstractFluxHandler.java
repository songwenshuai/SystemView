/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.flux;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.flux.FluxParser;
import de.toem.impulse.flux.IFluxHandler;
import de.toem.impulse.serializer.BinaryParseBuffer;
import de.toem.impulse.values.StructMember;
import de.toem.toolkits.pattern.threading.IProgress;

public abstract class AbstractFluxHandler
implements IFluxHandler {
    @Override
    public boolean requiresData() {
        return false;
    }

    @Override
    public boolean handleOpen(IProgress p, FluxParser.Trace trace, int itemId, String domain, long start, long rate, BinaryParseBuffer b) {
        return true;
    }

    @Override
    public boolean handleClose(IProgress p, FluxParser.Trace trace, int itemId, long end, BinaryParseBuffer b) {
        return true;
    }

    @Override
    public boolean handleControl(IProgress p, FluxParser.Trace trace, boolean request, int controlId, int messageId, StructMember[] members, BinaryParseBuffer b) {
        return true;
    }

    @Override
    public boolean handleData(IProgress p, FluxParser.Trace trace, int itemId, long delta, int size, BinaryParseBuffer b) {
        return true;
    }

    @Override
    public boolean isFinished() {
        return false;
    }

    @Override
    public boolean handleFlush(FluxParser.Trace trace, Signal signal) {
        return true;
    }

    @Override
    public String adjustItemName(boolean isScope, String name) {
        return name;
    }

    @Override
    public void handleCreated(IProgress p, FluxParser.Trace trace, BinaryParseBuffer b) {
    }

    @Override
    public void handleOpened(IProgress p, FluxParser.Trace trace, int itemId, BinaryParseBuffer b) {
    }

    @Override
    public void handleClosed(IProgress p, FluxParser.Trace trace, int itemId, BinaryParseBuffer b) {
    }
}

