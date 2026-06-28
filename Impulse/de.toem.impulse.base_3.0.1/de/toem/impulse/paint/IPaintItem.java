/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint;

import de.toem.impulse.paint.IPlotTree;
import de.toem.impulse.paint.ISelectItem;
import de.toem.toolkits.pattern.js.JsMethod;
import de.toem.toolkits.ui.tlk.controls.ITlkItem;

public interface IPaintItem
extends ITlkItem,
ISelectItem {
    public static final int CHANGED_NAME = 2;
    public static final int CHANGED_DESCRIPTION = 4;
    public static final int CHANGED_COLOR = 8;
    public static final int CHANGED_HOVER = 16;
    public static final int CHANGED_STYLE = 32;
    public static final int CHANGED_TEXT = 64;
    public static final int CHANGED_IMAGE = 128;
    public static final String Description = "Description";
    public static final String Color = "Color";
    public static final String[] Contents = new String[]{"Description", "Color"};

    public IPlotTree getTree();

    @Override
    @JsMethod
    public String getText();

    @Override
    public boolean setText(String var1);

    @JsMethod
    public String getDescription();

    public boolean setDescription(String var1);

    @JsMethod
    public Object getColor();

    public boolean setColor(Object var1);

    @Override
    public boolean setImage(Object var1);
}

