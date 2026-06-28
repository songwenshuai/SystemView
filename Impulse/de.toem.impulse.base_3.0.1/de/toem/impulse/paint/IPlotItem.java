/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint;

import de.toem.impulse.paint.IPaintStyle;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.plan.PaintStyle;
import de.toem.impulse.provider.ISimpleSamplesProvider;
import de.toem.impulse.samples.IReadableSamples;

public interface IPlotItem
extends ITreeItem,
ISimpleSamplesProvider {
    public static final int CHANGED_SAMPLES = 0x1000000;
    public static final int CHANGED_PAINTSTYLE = 0x2000000;
    public static final String Samples = "Samples";
    public static final String PaintStyle = "PaintStyle";
    public static final String[] Contents = new String[]{"Samples", "PaintStyle"};

    public int getSamplesRelease();

    public boolean setSamples(IReadableSamples var1);

    @Override
    public PaintStyle getPaintStyle();

    public boolean setPaintStyle(IPaintStyle var1, boolean var2);
}

