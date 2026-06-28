/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.values.AttachedLabel;
import de.toem.impulse.values.AttachedRelation;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.pattern.json.IJsonBase;
import de.toem.toolkits.pattern.json.Json;

public abstract class AbstractAttachment
implements IAttachment {
    protected String style;
    private String[] styles;
    protected long sourceUnits;
    protected int sourceIdx;
    protected int sourceGroup;
    protected byte sourceLayer;
    public static final int CLASS_MEMBERS = 1;
    public static final int CLASS_LABEL = 2;
    public static final int CLASS_RELATION = 3;

    public AbstractAttachment(String style, long sourceUnits, int sourceIdx, int sourceGroup, int sourceLayer) {
        this.style = style;
        this.sourceUnits = sourceUnits;
        this.sourceIdx = sourceIdx;
        this.sourceGroup = sourceGroup;
        this.sourceLayer = (byte)sourceLayer;
    }

    public static AbstractAttachment toJava(Object jsonObject) {
        if (jsonObject != null) {
            switch (Json.get(jsonObject, 0, "__")) {
                case 2: {
                    return new AttachedLabel(jsonObject);
                }
                case 3: {
                    return new AttachedRelation(jsonObject);
                }
            }
        }
        return null;
    }

    public AbstractAttachment(Object jsonObject) {
        this.style = Json.get(jsonObject, null, "st");
        this.sourceUnits = Json.get(jsonObject, 0L, "sp");
        this.sourceIdx = Json.get(jsonObject, -1, "si");
        this.sourceGroup = Json.get(jsonObject, -1, "sg");
        this.sourceLayer = (byte)Json.get(jsonObject, 0, "sl");
    }

    @Override
    public Object toJson(IJsonBase.IObjectConversion<Object, Object> conversion) {
        Object o = Json.newObject();
        Json.put(o, "st", this.style);
        Json.put(o, "sp", (Number)this.sourceUnits);
        Json.put(o, "si", (Number)this.sourceIdx);
        if (this.sourceGroup >= 0) {
            Json.put(o, "sg", (Number)this.sourceGroup);
        }
        if (this.sourceGroup > 0) {
            Json.put(o, "sl", (Number)this.sourceLayer);
        }
        return o;
    }

    @Override
    public String getStyle() {
        return this.style;
    }

    protected String getStyleContent(int idx) {
        if (this.styles == null) {
            this.styles = this.style != null ? this.style.trim().split("/") : null;
        }
        return this.styles != null && this.styles.length > idx ? this.styles[idx] : null;
    }

    @Override
    public boolean showMessage() {
        String message = this.getStyleContent(0);
        return message != null && !message.startsWith("#");
    }

    @Override
    public String getMessage() {
        String message = this.getStyleContent(0);
        if (message != null && message.startsWith("#")) {
            return message.substring(1);
        }
        return message;
    }

    @Override
    public int[] getRgb() {
        String color = this.getStyleContent(1);
        if (color != null) {
            try {
                int val = Integer.parseInt(color, 16);
                return new int[]{val >> 16 & 0xFF, val >> 8 & 0xFF, val & 0xFF};
            }
            catch (Throwable throwable) {}
        }
        return null;
    }

    @Override
    public long getSourceUnits() {
        return this.sourceUnits;
    }

    @Override
    public int getSourceIdx() {
        return this.sourceIdx;
    }

    @Override
    public int getSourceGroup() {
        return this.sourceGroup;
    }

    @Override
    public int getSourceLayer() {
        return this.sourceLayer;
    }
}

