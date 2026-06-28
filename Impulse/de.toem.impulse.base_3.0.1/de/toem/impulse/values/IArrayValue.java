/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.samples.ISample;

public interface IArrayValue<E>
extends ISample {
    public E getArray();

    public int length();
}

