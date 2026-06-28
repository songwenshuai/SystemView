/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cell.parts;

import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellAnnotation;

@CellAnnotation(type="impulse.subview.sample.type", dynamicChildren={})
public class SampleViewTypePreferences
extends Cell {
    public static final String TYPE = "impulse.subview.sample.type";
    public String signalType;
    public String content;
    public long showFormats = -1L;
    public boolean showImage;
    public boolean showBytes;

    public SampleViewTypePreferences() {
    }

    public SampleViewTypePreferences(ISamples.SignalType signalType, String content) {
        this.signalType = signalType.toString();
        this.content = content;
        this.init(signalType, content);
        this.setName(String.valueOf(this.signalType) + "/" + content);
    }

    private void init(ISamples.SignalType signalType, String content) {
        this.showFormats = 131072L;
        if (signalType == ISamples.SignalType.Integer) {
            this.showFormats |= 0x20L;
            this.showFormats |= 8L;
            this.showFormats |= 8L;
            this.showFormats |= 0x10L;
            this.showFormats |= 2L;
        } else if (signalType == ISamples.SignalType.Float) {
            this.showFormats |= 0x20L;
            this.showFormats |= 0xFF00000000L;
            this.showFormats |= 0x40000L;
        } else if (signalType == ISamples.SignalType.Text) {
            this.showFormats |= 0x40L;
        } else if (signalType == ISamples.SignalType.Event) {
            this.showFormats |= 0x40L;
            this.showFormats |= 0x80L;
            this.showFormats |= 0x20L;
        } else if (signalType == ISamples.SignalType.Binary) {
            this.showFormats |= 8L;
            this.showFormats |= 0x20L;
            this.showImage = true;
            this.showBytes = true;
        } else if (signalType == ISamples.SignalType.Logic) {
            this.showFormats |= 0x20L;
            this.showFormats |= 8L;
            this.showFormats |= 8L;
            this.showFormats |= 0x10L;
            this.showFormats |= 2L;
        } else if (signalType == ISamples.SignalType.IntegerArray) {
            this.showFormats |= 0x20L;
        } else if (signalType == ISamples.SignalType.FloatArray) {
            this.showFormats |= 0x20L;
        } else if (signalType == ISamples.SignalType.EventArray) {
            this.showFormats |= 0x20L;
        } else if (signalType == ISamples.SignalType.TextArray) {
            this.showFormats |= 0x20L;
        } else if (signalType == ISamples.SignalType.Struct) {
            this.showFormats |= 0x20L;
        }
    }

    @Override
    public String getDescription() {
        String formats = "";
        Object[] objectArray = SampleConverter.formatValueOptions();
        int n = objectArray.length;
        int n2 = 0;
        while (n2 < n) {
            Object o = objectArray[n2];
            int f = (Integer)o;
            if (f >= 1 && (f < 16 || f > 20) && f <= 39 && (this.showFormats & 1L << f) != 0L) {
                formats = String.valueOf(formats) + (Utils.isEmpty(formats) ? "" : ";") + SampleConverter.getFormatLabel(f);
            }
            ++n2;
        }
        return formats;
    }
}

