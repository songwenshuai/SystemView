/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.disclosures;

import de.toem.impulse.disclosures.AbstractReadableDisclosure;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.threading.Progress;
import java.util.List;

public class FirstEventDisclosure
extends AbstractReadableDisclosure {
    @Override
    public int getFormats() {
        return 9;
    }

    @Override
    protected Object extractData(ICell cell, IReadableSamples readable, Object properties) {
        if (readable != null && readable.ensureSettled(new Progress())) {
            DomainValue units = readable.getCount() >= 2 ? readable.positionAt(1) : DomainValue.MAX;
            return units;
        }
        return null;
    }

    @Override
    protected boolean combineReadables() {
        return true;
    }

    @Override
    protected Object combineData(ICell cell, List<Object> disclosedChildren, Object properties) {
        DomainValue min = DomainValue.MAX;
        for (Object data : disclosedChildren) {
            if (!(data instanceof DomainValue)) continue;
            min = DomainValue.min(min, (DomainValue)data);
        }
        return min;
    }

    @Override
    protected Object format(Object data, int format) {
        if (data != null && data instanceof DomainValue) {
            if ((format & 1) != 0) {
                return ((DomainValue)data).toString();
            }
            if ((format & 8) != 0) {
                return ((DomainValue)data).units;
            }
        }
        return null;
    }
}

