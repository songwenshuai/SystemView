/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.view.AbstractTemplate;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesCharacteristic;
import de.toem.toolkits.pattern.element.CellAnnotation;

@CellAnnotation(type="template.configuration.samples", dynamicChildren={"configuration.samples", "configuration.folder"})
public class PlotConfigurationTemplate
extends AbstractTemplate {
    public static final String TYPE = "template.configuration.samples";
    public boolean usePattern;
    public boolean useInMenu;
    public String processType;
    public String signalType;
    public boolean descriptorRegular;
    public String signalDescriptor;
    public boolean nameRegular;
    public String namePattern;
    public boolean descriptionRegular;
    public String descriptionPattern;
    public String scriptPattern;
    public String initScript;
    public boolean recursive = true;

    public boolean matches(ISamplesCharacteristic characteristic) {
        if (characteristic instanceof AbstractSignal) {
            characteristic = ((AbstractSignal)characteristic).getSignal();
        }
        if (characteristic instanceof Signal) {
            String value;
            Signal cell = (Signal)characteristic;
            if (!(this.processType == null || (value = cell.processType) != null && value.equals(this.processType))) {
                return false;
            }
            if (!(this.signalType == null || (value = cell.signalType) != null && value.equals(this.signalType))) {
                return false;
            }
            if (this.signalDescriptor != null) {
                ISamples.SignalType type = ISamples.SignalType.valueOf(cell);
                String value2 = ISamples.SignalDescriptor.valueOf(cell).toUserString(type);
                if (value2 == null || this.descriptorRegular && !value2.matches(".*" + this.signalDescriptor + ".*") || !this.descriptorRegular && !value2.contains(this.signalDescriptor)) {
                    return false;
                }
            }
            if (this.namePattern != null && ((value = cell.getName()) == null || this.nameRegular && !value.matches(this.namePattern) || !this.nameRegular && !value.contains(this.namePattern))) {
                return false;
            }
            if (this.descriptionPattern != null && ((value = cell.description) == null || this.descriptionRegular && !value.matches(this.descriptionPattern) || !this.descriptionRegular && !value.contains(this.descriptionPattern))) {
                return false;
            }
        } else if (characteristic != null) {
            String value;
            if (!(this.processType == null || (value = characteristic.getProcessType().toString()) != null && value.equals(this.processType))) {
                return false;
            }
            if (!(this.signalType == null || (value = characteristic.getSignalType().toString()) != null && value.equals(this.signalType))) {
                return false;
            }
            if (this.signalDescriptor != null) {
                ISamples.SignalType type = characteristic.getSignalType();
                String value3 = characteristic.getSignalDescriptor().toUserString(type);
                if (value3 == null || this.descriptorRegular && !value3.matches(".*" + this.signalDescriptor + ".*") || !this.descriptorRegular && !value3.contains(this.signalDescriptor)) {
                    return false;
                }
            }
            if (this.namePattern != null && ((value = characteristic.getLabel()) == null || this.nameRegular && !value.matches(this.namePattern) || !this.nameRegular && !value.contains(this.namePattern))) {
                return false;
            }
            if (this.descriptionPattern != null && ((value = characteristic.getDescription()) == null || this.descriptionRegular && !value.matches(this.descriptionPattern) || !this.descriptionRegular && !value.contains(this.descriptionPattern))) {
                return false;
            }
        }
        return true;
    }
}

