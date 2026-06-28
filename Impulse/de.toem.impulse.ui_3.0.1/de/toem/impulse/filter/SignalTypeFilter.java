/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.filter;

import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.RecordContent;
import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.record.SignalProxy;
import de.toem.impulse.cells.view.AbstractTreeConfiguration;
import de.toem.impulse.cells.view.FolderConfiguration;
import de.toem.impulse.provider.ISignalProvider;
import de.toem.impulse.samples.ISamples;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.ui.filter.AbstractElementFilter;

public class SignalTypeFilter
extends AbstractElementFilter {
    boolean records;
    boolean scopes;
    boolean proxies;
    boolean floatSignals;
    boolean logicSignals;
    boolean integerSignals;
    boolean mapSignals;
    boolean eventSignals;
    boolean textSignals;

    @Override
    public void sync(ICell cell, int index) {
        super.sync(cell, index);
        IPropertyModel properties = SignalTypeFilter.getPropertyModel().setTotal(this.getProperties());
        this.records = properties.get("records").equals(IPropertyModel.TRUE);
        this.scopes = properties.get("scopes").equals(IPropertyModel.TRUE);
        this.proxies = properties.get("proxies").equals(IPropertyModel.TRUE);
        this.floatSignals = properties.get("floats").equals(IPropertyModel.TRUE);
        this.logicSignals = properties.get("logics").equals(IPropertyModel.TRUE);
        this.integerSignals = properties.get("integers").equals(IPropertyModel.TRUE);
        this.mapSignals = properties.get("maps").equals(IPropertyModel.TRUE);
        this.eventSignals = properties.get("events").equals(IPropertyModel.TRUE);
        this.textSignals = properties.get("text").equals(IPropertyModel.TRUE);
    }

    public static IPropertyModel getPropertyModel() {
        String[] options = new String[]{IPropertyModel.TRUE, IPropertyModel.FALSE};
        return new PropertyModel().add("records", IPropertyModel.TRUE, "Records", options, null).add("scopes", IPropertyModel.TRUE, "Sopes/Folders", options, null).add("proxies", IPropertyModel.TRUE, "Proxies", options, null).add("floats", IPropertyModel.TRUE, "Float Signals", options, null).add("logics", IPropertyModel.TRUE, "Logic Signals", options, null).add("integers", IPropertyModel.TRUE, "Integer Signals", options, null).add("maps", IPropertyModel.TRUE, "Struct Signals", options, null).add("events", IPropertyModel.TRUE, "Event Signals", options, null).add("text", IPropertyModel.TRUE, "Text Signals", options, null);
    }

    @Override
    public boolean filter(IElement parent, IElement element) {
        if (!element.isBound()) {
            return false;
        }
        if (!element.hasCell(RecordContent.class) && element.hasCell(AbstractTreeConfiguration.class)) {
            return true;
        }
        if (element.hasCell(Record.class)) {
            return this.records ^ this.mode == 1;
        }
        if (element.hasCell(Scope.class)) {
            return this.scopes ^ this.mode == 1;
        }
        if (element.hasCell(FolderConfiguration.class)) {
            return this.scopes ^ this.mode == 1;
        }
        if (element.hasCell(SignalProxy.class)) {
            return this.proxies ^ this.mode == 1;
        }
        if (element.hasCell(ISignalProvider.class)) {
            Signal signal = ((ISignalProvider)element.getCell()).getSignal(null);
            if (signal == null) {
                return false;
            }
            if (ISamples.SignalType.valueOf(signal) == ISamples.SignalType.Float) {
                return this.floatSignals ^ this.mode == 1;
            }
            if (ISamples.SignalType.valueOf(signal) == ISamples.SignalType.FloatArray) {
                return this.floatSignals ^ this.mode == 1;
            }
            if (ISamples.SignalType.valueOf(signal) == ISamples.SignalType.Logic) {
                return this.logicSignals ^ this.mode == 1;
            }
            if (ISamples.SignalType.valueOf(signal) == ISamples.SignalType.Integer) {
                return this.integerSignals ^ this.mode == 1;
            }
            if (ISamples.SignalType.valueOf(signal) == ISamples.SignalType.IntegerArray) {
                return this.integerSignals ^ this.mode == 1;
            }
            if (ISamples.SignalType.valueOf(signal) == ISamples.SignalType.Struct) {
                return this.mapSignals ^ this.mode == 1;
            }
            if (ISamples.SignalType.valueOf(signal) == ISamples.SignalType.Event) {
                return this.eventSignals ^ this.mode == 1;
            }
            if (ISamples.SignalType.valueOf(signal) == ISamples.SignalType.EventArray) {
                return this.eventSignals ^ this.mode == 1;
            }
            if (ISamples.SignalType.valueOf(signal) == ISamples.SignalType.Text) {
                return this.textSignals ^ this.mode == 1;
            }
            if (ISamples.SignalType.valueOf(signal) == ISamples.SignalType.TextArray) {
                return this.textSignals ^ this.mode == 1;
            }
        }
        return false;
    }
}

