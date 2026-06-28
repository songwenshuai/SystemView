/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.parts.views;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cell.parts.SubViewPersitence;
import de.toem.impulse.cells.preferences.AbstractSubViewPreferenceCell;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.provider.ISamplesProvider;
import de.toem.impulse.provider.ISimpleSamplesProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.producer.AbstractSamplesFilter;
import de.toem.impulse.samples.producer.SamplesMerger;
import de.toem.impulse.ui.DomainPosition;
import de.toem.impulse.ui.IRecordViewer;
import de.toem.impulse.ui.IRecordViewerChildListener;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.ValueController;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.part.view.AbstractInputViewPart;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import java.util.List;
import java.util.Map;

public abstract class SubView
extends AbstractInputViewPart<Object> {
    protected Map<IReadableSamples, ISimpleSamplesProvider> inputBySamples;
    protected Map<ISimpleSamplesProvider, IReadableSamples> samplesByInput;
    protected IReadableSamples readable = new SamplesMerger();
    protected boolean showGroups;
    protected int index;
    protected IRecordViewer recordViewer;
    protected DomainPosition position;
    protected IController enableGroupsController;
    protected IRecordViewerChildListener recordViewerChildListener = new IRecordViewerChildListener(){
        protected DomainPosition position;
        protected IExecutable delayedRecordViewerPositionChanged = p -> SubView.this.recordViewerPositionChanged(this.position);

        @Override
        public void positionChanged(DomainPosition position, ITlkPart sender) {
            if (SubView.this.isValid() && position != null && sender != SubView.this && SubView.this.getSelectionSync() && !Utils.equals(this.position, position)) {
                this.position = position;
                Actives.runFinally(this.delayedRecordViewerPositionChanged, 100);
            }
        }

        @Override
        public void signalsChanged(List<Object> list, ITlkPart sender) {
            if (SubView.this.isValid() && sender != SubView.this) {
                SubView.this.handleSourceChanged(1, SubView.this.source, list);
                Actives.runFinally(this.delayedRecordViewerPositionChanged, 100);
            }
        }
    };
    protected IExecutable delayedSelectionChanged = p -> this.notifySelectionChanged();

    public SubView(ITlkPartContainer partContainer, int style) {
        super(partContainer, style);
    }

    public SubView(ITlkPartContainer.ITlkEditorSession session, int style) {
        super(session, style);
    }

    public SubView() {
    }

    @Override
    protected void createControls(Object container) {
        super.createControls(container);
        ImpulsePreferences.partsPreferences.addListener(this);
        try {
            this.enableGroupsController = this.tlk().addController(new ValueController(this, this.getPersistenceClass().getField("enableGroups")));
        }
        catch (Throwable throwable) {}
    }

    @Override
    public void dispose() {
        if (this.recordViewer != null && !this.recordViewer.isDisposed()) {
            this.recordViewer.removeChildListener(this.recordViewerChildListener);
        }
        ImpulsePreferences.partsPreferences.removeListener(this);
        super.dispose();
    }

    @Override
    protected String getPersitanceType() {
        return "persitence.impulse.subview";
    }

    @Override
    public SubViewPersitence getPersistence() {
        return (SubViewPersitence)super.getPersistence();
    }

    protected void setEnableGroups(boolean value) {
        if (this.enableGroupsController != null) {
            this.enableGroupsController.setValue(value);
        }
    }

    protected boolean getEnableGroups() {
        if (this.enableGroupsController != null) {
            return this.enableGroupsController.getValueAsBoolean();
        }
        return false;
    }

    public IControlProvider getPreferencesControls(Object owner) {
        return null;
    }

    public AbstractSubViewPreferenceCell getPreferences() {
        return null;
    }

    protected void recordViewerPositionChanged(DomainPosition position) {
        this.position = position;
    }

    protected void notifySelectionChanged() {
        if (this.recordViewer != null && !this.recordViewer.isDisposed() && this.position != null && this.getSelectionSync()) {
            this.recordViewer.moveActiveCursor(this.position, (ITlkPart)this);
        }
    }

    private ITlkPart getEditor(Object source) {
        return source instanceof ITlkPart ? (ITlkPart)source : null;
    }

    @Override
    protected boolean filterSource(int sourceType, Object source, Object sourceData) {
        if (sourceType == 0) {
            if (!this.getInputSync()) {
                return false;
            }
            ITlkPart editor = this.getEditor(source);
            return editor instanceof IRecordViewer;
        }
        if (sourceType == 1) {
            if (!this.getInputSync()) {
                return false;
            }
            ITlkPart editor = this.getEditor(source);
            return editor instanceof IRecordViewer && sourceData instanceof List && this.checkReaders((List)sourceData);
        }
        if (sourceType == 2) {
            return sourceData instanceof List && this.checkReaders((List)sourceData);
        }
        return false;
    }

    @Override
    protected List<Object> extractInput(int sourceType, Object source, Object sourceData) {
        List objects = null;
        if (sourceType == 0) {
            ITlkPart sourceEditor = this.getEditor(source);
            if (sourceEditor instanceof IRecordViewer) {
                objects = ((IRecordViewer)sourceEditor).getSelectedSamplesProvider();
            }
        } else if (sourceType == 1) {
            if (sourceData instanceof List) {
                objects = (List)sourceData;
            }
        } else if (sourceType == 2 && sourceData instanceof List) {
            objects = (List)sourceData;
        }
        return objects;
    }

    @Override
    protected boolean setInputObjects(int sourceType, Object source, Object sourceData, List<Object> inputObjects) {
        boolean changed = false;
        if (this.source != source) {
            if (this.recordViewer != null && !this.recordViewer.isDisposed()) {
                this.recordViewer.removeChildListener(this.recordViewerChildListener);
            }
            this.recordViewer = null;
            ITlkPart sourceEditor = this.getEditor(source);
            if (sourceEditor instanceof IRecordViewer) {
                this.recordViewer = (IRecordViewer)sourceEditor;
                this.recordViewer.addChildListener(this.recordViewerChildListener);
            }
        }
        if (sourceType == 2) {
            changed = true;
            this.setInputSync(false);
        }
        return super.setInputObjects(sourceType, source, sourceData, inputObjects) | changed;
    }

    @Override
    protected AbstractInputViewPart.HistoryStatus createHistoryStatus() {
        return new RecordViewerHistoryStatus();
    }

    @Override
    protected void applyHistoryStatus(AbstractInputViewPart.HistoryStatus status) {
        this.position = ((RecordViewerHistoryStatus)status).position;
        super.applyHistoryStatus(status);
    }

    @Override
    protected void goBack() {
        if (!this.getInputSync()) {
            super.goBack();
        } else if (this.recordViewer != null) {
            this.recordViewer.command("de.toem.impulse.commands.goto", "back", 0, this);
        }
    }

    @Override
    protected boolean canGoBack() {
        if (!this.getInputSync()) {
            return super.canGoBack();
        }
        if (this.recordViewer != null) {
            return Boolean.TRUE.equals(this.recordViewer.command("de.toem.impulse.commands.goto", "back", 1, this));
        }
        return false;
    }

    @Override
    protected boolean isSyncEnabled() {
        return true;
    }

    @Override
    public Object command(String id, Object data, int doIt, Object sender) {
        if (id.equals("de.toem.impulse.commands.about")) {
            if (doIt != 5) {
                return this.doAbout(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.support.preferences")) {
            if (doIt != 5) {
                return this.doShowPreferences(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.views.enableGroups")) {
            if (doIt != 5) {
                return this.doEnableGroups(data, doIt, sender);
            }
        } else {
            return super.command(id, data, doIt, sender);
        }
        if (doIt == 5) {
            return true;
        }
        return null;
    }

    protected Object doAbout(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            return true;
        }
        return null;
    }

    private Object doShowPreferences(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            return true;
        }
        return null;
    }

    public Object doEnableGroups(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                this.setEnableGroups(!this.getEnableGroups());
            }
            return true;
        }
        if (doIt == 3) {
            return this.getEnableGroups();
        }
        return null;
    }

    protected boolean checkReaders(List<Object> inputObjects) {
        boolean containsValid = false;
        int n = 0;
        while (n < inputObjects.size()) {
            Object inputObject = inputObjects.get(n);
            if (inputObject instanceof ISimpleSamplesProvider) {
                containsValid = true;
            }
            ++n;
        }
        return containsValid;
    }

    protected void extractReaders(List<Object> inputObjects, List<IReadableSamples> readers, Map<IReadableSamples, ISimpleSamplesProvider> inputBySamples, Map<ISimpleSamplesProvider, IReadableSamples> samplesByInput) {
        int n = 0;
        while (n < inputObjects.size()) {
            Object inputObject = inputObjects.get(n);
            if (inputObject instanceof ISimpleSamplesProvider) {
                IReadableSamples samples;
                ISimpleSamplesProvider provider = (ISimpleSamplesProvider)inputObject;
                IReadableSamples iReadableSamples = samples = this.recordViewer != null && provider instanceof ISamplesProvider ? ((ISamplesProvider)provider).getSamples(this.recordViewer) : provider.getSamples();
                if (samples != null && !readers.contains(samples)) {
                    readers.add(samples);
                    inputBySamples.put(samples, provider);
                    samplesByInput.put(provider, samples);
                }
            }
            ++n;
        }
    }

    protected int positionToIndex(DomainPosition position) {
        if (position != null && this.readable != null) {
            int index = -1;
            if (position.samplesProvider != null && this.samplesByInput != null && this.samplesByInput.containsKey(position.samplesProvider)) {
                IReadableSamples unfiltered;
                IReadableSamples samples = this.samplesByInput.get(position.samplesProvider);
                ISamples iSamples = unfiltered = this.readable instanceof AbstractSamplesFilter ? ((AbstractSamplesFilter)this.readable).getReference() : this.readable;
                if (samples == unfiltered) {
                    index = position.idx;
                } else if (unfiltered instanceof SamplesMerger) {
                    index = ((SamplesMerger)unfiltered).indexAtSource(samples, position.idx);
                }
                if (this.readable instanceof AbstractSamplesFilter) {
                    index = ((AbstractSamplesFilter)this.readable).src2FilIdx(index);
                }
            }
            if (index == -1) {
                index = this.readable.indexAt(position.domainValue);
            }
            if (index == -1 && this.readable.getCount() > 0) {
                index = 0;
            }
            if (index != -1) {
                if (this.showGroups) {
                    index = this.readable.groupAt(index);
                }
                return index;
            }
        }
        return -1;
    }

    protected DomainPosition index2Position(int index) {
        if (this.readable != null) {
            IReadableSamples unfiltered;
            DomainValue domainValue = this.readable.positionAt(index);
            int idx = index;
            if (this.showGroups) {
                idx = this.readable.indexAtGroup(idx);
            }
            if (this.readable instanceof AbstractSamplesFilter) {
                idx = ((AbstractSamplesFilter)this.readable).fil2SrcIdx(idx);
            }
            IReadableSamples samples = null;
            ISamples iSamples = unfiltered = this.readable instanceof AbstractSamplesFilter ? ((AbstractSamplesFilter)this.readable).getReference() : this.readable;
            if (unfiltered instanceof SamplesMerger) {
                samples = ((SamplesMerger)unfiltered).getSourceAt(idx);
                idx = ((SamplesMerger)unfiltered).getSourceIdxAt(idx);
            } else {
                samples = unfiltered;
            }
            return new DomainPosition(domainValue, this.inputBySamples != null ? this.inputBySamples.get(samples) : null, idx);
        }
        return null;
    }

    class RecordViewerHistoryStatus
    extends AbstractInputViewPart.HistoryStatus {
        protected IRecordViewer recordViewer;
        protected DomainPosition position;

        public RecordViewerHistoryStatus() {
            super(SubView.this);
            this.recordViewer = SubView.this.recordViewer;
            this.position = SubView.this.position;
        }
    }
}

