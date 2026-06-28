/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.impulse.cells.ports.MultiAdapterPort;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.Link;

@CellAnnotation(type="port.multi.resource")
public class MultiResourcePort
extends MultiAdapterPort {
    public static final String TYPE = "port.multi.resource";
    public Link resourceBase;
}

