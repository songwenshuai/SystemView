/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.paint.IPaintItem;

public interface ICursorItem
extends IPaintItem {
    public static final int CHANGED_POSITION = 128;
    public static final String Position = "Position";
    public static final String Delta = "Delta";
    public static final String[] Contents = new String[]{"Position", "Delta"};

    public DomainValue getPosition();

    public boolean hasDelta();

    public DomainValue getDeltaPosition();

    public IDomainBase getDomainBase();

    public boolean setPosition(DomainValue var1);

    public long getPositionInDomainUnits();

    public long getDeltaInDomainUnits();
}

