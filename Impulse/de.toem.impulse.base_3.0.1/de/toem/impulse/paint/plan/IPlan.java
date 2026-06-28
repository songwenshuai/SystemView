/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.axis.IValueAxis;
import de.toem.impulse.paint.IPaint;
import de.toem.impulse.paint.IPaintStyle;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.pattern.js.JsMethod;
import de.toem.toolkits.pattern.json.IJsonBase;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.tlk.ITlkPainter;
import java.util.Iterator;
import java.util.List;
import org.eclipse.swt.graphics.Rectangle;

public interface IPlan {
    public static final int SCHEME_AREA_XTEND = 1;
    public static final int SCHEME_APPLICABALE_STYLE_MATCH = 16;
    public static final int SCHEME_APPLICABALE_AXISAREA_MATCH = 32;
    public static final int SCHEME_APPLICABALE_SAMPLES_MATCH = 64;
    public static final int SCHEME_APPLICABALE_PAINTSTYLE_MATCH = 128;
    public static final int SCHEME_APPLICABALE_AREA_MATCH = 256;
    public static final int SCHEME_APPLICABALE_ACTIVECURSOR_MATCH = 512;
    public static final int SCHEME_APPLICABALE_COLOR_MATCH = 1024;
    public static final int SCHEME_IMAGE_APPLICABALE_STYLE_MATCH = 4096;
    public static final int SCHEME_IMAGE_APPLICABALE_AREA_MATCH = 8192;
    public static final int SCHEME_REUSE_IMAGE = 0x100000;
    public static final int SCHEME_DEFAULT = 4577;
    public static final int CHECK_INVALID = 0;
    public static final int CHECK_VALID = 1;
    public static final int CHECK_REQUIRES_PLANNING = 2;
    public static final int CHECK_UPDATED = 4;
    public static final int CHECK_IS_PLANNING = 8;
    public static final int CHECK_AND_MASK = 1;
    public static final int CHECK_OR_MASK = 14;
    public static final int CHECK_UNKNOWN = 1;
    public static final int YIELD_NONE = 0;
    public static final int YIELD_APPLICABLE = 1;
    public static final int YIELD_STRICT = 2;
    public static final int YIELD_MODIFIED = 4;
    public static final int YIELD_IMAGE_APPLICABLE = 16;
    public static final int YIELD_IMAGE_STRICT = 32;
    public static final int YIELD_IMAGE_MODIFIED = 64;
    public static final int YIELD_AND_MASK = 51;
    public static final int YIELD_OR_MASK = 68;
    public static final int YIELD_UNKNOWN = 51;
    public static final int CHANGED_NONE = 0;
    public static final int CHANGED_AXIS = 1;
    public static final int CHANGED_AREA = 2;
    public static final int CHANGED_STYLE = 4;
    public static final int CHANGED_PLAN = 8;
    public static final int CHANGED_SAMPLES = 16;
    public static final int CHANGED_PAINTSTYLE = 32;
    public static final int CHANGED_SELECTION = 64;
    public static final int CHANGED_ACTIVE_CURSOR = 128;
    public static final int CHANGED_COLOR = 256;
    public static final int CHANGED_PAINTING = 512;

    public static int CHECK_MERGE(int check1, int check2) {
        return check1 & check2 & 1 | (check1 | check2) & 0xE;
    }

    public static int CHECK_ADD(int check1, int check2) {
        return check1 | check2;
    }

    public static int YIELD_MERGE(int yield1, int yield2) {
        return yield1 & yield2 & 0x33 | (yield1 | yield2) & 0x44;
    }

    public static int YIELD_ADD(int yield1, int yield2) {
        return yield1 | yield2;
    }

    public static int YIELD_REMOVE(int yield1, int yield2) {
        return yield1 & ~yield2;
    }

    public static interface IExternalPlanner<T> {
        public void plan(ISinglePaintPlanner<IPlotItem> var1, IProgress var2, IPaintPlanGenerator var3, IPropertyModel var4, IDomainAxis var5, Rectangle var6, boolean var7);

        public int getPriority();
    }

    public static interface IMultiPaintPlanner<T extends ITreeItem>
    extends IPaintPlanner<T> {
        public Rectangle childArea(ISinglePaintPlanner<T> var1);
    }

