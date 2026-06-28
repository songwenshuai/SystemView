/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.parts.viewer;

import de.toem.impulse.ImpulseBase;
import de.toem.impulse.ImpulseLicense;
import de.toem.impulse.cells.ports.IPortAdapter;
import de.toem.impulse.cells.ports.IPortAdapter2;
import de.toem.impulse.cells.ports.IPortAdapter3;
import de.toem.impulse.cells.ports.IPortProgress;
import de.toem.impulse.cells.ports.IRecordPort;
import de.toem.impulse.cells.record.PortScope;
import de.toem.impulse.serializer.IRecordReader;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.element.serializer.ICellReader;
import de.toem.toolkits.pattern.ide.Ide;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.pattern.threading.Progress;
import java.io.Closeable;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

public abstract class PortInput {
    private IRecordPort port;
    private IElement baseElement;
    private boolean doStreaming = false;
    private boolean doUpdating = true;
    private boolean doIterating = false;
    private boolean canceled = false;
    private boolean finished = false;
    private boolean error = false;
    private List<AdapterStream> streams = new ArrayList<AdapterStream>();
    private int updateCount = 0;
    private IProgress progress = new PortProgress();

    public PortInput(IRecordPort port, IElement baseElement) {
        this.port = port;
        this.baseElement = baseElement;
        ArrayList<IPortAdapter> adapters = new ArrayList<IPortAdapter>();
        if (port instanceof IPortAdapter) {
            adapters.add((IPortAdapter)((Object)port));
        } else {
            for (ICell iCell : port.getChildren()) {
                if (!(iCell instanceof IPortAdapter)) continue;
                adapters.add((IPortAdapter)iCell);
            }
        }
        for (IPortAdapter iPortAdapter : adapters) {
            if (!iPortAdapter.getValueAsBoolean("enabled") || ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.ports", "de.toem.impulse.feature.default", iPortAdapter.getType())) continue;
            ICell insertPoint = baseElement.getCell();
            boolean asRoot = iPortAdapter.getValueAsBoolean("insertAsRoot");
            if (!(asRoot |= iPortAdapter instanceof IRecordPort && ((IRecordPort)((Object)iPortAdapter)).isPort())) {
                insertPoint = new PortScope();
                insertPoint.setName(iPortAdapter.getName());
                baseElement.getCell().addChild(insertPoint);
                if (iPortAdapter instanceof IPortAdapter3 && !ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.ports", "de.toem.impulse.feature.diagram.label", iPortAdapter)) {
                    int n = ((PortScope)insertPoint).synced = ((IPortAdapter3)((Object)iPortAdapter)).needsSync() ? 1 : 0;
                }
            }
            if (this.validate(insertPoint, iPortAdapter)) {
                this.add(insertPoint, iPortAdapter);
                continue;
            }
            this.error = true;
        }
    }

    public void dispose() {
        this.canceled = true;
        if (this.baseElement != null && this.baseElement.isBound() && this.baseElement.hasCell()) {
            this.baseElement.getCell().unBind(true, true);
        }
    }

    public boolean validate(ICell insertPoint, IPortAdapter adapter) {
        return adapter.validate(insertPoint);
    }

    public void add(ICell insertPoint, IPortAdapter adapter) {
        this.streams.add(new AdapterStream(insertPoint, adapter));
    }

    public Collection<AdapterStream> getStreams() {
        return this.streams;
    }

    public boolean isStarted() {
        for (AdapterStream stream : this.streams) {
            if (!stream.started) continue;
            return true;
        }
        return false;
    }

    public boolean isCanceled() {
        return this.canceled;
    }

    public boolean isFinished() {
        return this.finished;
    }

    public boolean isRunning() {
        return this.isStarted() && !this.isFinished();
    }

    public boolean isStreaming() {
        return this.isRunning() && this.doStreaming;
    }

    public boolean isUpdating() {
        return this.isRunning() && this.doUpdating;
    }

    public boolean isIterating() {
        return this.doIterating;
    }

    public void clear() {
        this.streams.clear();
    }

    public boolean isEmpty() {
        return this.streams.isEmpty();
    }

    public void setIterating(boolean doIterating) {
        this.doIterating = doIterating;
    }

    public void start() {
        for (AdapterStream stream : this.streams) {
            stream.start();
        }
        this.started();
        Actives.run(new IExecutable(){

            @Override
            public void execute(IProgress p) {
                Actives.runInMain(new IExecutable(){

                    @Override
                    public void execute(IProgress p) {
                        if (PortInput.this.error) {
                            Ide.openWarning(I18n.Adapter_StartingPort, I18n.Adapter_ErrorsOccuredWhileStarting);
                        }
                    }
                }, 500);
                while (!PortInput.this.streamsFinished() && !PortInput.this.canceled) {
                    PortInput.this.handleUpdate();
                    Actives.sleep(100);
                }
                for (AdapterStream stream : PortInput.this.streams) {
                    stream.close();
                }
                Actives.runInMain(new IExecutable(){

                    @Override
                    public void execute(IProgress p) {
                        Actives.sleep(100);
                        PortInput.this.handleUpdate();
                        if (PortInput.this.doUpdating) {
                            PortInput.this.leftUpdating();
                        }
                        PortInput.this.finished = true;
                        PortInput.this.doStreaming = false;
                        PortInput.this.doUpdating = false;
                        PortInput.this.finished();
                    }
                });
            }
        });
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void enterStreaming() {
        this.doStreaming = true;
        IProgress iProgress = this.progress;
        synchronized (iProgress) {
            this.progress.notify();
        }
        this.enteredStreaming();
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void enterUpdating(boolean doUpdating) {
        this.doUpdating = doUpdating;
        if (!this.isRunning()) {
            return;
        }
        if (doUpdating) {
            this.enteredUpdating((this.port.getNature() & 1) != 0);
        } else {
            this.leftUpdating();
        }
        IProgress iProgress = this.progress;
        synchronized (iProgress) {
            this.progress.notify();
        }
    }

    public boolean cancel() {
        this.canceled = true;
        return false;
    }

    private void handleUpdate() {
        if (this.updateCount > 0) {
            return;
        }
        ++this.updateCount;
        int changed = 0;
        for (AdapterStream stream : this.streams) {
            changed = Math.max(stream.flush(), changed);
        }
        for (AdapterStream stream : this.streams) {
            if (!stream.needsSync() || !stream.sync()) continue;
            changed |= 4;
        }
        final int achanged = changed;
        Actives.runInMain(new IExecutable(){

            @Override
            public void execute(IProgress p) {
                if (PortInput.this.isStreaming() && PortInput.this.isUpdating() && (PortInput.this.port.getNature() & 6) == 2 || achanged == 4) {
                    PortInput.this.update(achanged);
                }
                if (PortInput.this.isRunning() && !PortInput.this.isStreaming() && (PortInput.this.port.getNature() & 0x80) != 0) {
                    PortInput.this.updateCurrentValue();
                }
                PortInput.this.checkMode();
                PortInput.this.updateCount = 0;
            }
        }, true);
    }

    private boolean streamsFinished() {
        for (AdapterStream stream : this.streams) {
            if (stream.finished) continue;
            return false;
        }
        return true;
    }

    protected abstract void started();

    protected abstract void updateCurrentValue();

    protected abstract void update(int var1);

    protected abstract void enteredStreaming();

    protected abstract void enteredUpdating(boolean var1);

    protected abstract void leftUpdating();

    protected abstract void finished();

    protected abstract void checkMode();

    public class AdapterStream {
        private IPortAdapter adapter;
        private ICell insertPoint;
        private boolean started;
        private boolean finished;
        private boolean syncEnabled;
        private boolean synced;
        private Closeable input;
        private ICellReader reader;
        private Link configuration;

        public AdapterStream(ICell insertPoint, IPortAdapter adapter) {
            this.insertPoint = insertPoint;
            this.adapter = adapter;
            this.syncEnabled = adapter instanceof IPortAdapter3 && !ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.ports", "de.toem.impulse.feature.diagram.label", adapter);
        }

        public boolean sync() {
            if (this.syncEnabled && this.adapter instanceof IPortAdapter3) {
                this.synced = ((IPortAdapter3)((Object)this.adapter)).sync((PortScope)this.insertPoint, this.insertPoint.getParent());
            }
            return this.synced;
        }

        public boolean needsSync() {
            if (this.synced) {
                return false;
            }
            if (this.syncEnabled && this.adapter instanceof IPortAdapter3 && this.insertPoint instanceof PortScope) {
                return ((IPortAdapter3)((Object)this.adapter)).needsSync();
            }
            return false;
        }

        synchronized void start() {
            if (!this.started) {
                this.started = true;
                Actives.run(new IExecutable(){

                    @Override
                    public void execute(IProgress p) {
                        block8: {
                            try {
                                try {
                                    AdapterStream.this.input = AdapterStream.this.adapter.getInput(PortInput.this.progress);
                                    if (AdapterStream.this.input == null) {
                                        ImpulseBase.defaultConsoleStream().println(String.valueOf(AdapterStream.this.adapter.getName()) + ": " + I18n.Adapter_CouldNotCreateInput);
                                    }
                                    AdapterStream.this.reader = AdapterStream.this.adapter.newReader(AdapterStream.this.input);
                                    AdapterStream.this.configuration = AdapterStream.this.adapter instanceof IPortAdapter2 ? ((IPortAdapter2)((Object)AdapterStream.this.adapter)).getConfiguration() : null;
                                    if (AdapterStream.this.adapter instanceof IPortAdapter3) {
                                        ((IPortAdapter3)((Object)AdapterStream.this.adapter)).prepareInsertPoint(AdapterStream.this.insertPoint);
                                    }
                                    if (AdapterStream.this.reader != null) {
                                        AdapterStream.this.reader.read(PortInput.this.progress, AdapterStream.this.configuration != null ? AdapterStream.this.configuration.getPath() : null, AdapterStream.this.insertPoint, 2);
                                        break block8;
                                    }
                                    ImpulseBase.defaultConsoleStream().println(String.valueOf(AdapterStream.this.adapter.getName()) + ": " + I18n.Adapter_CouldNotCreateReader);
                                    PortInput.this.error = true;
                                }
                                catch (Throwable e) {
                                    PortInput.this.error = true;
                                    ImpulseBase.defaultConsoleStream().println(String.valueOf(AdapterStream.this.adapter.getName()) + ": " + I18n.Adapter_CouldNotCreateInputReader + " -- " + e.getLocalizedMessage());
                                    AdapterStream.this.close();
                                    AdapterStream.this.finished = true;
                                }
                            }
                            finally {
                                AdapterStream.this.close();
                                AdapterStream.this.finished = true;
                            }
                        }
                    }
                });
            }
        }

        public synchronized void close() {
            try {
                if (this.input != null) {
                    this.input.close();
                }
            }
            catch (IOException iOException) {}
            this.input = null;
        }

        public int flush() {
            IRecordReader recordReader;
            if (this.reader instanceof IRecordReader && (recordReader = (IRecordReader)this.reader).supports(4096, null)) {
                int changed = recordReader.hasChanged();
                if (changed != 0) {
                    recordReader.flush();
                }
                return changed;
            }
            return 0;
        }
    }

    class PortProgress
    extends Progress
    implements IPortProgress {
        PortProgress() {
        }

        @Override
        public boolean isCanceled() {
            return PortInput.this.isCanceled();
        }

        @Override
        public boolean cancel() {
            return PortInput.this.cancel();
        }

        @Override
        public boolean isStreaming() {
            return PortInput.this.isStreaming();
        }

        @Override
        public void startStreaming() {
            Actives.runInMain(new IExecutable(){

                @Override
                public void execute(IProgress p) {
                    PortInput.this.enterStreaming();
                }
            });
        }

        @Override
        public boolean isUpdating() {
            return PortInput.this.isUpdating();
        }

        @Override
        public boolean isIterating() {
            return PortInput.this.isIterating();
        }

        @Override
        @Deprecated
        public void doUpdate() {
            if (PortInput.this.isUpdating()) {
                PortInput.this.handleUpdate();
            }
        }
    }
}

