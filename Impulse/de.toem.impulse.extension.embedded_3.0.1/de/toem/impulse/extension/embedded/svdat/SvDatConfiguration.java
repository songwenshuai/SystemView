/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.svdat;

import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.cells.serializer.Serializer;
import de.toem.impulse.domain.TimeBase;
import de.toem.toolkits.pattern.element.CellAnnotation;

@CellAnnotation(type="impulse.serializer.configuration.svdat", dynamicChildOf={"impulse.serializer"})
public class SvDatConfiguration
extends ReaderConfiguration {
    public static final String TYPE = "impulse.serializer.configuration.svdat";
    public String domainBase = TimeBase.ns.toString();
    public String userEvents;

    @Override
    public boolean supports(Serializer serializer) {
        return "de.toem.impulse.serializer.svdat".equals(serializer.id);
    }
}

