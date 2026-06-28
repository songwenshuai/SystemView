/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.ImpulseBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IPointer;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesIterator;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.samples.producer.AbstractUpdatableMasterSamplesProducer;
import de.toem.impulse.samples.producer.IScriptProgress;
import de.toem.impulse.scripting.DefaultScriptContextProvider;
import de.toem.impulse.scripting.ScriptControls;
import de.toem.impulse.scripting.ScriptProperties;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.scripting.IScriptContextProvider;
import de.toem.toolkits.pattern.scripting.IScripting;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.pattern.threading.ReferencedProgress;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import java.lang.reflect.Field;
import java.util.Iterator;
import java.util.List;

public class ScriptProducer
extends AbstractUpdatableMasterSamplesProducer
implements IScriptContextProvider {
    private int timeout;
    private boolean methodScriptStructure;
    private int logging;
    private int[] inputidx;
    private ISamplePointer[] input;
    private IScripting scripting;
    private IScripting.IScriptInit lateInit;
    private ContinuationProgress progress;

    public ScriptProducer() {
    }

    public ScriptProducer(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, 0, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    public static IPropertyModel getPropertyModel() {
        return ScriptProperties.getPropertyModel();
    }

    protected void assertScripting() {
        if (this.scripting == null) {
            this.scripting = Scripting.create(this, "definition", s -> {
                int n = 0;
                while (n < this.input.length) {
                    s.setSymbol("in" + this.inputidx[n], this.input[n]);
                    ++n;
                }
                s.setSymbol("input", this.input);
                this.lateInit = ls -> {
                    s.setSymbol("thiz", this);
                    if (this.targetWriter != null) {
                        s.setSymbol("out", this.targetWriter);
                        s.setSymbol("base", this.targetWriter.getDomainBase());
                    }
                    if (this.iter != null) {
                        s.setSymbol("iter", this.iter);
                    }
                };
                this.lateInit.init(null);
                s.onException(e -> {
                    this.setError(e.getLocalizedMessage());
                    SystemLog.log(e);
                });
                s.onThreadDeath(e -> this.setError(I18n.Producer_Killed));
                s.setLogging(this.logging);
                s.setLoader(ImpulseBase.getClassLoader());
            });
        }
    }

    @Override
    protected IReadableSamples loopThroughSource() {
        return this.input != null && this.input.length >= 1 ? this.input[0] : null;
    }

    @Override
    public void init(String sstart, String send, String srate, IDomainBaseProvider readerBaseProvider) {
        this.timeout = ScriptProperties.timeout(this.parameters);
        this.methodScriptStructure = ScriptProperties.methodScriptStructure(this.parameters);
        this.logging = ScriptProperties.logging(this.parameters);
        if (this.sources != null) {
            int sourceSize = 0;
            for (IReadableSamples source : this.sources) {
                if (source == null) continue;
                ++sourceSize;
            }
            this.input = new ISamplePointer[sourceSize];
            this.inputidx = new int[sourceSize];
            int n = 0;
            int m = 0;
            for (IReadableSamples source : this.sources) {
                if (source != null) {
                    this.input[m] = new SamplePointer(source);
                    this.inputidx[m] = n;
                    ++m;
                }
                ++n;
            }
        } else {
            this.input = new ISamplePointer[0];
            this.inputidx = new int[0];
        }
        if (this.methodScriptStructure) {
            this.assertScripting();
            this.scripting.run(null);
            if (this.scripting.hasFunction("init")) {
                this.scripting.invoke("init", new Object[0]);
            }
        }
        super.init(sstart, send, srate, readerBaseProvider);
    }

    @Override
    protected boolean instatiate(IProgress p) {
        if (p != null && this.timeout > 0) {
            p.incTimeout(this.timeout);
        }
        if (this.mode != 2) {
            this.targetWriter = PackedSamples.createWriter(this.processType, this.signalType, this.signalDescriptor, this.productionBase);
            if (this.targetWriter == null) {
                return false;
            }
            this.targetWriter.open(this.start, (this.flags & 0x20) != 0 ? this.end : this.start, this.rate, 0, 0, null);
            this.iter = new SamplesIterator(this.targetWriter, this.input);
            this.setReference(PackedSamples.createReader(this.targetWriter, this.readerBase));
        } else {
            this.iter = new SamplesIterator(this.input);
        }
        if (this.mode != 0) {
            for (AbstractUpdatableMasterSamplesProducer.AbstractSlaveProduction slave : this.slaveProductions) {
                slave.instantiate();
            }
        }
        this.progress = new ContinuationProgress(p);
        this.assertScripting();
        if (this.lateInit != null) {
            this.lateInit.init(null);
        }
        if (this.methodScriptStructure && this.scripting.hasFunction("instantiate")) {
            this.scripting.invoke("instantiate", this.progress);
        }
        return this.reference != null;
    }

    @Override
    protected boolean execute(IProgress p) {
        this.progress.setReference(p);
        if (this.methodScriptStructure) {
            if (this.scripting.hasFunction("execute")) {
                this.scripting.invoke("execute", this.progress);
            }
        } else {
            this.scripting.run(this.progress);
        }
        return true;
    }

    @Override
    protected boolean continueProducing() {
        return super.continueProducing() && this.progress != null && this.progress.canContinue();
    }

    @Override
    protected void destroy() {
        super.destroy();
        this.progress = null;
        this.scripting = null;
    }

    public ISamplesProducer.IWritableSlaveProduction addSlave(String slaveId, String parentId, String name, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor) {
        return new AbstractUpdatableMasterSamplesProducer.AbstractWritableSlaveProduction(slaveId, parentId, name, signalType, signalDescriptor);
    }

    public void setMode(int mode) {
        this.mode = mode;
    }

    public static IControlProvider getDefinitionControls(final Field source, final Field language) {
        return new AbstractControlProvider(){

            @Override
            protected boolean fillThis() {
                try {
                    ScriptControls.fillScriptControls(this.tlk(), this.container(), this.editor(), source, language, this.tlk().ld(this.cols(), 4, 1, 4, 300));
                }
                catch (SecurityException securityException) {}
                return true;
            }
        };
    }

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        DefaultScriptContextProvider.provideDefaultScriptContext(context, true, false, false, false, false, true);
        int index = 0;
        context.addSymbol("thiz", ScriptProducer.class);
        Iterator iterator = this.sources.iterator();
        while (iterator.hasNext()) {
            IReadableSamples cfr_ignored_0 = (IReadableSamples)iterator.next();
            context.addSymbol("in" + index++, ISamplePointer.class, IReadableSamples.class);
        }
        Class<? extends ISamplesWriter> writer = PackedSamples.getWriterInterface(this.signalType);
        if (writer != null) {
            context.addSymbol("out", writer);
        }
        context.addSymbol("base", IDomainBase.class);
        context.addSymbol("input", IPointer[].class, ISamplePointer[].class, IReadableSamples[].class);
        context.addSymbol("iter", ISamplesIterator.class);
        context.addSymbol("progress", ContinuationProgress.class);
        context.setScript(this.definition, this.language);
        context.setLoader(ImpulseBase.getClassLoader());
    }

    public class ContinuationProgress
    extends ReferencedProgress
    implements IScriptProgress {
        private boolean cont;

        public ContinuationProgress(IProgress base) {
            super(base);
        }

        @Override
        public void cont() {
            this.cont = true;
        }

        @Override
        public boolean canContinue() {
            return this.cont;
        }
    }
}

