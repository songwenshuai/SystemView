/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.record;

import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.Link;

@CellAnnotation(type="record.portScope", dynamicChildren={"record.signal", "record.signalProxy", "record.scope", "serializer.message"}, properties={"imageExtension"})
public class PortScope
extends Scope {
    public static final String TYPE = "record.portScope";
    public static final int SYNC_NONE = 0;
    public static final int SYNC_UNSYNCED = 1;
    public static final int SYNC_SYNCED = 2;
    public int synced = 0;
    public String domainSync0;
    public String domainSync1;
    public Link source;

    public long getSamplesOffset(IDomainBase base) {
        DomainValue value;
        if (this.synced != 2 || base == null) {
            return 0L;
        }
        if (this.domainSync0 != null && (value = DomainValue.parse(this.domainSync0)) != null && base.isCompatible(value.base)) {
            return value.convertTo((IDomainBase)base).units;
        }
        if (this.domainSync1 != null && (value = DomainValue.parse(this.domainSync1)) != null && base.isCompatible(value.base)) {
            return value.convertTo((IDomainBase)base).units;
        }
        return 0L;
    }

    @Override
    public String imageExtension() {
        if (this.synced == 1) {
            return "-unsync";
        }
        return super.imageExtension();
    }
}

