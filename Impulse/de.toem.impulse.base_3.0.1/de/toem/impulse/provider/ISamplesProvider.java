/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.provider;

import de.toem.impulse.provider.ISimpleSamplesProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.toolkits.pattern.provider.IContext;

public interface ISamplesProvider
extends ISimpleSamplesProvider {
    public IReadableSamples getSamples(IContext var1);
}

