/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.ui;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.provider.ISimpleSamplesProvider;
import de.toem.toolkits.core.Utils;

public class DomainPosition {
    public DomainValue domainValue;
    public ISimpleSamplesProvider samplesProvider;
    public int idx;

    public boolean equals(Object that) {
        if (!(that instanceof DomainPosition)) {
            return false;
        }
        if (!Utils.equals(this.domainValue, ((DomainPosition)that).domainValue)) {
            return false;
        }
        if (this.samplesProvider != ((DomainPosition)that).samplesProvider) {
            return false;
        }
        return this.idx == ((DomainPosition)that).idx;
    }

    public DomainPosition(DomainValue domainValue, ISimpleSamplesProvider samplesProvider, int index) {
        this.domainValue = domainValue;
        this.samplesProvider = samplesProvider;
        this.idx = index;
    }
}

