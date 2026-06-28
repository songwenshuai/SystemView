/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.paint.IPaintStyle;
import de.toem.toolkits.pattern.information.IInformation;

public interface ISamplesDisplayInformation
extends IInformation {
    public IPaintStyle getPaintStyle();

    public int getValueColumnFormat();

    public Object getColor();
}