    public static interface IPaintPlan
    extends IPaintPlanBinary {
        public IPaintStyle getPaintStyle();

        public boolean hasPaintStyle();

        public boolean isEmpty();

        public boolean hasValueAxis();

        public IValueAxis getValueAxis();

        public int isApplicable(int var1, IDomainAxis var2, Rectangle var3, int var4, IPaint.CachedImage var5);

        public int yields(IDomainAxis var1, Rectangle var2, int var3, Object var4);

        public boolean hasMessage();

        public String getMessage();

        @Deprecated
        public int getEndX();

        public ISamples.TagDomain getTagDomaim();

        @Deprecated
        public int getLayers();

        @Deprecated
        public double getA();

        public boolean hasScripting();

        public Object invoke(String var1, ITlkPainter var2, IPaintPlanIterator var3, ITreeItem var4, Rectangle var5, Object ... var6);

        @JsMethod
        public String getStatus();
    }

    public static interface IPaintPlanBinary
    extends IPlanBasics {
        public static final int MAX_PAR = 6;
        public static final int MAX_X = 6;
        public static final int MAX_XN = 4;
        public static final int FLAG_XT_MASK = 192;
        public static final int FLAG_XT_SHIFT = 6;
        public static final int FLAG_XT_NONE = 0;
        public static final int FLAG_XT_1 = 64;
        public static final int FLAG_XT_2 = 128;
        public static final int FLAG_XT_6 = 192;
        public static final int FLAG_PAR_MASK = 48;
        public static final int FLAG_PAR_SHIFT = 4;
        public static final int FLAG_PAR_NONE = 0;
        public static final int FLAG_PAR_2 = 16;
        public static final int FLAG_PAR_4 = 32;
        public static final int FLAG_PAR_8 = 48;
        public static final int FLAG_IDX_MASK = 12;
        public static final int FLAG_IDX_SHIFT = 2;
        public static final int FLAG_IDX_NONE = 0;
        public static final int FLAG_IDX_1BYTE = 4;
        public static final int FLAG_IDX_2BYTE = 8;
        public static final int FLAG_IDX_4BYTE = 12;
        public static final int FLAG_VAL_MASK = 3;
        public static final int FLAG_VAL_SHIFT = 0;
        public static final int FLAG_VAL_NONE = 0;
        public static final int FLAG_VAL_1 = 1;
        public static final int FLAG_VAL_2 = 2;
        public static final int FLAG_VAL_N = 3;

        public ITreeItem getItem();

        public int getScheme();
    }

    public static interface IPaintPlanGenerator
    extends ISinglePaintPlanProvision,
    IPaintPlanBinary {
        public static final int SCALE_LINEAR = 0;
        public static final int SCALE_LOG10 = 1;

        @Override
        public ITreeItem getItem();

        public boolean isEmpty();

        public void setTagDomain(ISamples.TagDomain var1);

        public void setNoOfLayers(int var1);

        @Deprecated
        public void extendToInfinity(boolean var1, boolean var2);

        public void setVaxis(IValueAxis var1);

        public void setScheme(int var1);

        public void addScheme(int var1);

        public void removeScheme(int var1);

        public void setScript(String var1);

        public void setStatus(String var1);

        public void add(int var1, int var2, int var3);

        public void add(int var1, int var2, int var3, String var4);

        public void add(int var1, int var2, int var3, byte[] var4);

        public void add(int var1, int var2, int var3, List<String> var4);

        public void add(int var1, int var2, short var3, short var4, int var5, String var6);

        public void add(int var1, int[] var2, short var3, short var4, int var5, List<String> var6);

        public void add(int var1, int var2, short var3, short var4, double var5, int var7);

        public void add(int var1, int var2, double var3, int var5);

        public void add(int var1, int var2, double var3, double var5, int var7);

        public void add(int var1, int var2, double var3, double var5, double var7, double var9, int var11);

        public double[] getScaleBorder();

        public void applyScale(int var1, String var2, Double var3, Double var4);

        public void setEndX(int var1);

        public void attach(int var1, int var2, int var3, IAttachment var4);

        public void attach(int var1, int var2, Double var3, int var4, IAttachment var5);

        public void applyAttachments();

        public void extend(int var1);
    }

