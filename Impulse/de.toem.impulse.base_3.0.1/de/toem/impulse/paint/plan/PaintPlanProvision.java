/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.axis.ValueAxis;
import de.toem.impulse.paint.IPaint;
import de.toem.impulse.paint.IPaintStyle;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.MultiPaintPlanProvision;
import de.toem.impulse.paint.plan.PaintPlanGenerator;
import de.toem.impulse.paint.plan.SinglePaintPlanProvision;
import de.toem.impulse.samples.ISamples;
import de.toem.toolkits.core.Assert;
import de.toem.toolkits.pattern.json.IJsonBase;
import de.toem.toolkits.pattern.json.Json;
import de.toem.toolkits.ui.tlk.ITlkPainter;
import java.util.HashMap;
import org.eclipse.swt.graphics.Rectangle;

public class PaintPlanProvision {
    public static IPlan.IPaintPlanGenerator newPaintPlanGenerator(IPlotItem item, IDomainAxis planAxis, Rectangle planArea, int style, int scheme) {
        return new PaintPlanGenerator(item, planAxis, planArea, style, scheme);
    }

    public static IPlan.ISinglePaintPlanProvision newMessage(IPlotItem item, int scheme, String message) {
        return new MessageProvision(item, scheme, message);
    }

    public static IPlan.IPaintPlanProvision toJava(Object jsonObject, IJsonBase.IObjectConversion<Object, Object> replacement) {
        if (Json.has(jsonObject, "multi")) {
            return new MultiPaintPlanProvision(jsonObject, replacement);
        }
        if (Json.has(jsonObject, "message")) {
            return new MessageProvision(jsonObject, replacement);
        }
        return new SinglePaintPlanProvision(jsonObject, replacement);
    }

    protected static class MessageProvision
    implements IPlan.IPaintPlan,
    IPlan.ISinglePaintPlanProvision {
        private ITreeItem item;
        private String message;
        private int scheme;

        public MessageProvision(ITreeItem item, int scheme, String message) {
            this.item = item;
            this.message = message;
            this.scheme = scheme;
        }

        protected MessageProvision(Object jsonObject, IJsonBase.IObjectConversion<Object, Object> replacement) {
            this.item = (ITreeItem)(replacement != null ? replacement.replace(Json.get(jsonObject, 0, "it")) : null);
            this.scheme = Json.get(jsonObject, 0, "sh");
            this.message = Json.get(jsonObject, null, "message");
        }

        public Object toJson(IJsonBase.IObjectConversion replacement) {
            HashMap<String, Object> map = new HashMap<String, Object>();
            map.put("message", this.message);
            map.put("it", replacement != null ? replacement.replace(this.item) : this.item);
            map.put("sh", this.scheme);
            return map;
        }

        @Override
        public int size() {
            return 1;
        }

        @Override
        public ITreeItem getItem() {
            return this.item;
        }

        @Override
        public IPlan.IPaintPlan preparePlan() {
            return this;
        }

        @Override
        public ITreeItem getItem(int n) {
            return this.item;
        }

        @Override
        public IPlan.IPaintPlan preparePlan(int n) {
            return this;
        }

        @Override
        public int getScheme() {
            return this.scheme;
        }

        @Override
        public IDomainAxis getAxis() {
            return null;
        }

        @Override
        public Rectangle getArea() {
            return null;
        }

        @Override
        public int getStyle() {
            return 0;
        }

        @Override
        public IPaintStyle getPaintStyle() {
            return null;
        }

        @Override
        public boolean hasPaintStyle() {
            return false;
        }

        @Override
        public boolean isEmpty() {
            return true;
        }

        @Override
        public boolean hasValueAxis() {
            return false;
        }

        @Override
        public ValueAxis getValueAxis() {
            return null;
        }

        @Override
        public int getEndX() {
            return 0;
        }

        @Override
        public ISamples.TagDomain getTagDomaim() {
            return ISamples.TagDomain.Unknown;
        }

        @Override
        public int getLayers() {
            return 0;
        }

        @Override
        public double getA() {
            return 0.0;
        }

        @Override
        public int yields(IDomainAxis axis, Rectangle area, int style, Object image) {
            if (image instanceof Rectangle) {
                boolean copy;
                Rectangle newBounds = area;
                Rectangle newImageBounds = (Rectangle)image;
                boolean bl = copy = newImageBounds.width == 0;
                if (newImageBounds.width != 0 && !newImageBounds.equals(newBounds)) {
                    if ((newBounds = newImageBounds.intersection(newBounds)) == null || newBounds.width == 0 || newBounds.height == 0) {
                        return 0;
                    }
                    copy = true;
                }
                if (copy) {
                    newImageBounds.x = newBounds.x;
                    newImageBounds.y = newBounds.x;
                    newImageBounds.width = newBounds.width;
                    newImageBounds.height = newBounds.height;
                }
            }
            return 3;
        }

        @Override
        public int isApplicable(int changed, IDomainAxis axis, Rectangle area, int style, IPaint.CachedImage imageInfo) {
            Assert.isTrue(axis != null && area != null);
            int mask = 48;
            if (changed != 0 && (changed & mask) != 0) {
                return 0;
            }
            return 3;
        }

        @Override
        public ITheme getTheme() {
            return null;
        }

        @Override
        public boolean hasMessage() {
            return true;
        }

        @Override
        public String getMessage() {
            return this.message;
        }

        @Override
        public boolean hasScripting() {
            return false;
        }

        @Override
        public Object invoke(String function, ITlkPainter painter, IPlan.IPaintPlanIterator iter, ITreeItem item, Rectangle area, Object ... args) {
            return null;
        }

        @Override
        public String getStatus() {
            return null;
        }

        @Override
        public boolean isDelta() {
            return false;
        }

        @Override
        public int getDelta() {
            return 0;
        }

        @Override
        public IPlan.IPaintPlanProvision extend(IPlan.IPaintPlanProvision previous) {
            if (!this.isDelta()) {
                return this;
            }
            return null;
        }
    }
}

