/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.values.AbstractAttachment;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.json.IJsonBase;
import de.toem.toolkits.pattern.json.Json;

public class AttachedRelation
extends AbstractAttachment
implements IAttachment.IAttachedRelation {
    private byte type;
    private String target;
    private IDomainBase targetBase;
    private long targetPosition;
    private int targetIdx;
    private byte targetLayer;

    public AttachedRelation(byte type, String style, String target, long targetUnits, IDomainBase targetBase, int targetIdx, int targetLayer, long sourceUnits, int sourceIdx, int sourceGroup, int sourceLayer) {
        super(style, sourceUnits, sourceIdx, sourceGroup, sourceLayer);
        this.type = type;
        this.target = target;
        this.targetBase = targetBase;
        this.targetPosition = targetUnits;
        this.targetIdx = targetIdx;
        this.targetLayer = (byte)targetLayer;
    }

    public AttachedRelation(Object jsonObject) {
        super(jsonObject);
        this.type = (byte)Json.get(jsonObject, 0, "ty");
        this.target = Json.get(jsonObject, null, "ta");
        this.targetBase = DomainBase.parse(Json.get(jsonObject, null, "tb"));
        this.targetPosition = Json.get(jsonObject, 0L, "tp");
        this.targetIdx = this.hasTargetIdx() ? Json.get(jsonObject, -1, "ti") : -1;
        if (this.hasTargetLayer()) {
            this.targetLayer = (byte)Json.get(jsonObject, 0, "tl");
        }
    }

    @Override
    public Object toJson(IJsonBase.IObjectConversion<Object, Object> conversion) {
        Object o = super.toJson(conversion);
        Json.put(o, "__", (Number)3);
        Json.put(o, "ty", (Number)this.type);
        Json.put(o, "ta", this.target);
        Json.put(o, "tb", this.targetBase.toString());
        Json.put(o, "tp", (Number)this.targetPosition);
        if (this.hasTargetIdx()) {
            Json.put(o, "ti", (Number)this.targetIdx);
        }
        if (this.hasTargetLayer() && this.targetLayer != 0) {
            Json.put(o, "tl", (Number)this.targetLayer);
        }
        return o;
    }

    @Override
    public int getType() {
        return this.type;
    }

    @Override
    public boolean isReverse() {
        return (this.type & 1) != 0;
    }

    @Override
    public boolean isDelta() {
        return (this.type & 2) == 0;
    }

    @Override
    public boolean hasTargetIdx() {
        return (this.type & 4) != 0;
    }

    @Override
    public boolean hasTargetLayer() {
        return (this.type & 8) != 0;
    }

    @Override
    public String getTargetId() {
        return this.target;
    }

    @Override
    public IDomainBase getTargetBase() {
        return this.targetBase;
    }

    @Override
    public long getTargetPosition() {
        return this.targetPosition;
    }

    @Override
    public int getTargetIdx() {
        return this.targetIdx;
    }

    @Override
    public int getTargetLayer() {
        return this.targetLayer;
    }

    @Override
    public Link getLink() {
        long p = this.isDelta() ? this.sourceUnits + this.targetPosition : this.targetPosition;
        Link link = Link.parse(this.target);
        if (this.targetBase != null) {
            link.setParameter(this.targetBase.getDomainLabel(), new DomainValue(this.targetBase, p).toString());
        } else {
            link.setParameter("units", String.valueOf(p));
        }
        if (this.hasTargetIdx()) {
            link.setParameter("idx", String.valueOf(this.targetIdx));
        }
        return link;
    }

    @Override
    public long getAbsoluteTargetUnits(IDomainBase domainBase) {
        long p;
        long l = p = this.isDelta() ? this.sourceUnits + this.targetPosition : this.targetPosition;
        if (this.targetBase != null) {
            return this.targetBase.convertTo(domainBase, p);
        }
        return p;
    }

    @Override
    public String getLineStyle() {
        return this.getStyleContent(2);
    }

    @Override
    public String getArrowStyle() {
        return this.getStyleContent(3);
    }

    @Override
    public String getSymbol() {
        return this.getStyleContent(4);
    }

    public String getSourceLabel() {
        return this.getStyleContent(5);
    }

    public String getTargetLabel() {
        return this.getStyleContent(6);
    }

    @Override
    public String getMessage() {
        String message = super.getMessage();
        if (message.contains("${target}")) {
            message = message.replace("${target}", this.getTargetId());
        }
        return message;
    }

    @Override
    public String format(int format) {
        String target;
        String source = this.getSourceLabel();
        if (source == null) {
            source = "";
        }
        if (this.sourceIdx >= 0) {
            source = String.valueOf(source) + "@" + this.sourceIdx;
        }
        if (this.sourceGroup >= 0) {
            source = String.valueOf(source) + "#" + this.sourceGroup;
        }
        if ((target = this.getTargetLabel()) == null) {
            target = this.getTargetId();
        }
        if (this.hasTargetIdx()) {
            target = String.valueOf(target) + "@" + this.targetIdx + " ";
        }
        target = !this.isDelta() ? (this.targetBase != null ? String.valueOf(target) + this.targetBase.toString(this.targetPosition, 14) : String.valueOf(target) + this.targetPosition) : (this.targetBase != null ? String.valueOf(target) + "\u0394" + this.targetBase.toString(this.targetPosition, 30) : String.valueOf(target) + "\u0394" + this.targetPosition);
        if (format == -1 || format == 3) {
            return String.valueOf(source) + (this.isReverse() ? "\u2190" : "\u2192") + this.getMessage() + (this.isReverse() ? "\u2190" : "\u2192") + target;
        }
        if (format == 2) {
            return String.valueOf(this.getMessage()) + (this.isReverse() ? "\u2190" : "\u2192") + target;
        }
        if (format == 1) {
            return target;
        }
        return "";
    }

    public boolean equals(Object obj) {
        if (obj instanceof AttachedRelation) {
            AttachedRelation that = (AttachedRelation)obj;
            if (!Utils.equals(this.style, that.style)) {
                return false;
            }
            if (this.type != that.type) {
                return false;
            }
            if (!Utils.equals(this.target, that.target)) {
                return false;
            }
            if (this.targetBase != that.targetBase) {
                return false;
            }
            if (this.sourceUnits != that.sourceUnits) {
                return false;
            }
            return this.targetPosition == that.targetPosition;
        }
        return false;
    }

    public String toString() {
        return "->" + this.target + "?" + this.style + "@" + this.targetPosition;
    }
}

