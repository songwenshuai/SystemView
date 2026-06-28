/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.provider;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.provider.ISamplesProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.toolkits.pattern.element.ICell;
import java.util.Set;

public interface ISamplesCache {
    public ISamplesCache derive(int var1);

    public void apply(ISamplesCache var1);

    public void invalidate();

    public void clear();

    public IReadableSamples getInvalidated(ISamplesProvider var1);

    public IReadableSamples get(ISamplesProvider var1);

    public void put(ISamplesProvider var1, IReadableSamples var2);

    public Set<String> getDomainClasses();

    public IDomainBase get(String var1);

    public void put(String var1, IDomainBase var2);

    public Object get(ICell var1, String var2);

    public void put(ICell var1, String var2, Object var3);
}

