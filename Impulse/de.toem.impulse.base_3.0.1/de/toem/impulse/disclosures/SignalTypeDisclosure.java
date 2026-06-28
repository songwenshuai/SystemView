/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.disclosures;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.provider.ISignalContext;
import de.toem.impulse.provider.ISignalProvider;
import de.toem.impulse.samples.ISamples;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.disclosure.AbstractDisclosure;
import de.toem.toolkits.pattern.provider.IContext;

public class SignalTypeDisclosure
extends AbstractDisclosure {
    @Override
    public int getFormats() {
        return 1;
    }

    @Override
    public Object disclose(ICell cell, int format, Object properties, IContext context) {
        Signal signal;
        if ((cell instanceof ISignalProvider && context instanceof ISignalContext || context == null) && (signal = ((ISignalProvider)cell).getSignal((ISignalContext)context)) != null && (format & 1) != 0) {
            return ISamples.SignalType.valueOf(signal).toString();
        }
        return null;
    }
}

