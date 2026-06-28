/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.flux;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.flux.FluxParser;
import de.toem.impulse.serializer.BinaryParseBuffer;
import de.toem.impulse.values.StructMember;
import de.toem.toolkits.pattern.threading.IProgress;

public interface IFluxHandler {
    public boolean requiresData();

    public boolean handleOpen(IProgress var1, FluxParser.Trace var2, int var3, String var4, long var5, long var7, BinaryParseBuffer var9);

    public boolean handleClose(IProgress var1, FluxParser.Trace var2, int var3, long var4, BinaryParseBuffer var6);

    public boolean handleControl(IProgress var1, FluxParser.Trace var2, boolean var3, int var4, int var5, StructMember[] var6, BinaryParseBuffer var7);

    public boolean handleData(IProgress var1, FluxParser.Trace var2, int var3, long var4, int var6, BinaryParseBuffer var7);

    public void handleCreated(IProgress var1, FluxParser.Trace var2, BinaryParseBuffer var3);

    public void handleOpened(IProgress var1, FluxParser.Trace var2, int var3, BinaryParseBuffer var4);

    public void handleClosed(IProgress var1, FluxParser.Trace var2, int var3, BinaryParseBuffer var4);

    public boolean isFinished();

    public boolean handleFlush(FluxParser.Trace var1, Signal var2);

    public String adjustItemName(boolean var1, String var2);
}

