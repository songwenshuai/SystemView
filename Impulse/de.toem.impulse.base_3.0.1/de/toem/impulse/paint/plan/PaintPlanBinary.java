/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.Axes;
import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.axis.IValueAxis;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.samples.ISamples;
import de.toem.toolkits.core.Base64;
import de.toem.toolkits.pattern.json.IJsonBase;
import de.toem.toolkits.pattern.json.Json;
import de.toem.toolkits.text.MultilineText;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.swt.graphics.Rectangle;

public class PaintPlanBinary
implements IPlan.IPaintPlanBinary,
IJsonBase {
    protected ITreeItem item;
    protected IDomainAxis axis;
    protected Rectangle area;
    protected int style;
    protected int scheme;
    protected int layers;
    protected int xEnd;
    protected ISamples.TagDomain tagDomain;
    protected IValueAxis vaxis;
    protected String script;
    protected String status;
    protected byte[] pPaints;
    protected int pLen;
    protected byte[] iPaints;
    protected int iLen;
    protected List<String> values = new ArrayList<String>();
    protected int vLen;
    protected int delta;
    protected String dStatus;
    protected int dpLen;
    protected int diLen;
    protected int dvLen;

    protected PaintPlanBinary() {
    }

    protected PaintPlanBinary(IPlotItem item, IDomainAxis axis, Rectangle area, int style, int scheme) {
        this.item = item;
        this.axis = axis;
        this.area = area;
        this.style = style;
        this.scheme = scheme;
    }

    protected PaintPlanBinary(PaintPlanBinary that) {
        this.item = that.item;
        this.axis = that.axis;
        this.area = that.area;
        this.style = that.style;
        this.scheme = that.scheme;
        this.assign(that);
    }

    protected PaintPlanBinary(Object jsonObject, IJsonBase.IObjectConversion<Object, Object> replacement) {
        try {
            this.delta = Json.get(jsonObject, 0, "delta");
            if (this.delta == 0) {
                this.item = (ITreeItem)(replacement != null ? replacement.replace(Json.get(jsonObject, 0, "it")) : null);
                this.axis = Axes.parseDomainAxis(Json.get(jsonObject, null, "as"));
                this.area = new Rectangle(Json.get(jsonObject, 0, "ax"), Json.get(jsonObject, 0, "ay"), Json.get(jsonObject, 0, "aw"), Json.get(jsonObject, 0, "ah"));
                this.style = Json.get(jsonObject, 0, "ss");
                this.scheme = Json.get(jsonObject, 0, "sh");
                this.layers = Json.get(jsonObject, 0, "la");
                this.xEnd = Json.get(jsonObject, 0, "xe");
                this.vaxis = Json.get(jsonObject, "vs", o -> o instanceof String ? Axes.parseValueAxis((String)o) : null);
                this.script = Json.get(jsonObject, null, "sc");
            }
            if (this.delta == 0 || (this.delta & 1) != 0) {
                this.status = Json.get(jsonObject, null, "st");
            }
            if (this.delta == 0) {
                this.pPaints = Json.get(jsonObject, "pp", o -> o instanceof String ? Base64.decode((String)o) : null);
                this.pLen = this.pPaints != null ? this.pPaints.length : 0;
                this.iPaints = Json.get(jsonObject, "ip", o -> o instanceof String ? Base64.decode((String)o) : null);
                this.iLen = this.iPaints != null ? this.iPaints.length : 0;
                Object vl = Json.get(jsonObject, "vl");
                int n = 0;
                while (n < Json.length(vl)) {
                    this.values.add(Json.get(vl, n, o -> String.valueOf(o)));
                    ++n;
                }
            }
        }
        catch (Throwable throwable) {}
    }

    protected void assign(PaintPlanBinary that) {
        if (that != null) {
            this.delta = that.delta;
            if (that.delta == 0) {
                this.scheme = that.scheme;
                this.layers = that.layers;
                this.xEnd = that.xEnd;
                this.tagDomain = that.tagDomain;
                this.vaxis = that.vaxis;
                this.script = that.script;
            }
            if (this.delta == 0 || (this.delta & 1) != 0) {
                this.status = that.status;
            }
            if (this.delta == 0) {
                this.pPaints = that.pPaints;
                this.pLen = that.pLen;
                this.iPaints = that.iPaints;
                this.iLen = that.iLen;
                this.values = that.values;
            }
        }
    }

    @Override
    public Map<String, Object> toJson(IJsonBase.IObjectConversion<Object, Object> replacement) {
        HashMap<String, Object> map = new HashMap<String, Object>();
        map.put("delta", this.delta);
        if (this.delta == 0) {
            map.put("it", replacement != null ? replacement.replace(this.item) : this.item);
            map.put("as", this.axis.toString());
            map.put("ax", this.area.x);
            map.put("ay", this.area.y);
            map.put("aw", this.area.width);
            map.put("ah", this.area.height);
            map.put("sy", this.style);
            map.put("sh", this.scheme);
            if (this.layers != 0) {
                map.put("la", this.layers);
            }
            map.put("xe", this.xEnd);
            if (this.tagDomain != null) {
                map.put("td", this.tagDomain.toString());
            }
            if (this.vaxis != null) {
                map.put("vs", this.vaxis.toString());
            }
            if (this.script != null) {
                map.put("sc", MultilineText.toAscii(this.script));
            }
        }
        if ((this.delta == 0 || (this.delta & 1) != 0) && this.status != null) {
            map.put("st", this.status);
        }
        if (this.delta == 0 && this.pPaints != null) {
            map.put("pp", String.valueOf(Base64.encode(this.pPaints, this.pLen)));
        }
        if (this.delta == 0 && this.iPaints != null) {
            map.put("ip", String.valueOf(Base64.encode(this.iPaints, this.iLen)));
        }
        if (this.delta == 0 && !this.values.isEmpty()) {
            map.put("vl", this.values);
        }
        return map;
    }

    protected void extend(int delta) {
        this.delta = delta;
        this.dpLen = this.pLen;
        this.diLen = this.iLen;
        this.dvLen = this.vLen;
    }

    @Override
    public ITreeItem getItem() {
        return this.item;
    }

    @Override
    public IDomainAxis getAxis() {
        return this.axis;
    }

    @Override
    public Rectangle getArea() {
        return this.area;
    }

    @Override
    public int getStyle() {
        return this.style;
    }

    @Override
    public int getScheme() {
        return this.scheme;
    }

    @Override
    public ITheme getTheme() {
        return this.item != null && this.item.getTree() != null ? this.item.getTree().getTheme() : null;
    }
}

