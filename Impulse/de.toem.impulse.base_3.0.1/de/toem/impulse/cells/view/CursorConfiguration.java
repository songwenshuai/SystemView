/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.cells.view.AbstractViewConfiguration;
import de.toem.toolkits.pattern.element.CellAnnotation;

@CellAnnotation(type="configuration.cursor")
public class CursorConfiguration
extends AbstractViewConfiguration {
    public static final String TYPE = "configuration.cursor";
    public int color = 0xFFFFFF;
    public int initalTime;
}

