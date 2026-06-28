/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.base;

import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.RecordContent;
import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.record.SignalProxy;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.INumberSamplesWriter;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.convert.ConvertedSample;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.samples.iterator.SamplesIterator;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;

public class RecordComparator {
    public static final String HINT_CONFIGURED = "compare.configured";
    public static final String HINT_DISABLE = "compare.disable";
    public static final String HINT_IGNORE_MORE = "compare.ignore.more";
    public static final String HINT_IGNORE_LESS = "compare.ignore.less";
    public static final String HINT_HIDE_IDENTICAL = "compare.hide.identical";
    public static final String HINT_INTEGER_DELTA = "compare.integer.delta";
    public static final String HINT_FLOAT_DELTA = "compare.float.delta";
    public static final String HINT_APPEND_ORIGINAL = "compare.original.append";
    public static final String HINT_FILTER = "compare.filter";
    public static final String HINT_FILTER_REGULAR = "compare.filter.delta";
    private Record a;
    private Record b;
    private Record diff;
    private boolean ignoreMore;
    private boolean ignoreLess;
    private boolean hideIdentical;
    private boolean integerDelta;
    private boolean floatDelta;
    private boolean appendOriginal;
    private String filter;
    private boolean regular;
    private Pattern pattern;
    private Map<ICell, List<ICell>> handled = new HashMap<ICell, List<ICell>>();
    private int toBeCompared;
    private int compared;

    public RecordComparator(Record a, Record b, Record diff) {
        this.a = a;
        this.b = b;
        this.diff = diff != null ? diff : new Record();
        this.ignoreMore = false;
        this.ignoreLess = false;
        this.hideIdentical = true;
        this.integerDelta = false;
        this.floatDelta = false;
        this.appendOriginal = true;
        this.filter = "";
        this.regular = false;
    }

    public static Record createDiff(IProgress p, Record a, Record b, Record diff) {
        return new RecordComparator(a, b, diff).createDiff(p);
    }

    public Record createDiff(IProgress p) {
        try {
            boolean modified;
            if (p != null) {
                p.doing(I18n.Diff_PrepareInputs);
            }
            this.prepare(this.a, this.b);
            if (p != null) {
                p.doing(I18n.Diff_PrepareDiff);
            }
            if (modified = this.create(p, this.a, this.b, this.diff)) {
                this.diff.diff = 1;
            }
            return this.diff;
        }
        catch (Throwable throwable) {
            return this.a;
        }
    }

    private void prepare(ICell ca, ICell cb) {
        if (!this.isDisabled(ca)) {
            for (ICell iCell : ca.getChildren(RecordContent.class)) {
                if (iCell instanceof SignalProxy) continue;
                RecordContent childB = cb.getChildByName(iCell.getName(), iCell instanceof Scope ? Scope.class : null);
                if (iCell instanceof Signal && childB instanceof SignalProxy) {
                    childB = ((SignalProxy)childB).getSignal();
                }
                if (childB == null || !Utils.equals(iCell.getClass(), childB.getClass())) continue;
                boolean different = false;
                if (iCell instanceof Scope) {
                    if (this.isDisabled(iCell)) continue;
                    this.prepare(iCell, childB);
                    continue;
                }
                if (!(iCell instanceof Signal) || !this.matchesFilter(iCell)) continue;
                different |= !Utils.equals((Object)ISamples.SignalType.valueOf((Signal)iCell), (Object)ISamples.SignalType.valueOf((Signal)childB));
                different |= !Utils.equals(ISamples.SignalDescriptor.valueOf((Signal)iCell).getScale(), ISamples.SignalDescriptor.valueOf((Signal)childB).getScale());
                if (different |= !Utils.equals(DomainBase.valueOf((Signal)iCell).getClass(), DomainBase.valueOf((Signal)childB).getClass())) continue;
                ++this.toBeCompared;
                this.prepareReader((Signal)iCell, (Signal)childB);
            }
        }
    }

    private void prepareReader(Signal sa, Signal sb) {
        if (sa.samples != null) {
            sa.samples.prepareGeneration();
        }
        if (sb.samples != null) {
            sb.samples.prepareGeneration();
        }
    }

