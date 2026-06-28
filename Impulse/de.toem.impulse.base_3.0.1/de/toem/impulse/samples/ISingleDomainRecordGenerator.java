/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IRecordGenerator;
import de.toem.impulse.samples.ISamples;
import de.toem.toolkits.pattern.element.ICell;

public interface ISingleDomainRecordGenerator
extends IRecordGenerator {
    public void initRecord(String var1, IDomainBase var2);

    public Signal addSignal(ICell var1, String var2, String var3, ISamples.ProcessType var4, ISamples.SignalType var5, ISamples.SignalDescriptor var6);

    public void open(long var1);

    public void open(long var1, long var3);

    public void open(long var1, long var3, int var5, int var6);

    public void close(long var1);
}

