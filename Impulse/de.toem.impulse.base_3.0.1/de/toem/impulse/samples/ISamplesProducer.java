/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.producer.SamplesProducerDescriptors;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import java.util.List;

public interface ISamplesProducer
extends IReadableSamples {
    public static final int MODE_NORMAL = 0;
    public static final int MODE_MASTER_SLAVE = 1;
    public static final int MODE_LOOPTHROUGH_SLAVE = 2;
    public static final SamplesProducerDescriptors all = new SamplesProducerDescriptors();

    public IDomainBase getProductionBase();

    public List<IReadableSamples> getSources();

    public boolean areSourcesSettling();

    public int update(String var1, String var2, Object var3, ISamples.ProcessType var4, ISamples.SignalType var5, ISamples.SignalDescriptor var6, IDomainBase var7, String var8, String var9, String var10, String var11, String var12, IPropertyModel var13, IDomainBaseProvider var14);

    public int update(Object var1);

    public int update();

    public static interface IMasterProducer
    extends ISamplesProducer {
        public boolean hasChildSlaves(String var1);

        public List<String> getChildSlaveIds(String var1);

        public IReadableSamples getSlaveProduction(String var1);
    }

    public static interface ISlaveProduction
    extends IReadableSamples {
        public String getSlaveId();
    }

    public static interface IWritableSlaveProduction
    extends ISlaveProduction {
        public ISamplesWriter getWriter();
    }
}