    private boolean create(IProgress p, ICell ca, ICell cb, ICell cd) {
        if (p != null) {
            p.doing(String.valueOf(I18n.Diff_PrepareDiff) + " (" + this.compared + "of" + this.toBeCompared + ")");
        }
        boolean modification = false;
        for (ICell iCell : ca.getChildren(RecordContent.class)) {
            if (p != null && p.isCanceled()) {
                return modification;
            }
            if (iCell instanceof SignalProxy) continue;
            RecordContent childB = cb.getChildByName(iCell.getName(), iCell instanceof Scope ? Scope.class : null);
            if (iCell instanceof Signal && childB instanceof SignalProxy) {
                childB = ((SignalProxy)childB).getSignal();
            }
            if (childB != null && Utils.equals(iCell.getClass(), childB.getClass())) {
                boolean modified = false;
                boolean different = false;
                if (!this.handled.containsKey(cd)) {
                    this.handled.put(cd, new ArrayList());
                }
                this.handled.get(cd).add(childB);
                if (iCell instanceof Scope) {
                    if (this.isDisabled(iCell)) continue;
                    RecordContent clone = (RecordContent)iCell.clone(false);
                    if ((modified |= this.create(p, iCell, childB, clone)) | !this.hideIdentical) {
                        RecordComparator.append(cd, clone, modified ? 1 : 0, null);
                    }
                    modification |= modified;
                    continue;
                }
                if (!(iCell instanceof Signal) || !this.matchesFilter(iCell)) continue;
                different |= !Utils.equals((Object)ISamples.SignalType.valueOf((Signal)iCell), (Object)ISamples.SignalType.valueOf((Signal)childB));
                different |= !Utils.equals(ISamples.SignalDescriptor.valueOf((Signal)iCell).getScale(), ISamples.SignalDescriptor.valueOf((Signal)childB).getScale());
                if (!(different |= !Utils.equals(DomainBase.valueOf((Signal)iCell).getClass(), DomainBase.valueOf((Signal)childB).getClass()))) {
                    Signal sd = this.createDiff(p, (Signal)iCell, (Signal)childB);
                    ++this.compared;
                    if (sd != null) {
                        modified = true;
                        RecordComparator.append(cd, sd, 1, null);
                        if (this.appendOriginal) {
                            RecordComparator.append(sd, (RecordContent)this.extract(iCell, false), 0, "\u2780");
                            RecordComparator.append(sd, (RecordContent)this.extract(childB, false), 0, "\u2781");
                        }
                    } else if (!this.hideIdentical) {
                        RecordComparator.append(cd, (RecordContent)this.extract(iCell, false), 0, null);
                    }
                } else {
                    modified = true;
                    RecordComparator.append(cd, (RecordContent)this.extract(iCell, true), 3, null);
                    RecordComparator.append(cd, (RecordContent)this.extract(childB, true), 2, null);
                }
                modification |= modified;
                continue;
            }
            RecordComparator.append(cd, (RecordContent)this.extract(iCell, true), 3, null);
            modification = true;
        }
        for (ICell iCell : cb.getChildren(RecordContent.class)) {
            ICell childA = ca.getChildByName(iCell.getName());
            if (childA != null && this.isDisabled(childA) || iCell instanceof SignalProxy || iCell instanceof Signal && childA instanceof SignalProxy || !this.matchesFilter(iCell) || this.handled.get(cd) != null && this.handled.get(cd).contains(iCell)) continue;
            RecordComparator.append(cd, (RecordContent)this.extract(iCell, true), 2, null);
            modification = true;
        }
        return modification;
    }