    public static interface IPaintPlanIterator
    extends IPaintPlanBinary,
    Iterator<Integer> {
        @Override
        @JsMethod
        public boolean hasNext();

        @JsMethod
        public boolean hasPrev();

        @Override
        @JsMethod
        public Integer next();

        @JsMethod
        public Integer prev();

        @JsMethod
        public int current();

        @JsMethod
        public int paint();

        @JsMethod
        public int nextPaint();

        @JsMethod
        public int prevPaint();

        @JsMethod
        public boolean hasX();

        @JsMethod
        public int x();

        @JsMethod
        public boolean hasX2();

        @JsMethod
        public int x2();

        @JsMethod
        public boolean hasType();

        @JsMethod
        public int type();

        @JsMethod
        public boolean hasXn();

        @JsMethod
        public int xn(int var1);

        @JsMethod
        public boolean hasPosition();

        @JsMethod
        public long position();

        @JsMethod
        public int nextX();

        @JsMethod
        public int prevX();

        @JsMethod
        public boolean hasP();

        @JsMethod
        public boolean has2P();

        @JsMethod
        public boolean has4P();

        @JsMethod
        public boolean has8P();

        @JsMethod
        public short p();

        @JsMethod
        public short p2();

        @JsMethod
        public short p3();

        @JsMethod
        public short p4();

        @JsMethod
        public short p5();

        @JsMethod
        public short p6();

        @JsMethod
        public short p7();

        @JsMethod
        public short p8();

        @JsMethod
        public boolean hasA();

        @JsMethod
        public boolean hasAb();

        @JsMethod
        public boolean hasAbcd();

        @JsMethod
        public boolean hasNextAb();

        @JsMethod
        public double a();

        @JsMethod
        public double b();

        @JsMethod
        public double c();

        @JsMethod
        public double d();

        @JsMethod
        public double nextA();

        @JsMethod
        public double nextB();

        @JsMethod
        public int index();

        @JsMethod
        public boolean hasVal();

        @JsMethod
        public int noOfVals();

        @JsMethod
        public String val();

        @JsMethod
        public byte[] bval();

        @JsMethod
        public String valAt(int var1);
    }

    public static interface IPaintPlanProvision
    extends IJsonBase {
        public static final int DELTA_NONE = 0;
        public static final int DELTA_STATUS = 1;
        public static final int DELTA_PAINTS = 2;

        public int size();

        public ITreeItem getItem(int var1);

        public IPaintPlan preparePlan(int var1);

        public boolean isDelta();

        public int getDelta();

        public IPaintPlanProvision extend(IPaintPlanProvision var1);
    }

    public static interface IPaintPlanner<T extends ITreeItem>
    extends IPlanRequest<T> {
        public static final int PRIORITY_HIGH = 5;
        public static final int PRIORITY_MID = 2;
        public static final int PRIORITY_LOW = 0;

        public void dispose();

        public boolean isDisposed();

        public int getPriority();

        public int checkAndUpdate(IDomainAxis var1, Rectangle var2, int var3);

        public void setClient(Object var1, int var2);

        public Object getClient();

        public int getCounter();

        public void setCounter(int var1);

        public IPaintPlanProvision createTimeoutProvision(boolean var1);

        public IPaintPlanProvision plan(IProgress var1);
    }

    public static interface IPlanBasics
    extends IPaint {
        public ITheme getTheme();

        public IDomainAxis getAxis();

        public Rectangle getArea();

        public int getStyle();
    }

    public static interface IPlanListener {
        public static final int NOTIFY_NO_UPDATE_NEEDED = 0;
        public static final int NOTIFY_FILTERED = 1;
        public static final int NOTIFY_DELAYED = 2;
        public static final int NOTIFY_DESTROYED = 3;
        public static final int NOTIFY_FAILED = 4;
    }

    public static interface IPlanRequest<T extends ITreeItem>
    extends IPlanBasics {
        public List<T> getItems();
    }

    public static interface ISinglePaintPlanProvision
    extends IPaintPlanProvision {
        public ITreeItem getItem();

        public IPaintPlan preparePlan();
    }

    public static interface ISinglePaintPlanner<T extends ITreeItem>
    extends IPaintPlanner<T> {
        public T getItem();
    }
}

