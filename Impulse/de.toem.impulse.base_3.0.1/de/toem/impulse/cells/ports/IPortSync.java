/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;

public interface IPortSync {
    public IReadableSamples getReadable(String var1);

    public ISamplePointer getPointer(String var1);

    public void setSynced(DomainValue var1);
}

