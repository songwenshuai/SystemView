/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.record.SignalProxy;
import de.toem.impulse.cells.view.FolderConfiguration;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.cells.view.ViewConfiguration;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.writer.IConvergingSamplesWriter;
import de.toem.impulse.samples.writer.IDivergingSamplesWriter;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.pageable.Pageable;
import java.util.List;

public interface IRecordGenerator {
    public void initRecord(String var1);

    public ICell getBase();

    public Signal getSignal(String var1);

    public List<ICell> getAllSignals();

    public Scope getScope(String var1);

    public Scope addScope(ICell var1, String var2);

    public Scope addScope(ICell var1, String var2, String var3);

    public Signal addSignal(ICell var1, String var2, String var3, ISamples.ProcessType var4, ISamples.SignalType var5, ISamples.SignalDescriptor var6, IDomainBase var7);

    public Signal addSignal(ICell var1, String var2, String var3, ISamples.ProcessType var4, ISamples.SignalType var5, ISamples.SignalDescriptor var6, IDomainBase var7, boolean var8);

    public SignalProxy addSignalProxy(ICell var1, String var2, String var3, Signal var4);

    public ISamplesWriter createWriter(Signal var1);

    public ISamplesWriter createWriter(Signal var1, ISamples.ProcessType var2, ISamples.SignalType var3, ISamples.SignalDescriptor var4, IDomainBase var5);

    public IConvergingSamplesWriter createConvergingWriter(Signal var1, ISamples.ProcessType var2, ISamples.SignalType var3, ISamples.SignalDescriptor var4, IDomainBase var5);

    public IDivergingSamplesWriter createDivergingWriter(ISamplesWriter var1);

    public ISamplesWriter getWriter(Signal var1);

    public void createWriteHandler(Signal var1, IWriteHandler var2);

    public void apply();

    public ViewConfiguration addViewConfiguration(String var1, String var2);

    public ViewConfiguration addViewConfiguration(ViewConfiguration var1);

    public ViewConfiguration addViewConfigurations(byte[] var1);

    public FolderConfiguration addFolderConfiguration(ICell var1, String var2, String var3);

    public FolderConfiguration addFolderConfiguration(ICell var1, Scope var2);

    public PlotConfiguration addPlotConfiguration(ICell var1, String var2, String var3);

    public PlotConfiguration addPlotConfiguration(ICell var1, AbstractSignal var2);

    public ISamplesReader createReader(Signal var1);

    public static interface IWriteHandler {
        public static final int PREPARE = 0;
        public static final int FILL = 1;

        public void handle(int var1, Signal var2, Pageable<?> var3);
    }
}

