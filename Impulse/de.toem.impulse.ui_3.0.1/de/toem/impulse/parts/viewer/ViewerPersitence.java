/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.parts.viewer;

import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.Link;

@CellAnnotation(type="persitence.impulse.viewer", dynamicChildOf={"preferences.impulse.parts"})
public class ViewerPersitence
extends Cell {
    public static final String TYPE = "persitence.impulse.viewer";
    public Link partPreferences;
}

