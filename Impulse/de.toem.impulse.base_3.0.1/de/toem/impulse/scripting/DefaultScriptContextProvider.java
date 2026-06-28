/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.scripting;

import de.toem.impulse.cells.ports.IPortProgress;
import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.PortScope;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.RecordContent;
import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.record.SignalProxy;
import de.toem.impulse.cells.view.AxisConfiguration;
import de.toem.impulse.cells.view.CursorConfiguration;
import de.toem.impulse.cells.view.FolderConfiguration;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.cells.view.SourceReference;
import de.toem.impulse.cells.view.ViewConfiguration;
import de.toem.impulse.domain.AmpsBase;
import de.toem.impulse.domain.DateBase;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.FloatBase;
import de.toem.impulse.domain.FrequencyBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IndexBase;
import de.toem.impulse.domain.PixelBase;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.domain.UnknownBase;
import de.toem.impulse.domain.VoltsBase;
import de.toem.impulse.paint.IActiveValueProvider;
import de.toem.impulse.paint.ICursorItem;
import de.toem.impulse.paint.IFolderItem;
import de.toem.impulse.paint.IPaint;
import de.toem.impulse.paint.IPaintItem;
import de.toem.impulse.paint.IPaintStyle;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.IPlotTree;
import de.toem.impulse.paint.IPlotTreeMouseListener;
import de.toem.impulse.paint.ISelectItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.ITooltipDataProvider;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.samples.IBinarySamplesWriter;
import de.toem.impulse.samples.IEventSamplesWriter;
import de.toem.impulse.samples.IFloatSamplesWriter;
import de.toem.impulse.samples.IIntegerSamplesWriter;
import de.toem.impulse.samples.ILogicSamplesWriter;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.IStructSamplesWriter;
import de.toem.impulse.samples.ITextSamplesWriter;
import de.toem.impulse.samples.iterator.GroupPointer;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.serializer.IRecordReader;
import de.toem.impulse.values.AttachedLabel;
import de.toem.impulse.values.AttachedRelation;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.Logic;
import de.toem.impulse.values.Struct;
import de.toem.impulse.values.StructMember;
import de.toem.impulse.values.Transaction;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.element.exploits.Marker;
import de.toem.toolkits.pattern.element.serializer.Message;
import de.toem.toolkits.pattern.ide.IConsoleStream;
import de.toem.toolkits.pattern.information.IInformation;
import de.toem.toolkits.pattern.scripting.IScriptContextProvider;
import de.toem.toolkits.pattern.scripting.JavaDoc;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.InputStreamReader;

public class DefaultScriptContextProvider
implements IScriptContextProvider {
    boolean samples;
    boolean record;
    boolean port;
    boolean paint;
    boolean view;
    boolean element;
    static JavaDoc javadoc = new JavaDoc("de.toem.impulse.base", "javadoc/prop");
    static JavaDoc jre = new JavaDoc("de.toem.toolkits.javadoc", "javadoc/prop");
    static final Class<?>[] samplesClasses = new Class[]{ISample.class, ISamples.class, ISamples.ProcessType.class, ISamples.SignalType.class, ISamples.SignalDescriptor.class, ISamples.TagDomain.class, IRecordReader.class, InputStreamReader.class, FileReader.class, BufferedReader.class, File.class};
    static final Class<?>[] writerClasses = new Class[]{ISamplesWriter.class, IIntegerSamplesWriter.class, IFloatSamplesWriter.class, ILogicSamplesWriter.class, IEventSamplesWriter.class, IStructSamplesWriter.class, ITextSamplesWriter.class, IBinarySamplesWriter.class, ISamplesProducer.class, ISamplesProducer.IWritableSlaveProduction.class};
    static final Class<?>[] iteratorClasses = new Class[]{GroupPointer.class, SamplePointer.class, SamplesIterator.class, ISamplePointer.class};
    static final Class<?>[] valuesClass = new Class[]{AttachedLabel.class, AttachedRelation.class, Enumeration.class, Logic.class, Struct.class, StructMember.class, Transaction.class, CompoundValue.class, CompoundPack.class};
    static final Class<?>[] domainClasses = new Class[]{AmpsBase.class, DateBase.class, DomainBase.class, DomainValue.class, FloatBase.class, FrequencyBase.class, IDomainBase.class, IndexBase.class, PixelBase.class, TimeBase.class, UnknownBase.class, VoltsBase.class};
    static final Class<?>[] paintClasses = new Class[]{IActiveValueProvider.class, ICursorItem.class, IFolderItem.class, IPaint.class, IPaintItem.class, IPaintStyle.class, IPlotItem.class, IPlotTree.class, IPlotTreeMouseListener.class, IInformation.class, ISelectItem.class, ITheme.class, ITooltipDataProvider.class, ITreeItem.class};
    static final Class<?>[] elementClasses = new Class[]{Elements.class, ICell.class, ICover.class, IElement.class, Link.class, Marker.class};
    static final Class<?>[] viewClasses = new Class[]{AxisConfiguration.class, CursorConfiguration.class, FolderConfiguration.class, PlotConfiguration.class, SourceReference.class, ViewConfiguration.class};
    static final Class<?>[] recordClasses = new Class[]{AbstractSignal.class, Message.class, PortScope.class, Record.class, RecordContent.class, Scope.class, Signal.class, SignalProxy.class};

    public DefaultScriptContextProvider(boolean samples, boolean record, boolean port, boolean paint, boolean view, boolean element) {
        this.samples = samples;
        this.record = record;
        this.port = port;
        this.paint = paint;
        this.view = view;
        this.element = element;
    }

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        DefaultScriptContextProvider.provideDefaultScriptContext(context, this.samples, this.record, false, this.paint, this.view, this.element);
    }

    public static void provideDefaultScriptContext(IScriptContextProvider.IScriptContextInterface context, boolean samples, boolean record, boolean port, boolean paint, boolean view, boolean element) {
        if (samples) {
            context.addClasses(samplesClasses);
            context.addClasses(iteratorClasses);
            context.addClasses(valuesClass);
            context.addClasses(domainClasses);
            context.addClasses(writerClasses);
        }
        if (record) {
            context.addClasses(recordClasses);
        }
        if (paint) {
            context.addClasses(paintClasses);
        }
        if (view) {
            context.addClasses(viewClasses);
        }
        if (element) {
            context.addClasses(elementClasses);
        }
        context.addSymbol("console", IConsoleStream.class);
        if (port) {
            context.addSymbol("progress", IPortProgress.class);
        } else {
            context.addSymbol("progress", IProgress.class);
        }
        context.addDocProvider(javadoc);
        context.addDocProvider(jre);
    }
}

