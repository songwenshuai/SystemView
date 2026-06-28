/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.writer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.writer.ConvergingDiscreteLogicSamplesWriterSource;
import de.toem.impulse.samples.writer.IConvergingSamplesWriter;
import de.toem.impulse.samples.writer.LogicSamplesWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class ConvergingLogicSamplesWriter
extends LogicSamplesWriter
implements IConvergingSamplesWriter {
    protected byte[] states;
    protected byte[] previousStates;
    protected int stateTags;
    protected int previousStateTags;
    protected boolean writeOnChangeOnly = false;
    long current = Long.MIN_VALUE;
    List<ConvergingLogicSamplesWriter> writers = new ArrayList<ConvergingLogicSamplesWriter>();

    public ConvergingLogicSamplesWriter(String id, String name, ISamples.ProcessType processType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        super(id, name, processType, ISamples.SignalType.Logic, signalDescriptor, domainBase);
        this.states = new byte[this.signalDescriptor.getScale()];
        this.previousStates = new byte[this.states.length];
        int n = 0;
        while (n < this.states.length) {
            this.previousStates[n] = -1;
            this.states[n] = 6;
            ++n;
        }
    }

    public ConvergingLogicSamplesWriter setWriteOnChangeOnly(boolean writeOnChangeOnly) {
        this.writeOnChangeOnly = writeOnChangeOnly;
        return this;
    }

    public boolean isWriteOnChangeOnly() {
        return this.writeOnChangeOnly;
    }

    public void flushLogicStates(long units) {
        int scale = this.signalDescriptor.getScale();
        if (units > this.current) {
            if (this.current != Long.MIN_VALUE) {
                byte preceding = this.states[0];
                int first = 1;
                while (first < scale) {
                    if (this.states[first] != preceding) break;
                    ++first;
                }
                int level = 1;
                int index = 0;
                while (index < scale) {
                    if (this.states[index] > 1) {
                        level = 2;
                        break;
                    }
                    ++index;
                }
                while (index < scale) {
                    if (this.states[index] > 3) {
                        level = 3;
                        break;
                    }
                    ++index;
                }
                if (!this.writeOnChangeOnly || !Arrays.equals(this.previousStates, this.states) || this.previousStateTags != this.stateTags) {
                    this.write(this.current, this.stateTags > 0, level, preceding, this.states, first, scale - first);
                    if (this.writeOnChangeOnly) {
                        System.arraycopy(this.states, 0, this.previousStates, 0, this.states.length);
                        this.previousStateTags = this.stateTags;
                    }
                }
            }
            this.current = units;
        }
    }

    @Override
    public void close(long time) {
        this.flushLogicStates(time + 1L);
        super.close(time);
    }

    @Override
    public ISamplesWriter createSource(int index, int width) {
        return new ConvergingDiscreteLogicSamplesWriterSource(this, index, width);
    }
}

