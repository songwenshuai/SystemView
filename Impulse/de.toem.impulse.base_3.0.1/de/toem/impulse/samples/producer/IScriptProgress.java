/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.toolkits.pattern.threading.IProgress;

public interface IScriptProgress
extends IProgress {
    public void cont();

    public boolean canContinue();
}

