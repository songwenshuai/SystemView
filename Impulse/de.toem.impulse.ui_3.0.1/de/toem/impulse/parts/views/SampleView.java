/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.parts.views;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cell.parts.SampleViewMemberPreferences;
import de.toem.impulse.cell.parts.SampleViewPreferences;
import de.toem.impulse.cell.parts.SampleViewTypePreferences;
import de.toem.impulse.dialog.parts.SampleViewPreferenceDialog;
import de.toem.impulse.dialog.sample.SampleDialog;
import de.toem.impulse.parts.views.SubView;
import de.toem.impulse.provider.ISimpleSamplesProvider;
import de.toem.impulse.samples.IGroupPointer;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IPointer;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesDisplayInformation;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.iterator.GroupPointer;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.producer.AbstractSamplesFilter;
import de.toem.impulse.samples.producer.SamplesMerger;
import de.toem.impulse.ui.DomainPosition;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.GroupedValue;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.element.ElementModifierEvent;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.pattern.threading.Progress;
import de.toem.toolkits.ui.controller.layout.ShareLayoutController;
import de.toem.toolkits.ui.handler.ICommandHandler;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import de.toem.toolkits.ui.tlk.layout.TlkShareData;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class SampleView
extends SubView {
    protected SampleDialog.ISampleDialogInput target;
    private SampleDialog.ISampleDialogInput.IInputListener viewerInputChangedHandler;
    protected IPointer pointer;
    protected IControlProvider provider;
    SampleViewPreferences defaultPreferences = new SampleViewPreferences();
    protected IExecutable delayedApplyPresentation = p -> this.applyPresentation();
    private int applyPresentationCounter;

    public SampleView(ITlkPartContainer partContainer, int style) {
        super(partContainer, style);
    }

    public SampleView(ITlkPartContainer.ITlkEditorSession session, int style) {
        super(session, style);
    }

    public SampleView() {
    }

    @Override
    protected void createControls(Object container) {
        super.createControls(container);
        ITlkComposite composite = this.tlk().addComposite(container, new ShareLayoutController(this, String.valueOf(this.getId()) + ".details.sash", 2, 0).initDefaults(250.0f, 0), null, this.mainLayoutData, 0, null, null);
        ITlkComposite main = this.tlk().addComposite(composite, null, this.cols(), new TlkShareData(1), 0x100000, null, null);
        this.tlk().addSash(composite, null, null, 4);
        ITlkComposite details = this.tlk().addComposite(composite, null, this.cols(), new TlkShareData(2), 0, null, null);
        composite.setData("inspection.main", main);
        composite.setData("inspection.details", details);
        this.target = new SampleDialog.ISampleDialogInput(){

            @Override
            public void onInputChange(SampleDialog.ISampleDialogInput.IInputListener handler) {
                SampleView.this.viewerInputChangedHandler = handler;
            }

            @Override
            public String getLabel() {
                if (SampleView.this.inputBySamples == null || SampleView.this.readable == null || SampleView.this.readable.getDomainBase() == null || SampleView.this.pointer == null) {
                    return null;
                }
                if (SampleView.this.pointer instanceof ISamplePointer) {
                    if (SampleView.this.readable instanceof SamplesMerger) {
                        IReadableSamples providerSamples = SampleView.this.readable instanceof SamplesMerger ? ((SamplesMerger)SampleView.this.readable).getSourceAt(SampleView.this.pointer.getIndex()) : SampleView.this.readable;
                        ISamplesDisplayInformation descriptor = SampleView.this.inputBySamples.get(providerSamples) instanceof ISamplesDisplayInformation ? (ISamplesDisplayInformation)SampleView.this.inputBySamples.get(providerSamples) : null;
                        CompoundValue value = ((ISamplePointer)SampleView.this.pointer).compound();
                        if (value != null && descriptor != null) {
                            return SampleView.this.pointer.getIndex() + " (" + SampleView.this.pointer.getCount() + ")" + " / @" + value.getIndex() + " (" + providerSamples.getCount() + ")" + " in " + descriptor.getLabel();
                        }
                    }
                    return "Sample @" + SampleView.this.pointer.getIndex() + " (" + SampleView.this.pointer.getCount() + ")";
                }
                if (SampleView.this.pointer instanceof IGroupPointer) {
                    if (SampleView.this.readable instanceof SamplesMerger) {
                        IReadableSamples providerSamples = SampleView.this.readable;
                        if (SampleView.this.readable instanceof SamplesMerger) {
                            int index = ((SamplesMerger)SampleView.this.readable).indexAtGroup(SampleView.this.pointer.getIndex());
                            providerSamples = ((SamplesMerger)SampleView.this.readable).getSourceAt(index);
                        }
                        ISamplesDisplayInformation descriptor = SampleView.this.inputBySamples.get(providerSamples) instanceof ISamplesDisplayInformation ? (ISamplesDisplayInformation)SampleView.this.inputBySamples.get(providerSamples) : null;
                        GroupedValue value = ((IGroupPointer)SampleView.this.pointer).val();
                        if (value != null && descriptor != null) {
                            return SampleView.this.pointer.getIndex() + " (" + SampleView.this.pointer.getGroups() + ")" + " / #" + value.getIndex() + " (" + providerSamples.getGroups() + ")" + " in " + descriptor.getLabel();
                        }
                    }
                    return "Group #" + SampleView.this.pointer.getIndex() + " (" + SampleView.this.pointer.getGroups() + ")";
                }
                return null;
            }

            @Override
            public Object getColor() {
                ISamplesDisplayInformation descriptor;
                IReadableSamples providerSamples;
                if (SampleView.this.inputBySamples == null || SampleView.this.readable == null || SampleView.this.readable.getDomainBase() == null || SampleView.this.pointer == null) {
                    return -1;
                }
                if (SampleView.this.pointer instanceof ISamplePointer) {
                    if (SampleView.this.readable instanceof SamplesMerger) {
                        ISamplesDisplayInformation descriptor2;
                        providerSamples = SampleView.this.readable instanceof SamplesMerger ? ((SamplesMerger)SampleView.this.readable).getSourceAt(SampleView.this.pointer.getIndex()) : SampleView.this.readable;
                        ISamplesDisplayInformation iSamplesDisplayInformation = descriptor2 = SampleView.this.inputBySamples.get(providerSamples) instanceof ISamplesDisplayInformation ? (ISamplesDisplayInformation)SampleView.this.inputBySamples.get(providerSamples) : null;
                        if (descriptor2 != null) {
                            return descriptor2.getColor();
                        }
                    }
                } else if (SampleView.this.pointer instanceof IGroupPointer && SampleView.this.readable instanceof SamplesMerger) {
                    ISamplesDisplayInformation descriptor3;
                    providerSamples = SampleView.this.readable;
                    if (SampleView.this.readable instanceof SamplesMerger) {
                        int index = ((SamplesMerger)SampleView.this.readable).indexAtGroup(SampleView.this.pointer.getIndex());
                        providerSamples = ((SamplesMerger)SampleView.this.readable).getSourceAt(index);
                    }
                    ISamplesDisplayInformation iSamplesDisplayInformation = descriptor3 = SampleView.this.inputBySamples.get(providerSamples) instanceof ISamplesDisplayInformation ? (ISamplesDisplayInformation)SampleView.this.inputBySamples.get(providerSamples) : null;
                    if (descriptor3 != null) {
                        return descriptor3.getColor();
                    }
                }
                ISamplesDisplayInformation iSamplesDisplayInformation = descriptor = SampleView.this.inputBySamples.get(SampleView.this.readable) instanceof ISamplesDisplayInformation ? (ISamplesDisplayInformation)SampleView.this.inputBySamples.get(SampleView.this.readable) : null;
                if (descriptor != null) {
                    return descriptor.getColor();
                }
                return null;
            }

            @Override
            public IPointer getPointer() {
                return SampleView.this.pointer;
            }

            @Override
            public ICell getPreferences(int type, Object selector) {
                List<IMemberDescriptor> descriptors;
                SampleViewPreferences preferences = SampleView.this.getPreferences();
                if (preferences == null) {
                    preferences = SampleView.this.defaultPreferences;
                }
                if (type == 0 || preferences == null) {
                    return preferences;
                }
                SampleViewTypePreferences typePreferences = null;
                ISamples.SignalType signalType = SampleView.this.pointer != null ? SampleView.this.pointer.getSignalType() : ISamples.SignalType.Unknown;
                String content = SampleView.this.pointer != null && SampleView.this.pointer.getContent() != null ? SampleView.this.pointer.getContent() : "default";
                for (ICell p : preferences.getChildren()) {
                    if (!(p instanceof SampleViewTypePreferences) || !Utils.equals(signalType.toString(), ((SampleViewTypePreferences)p).signalType) || !Utils.equals(content, ((SampleViewTypePreferences)p).content)) continue;
                    typePreferences = (SampleViewTypePreferences)p;
                    break;
                }
                if (typePreferences == null) {
                    typePreferences = new SampleViewTypePreferences(signalType, content);
                    preferences.addChild(typePreferences);
                }
                SampleViewMemberPreferences memberPreferences = null;
                if (signalType.isArrayOrStruct() && (descriptors = SampleView.this.pointer.getMemberDescriptors()) != null) {
                    for (IMemberDescriptor descriptor : descriptors) {
                        String member = descriptor != null ? descriptor.getPath() : null;
                        String mcontent = descriptor != null ? descriptor.getContent() : null;
                        SampleViewMemberPreferences m = null;
                        for (ICell p : typePreferences.getChildren()) {
                            if (!(p instanceof SampleViewMemberPreferences) || !Utils.equals(member, ((SampleViewMemberPreferences)p).member) || !Utils.equals(mcontent, ((SampleViewMemberPreferences)p).content)) continue;
                            m = (SampleViewMemberPreferences)p;
                            break;
                        }
                        if (m == null) {
                            m = new SampleViewMemberPreferences(signalType, member, mcontent);
                            typePreferences.addChild(m);
                        }
                        if (!member.equals(selector)) continue;
                        memberPreferences = m;
                    }
                }
                if (type == 1) {
                    return typePreferences;
                }
                if (type == 2) {
                    return memberPreferences;
                }
                return null;
            }

            @Override
            public boolean filterPreferences(ICell p) {
                ISamples.SignalType signalType;
                ISamples.SignalType signalType2 = signalType = SampleView.this.pointer != null ? SampleView.this.pointer.getSignalType() : ISamples.SignalType.Unknown;
                if (p instanceof SampleViewTypePreferences) {
                    String content = SampleView.this.pointer != null ? SampleView.this.pointer.getContent() : "default";
                    return !Utils.equals(signalType.toString(), ((SampleViewTypePreferences)p).signalType) || !Utils.equals(content, ((SampleViewTypePreferences)p).content);
                }
                if (p instanceof SampleViewMemberPreferences && signalType.isArrayOrStruct()) {
                    List<IMemberDescriptor> descriptors = SampleView.this.pointer.getMemberDescriptors();
                    for (IMemberDescriptor descriptor : descriptors) {
                        String mcontent;
                        String member = descriptor != null ? descriptor.getPath() : null;
                        String string = mcontent = descriptor != null ? descriptor.getContent() : null;
                        if (!Utils.equals(member, ((SampleViewMemberPreferences)p).member) || !Utils.equals(mcontent, ((SampleViewMemberPreferences)p).content)) continue;
                        return false;
                    }
                }
                return true;
            }

            private void syncBack(IPointer pointer) {
                SampleView.this.index = pointer != null ? pointer.getIndex() : 0;
                SampleView.this.position = SampleView.this.index2Position(SampleView.this.index);
                if (SampleView.this.getSelectionSync()) {
                    Actives.runFinally(SampleView.this.delayedSelectionChanged, 100);
                }
            }

            @Override
            public boolean canGoPrev() {
                return SampleView.this.pointer != null && SampleView.this.pointer.hasPrev();
            }

            @Override
            public boolean canGoNext() {
                return SampleView.this.pointer != null && SampleView.this.pointer.hasNext();
            }

            @Override
            public boolean canGoPos1() {
                return SampleView.this.pointer != null && SampleView.this.pointer.getIndex() > SampleView.this.pointer.getMinIndex();
            }

            @Override
            public boolean canGoEnd() {
                return SampleView.this.pointer != null && SampleView.this.pointer.getIndex() < SampleView.this.pointer.getMaxIndex();
            }

            @Override
            public boolean canGoTarget(Link link) {
                return link != null;
            }

            @Override
            public boolean canGoBack() {
                return SampleView.this.canGoBack();
            }

            @Override
            public void goPrev() {
                if (SampleView.this.pointer != null) {
                    SampleView.this.pointer.goPrev();
                    this.syncBack(SampleView.this.pointer);
                }
            }

            @Override
            public void goNext() {
                if (SampleView.this.pointer != null) {
                    SampleView.this.pointer.goNext();
                    this.syncBack(SampleView.this.pointer);
                }
            }

            @Override
            public void goPos1() {
                if (SampleView.this.pointer != null) {
                    SampleView.this.pointer.goPos1();
                    this.syncBack(SampleView.this.pointer);
                }
            }

            @Override
            public void goEnd() {
                if (SampleView.this.pointer != null) {
                    SampleView.this.pointer.goEnd();
                    this.syncBack(SampleView.this.pointer);
                }
            }

            @Override
            public void highlightAttachment(IAttachment attachment) {
                if (SampleView.this.recordViewer != null && !SampleView.this.recordViewer.isDisposed() && SampleView.this.position != null && SampleView.this.getSelectionSync() && SampleView.this.readable != null && SampleView.this.inputBySamples != null) {
                    IReadableSamples unfiltered;
                    int idx = SampleView.this.index;
                    if (SampleView.this.showGroups) {
                        idx = SampleView.this.readable.indexAtGroup(idx);
                    }
                    if (SampleView.this.readable instanceof AbstractSamplesFilter) {
                        idx = ((AbstractSamplesFilter)SampleView.this.readable).fil2SrcIdx(idx);
                    }
                    IReadableSamples samples = null;
                    ISamples iSamples = unfiltered = SampleView.this.readable instanceof AbstractSamplesFilter ? ((AbstractSamplesFilter)SampleView.this.readable).getReference() : SampleView.this.readable;
                    if (unfiltered instanceof SamplesMerger) {
                        samples = ((SamplesMerger)unfiltered).getSourceAt(idx);
                        ((SamplesMerger)unfiltered).getSourceIdxAt(idx);
                    } else {
                        samples = unfiltered;
                    }
                    SampleView.this.recordViewer.highlightAttachment((ISimpleSamplesProvider)SampleView.this.inputBySamples.get(samples), attachment);
                }
            }

            @Override
            public void goTarget(Link link) {
                if (SampleView.this.recordViewer != null && !SampleView.this.recordViewer.isDisposed()) {
                    if (SampleView.this.getInputSync()) {
                        SampleView.this.recordViewer.gotoTarget(link);
                    } else {
                        SampleView.this.recordViewer.requestTarget(link, SampleView.this.recordViewerChildListener);
                    }
                }
            }

            @Override
            public void goBack() {
                SampleView.this.goBack();
            }

            @Override
            public String getSourceIconId() {
                if (SampleView.this.recordViewer != null && !SampleView.this.recordViewer.isDisposed()) {
                    return SampleView.this.recordViewer.getIconId();
                }
                return null;
            }

            @Override
            public String getSourceLabel() {
                if (SampleView.this.recordViewer != null && !SampleView.this.recordViewer.isDisposed()) {
                    return SampleView.this.recordViewer.getLabel();
                }
                return null;
            }
        };
        this.provider = SampleDialog.getControls(this.target, true);
        this.provider.setLayout(this.cols(), this.cols());
        this.tlk().addControls(composite, this.provider);
        this.provider.editorOpened();
    }

    public void setFocus() {
        this.tlk().setFocusIfNotContained(this.provider);
    }

    @Override
    protected boolean setInputObjects(int sourceType, Object source, Object sourceData, List<Object> inputObjects) {
        return super.setInputObjects(sourceType, source, sourceData, inputObjects);
    }

    @Override
    protected void updateControls(ElementModifierEvent event) {
        if (event == null || event.getField() != null && event.getField().getName().toLowerCase().contains("show")) {
            this.presentationChanged = true;
        }
        super.updateControls(event);
    }

    @Override
    public SampleViewPreferences getPreferences() {
        if (this.getPersistence() != null) {
            return (SampleViewPreferences)ImpulsePreferences.partsPreferences.getCellByLink(this.getPersistence().partPreferences, SampleViewPreferences.class);
        }
        return this.defaultPreferences;
    }

    @Override
    public IControlProvider getPreferencesControls(Object owner) {
        IControlProvider preferenceControls = SampleViewPreferenceDialog.getControls(owner, new SampleViewPreferenceDialog.ITypeFilter(){

            @Override
            public boolean filter(ICell p) {
                return SampleView.this.target.filterPreferences(p);
            }
        }, 1);
        return preferenceControls;
    }

    @Override
    protected void recordViewerPositionChanged(DomainPosition position) {
        super.recordViewerPositionChanged(position);
        this.index = this.positionToIndex(position);
        if (this.pointer != null) {
            this.pointer.setIndex(this.index);
        }
        if (this.viewerInputChangedHandler != null) {
            this.viewerInputChangedHandler.indexChanged();
        }
    }

    @Override
    protected void applyPresentation() {
        super.applyPresentation();
        final int nextApplyPresentationCounter = ++this.applyPresentationCounter;
        final List<Object> inputObjects = this.getInputObjects();
        if (inputObjects == null) {
            return;
        }
        final ArrayList<IReadableSamples> nextReaders = new ArrayList<IReadableSamples>();
        final HashMap<IReadableSamples, ISimpleSamplesProvider> nextInputBySamples = new HashMap<IReadableSamples, ISimpleSamplesProvider>();
        final HashMap<ISimpleSamplesProvider, IReadableSamples> nextSamplesByInput = new HashMap<ISimpleSamplesProvider, IReadableSamples>();
        this.getSource();
        this.extractReaders(inputObjects, nextReaders, nextInputBySamples, nextSamplesByInput);
        Actives.run(new IExecutable(){

            @Override
            public void execute(IProgress p) {
                IReadableSamples unfiltered;
                IReadableSamples iReadableSamples = unfiltered = nextReaders.size() == 1 ? (IReadableSamples)nextReaders.get(0) : new SamplesMerger(null, null, nextReaders){

                    @Override
                    protected boolean continueExecution() {
                        return nextApplyPresentationCounter == SampleView.this.applyPresentationCounter;
                    }
                };
                if (nextApplyPresentationCounter != SampleView.this.applyPresentationCounter) {
                    return;
                }
                final IReadableSamples nextReadable = unfiltered;
                nextReadable.ensureSettled(new Progress());
                Actives.runInMain(new IExecutable(){

                    @Override
                    public void execute(IProgress p) {
                        (this).SampleView.this.readable = nextReadable;
                        (this).SampleView.this.inputBySamples = nextInputBySamples;
                        (this).SampleView.this.samplesByInput = nextSamplesByInput;
                        boolean showGroups = SampleView.this.getEnableGroups();
                        for (IReadableSamples reader : nextReaders) {
                            if (reader == null || reader.getGroups() > 0) continue;
                            showGroups = false;
                        }
                        (this).SampleView.this.pointer = showGroups ? ((this).SampleView.this.position != null && (this).SampleView.this.position.domainValue != null ? new GroupPointer(nextReadable, (this).SampleView.this.position.domainValue) : new GroupPointer(nextReadable, (this).SampleView.this.index)) : ((this).SampleView.this.position != null && (this).SampleView.this.position.domainValue != null ? new SamplePointer(nextReadable, (this).SampleView.this.position.domainValue) : new SamplePointer(nextReadable, (this).SampleView.this.index));
                        (this).SampleView.this.showGroups = showGroups;
                        if (SampleView.this.viewerInputChangedHandler != null) {
                            SampleView.this.viewerInputChangedHandler.pointerChanged();
                        }
                        if (SampleView.this.inputController != null) {
                            ArrayList<String> text = new ArrayList<String>();
                            for (ISimpleSamplesProvider input : nextInputBySamples.values()) {
                                if (!(input instanceof ISamplesDisplayInformation)) continue;
                                text.add(((ISamplesDisplayInformation)((Object)input)).getLabel());
                            }
                            SampleView.this.inputController.setValue(text.toArray(new String[text.size()]));
                        }
                    }
                });
                while (nextApplyPresentationCounter == SampleView.this.applyPresentationCounter && nextReadable.isVolatile()) {
                    Actives.sleep(1000);
                    if (!SampleView.this.getInputRefresh() || nextApplyPresentationCounter != SampleView.this.applyPresentationCounter) continue;
                    Actives.runInMain(new IExecutable(){

                        @Override
                        public void execute(IProgress p) {
                            ArrayList<IReadableSamples> updatedReaders = new ArrayList<IReadableSamples>();
                            SampleView.this.extractReaders(inputObjects, updatedReaders, nextInputBySamples, nextSamplesByInput);
                            if (!nextReaders.equals(updatedReaders)) {
                                SampleView.this.applyPresentation();
                                return;
                            }
                            if (unfiltered instanceof ISamplesProducer) {
                                int update = ((ISamplesProducer)unfiltered).update();
                                if (update < 0) {
                                    SampleView.this.applyPresentation();
                                    return;
                                }
                                if (update > 0) {
                                    unfiltered.ensureSettled(new Progress());
                                }
                            }
                        }
                    });
                }
            }
        });
    }

    @Override
    protected void applyInputText() {
    }

    @Override
    public Object command(String id, Object data, int doIt, Object sender) {
        Object result;
        if (!this.isValid()) {
            return null;
        }
        if (this.provider instanceof ICommandHandler && (result = ((ICommandHandler)((Object)this.provider)).command(id, data, doIt, sender)) != null) {
            return result;
        }
        if (id.equals("dummy")) {
            if (doIt != 5) {
                return false;
            }
        } else {
            return super.command(id, data, doIt, sender);
        }
        if (doIt == 5) {
            return true;
        }
        return null;
    }
}

