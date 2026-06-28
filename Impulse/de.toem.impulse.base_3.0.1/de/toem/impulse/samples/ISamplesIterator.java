/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamplesWriter;
import java.util.Iterator;

public interface ISamplesIterator
extends Iterator<Long> {
    public boolean hasPrev();

    public Long prev();

    public Long prev(ISamplesWriter var1);

    @Override
    public Long next();

    public Long next(ISamplesWriter var1);

    public long current();

    public long current(ISamplesWriter var1);

    public void setCurrent(long var1, boolean var3);

    public boolean isCurrent(ISamplePointer var1);

    public void setCurrent(DomainValue var1, boolean var2);

    public IDomainBase getDomainBase();

    public long getStart();

    public long getEnd();

    public DomainValue getCurrentPosition();

    public DomainValue getStartPosition();

    public DomainValue getEndPosition();

    public boolean prev(ISamplePointer var1);

    public boolean next(ISamplePointer var1);

    public boolean prevEdge(ISamplePointer var1, int var2);

    public boolean prevEdge(ISamplePointer var1, int var2, ILogicDetector var3);

    public boolean nextEdge(ISamplePointer var1, int var2);

    public boolean nextEdge(ISamplePointer var1, int var2, ILogicDetector var3);
}

