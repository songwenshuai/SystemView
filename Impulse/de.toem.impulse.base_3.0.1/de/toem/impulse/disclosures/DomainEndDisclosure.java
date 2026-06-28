/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.disclosures;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.provider.ISignalContext;
import de.toem.impulse.provider.ISignalProvider;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.disclosure.AbstractDisclosure;
import de.toem.toolkits.pattern.provider.IContext;

public class DomainEndDisclosure
extends AbstractDisclosure {
    @Override
    public int getFormats() {
        return 9;
    }

    @Override
    public Object disclose(ICell cell, int format, Object properties, IContext context) {
        if (cell instanceof ISignalProvider && context instanceof ISignalContext || context == null) {
            Signal signal = ((ISignalProvider)cell).getSignal((ISignalContext)context);
            if (signal != null && (format & 8) != 0) {
                return signal.end;
            }
            if (signal != null && (format & 1) != 0) {
                return new DomainValue(DomainBase.valueOf(signal), signal.end).toString();
            }
        }
        return null;
    }
}

