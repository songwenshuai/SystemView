/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.plan.AbstractPaintPlanner;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.MultiPaintPlanner;
import de.toem.impulse.paint.plan.PaintStyle;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.eclipse.swt.graphics.Rectangle;

public abstract class SinglePaintPlanner
extends AbstractPaintPlanner
implements IPlan.ISinglePaintPlanner<IPlotItem> {
    protected IPlan.IMultiPaintPlanner<IPlotItem> master;
    protected IPlotItem item;
    protected PaintStyle paintStyle;
    protected boolean labelsLocked;
    protected boolean relationLocked;
    protected IReadableSamples samples;
    protected int samplesRelease;
    protected long end;
    protected int minSampleWidth;
    private IPlan.IPaintPlanGenerator generator;

    protected SinglePaintPlanner(ITheme theme, IPlotItem item, IDomainAxis axis, Rectangle area, int style) {
        super(theme, axis, area, style);
        this.item = item;
    }

    protected SinglePaintPlanner(MultiPaintPlanner master, IPlotItem item, Rectangle area) {
        super(master, area);
        this.master = master;
        this.item = item;
    }

    protected SinglePaintPlanner(MultiPaintPlanner master, IPlotItem item) {
        super(master, null);
        this.master = master;
        this.item = item;
    }

    @Override
    public void dispose() {
        super.dispose();
    }

    @Override
    public IPlotItem getItem() {
        return this.item;
    }

    @Override
    public List<IPlotItem> getItems() {
        return this.item != null ? Arrays.asList(this.item) : Collections.EMPTY_LIST;
    }

    protected PaintStyle getPaintStyle() {
        return this.paintStyle;
    }

    protected boolean ensureSettled(IProgress p) {
        return this.samples != null ? this.samples.ensureSettled(p) : false;
    }

    @Override
    public synchronized int checkAndUpdate(IDomainAxis axis, Rectangle area, int style) {
        if (this.state == -2) {
            return 0;
        }
        int checked = super.checkAndUpdate(axis, area, style);
        checked = IPlan.CHECK_MERGE(checked, this.checkAndUpdateReadable(style));
        checked = IPlan.CHECK_MERGE(checked, this.checkAndUpdatePaintStyle(style));
        return checked;
    }

    protected int checkAndUpdateReadable(int style) {
        int checked = 1;
        if ((this.scheme & 0x40) != 0 && this.state != 0) {
            if (this.samples != this.item.getSamples()) {
                return 0;
            }
            if (this.samples != null && this.samplesRelease != this.samples.getRelease()) {
                return 0;
            }
        }
        return checked;
    }

    protected int checkAndUpdatePaintStyle(int style) {
        int checked = 1;
        if (!((this.scheme & 0x80) == 0 || this.state == 0 || Utils.equals(this.paintStyle, this.item.getPaintStyle()) && this.paintStyle.getType() != 8)) {
            return 0;
        }
        return checked;
    }

    @Override
    public IPlan.IPaintPlanProvision plan(IProgress p) {
        return this.plan(p, CLONE ? (this.axis != null ? this.axis.clone() : null) : this.axis);
    }

    /*
     * Exception decompiling
     */
    public IPlan.ISinglePaintPlanProvision plan(IProgress p, IDomainAxis planAxis) {
        /*
         * This method has failed to decompile.  When submitting a bug report, please provide this stack trace, and (if you hold appropriate legal rights) the relevant class file.
         * 
         * org.benf.cfr.reader.util.ConfusedCFRException: Started 3 blocks at once
         *     at org.benf.cfr.reader.bytecode.analysis.opgraph.Op04StructuredStatement.getStartingBlocks(Op04StructuredStatement.java:412)
         *     at org.benf.cfr.reader.bytecode.analysis.opgraph.Op04StructuredStatement.buildNestedBlocks(Op04StructuredStatement.java:487)
         *     at org.benf.cfr.reader.bytecode.analysis.opgraph.Op03SimpleStatement.createInitialStructuredBlock(Op03SimpleStatement.java:736)
         *     at org.benf.cfr.reader.bytecode.CodeAnalyser.getAnalysisInner(CodeAnalyser.java:850)
         *     at org.benf.cfr.reader.bytecode.CodeAnalyser.getAnalysisOrWrapFail(CodeAnalyser.java:278)
         *     at org.benf.cfr.reader.bytecode.CodeAnalyser.getAnalysis(CodeAnalyser.java:201)
         *     at org.benf.cfr.reader.entities.attributes.AttributeCode.analyse(AttributeCode.java:94)
         *     at org.benf.cfr.reader.entities.Method.analyse(Method.java:531)
         *     at org.benf.cfr.reader.entities.ClassFile.analyseMid(ClassFile.java:1055)
         *     at org.benf.cfr.reader.entities.ClassFile.analyseTop(ClassFile.java:942)
         *     at org.benf.cfr.reader.Driver.doJarVersionTypes(Driver.java:257)
         *     at org.benf.cfr.reader.Driver.doJar(Driver.java:139)
         *     at org.benf.cfr.reader.CfrDriverImpl.analyse(CfrDriverImpl.java:76)
         *     at org.benf.cfr.reader.Main.main(Main.java:54)
         */
        throw new IllegalStateException("Decompilation failed");
    }

    protected String checkPlan() {
        return null;
    }

    protected abstract void createPlan(IProgress var1, IPlan.IPaintPlanGenerator var2, IDomainAxis var3, Rectangle var4, boolean var5);

    protected IPlan.ISinglePaintPlanProvision createErrorProvision(String message) {
        return this.createErrorProvision(this.item, message);
    }

    protected IPlan.ISinglePaintPlanProvision createCancelProvision(IProgress p) {
        return this.createCancelProvision(this.item, p);
    }

    @Override
    public IPlan.ISinglePaintPlanProvision createTimeoutProvision(boolean killed) {
        return this.createTimeoutProvision(this.item, killed);
    }
}

