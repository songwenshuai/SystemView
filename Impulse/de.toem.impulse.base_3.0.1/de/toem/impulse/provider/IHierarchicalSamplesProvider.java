/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.provider;

import de.toem.impulse.provider.ISamplesProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.toolkits.pattern.provider.IContext;
import java.util.List;

public interface IHierarchicalSamplesProvider
extends ISamplesProvider {
    public boolean hasChildSamples(String var1, IContext var2);

    public List<String> getChildSampleIds(String var1, IContext var2);

    public IReadableSamples getSamples(String var1, IContext var2);
}

