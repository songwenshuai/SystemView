/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.control;

import de.toem.toolkits.pattern.js.JsMethod;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;

public class TooltipDataProvider {
    private HoverData hoverData;

    public void clearHoverData() {
        this.hoverData = null;
    }

    @JsMethod
    public void addData(int x, int y, int width, int height, Object data) {
        this.hoverData = new HoverData(new Rectangle(x, y, width, height), data, this.hoverData);
    }

    public void addData(Rectangle area, Object data) {
        this.hoverData = new HoverData(area, data, this.hoverData);
    }

    public String getTooltipData(Point p) {
        HoverData data = this.hoverData;
        while (data != null) {
            if (data.area.contains(p) && data.data instanceof String) {
                return (String)data.data;
            }
            data = data.next;
        }
        return null;
    }

    class HoverData {
        Rectangle area;
        Object data;
        HoverData next;

        public HoverData(Rectangle area, Object data, HoverData next) {
            this.area = area;
            this.data = data;
            this.next = next;
        }
    }
}