    private Signal createDiff(IProgress p, Signal a, Signal b) {
        boolean prepareDelta;
        ISamplesReader ra = PackedSamples.createReader(a);
        ISamplesReader rb = PackedSamples.createReader(b, ra.getDomainBase());
        ra.ensureSettled(p);
        rb.ensureSettled(p);
        ISamples.SignalDescriptor sd = ra.getSignalDescriptor();
        if (sd.isTransaction()) {
            sd = ISamples.SignalDescriptor.DEFAULT;
        }
        ISamplesWriter wd = PackedSamples.createWriter(ra.getProcessType(), ra.getSignalType(), sd, ra.getDomainBase());
        wd.setTagDomain(ISamples.TagDomain.Diff);
        wd.setLegend(ra.getLegend());
        SamplePointer pa = new SamplePointer(ra);
        SamplePointer pb = new SamplePointer(rb);
        boolean hasDifference = false;
        boolean bl = prepareDelta = wd.getSignalType() == ISamples.SignalType.Integer && this.integerDelta || wd.getSignalType() == ISamples.SignalType.Float && this.floatDelta;
        if (ra != null && rb != null && wd != null) {
            SamplesIterator it = new SamplesIterator(pa, pb);
            wd.open(it.getStart());
            boolean wasBothNull = true;
            boolean wasDifferentNull = false;
            while (it.hasNext()) {
                boolean differentPos;
                boolean differentValue;
                boolean bothNull;
                boolean nullValue;
                ISample packb;
                ISample packa;
                long current = it.next();
                if (current >= pa.getEndUnits() && this.ignoreMore) {
                    wd.close(current);
                    break;
                }
                if (current >= pb.getEndUnits() && this.ignoreLess) {
                    wd.close(current);
                    break;
                }
                if (prepareDelta) {
                    packa = pa.compound();
                    packb = pb.compound();
                    nullValue = packa == null || packb == null;
                    bothNull = packa == null && packb == null;
                    differentValue = nullValue && !bothNull || !nullValue && !((CompoundValue)packa).equalValues((CompoundValue)packb);
                    boolean bl2 = differentPos = !nullValue && ((CompoundValue)packa).getUnits() != ((CompoundValue)packb).getUnits();
                    if (!differentValue && differentPos) {
                        ((INumberSamplesWriter)wd).write(current, false, 0);
                        hasDifference = true;
                        wasDifferentNull = false;
                        wasBothNull = false;
                        continue;
                    }
                    if (differentValue && !nullValue) {
                        Double delta = (packb != null ? ((ConvertedSample)packb).numberValue().doubleValue() : 0.0) - (packa != null ? ((ConvertedSample)packa).numberValue().doubleValue() : 0.0);
                        ((INumberSamplesWriter)wd).write(current, false, delta);
                        hasDifference = true;
                        wasDifferentNull = false;
                        wasBothNull = false;
                        continue;
                    }
                    if (differentValue) {
                        if (!wasDifferentNull) {
                            wd.writeSample(current, (byte)1);
                        }
                        hasDifference = true;
                        wasDifferentNull = true;
                        wasBothNull = false;
                        continue;
                    }
                    if (bothNull) {
                        if (!wasBothNull) {
                            wd.writeSample(current, (byte)0);
                        }
                        hasDifference = false;
                        wasDifferentNull = false;
                        wasBothNull = true;
                        continue;
                    }
                    ((INumberSamplesWriter)wd).write(current, false, 0);
                    wasBothNull = false;
                    wasDifferentNull = false;
                    continue;
                }
                packa = pa.packed();
                packb = pb.packed();
                nullValue = packa == null || packb == null;
                bothNull = packa == null && packb == null;
                differentValue = nullValue && !bothNull || !nullValue && !((CompoundPack)packa).equalValues((CompoundPack)packb);
                boolean bl3 = differentPos = !nullValue && ((CompoundPack)packa).getUnits() != ((CompoundPack)packb).getUnits();
                if (!differentValue && differentPos) {
                    ((CompoundPack)packa).setUnits(current);
                    ((CompoundPack)packa).setTag(true);
                    wd.writeSample((CompoundPack)packa);
                    hasDifference = true;
                    wasDifferentNull = false;
                    wasBothNull = false;
                    continue;
                }
                if (differentValue) {
                    if (!wasDifferentNull) {
                        wd.writeSample(current, (byte)1);
                    }
                    hasDifference = true;
                    wasDifferentNull = true;
                    wasBothNull = false;
                    continue;
                }
                if (bothNull) {
                    if (!wasBothNull) {
                        wd.writeSample(current, (byte)0);
                    }
                    hasDifference = false;
                    wasDifferentNull = false;
                    wasBothNull = true;
                    continue;
                }
                ((CompoundPack)packa).setUnits(current);
                ((CompoundPack)packa).setTag(false);
                wd.writeSample((CompoundPack)packa);
                wasDifferentNull = false;
                wasBothNull = false;
            }
            wd.close(it.getEnd());
        }
        if (!hasDifference) {
            return null;
        }
        Signal diff = new Signal();
        diff.setName(String.valueOf(prepareDelta ? "\u0394" : "\u2248") + a.getName());
        diff.description = a.description;
        wd.apply(diff);
        return diff;
    }

    private boolean matchesFilter(ICell cell) {
        if (Utils.isEmpty(this.filter)) {
            return true;
        }
        if (this.regular) {
            if (this.pattern == null && !Utils.isEmpty(this.filter)) {
                try {
                    this.pattern = Pattern.compile(this.filter);
                }
                catch (Throwable throwable) {
                    this.filter = "";
                }
            }
            if (this.pattern != null) {
                return this.pattern.matcher(cell.getName()).matches();
            }
            return false;
        }
        return cell.getName().toLowerCase().contains(this.filter.toLowerCase());
    }

    private ICell extract(ICell cell, boolean children) {
        if (cell.getParent() != null) {
            cell.getParent().extractChild(cell);
        }
        return cell;
    }

    private boolean isDisabled(ICell cell) {
        return cell.getElement().isBound() && cell.getElement().getHint(HINT_DISABLE) != null;
    }

    private static RecordContent append(ICell parent, RecordContent cell, int diff, String prefix) {
        if (prefix != null) {
            cell.setName(String.valueOf(prefix) + cell.getName());
        }
        parent.addChild(cell);
        cell.diff = diff;
        return cell;
    }
}

