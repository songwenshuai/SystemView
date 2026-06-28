/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.record;

import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.samples.ISamples;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.FieldAnnotation;
import de.toem.toolkits.pattern.element.exploits.Markers;
import de.toem.toolkits.pattern.pageable.Pageable;

@CellAnnotation(type="record.signal", properties={"imageExtension"})
public class Signal
extends AbstractSignal {
    public static final String TYPE = "record.signal";
    public String processType = ISamples.ProcessType.Discrete.toString();
    public String signalType = ISamples.SignalType.Logic.toString();
    public String signalDescriptor;
    public String tagDomain;
    public boolean tag;
    public long start;
    public long end;
    public long rate;
    public String domainBase;
    @FieldAnnotation(exploit="general.markers")
    public String markers;
    public Pageable<byte[]> samples;
    public Pageable<byte[]> legend;

    @Override
    public Signal getSignal() {
        return this;
    }

    public String imageExtension() {
        return this.imageExtension(this.diff);
    }

    @Override
    public boolean hasMarkers() {
        return this.markers != null;
    }

    @Override
    public boolean fillMarkers(Markers toBeFilled) {
        if (this.markers != null) {
            Markers m = new Markers();
            m.setValue(this.markers);
            return m.fillMarkers(toBeFilled, null);
        }
        return false;
    }

    public String imageExtension(int diff) {
        StringBuilder builder = new StringBuilder();
        builder.append('-');
        switch (ISamples.SignalType.valueOf(this.signalType)) {
            case EventArray: {
                builder.append(String.valueOf(String.valueOf((Object)ISamples.SignalType.Event).toLowerCase()) + "-N");
                break;
            }
            case IntegerArray: {
                builder.append(String.valueOf(String.valueOf((Object)ISamples.SignalType.Integer).toLowerCase()) + "-N");
                break;
            }
            case FloatArray: {
                builder.append(String.valueOf(String.valueOf((Object)ISamples.SignalType.Float).toLowerCase()) + "-N");
                break;
            }
            case TextArray: {
                builder.append(String.valueOf(String.valueOf((Object)ISamples.SignalType.Text).toLowerCase()) + "-N");
                break;
            }
            case Event: {
                builder.append(String.valueOf(String.valueOf((Object)ISamples.SignalType.Event).toLowerCase()) + "-1");
                break;
            }
            case Integer: {
                builder.append(String.valueOf(String.valueOf((Object)ISamples.SignalType.Integer).toLowerCase()) + "-1");
                break;
            }
            case Float: {
                builder.append(String.valueOf(String.valueOf((Object)ISamples.SignalType.Float).toLowerCase()) + "-1");
                break;
            }
            case Text: {
                builder.append(String.valueOf(String.valueOf((Object)ISamples.SignalType.Text).toLowerCase()) + "-1");
                break;
            }
            default: {
                builder.append(String.valueOf(this.signalType).toLowerCase());
            }
        }
        if (ISamples.SignalType.valueOf(this) == ISamples.SignalType.Logic) {
            builder.append(ISamples.SignalDescriptor.valueOf(this).getScale() > 1 ? "-N" : "-1");
        }
        if (diff == 1) {
            builder.append("-mod");
        } else if (diff == 2) {
            builder.append("-add");
        } else if (diff == 3) {
            builder.append("-del");
        } else if (this.tag) {
            builder.append("-tag");
        }
        return builder.toString();
    }

    public static String fieldNameForAttribute(String old) {
        if ("context".equals(old)) {
            return "legend";
        }
        if ("conflict".equals(old)) {
            return "tag";
        }
        return null;
    }
}

