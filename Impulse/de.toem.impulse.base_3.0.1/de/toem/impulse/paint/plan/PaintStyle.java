/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.paint.IPaint;
import de.toem.impulse.paint.IPaintStyle;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Link;

public class PaintStyle
implements IPaint,
IPaintStyle,
Cloneable {
    public int diagramType;
    public int diagramMods;
    public int format;
    public int valueColumnFormat;
    public int scaleType;
    public String scaleUnit;
    public double scaleFrom;
    public double scaleTo;
    public Link descriptor;
    public String[][] additional;

    public PaintStyle() {
    }

    public PaintStyle(PaintStyle style) {
        this.diagramType = style.diagramType;
        this.diagramMods = style.diagramMods;
        this.format = style.format;
        this.valueColumnFormat = style.valueColumnFormat;
        this.scaleFrom = style.scaleFrom;
        this.scaleTo = style.scaleTo;
        this.scaleType = style.scaleType;
        this.scaleUnit = style.scaleUnit;
        this.descriptor = style.descriptor;
        this.additional = style.additional;
    }

    @Override
    public PaintStyle clone() {
        return new PaintStyle(this);
    }

    public String toString() {
        return this.diagramType + "/" + this.diagramMods + "/" + this.format + "/" + this.valueColumnFormat + "/" + this.scaleType + "/" + this.scaleUnit + "/" + this.scaleFrom + "/" + this.scaleTo;
    }

    public static PaintStyle parse(String value) {
        String[] splitted;
        PaintStyle style = new PaintStyle();
        String[] stringArray = splitted = value != null ? value.split("/") : null;
        if (splitted != null && splitted.length > 7) {
            style.diagramType = Utils.parseInt(splitted[0], 0);
            style.diagramMods = Utils.parseInt(splitted[1], 0);
            style.format = Utils.parseInt(splitted[2], 0);
            style.valueColumnFormat = Utils.parseInt(splitted[3], 0);
            style.scaleType = Utils.parseInt(splitted[4], 0);
            style.scaleUnit = splitted[5];
            style.scaleFrom = Utils.parseDouble(splitted[6], 0.0);
            style.scaleTo = Utils.parseDouble(splitted[7], 0.0);
        }
        return style;
    }

    @Override
    public boolean hasValueAxis() {
        return (this.diagramType == 4 || this.diagramType == 9) && (this.diagramMods & 0x80) != 0 || this.diagramType == 8;
    }

    @Override
    public int getType() {
        return this.diagramType;
    }

    @Override
    public boolean hasMod(int modifier) {
        return (this.diagramMods & modifier) != 0;
    }

    @Override
    public int getFormat() {
        return this.format;
    }

    @Override
    public int getValueColumnFormat() {
        return this.valueColumnFormat;
    }

    @Override
    public int getMods() {
        return this.diagramMods;
    }

    @Override
    public double getScaleFrom() {
        return this.scaleFrom;
    }

    @Override
    public double getScaleTo() {
        return this.scaleTo;
    }

    @Override
    public int getScaleType() {
        return this.scaleType;
    }

    @Override
    public String getScaleUnit() {
        return this.scaleUnit;
    }

    @Override
    public Link getDescriptor() {
        return this.descriptor;
    }

    @Override
    public String[][] getAdditional() {
        return this.additional;
    }

    public boolean equals(Object obj) {
        if (obj instanceof PaintStyle) {
            PaintStyle that = (PaintStyle)obj;
            if (this.diagramType != that.diagramType) {
                return false;
            }
            if (this.diagramMods != that.diagramMods) {
                return false;
            }
            if (this.format != that.format) {
                return false;
            }
            if (this.valueColumnFormat != that.valueColumnFormat) {
                return false;
            }
            if (this.scaleFrom != that.scaleFrom) {
                return false;
            }
            if (this.scaleTo != that.scaleTo) {
                return false;
            }
            if (this.scaleType != that.scaleType) {
                return false;
            }
            if (!Utils.equals(this.scaleUnit, that.scaleUnit)) {
                return false;
            }
            if (!Utils.equals(this.descriptor, that.descriptor)) {
                return false;
            }
            return Utils.equals(this.additional, that.additional);
        }
        return false;
    }
}

