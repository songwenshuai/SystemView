/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.provider;

import de.toem.impulse.samples.IReadableSamples;
import de.toem.toolkits.pattern.provider.IProvider;

public interface ISimpleSamplesProvider
extends IProvider {
    public IReadableSamples getSamples();
}

