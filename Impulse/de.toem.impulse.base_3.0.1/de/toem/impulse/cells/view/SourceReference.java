/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.Link;

@CellAnnotation(type="configuration.srcref")
public class SourceReference
extends Cell {
    public static final String TYPE = "configuration.srcref";
    public String description;
    public boolean enabled = true;
    public Link reference;
    public Link base;
}

