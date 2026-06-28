/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.parts.views;

import de.toem.impulse.cell.parts.SampleViewMemberPreferences;
import de.toem.impulse.cell.parts.SampleViewPreferences;
import de.toem.impulse.cell.parts.SampleViewTypePreferences;
import de.toem.impulse.dialog.sample.SampleDialog;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.ISamples;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICell;
import java.util.List;

public class SampleViewDialog {

    public static abstract class AbstractSampleDialogInput
    extends SampleDialog.AbstractSampleDialogInput {
        @Override
        public ICell getPreferences(int type, Object selector) {
            SampleViewPreferences preferences = SampleViewPreferences.getDefaultPreferences();
            if (type == 0 || preferences == null) {
                return preferences;
            }
            SampleViewTypePreferences typePreferences = null;
            ISamples.SignalType signalType = this.pointer != null ? this.pointer.getSignalType() : ISamples.SignalType.Unknown;
            String content = this.pointer != null && this.pointer.getContent() != null ? this.pointer.getContent() : "default";
            for (ICell p : preferences.getChildren()) {
                if (!(p instanceof SampleViewTypePreferences) || !Utils.equals(signalType.toString(), ((SampleViewTypePreferences)p).signalType) || !Utils.equals(content, ((SampleViewTypePreferences)p).content)) continue;
                typePreferences = (SampleViewTypePreferences)p;
                break;
            }
            if (typePreferences == null) {
                typePreferences = new SampleViewTypePreferences(signalType, content);
                preferences.addChild(typePreferences);
            }
            SampleViewMemberPreferences memberPreferences = null;
            if (signalType.isArrayOrStruct()) {
                List<IMemberDescriptor> descriptors = this.pointer.getMemberDescriptors();
                for (IMemberDescriptor descriptor : descriptors) {
                    String member = descriptor != null ? descriptor.getName() : null;
                    String mcontent = descriptor != null ? descriptor.getContent() : null;
                    SampleViewMemberPreferences m = null;
                    for (ICell p : typePreferences.getChildren()) {
                        if (!(p instanceof SampleViewMemberPreferences) || !Utils.equals(member, ((SampleViewMemberPreferences)p).member) || !Utils.equals(mcontent, ((SampleViewMemberPreferences)p).content)) continue;
                        m = (SampleViewMemberPreferences)p;
                        break;
                    }
                    if (m == null) {
                        m = new SampleViewMemberPreferences(signalType, member, mcontent);
                        typePreferences.addChild(m);
                    }
                    if (!member.equals(selector)) continue;
                    memberPreferences = m;
                }
            }
            if (type == 1) {
                return typePreferences;
            }
            if (type == 2) {
                return memberPreferences;
            }
            return null;
        }

        @Override
        public boolean filterPreferences(ICell p) {
            ISamples.SignalType signalType;
            ISamples.SignalType signalType2 = signalType = this.pointer != null ? this.pointer.getSignalType() : ISamples.SignalType.Unknown;
            if (p instanceof SampleViewTypePreferences) {
                String content = this.pointer != null ? this.pointer.getContent() : "default";
                return !Utils.equals(signalType.toString(), ((SampleViewTypePreferences)p).signalType) || !Utils.equals(content, ((SampleViewTypePreferences)p).content);
            }
            if (p instanceof SampleViewMemberPreferences && signalType.isArrayOrStruct()) {
                List<IMemberDescriptor> descriptors = this.pointer.getMemberDescriptors();
                for (IMemberDescriptor descriptor : descriptors) {
                    String mcontent;
                    String member = descriptor != null ? descriptor.getName() : null;
                    String string = mcontent = descriptor != null ? descriptor.getContent() : null;
                    if (!Utils.equals(member, ((SampleViewMemberPreferences)p).member) || !Utils.equals(mcontent, ((SampleViewMemberPreferences)p).content)) continue;
                    return false;
                }
            }
            return true;
        }
    }
}

