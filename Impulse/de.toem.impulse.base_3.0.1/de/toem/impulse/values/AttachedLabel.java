/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.values.AbstractAttachment;
import de.toem.impulse.values.AttachedRelation;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.json.IJsonBase;
import de.toem.toolkits.pattern.json.Json;

public class AttachedLabel
extends AbstractAttachment
implements IAttachment.IAttachedLabel {
    public AttachedLabel(String style, long sourceUnits, int sourceIdx, int sourceGroup, int sourceLayer) {
        super(style, sourceUnits, sourceIdx, sourceGroup, sourceLayer);
    }

    public AttachedLabel(Object jsonObject) {
        super(jsonObject);
    }

    @Override
    public Object toJson(IJsonBase.IObjectConversion<Object, Object> conversion) {
        Object o = super.toJson(conversion);
        Json.put(o, "__", (Number)2);
        return o;
    }

    @Override
    public String getSymbol() {
        return this.getStyleContent(2);
    }

    @Override
    public String format(int format) {
        return this.getMessage();
    }

    public boolean equals(Object obj) {
        if (obj instanceof AttachedRelation) {
            AttachedRelation that = (AttachedRelation)obj;
            return Utils.equals(this.style, that.style);
        }
        return false;
    }

    public String toString() {
        return "->" + this.style;
    }
}

