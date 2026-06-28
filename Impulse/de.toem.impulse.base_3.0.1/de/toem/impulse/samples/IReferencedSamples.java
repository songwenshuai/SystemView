/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.ISamples;

public interface IReferencedSamples
extends ISamples {
    public ISamples getReference();

    public ISamples getReference(Class<? extends ISamples> var1);
}

