/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.view;

import de.toem.impulse.provider.ISamplesCache;
import de.toem.impulse.provider.ISamplesContext;
import de.toem.impulse.provider.ISignalContext;
import de.toem.impulse.provider.SamplesCache;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.provider.IContext;

public class DerivedSamplesContext
implements ISignalContext,
ISamplesContext {
    private IContext context;
    private ISamplesCache cache;
    private int mode;

    public DerivedSamplesContext(IContext context, int mode) {
        this.context = context;
        this.mode = mode;
        ISamplesCache cache = context instanceof ISamplesContext ? ((ISamplesContext)context).getSamplesCache() : null;
        this.cache = cache != null ? cache.derive(mode) : new SamplesCache();
    }

    @Override
    public int getSamplesMode() {
        return this.mode;
    }

    @Override
    public ISamplesCache getSamplesCache() {
        return this.cache;
    }

    @Override
    public IElement getRecord() {
        return this.context instanceof ISignalContext ? ((ISignalContext)this.context).getRecord() : null;
    }

    public void apply() {
        ISamplesCache cache;
        ISamplesCache iSamplesCache = cache = this.context instanceof ISamplesContext ? ((ISamplesContext)this.context).getSamplesCache() : null;
        if (cache != null) {
            cache.apply(this.cache);
        }
    }
}

