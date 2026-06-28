/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cell.parts;

import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellAnnotation;

@CellAnnotation(type="impulse.subview.sample.member", dynamicChildren={})
public class SampleViewMemberPreferences
extends Cell {
    public static final String TYPE = "impulse.subview.sample.member";
    public String member;
    public String content;
    public boolean showDefaultFormat = true;
    public boolean showNonAvailableFormats;
    public long showFormats = 0L;
    public boolean showImage;
    public boolean showBytes;

    public SampleViewMemberPreferences() {
    }

    public SampleViewMemberPreferences(ISamples.SignalType signalType, String member, String content) {
        this.member = member;
        this.content = content;
        this.init(signalType);
        this.setName(String.valueOf(this.member) + "/" + content);
    }

    private void init(ISamples.SignalType signalType) {
    }

    @Override
    public String getDescription() {
        String formats = "";
        if (this.showDefaultFormat) {
            formats = SampleConverter.getFormatLabel(-1);
        }
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

