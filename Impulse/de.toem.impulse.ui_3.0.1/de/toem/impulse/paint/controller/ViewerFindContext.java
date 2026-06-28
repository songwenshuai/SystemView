/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.controller;

import de.toem.impulse.ImpulseBase;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.cells.view.SearchConfiguration;
import de.toem.impulse.cells.view.SourceReference;
import de.toem.impulse.dialog.sample.FindDialog;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.paint.ICursorItem;
import de.toem.impulse.provider.ISamplesProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.impulse.values.CompoundValue;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.ide.IConsoleStream;
import de.toem.toolkits.pattern.provider.IContext;
import de.toem.toolkits.pattern.scripting.IScripting;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IEvaluable;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.abstrac.IFindReplaceContext;
import de.toem.toolkits.ui.part.ITlkPart;
import java.util.List;

public class ViewerFindContext
implements IFindReplaceContext {
    private IFindReplaceTarget target;
    public static final String VAR_PREFIX = "s";
    private IFindReplaceTargetController controller;
    private ITlkPart parent;
    private IElement configuration;
    private SearchConfiguration search;
    private IScripting scripting;
    private SamplesIterator iterator;
    private FindDialog dialog;
    IConsoleStream console;

    public ViewerFindContext(IFindReplaceTargetController controller) {
        this.controller = controller;
        this.parent = controller.getEditor();
        this.console = ImpulseBase.newConsoleStream();
    }

    public IFindReplaceTargetController getController() {
        return this.controller;
    }

    @Override
    public void open() {
        this.configuration = this.controller.getView();
        if (this.configuration.isBound()) {
            this.configuration.hasCell();
        }
    }

    @Override
    public void findPrevious() {
        this.find(true);
    }

    @Override
    public void findNext() {
        this.find(false);
    }

    public ITlkPart getEditorParent() {
        return this.parent;
    }

    public IElement getConfiguration() {
        return this.configuration;
    }

    public void prepareInitialSearchConfiguration(SearchConfiguration search) {
        if (this.controller.getActiveCursor() == null) {
            return;
        }
        StringBuilder builder = new StringBuilder();
        search.name = null;
        search.description = null;
        search.removeAllChildren();
        int index = 0;
        DomainValue position = this.controller.getActiveCursor().getPosition();
        for (ICell cell : this.controller.getSelectedCells()) {
            if (!(cell instanceof PlotConfiguration)) continue;
            SourceReference ref = new SourceReference();
            ref.reference = cell.getLink(this.controller.getView());
            ref.base = this.controller.getView().getLink();
            search.addChild(ref);
            String term = VAR_PREFIX + index;
            IReadableSamples readable = ((PlotConfiguration)cell).getSamples(this.controller);
            if (readable != null) {
                int idx = readable.indexAt(position);
                CompoundValue cv = null;
                String member = null;
                String txt = null;
                if (idx >= 0) {
                    switch (readable.getSignalType()) {
                        case Logic: {
                            if (readable.getSignalDescriptor().getScale() == 1) {
                                boolean onIndex;
                                boolean bl = onIndex = position != null && position.equals(readable.positionAt(idx));
                                if (onIndex && readable.isEdgeAt(idx, 1)) {
                                    term = String.valueOf(term) + ".isEdge(1)";
                                    break;
                                }
                                if (onIndex && readable.isEdgeAt(idx, 0, null)) {
                                    term = String.valueOf(term) + ".isEdge(-1)";
                                    break;
                                }
                                if (readable.isHighAt(idx, null)) {
                                    term = String.valueOf(term) + ".isHigh()";
                                    break;
                                }
                                if (readable.isLowAt(idx, null)) {
                                    term = String.valueOf(term) + ".isLow()";
                                    break;
                                }
                                term = String.valueOf(term) + ".fbin()=='" + readable.formatAt(idx, 1) + "'";
                                break;
                            }
                            term = String.valueOf(term) + ".fhex()=='" + readable.formatAt(idx, 3) + "'";
                            break;
                        }
                        case Integer: {
                            term = String.valueOf(term) + ".longValue()==" + readable.longValueAt(idx);
                            break;
                        }
                        case Float: {
                            term = String.valueOf(term) + ".doubleValue()==" + readable.doubleValueAt(idx);
                            break;
                        }
                        case Event: 
                        case Text: {
                            txt = readable.stringValueAt(idx);
                            term = String.valueOf(term) + ".stringValue()==" + (txt != null ? "'" + txt + "'" : "null");
                            break;
                        }
                        case Struct: {
                            cv = readable.compoundAt(idx);
                            member = cv.nameOf(0);
                            txt = cv.stringValueOf(member);
                            if (member == null) break;
                            term = String.valueOf(term) + ".stringValueOf('" + member + "')==" + (txt != null ? "'" + txt + "'" : "null");
                            break;
                        }
                        case IntegerArray: {
                            cv = readable.compoundAt(idx);
                            if (cv == null) break;
                            term = String.valueOf(term) + ".longValueOf(0)==" + cv.longValueOf(0);
                            break;
                        }
                        case FloatArray: {
                            cv = readable.compoundAt(idx);
                            if (cv == null) break;
                            term = String.valueOf(term) + ".doubleValueOf(0)==" + cv.doubleValueOf(0);
                            break;
                        }
                        case EventArray: 
                        case TextArray: {
                            cv = readable.compoundAt(idx);
                            txt = cv.stringValueOf(0);
                            if (cv == null) break;
                            term = String.valueOf(term) + ".stringValueOf(0)==" + (txt != null ? "'" + txt + "'" : "null");
                            break;
                        }
                        case Binary: {
                            byte[] bytes = readable.bytesValueAt(idx);
                            term = String.valueOf(term) + ".val()[0] == " + (bytes != null && bytes.length > 0 ? bytes[0] : 0);
                            break;
                        }
                    }
                }
            }
            if (!term.equals(VAR_PREFIX + index)) {
                if (builder.length() > 0) {
                    builder.append(" && \n");
                }
                builder.append(term);
            }
            ++index;
        }
        search.expression = builder.toString();
    }

    public String find(boolean reverse) {
        if (this.search != null) {
            return this.find(this.search, reverse, this.search.wrap);
        }
        return I18n.SamplesFindContext_NoSearchConfiguration;
    }

    public String find(IElement search) {
        if (search.isBound() && search.hasCell(SearchConfiguration.class)) {
            SearchConfiguration s = (SearchConfiguration)search.getCell();
            return this.find(s, s.reverse, s.wrap);
        }
        return I18n.SamplesFindContext_NoSearchConfiguration;
    }

    public String find(final SearchConfiguration search, final boolean reverse, final boolean wrap) {
        try {
            return (String)Actives.timeout(new IEvaluable(){

                @Override
                public Object evaluate(IProgress p) throws Throwable {
                    block13: {
                        Object result;
                        ViewerFindContext.this.search = search;
                        if (!ViewerFindContext.this.initScript()) {
                            return I18n.SamplesFindContext_CouldNotInitScripting;
                        }
                        boolean wrapped = false;
                        if (ViewerFindContext.this.controller.getActiveCursor() == null) {
                            return I18n.SamplesFindContext_NoActiveCursor;
                        }
                        DomainValue position = ViewerFindContext.this.controller.getActiveCursor().getPosition();
                        position.units = position.units + (long)(reverse ? -1 : 1);
                        ViewerFindContext.this.iterator.setCurrent(position, false);
                        do {
                            if (reverse) {
                                if (ViewerFindContext.this.iterator.hasPrev()) {
                                    ViewerFindContext.this.iterator.prev();
                                } else {
                                    if (wrapped || !wrap) {
                                        return I18n.SamplesFindContext_NotFound;
                                    }
                                    ViewerFindContext.this.iterator.setCurrent(ViewerFindContext.this.iterator.getEndPosition(), false);
                                    ViewerFindContext.this.iterator.prev();
                                    wrapped = true;
                                }
                            } else if (ViewerFindContext.this.iterator.hasNext()) {
                                ViewerFindContext.this.iterator.next();
                            } else {
                                if (wrapped || !wrap) {
                                    return I18n.SamplesFindContext_NotFound;
                                }
                                ViewerFindContext.this.iterator.setCurrent(ViewerFindContext.this.iterator.getStartPosition(), true);
                                ViewerFindContext.this.iterator.next();
                                wrapped = true;
                            }
                            result = null;
                            try {
                                result = ViewerFindContext.this.scripting.run(null);
                            }
                            catch (Throwable e) {
                                return String.valueOf(I18n.SamplesFindContext_InvalidExpression_) + e.getMessage();
                            }
                            if (result != null && result.equals(true)) break block13;
                        } while (result instanceof Boolean);
                        return I18n.SamplesFindContext_NoBooleanExpression;
                    }
                    Actives.runInMain(new IExecutable(){

                        @Override
                        public void execute(IProgress p) {
                            ViewerFindContext.this.controller.moveActiveCursor(ViewerFindContext.this.iterator.getCurrentPosition(), true, 0, true);
                        }
                    });
                    return null;
                }
            }, (Object)I18n.SamplesFindContext_TimeOut, 8000);
        }
        catch (Throwable throwable) {
            return null;
        }
    }

    public boolean initScript() {
        List<SourceReference> references = this.search.getChildren(SourceReference.class);
        int idx = 0;
        ISamplePointer[] pointers = new ISamplePointer[references.size()];
        for (ICell iCell : this.search.getChildren(SourceReference.class)) {
            IReadableSamples readable;
            ICell refered;
            SourceReference ref = (SourceReference)iCell;
            if (ref.reference != null && (refered = ref.reference.resolveCell(this.getConfiguration())) instanceof ISamplesProvider && (readable = ((ISamplesProvider)((Object)refered)).getSamples(this.controller)) != null) {
                pointers[idx] = new SamplePointer(readable);
            }
            ++idx;
        }
        this.iterator = new SamplesIterator(pointers);
        int n = idx;
        this.scripting = Scripting.create(this.search, "expression", s -> {
            int i = 0;
            while (i < N) {
                s.setSymbol(VAR_PREFIX + i, pointers[i]);
                ++i;
            }
            s.setSymbol("signals", pointers);
            s.setSymbol("iter", this.iterator);
            s.setSymbol("console", this.console);
        });
        return this.scripting != null;
    }

    public static interface IFindReplaceTarget {
    }

    public static interface IFindReplaceTargetController
    extends IController,
    IContext {
        public IElement getView();

        public ICursorItem getActiveCursor();

        public void moveActiveCursor(DomainValue var1, boolean var2, int var3, boolean var4);
    }
}

