/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.disclosures;

import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.disclosure.AbstractDisclosure;
import de.toem.toolkits.pattern.provider.IContext;

public class SignalClassDisclosure
extends AbstractDisclosure {
    @Override
    public int getFormats() {
        return 1;
    }

    @Override
    public Object disclose(ICell cell, int format, Object properties, IContext context) {
        if (cell != null && (format & 1) != 0) {
            return cell.getClass().getSimpleName();
        }
        return null;
    }
}

