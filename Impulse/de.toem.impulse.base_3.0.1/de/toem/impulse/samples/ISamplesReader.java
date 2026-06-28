/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IPackedSamples;
import de.toem.impulse.samples.IReadableSamples;

public interface ISamplesReader
extends IReadableSamples,
IPackedSamples {
    public int update(Signal var1, IDomainBase var2);

    public int update(IPackedSamples var1, IDomainBase var2);
}

