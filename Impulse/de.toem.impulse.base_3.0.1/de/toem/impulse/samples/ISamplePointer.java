/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.IPointer;
import de.toem.impulse.samples.IReadableMembers;
import de.toem.impulse.samples.IReadableSample;

public interface ISamplePointer
extends IReadableSample,
IReadableMembers,
IPointer {
    public boolean isEdge(int var1, ILogicDetector var2);

    public boolean isEdge(int var1);

    public boolean goPrevEdge(int var1, ILogicDetector var2);

    public boolean goPrevEdge(int var1);

    public boolean goNextEdge(int var1, ILogicDetector var2);

    public boolean goNextEdge(int var1);
}

