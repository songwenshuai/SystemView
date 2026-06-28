/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.provider;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.provider.ISamplesCache;
import de.toem.impulse.provider.ISamplesProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.provider.IProvider;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

public class SamplesCache
implements ISamplesCache {
    private HashMap<IProvider, IReadableSamples> invalidated;
    private HashMap<IProvider, IReadableSamples> readables;
    private Map<String, IDomainBase> domainBases;
    private HashMap<String, Object> data;

    @Override
    public synchronized SamplesCache derive(int mode) {
        SamplesCache that = new SamplesCache();
        if (mode == 0) {
            that.readables = this.readables;
            that.invalidated = this.invalidated;
            that.domainBases = this.domainBases;
            that.data = this.data;
        } else if (mode == 1) {
            that.invalidated = this.readables;
        } else if (mode == 2) {
            that.invalidated = this.readables;
            that.domainBases = this.domainBases;
            that.data = this.data;
        }
        return that;
    }

    @Override
    public synchronized void apply(ISamplesCache that) {
        if (that instanceof SamplesCache) {
            this.readables = ((SamplesCache)that).readables;
        }
        this.domainBases = ((SamplesCache)that).domainBases;
        this.data = ((SamplesCache)that).data;
        this.invalidated = null;
    }

    @Override
    public synchronized void invalidate() {
        this.invalidated = this.readables;
        this.domainBases = null;
        this.data = null;
    }

    @Override
    public synchronized void clear() {
        this.invalidated = null;
        this.readables = null;
        this.domainBases = null;
        this.data = null;
    }

    @Override
    public synchronized Set<String> getDomainClasses() {
        if (this.domainBases != null) {
            return this.domainBases.keySet();
        }
        return Collections.EMPTY_SET;
    }

    @Override
    public synchronized IDomainBase get(String domainClass) {
        if (this.domainBases != null) {
            return this.domainBases.get(domainClass);
        }
        return null;
    }

    @Override
    public synchronized void put(String domainClass, IDomainBase domainBase) {
        if (this.domainBases == null) {
            this.domainBases = new HashMap<String, IDomainBase>();
        }
        if (this.domainBases != null) {
            this.domainBases.put(domainClass, domainBase);
        }
    }

    @Override
    public synchronized IReadableSamples getInvalidated(ISamplesProvider provider) {
        if (this.invalidated != null) {
            return this.invalidated.get(provider);
        }
        return null;
    }

    @Override
    public synchronized IReadableSamples get(ISamplesProvider provider) {
        if (this.readables != null) {
            return this.readables.get(provider);
        }
        return null;
    }

    @Override
    public synchronized void put(ISamplesProvider provider, IReadableSamples samples) {
        if (this.readables == null) {
            this.readables = new HashMap();
        }
        if (this.readables != null) {
            this.readables.put(provider, samples);
        }
    }

    @Override
    public synchronized Object get(ICell cell, String key) {
        if (this.data != null) {
            return this.data.get(String.valueOf(key) + cell.hashCode());
        }
        return null;
    }

    @Override
    public synchronized void put(ICell cell, String key, Object value) {
        if (this.data == null) {
            this.data = new HashMap();
        }
        if (this.data != null) {
            this.data.put(String.valueOf(key) + cell.hashCode(), value);
        }
    }
}

