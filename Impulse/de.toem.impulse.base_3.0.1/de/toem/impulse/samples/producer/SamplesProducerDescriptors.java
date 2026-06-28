/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.producer.SamplesProducerDescriptor;
import de.toem.toolkits.pattern.registry.AbstractRegisteredDescriptors;

public class SamplesProducerDescriptors
extends AbstractRegisteredDescriptors<SamplesProducerDescriptor, ISamplesProducer> {
    public SamplesProducerDescriptors() {
        super("de.toem.impulse.base.samplesProducer", SamplesProducerDescriptor.class, true);
    }
}

