/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cell.parts;

import de.toem.impulse.cell.parts.SamplesViewPreferences;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellAnnotation;

@CellAnnotation(type="impulse.subview.samples.member", dynamicChildren={})
public class SamplesViewMemberPreferences
extends Cell {
    public static final String TYPE = "impulse.subview.samples.member";
    public String member;
    public int memberShowMode = 1;
    public int memberAlignment = 0;
    public boolean memberFixedFont = false;
    public boolean memberWrap = false;
    public int memberFormat = -1;

    public SamplesViewMemberPreferences() {
    }

    public SamplesViewMemberPreferences(String member) {
        this.member = member;
        this.setName(member);
    }

    @Override
    public String getDescription() {
        String info = "";
        switch (this.memberShowMode) {
            case 1: {
                info = String.valueOf(SamplesViewPreferences.alignment2TableFormat(this.memberAlignment)) + ";" + SampleConverter.getFormatLabel(this.memberFormat);
                if (!this.memberFixedFont) break;
                info = String.valueOf(info) + ";" + SamplesViewPreferences.fixedFont2TableFormat(true);
            }
        }
        return info;
    }
}

