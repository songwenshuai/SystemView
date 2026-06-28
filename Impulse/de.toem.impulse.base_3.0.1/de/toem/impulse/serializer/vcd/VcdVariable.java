/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer.vcd;

import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.record.SignalProxy;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.writer.DivergingSamplesWriter;
import de.toem.impulse.samples.writer.IConvergingSamplesWriter;
import de.toem.impulse.samples.writer.IDivergingSamplesWriter;
import de.toem.impulse.serializer.vcd.IVcdVariableSignalExtender;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.ICell;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class VcdVariable<E> {
    public ICell scope;
    public E handle;
    public boolean sharedHandle;
    public String name;
    public String description;
    public ISamples.SignalType signalType;
    public int scale;
    public int idx0;
    public int idx1;
    private Object group;
    private ISamplesWriter writer;
    public Signal signal;

    public boolean isGroupMember() {
        return this.group != this && this.group instanceof VcdVariable;
    }

    public boolean isGroupTop() {
        return this.group instanceof ArrayList;
    }

    public ArrayList<VcdVariable<E>> getGroup() {
        if (this.isGroupTop()) {
            return (ArrayList)this.group;
        }
        return null;
    }

    public boolean isNoGroup() {
        return this.group == this;
    }

    public int getScale() {
        if (this.isGroupTop()) {
            List group = (List)this.group;
            int from = ((VcdVariable)group.get((int)0)).idx1 >= 0 ? ((VcdVariable)group.get((int)0)).idx1 : ((VcdVariable)group.get((int)0)).idx0;
            int to = ((VcdVariable)group.get((int)(group.size() - 1))).idx0;
            return to - from + 1;
        }
        if (this.isNoGroup()) {
            return this.scale;
        }
        return 0;
    }

    public String getScaleText() {
        if (this.isGroupTop()) {
            List group = (List)this.group;
            int from = ((VcdVariable)group.get((int)0)).idx1 >= 0 ? ((VcdVariable)group.get((int)0)).idx1 : ((VcdVariable)group.get((int)0)).idx0;
            int to = ((VcdVariable)group.get((int)(group.size() - 1))).idx0;
            return "[" + to + ":" + from + "]";
        }
        return this.idx0 != -1 ? "[" + this.idx0 + (this.idx1 != -1 ? ":" + this.idx1 : "") + "]" : "";
    }

    public ISamples.SignalDescriptor getSignalDescriptor() {
        if (this.signalType == ISamples.SignalType.Float) {
            return this.scale > 32 ? ISamples.SignalDescriptor.Float64 : ISamples.SignalDescriptor.Float32;
        }
        if (this.signalType == ISamples.SignalType.Logic) {
            return ISamples.SignalDescriptor.LogicWidth(this.getScale());
        }
        return ISamples.SignalDescriptor.DEFAULT;
    }

    public static <E> void identifyGroups(Map<ICell, List<VcdVariable<E>>> varsByScope) {
        for (ICell scope : varsByScope.keySet()) {
            for (VcdVariable<E> var : varsByScope.get(scope)) {
                if (var.group != null) continue;
                var.group = var;
                ArrayList<VcdVariable<E>> group = new ArrayList<VcdVariable<E>>();
                for (VcdVariable<E> other : varsByScope.get(scope)) {
                    if (other.group != null || var.idx0 < -1 || !other.name.equals(var.name) || !other.signalType.equals((Object)var.signalType)) continue;
                    group.add(other);
                }
                if (group.isEmpty()) continue;
                group.add(var);
                Collections.sort(group, new Comparator<VcdVariable<E>>(){

                    @Override
                    public int compare(VcdVariable<E> o1, VcdVariable<E> o2) {
                        return Utils.compare(o1.idx0, o2.idx0, 2);
                    }
                });
                boolean continuousGroup = true;
                int min = 0;
                for (VcdVariable vcdVariable : group) {
                    if (vcdVariable.idx1 >= 0) {
                        if (vcdVariable.idx1 != min) {
                            continuousGroup = false;
                            break;
                        }
                        if (vcdVariable.idx0 - vcdVariable.idx1 + 1 != vcdVariable.scale) {
                            continuousGroup = false;
                            break;
                        }
                    } else if (vcdVariable.idx0 != min || vcdVariable.scale != 1) {
                        continuousGroup = false;
                        break;
                    }
                    min = vcdVariable.idx0 + 1;
                }
                if (continuousGroup) {
                    for (VcdVariable vcdVariable : group) {
                        vcdVariable.group = group.get(0);
                    }
                    ((VcdVariable)group.get((int)0)).group = group;
                    continue;
                }
                for (VcdVariable vcdVariable : group) {
                    vcdVariable.name = String.valueOf(vcdVariable.name) + vcdVariable.getScaleText();
                }
            }
        }
    }

    public static <E> void createSignals(Map<ICell, List<VcdVariable<E>>> varsByScope, Record record, TimeBase timeBase, IVcdVariableSignalExtender<E> extender) {
        HashMap<E, VcdVariable<E>> proxibleVarMap = new HashMap<E, VcdVariable<E>>();
        HashMap<E, ArrayList<List>> proxibleGroupsMap = new HashMap<E, ArrayList<List>>();
        for (ICell scope : varsByScope.keySet()) {
            List<VcdVariable<E>> scopeVars = varsByScope.get(scope);
            for (VcdVariable<E> var : scopeVars) {
                if (var.isGroupMember()) continue;
                Cell proxySignal = null;
                if (var.isGroupTop()) {
                    List group = (List)var.group;
                    if (var.sharedHandle) {
                        ArrayList<List> proxibleGroups = (ArrayList<List>)proxibleGroupsMap.get(var.handle);
                        if (proxibleGroups != null) {
                            for (List proxibleGroup : proxibleGroups) {
                                if (proxibleGroup.size() != group.size()) continue;
                                boolean ok = true;
                                int n = 0;
                                while (n < group.size()) {
                                    VcdVariable groupVar = (VcdVariable)group.get(n);
                                    VcdVariable refVar = (VcdVariable)proxibleGroup.get(n);
                                    if (!groupVar.handle.equals(refVar.handle) || groupVar.idx0 != refVar.idx0 || groupVar.idx1 != refVar.idx1 || groupVar.scale != refVar.scale) {
                                        ok = false;
                                        break;
                                    }
                                    ++n;
                                }
                                if (!ok) continue;
                                proxySignal = ((VcdVariable)proxibleGroup.get((int)0)).signal;
                                break;
                            }
                        }
                        if (proxySignal == null) {
                            if (proxibleGroups == null) {
                                proxibleGroups = new ArrayList<List>();
                                proxibleGroupsMap.put(var.handle, proxibleGroups);
                            }
                            proxibleGroups.add(group);
                        }
                    }
                } else if (var.sharedHandle) {
                    VcdVariable proxyVar = (VcdVariable)proxibleVarMap.get(var.handle);
                    if (proxyVar != null) {
                        proxySignal = proxyVar.signal;
                    } else {
                        proxibleVarMap.put(var.handle, var);
                    }
                }
                if (proxySignal != null) {
                    SignalProxy proxy = new SignalProxy();
                    proxy.signal = proxySignal.getLink(record);
                    proxy.description = ((Signal)proxySignal).description != null ? ((Signal)proxySignal).description.intern() : null;
                    var.scope.addChild(proxy);
                    proxy.setName(var.scope.uniqueChildName(var.name));
                    if (extender == null) continue;
                    extender.extend(var, var.scope, proxy, varsByScope);
                    continue;
                }
                Signal signal = new Signal();
                signal.signalType = var.signalType.toString();
                signal.signalDescriptor = var.getSignalDescriptor().toString();
                signal.description = (String.valueOf(var.description != null ? String.valueOf(var.description) + " " : "") + var.getScaleText()).intern();
                signal.domainBase = timeBase.toString();
                var.scope.addChild(signal);
                signal.setName(var.scope.uniqueChildName(var.name));
                var.signal = signal;
                if (extender == null) continue;
                extender.extend(var, var.scope, signal, varsByScope);
            }
        }
    }

    public static <E> Map<E, ISamplesWriter> createWriters(Map<ICell, List<VcdVariable<E>>> varsByScope, TimeBase timeBase, Object serializer) {
        HashMap<E, ISamplesWriter> writerByHandle = new HashMap<E, ISamplesWriter>();
        for (ICell scope : varsByScope.keySet()) {
            for (VcdVariable<E> var : varsByScope.get(scope)) {
                if (var.isGroupMember() || var.signal == null) continue;
                if (var.isGroupTop()) {
                    List group = (List)var.group;
                    ISamples.SignalType type = var.signalType;
                    ISamples.SignalDescriptor descr = var.getSignalDescriptor();
                    ISamplesWriter divisible = PackedSamples.createWriter(ISamples.ProcessType.Discrete, type, descr, timeBase, true);
                    var.signal.setData("SERIALIZER", serializer);
                    var.signal.setData("WRITER", divisible);
                    for (VcdVariable groupVar : group) {
                        ISamplesWriter writer;
                        groupVar.writer = writer = ((IConvergingSamplesWriter)divisible).createSource(groupVar.idx1 >= 0 ? groupVar.idx1 : groupVar.idx0, groupVar.scale);
                        if (writerByHandle.containsKey(groupVar.handle)) {
                            if (!(writerByHandle.get(groupVar.handle) instanceof IDivergingSamplesWriter)) {
                                writerByHandle.put(groupVar.handle, new DivergingSamplesWriter((ISamplesWriter)writerByHandle.get(groupVar.handle)));
                            }
                            ((IDivergingSamplesWriter)writerByHandle.get(groupVar.handle)).addDestination(writer);
                            continue;
                        }
                        writerByHandle.put(groupVar.handle, writer);
                    }
                    continue;
                }
                ISamples.SignalType type = var.signalType;
                ISamples.SignalDescriptor descr = var.getSignalDescriptor();
                var.signal.setData("SERIALIZER", serializer);
                ISamplesWriter writer = PackedSamples.createWriter(ISamples.ProcessType.Discrete, type, descr, timeBase, false);
                var.signal.setData("WRITER", writer);
                writerByHandle.put(var.handle, writer);
                var.writer = writer;
            }
        }
        return writerByHandle;
    }

    public static <E> Map<E, ISamplesWriter> createWriters(VcdVariable<E> var, Object serializer) {
        HashMap<E, ISamplesWriter> writerByHandle = new HashMap<E, ISamplesWriter>();
        if (var.isGroupTop()) {
            List group = (List)var.group;
            ISamplesWriter converging = PackedSamples.createWriter(var.signal, true);
            var.signal.setData("SERIALIZER", serializer);
            var.signal.setData("WRITER", converging);
            for (VcdVariable groupVar : group) {
                ISamplesWriter writer;
                groupVar.writer = writer = ((IConvergingSamplesWriter)converging).createSource(groupVar.idx1 >= 0 ? groupVar.idx1 : groupVar.idx0, groupVar.scale);
                writerByHandle.put(groupVar.handle, writer);
            }
        } else {
            ISamplesWriter writer = PackedSamples.createWriter(var.signal, false);
            var.signal.setData("SERIALIZER", serializer);
            var.signal.setData("WRITER", writer);
            writerByHandle.put(var.handle, writer);
            var.writer = writer;
        }
        return writerByHandle;
    }

    public boolean equals(Object o) {
        if (!(o instanceof VcdVariable)) {
            return false;
        }
        VcdVariable that = (VcdVariable)o;
        if (this.scope != that.scope) {
            return false;
        }
        if (!Utils.equals(this.handle, that.handle)) {
            return false;
        }
        if (!Utils.equals(this.name, that.name)) {
            return false;
        }
        if (!Utils.equals(this.description, that.description)) {
            return false;
        }
        if (this.signalType != that.signalType) {
            return false;
        }
        if (this.scale != that.scale) {
            return false;
        }
        if (this.idx0 != that.idx0) {
            return false;
        }
        return this.idx1 == that.idx1;
    }
}

