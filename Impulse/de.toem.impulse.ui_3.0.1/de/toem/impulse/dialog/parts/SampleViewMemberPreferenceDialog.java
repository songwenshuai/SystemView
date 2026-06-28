/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.parts;

import de.toem.impulse.cell.parts.SampleViewMemberPreferences;
import de.toem.impulse.dialog.view.AbstractSignalControlProvider;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.ui.controller.base.CheckController;
import de.toem.toolkits.ui.controller.base.CheckSetController;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import java.util.ArrayList;

public class SampleViewMemberPreferenceDialog
extends ControlProviderElementDialog {
    public SampleViewMemberPreferenceDialog(ITlkPartContainer parent, int style) {
        super(parent, SampleViewMemberPreferenceDialog.getControls(), style);
    }

    public SampleViewMemberPreferenceDialog() {
    }

    public static IControlProvider getControls() {
        return new AbstractSignalControlProvider(){

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.sampleview_member_dialog";
            }

            @Override
            protected boolean fillThis() {
                this.setCellClass(SampleViewMemberPreferences.class);
                try {
                    this.tlk().addButton(this.container(), new CheckController(this.editor(), this.source("showDefaultFormat")), this.cols(), 2048, "Show default format", null);
                    this.tlk().addButton(this.container(), new CheckController(this.editor(), this.source("showNonAvailableFormats")), this.cols(), 2048, "Show non-available formats/infos", null);
                    this.tlk().addButton(this.container(), new CheckController(this.editor(), this.source("showBytes")){

                        @Override
                        protected boolean enabled() {
                            return super.enabled() && ISamples.SignalType.Binary.toString().equals(this.getCellValueAsString("signalType"));
                        }
                    }, this.cols(), 2048, "Show bytes", null);
                    this.tlk().addButton(this.container(), new CheckController(this.editor(), this.source("showImage")){

                        @Override
                        protected boolean enabled() {
                            return super.enabled() && ISamples.SignalType.Binary.toString().equals(this.getCellValueAsString("signalType"));
                        }
                    }, this.cols(), 2048, "Show image", null);
                    ArrayList<String> labels = new ArrayList<String>();
                    ArrayList<Long> options = new ArrayList<Long>();
                    Object[] objectArray = SampleConverter.formatValueOptions();
                    int n = objectArray.length;
                    int n2 = 0;
                    while (n2 < n) {
                        Object o = objectArray[n2];
                        int f = (Integer)o;
                        if (f >= 1 && (f < 16 || f > 20) && f <= 39) {
                            labels.add(SampleConverter.getFormatLabel(f));
                            options.add(1L << f);
                        }
                        ++n2;
                    }
                    this.tlk().addLabel(this.container(), this.cols(), 0, "Show formats:", null);
                    this.tlk().addButtonSet(this.container(), new CheckSetController(this.editor(), this.source("showFormats"), options.toArray()), this.cols(), this.cols(), 2048, null, labels.toArray(new String[labels.size()]), null);
                }
                catch (SecurityException e) {
                    SystemLog.log(e);
                }
                return true;
            }
        };
    }
}

