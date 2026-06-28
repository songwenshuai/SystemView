/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.disclosures;

import de.toem.impulse.disclosures.AbstractReadableDisclosure;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.toolkits.pattern.element.ICell;
import java.util.List;

public class NoOfEventsDisclosure
extends AbstractReadableDisclosure {
    @Override
    public int getFormats() {
        return 9;
    }

    @Override
    protected Object extractData(ICell cell, IReadableSamples readable, Object properties) {
        if (readable != null) {
            return readable.getCount();
        }
        return null;
    }

    @Override
    protected boolean combineReadables() {
        return true;
    }

    @Override
    protected Object combineData(ICell cell, List<Object> disclosedChildren, Object properties) {
        int total = 0;
        for (Object data : disclosedChildren) {
            if (!(data instanceof Integer)) continue;
            total += ((Integer)data).intValue();
        }
        return total;
    }

    @Override
    protected Object format(Object data, int format) {
        if ((format & 1) != 0) {
            return String.valueOf(data);
        }
        if ((format & 8) != 0) {
            return data;
        }
        return null;
    }
}

