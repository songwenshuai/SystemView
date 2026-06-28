/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.values.CompoundValue;

public interface ISamplesEditor {
    public boolean reOpen();

    public void close();

    public boolean isOpen();

    public boolean modifyRange(long var1, long var3, long var5);

    public boolean modifyDomainBase(IDomainBase var1);

    public boolean changeSample(CompoundValue var1, int var2);

    public boolean insertSample(CompoundValue var1, int var2);

    public boolean removeSample(int var1, int var2);
}

