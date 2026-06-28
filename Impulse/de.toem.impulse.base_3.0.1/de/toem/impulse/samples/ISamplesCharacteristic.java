/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ISamples;
import de.toem.toolkits.pattern.information.IInformation;

public interface ISamplesCharacteristic
extends IInformation {
    public ISamples.ProcessType getProcessType();

    public ISamples.SignalType getSignalType();

    public ISamples.SignalDescriptor getSignalDescriptor();

    public IDomainBase getDomainBase();
}

