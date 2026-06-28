/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.writer;

import de.toem.impulse.samples.ILogicSamplesWriter;
import de.toem.impulse.samples.writer.ConvergingLogicSamplesWriter;
import de.toem.impulse.samples.writer.ReferencedSamplesWriter;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.Logic;

public class ConvergingDiscreteLogicSamplesWriterSource
extends ReferencedSamplesWriter
implements ILogicSamplesWriter {
    int bit;
    int width;
    boolean tagged;

    protected ConvergingDiscreteLogicSamplesWriterSource(ConvergingLogicSamplesWriter writer, int bit, int width) {
        super(writer);
        this.bit = bit;
        this.width = width;
    }

    @Override
    public boolean write(long units, boolean tag) {
        ((ConvergingLogicSamplesWriter)this.writer).flushLogicStates(units);
        return this.writer.write(units, tag);
    }

    @Override
    public boolean write(long units, boolean tag, byte states) {
        ConvergingLogicSamplesWriter writer = (ConvergingLogicSamplesWriter)this.writer;
        writer.flushLogicStates(units);
        int b = this.bit;
        while (b < this.bit + this.width) {
            writer.states[writer.getBitWidth() - b - 1] = states;
            ++b;
        }
        writer.stateTags = writer.stateTags + ((tag ? 1 : 0) - (this.tagged ? 1 : 0));
        this.tagged = tag;
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, int stateLevel, byte states) {
        ConvergingLogicSamplesWriter writer = (ConvergingLogicSamplesWriter)this.writer;
        writer.flushLogicStates(units);
        int b = this.bit;
        while (b < this.bit + this.width) {
            writer.states[writer.getBitWidth() - b - 1] = states;
            ++b;
        }
        writer.stateTags = writer.stateTags + ((tag ? 1 : 0) - (this.tagged ? 1 : 0));
        this.tagged = tag;
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, int stateLevel, byte precedingStates, byte state) {
        ConvergingLogicSamplesWriter writer = (ConvergingLogicSamplesWriter)this.writer;
        writer.flushLogicStates(units);
        int b = this.bit + 1;
        while (b < this.bit + this.width) {
            writer.states[writer.getBitWidth() - b - 1] = precedingStates;
            ++b;
        }
        writer.states[writer.getBitWidth() - this.bit - 1] = state;
        writer.stateTags = writer.stateTags + ((tag ? 1 : 0) - (this.tagged ? 1 : 0));
        this.tagged = tag;
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, int stateLevel, byte precedingStates, byte[] states, int start, int length) {
        ConvergingLogicSamplesWriter writer = (ConvergingLogicSamplesWriter)this.writer;
        writer.flushLogicStates(units);
        int b = this.bit + length;
        while (b < this.bit + this.width) {
            writer.states[writer.getBitWidth() - b - 1] = precedingStates;
            ++b;
        }
        b = 0;
        while (b < length && b < this.width) {
            writer.states[writer.getBitWidth() - b - 1 - this.bit] = states[start + length - b - 1];
            ++b;
        }
        writer.stateTags = writer.stateTags + ((tag ? 1 : 0) - (this.tagged ? 1 : 0));
        this.tagged = tag;
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, byte precedingStates, String states) {
        char[] chars = new char[states.length()];
        states.getChars(0, states.length(), chars, 0);
        byte[] bstates = new byte[states.length()];
        int n = 0;
        while (n < chars.length) {
            bstates[n] = Logic.char2State[chars[n] & 0xFF];
            ++n;
        }
        return this.write(units, tag, precedingStates, bstates, 0, bstates.length);
    }

    @Override
    public boolean write(long units, boolean tag, byte precedingStates, byte[] states, int start, int length) {
        return this.write(units, tag, 0, precedingStates, states, start, length);
    }

    @Override
    public boolean write(long units, Logic logic) {
        return this.write(units, false, logic);
    }

    @Override
    public boolean write(long units, boolean tag, Logic logic) {
        return logic.write(this, units, tag);
    }

    @Override
    public boolean writeNone(long units, boolean tag) {
        return this.write(units, tag);
    }

    @Override
    public boolean writeByte(long units, boolean tag, byte states) {
        return this.write(units, tag, states);
    }

    @Override
    public boolean writeBytesP(long units, boolean tag, byte precedingStates, byte[] states, int start, int length) {
        return this.write(units, tag, precedingStates, states, start, length);
    }

    @Override
    public boolean writeStringP(long units, boolean tag, byte precedingStates, String states) {
        return this.write(units, tag, precedingStates, states);
    }

    @Override
    public boolean writeByteS(long units, boolean tag, int stateLevel, byte states) {
        return this.write(units, tag, stateLevel, states);
    }

    @Override
    public boolean writeByteSP(long units, boolean tag, int stateLevel, byte precedingStates, byte state) {
        return this.write(units, tag, stateLevel, precedingStates, state);
    }

    @Override
    public boolean writeBytesSP(long units, boolean tag, int stateLevel, byte precedingStates, byte[] states, int start, int length) {
        return this.write(units, tag, stateLevel, precedingStates, states, start, length);
    }

    @Override
    public boolean writeLogic(long units, Logic logic) {
        return this.write(units, false, logic);
    }

    @Override
    public boolean writeLogic(long units, boolean tag, Logic logic) {
        return this.write(units, tag, logic);
    }

    @Override
    public int getBitWidth() {
        return this.width;
    }

    @Override
    public boolean writeSample(long units, byte format0) {
        ((ConvergingLogicSamplesWriter)this.writer).flushLogicStates(units);
        return Logic.expand(this.width, format0, null, 0, 0).write(this, units, (format0 & 1) != 0);
    }

    @Override
    public boolean writeSample(long units, byte format0, byte data0) {
        ((ConvergingLogicSamplesWriter)this.writer).flushLogicStates(units);
        return Logic.expand(this.width, format0, new byte[]{data0}, 0, 1).write(this, units, (format0 & 1) != 0);
    }

    @Override
    public boolean writeSample(long units, byte format0, byte[] data, int start, int dlength) {
        ((ConvergingLogicSamplesWriter)this.writer).flushLogicStates(units);
        return Logic.expand(this.width, format0, data, start, dlength).write(this, units, (format0 & 1) != 0);
    }

    @Override
    public boolean writeSample(long units, byte format0, int group, int layer, byte[] data, int start, int dlength) {
        ((ConvergingLogicSamplesWriter)this.writer).flushLogicStates(units);
        return Logic.expand(this.width, format0, data, start, dlength).write(this, units, (format0 & 1) != 0);
    }

    @Override
    public boolean writeSample(CompoundPack packed) {
        ((ConvergingLogicSamplesWriter)this.writer).flushLogicStates(packed.getUnits());
        return Logic.expand(this.width, packed.getFormat0(), packed.getBytes(), 0, packed.getBytes() != null ? packed.getBytes().length : 0).write(this, packed.getUnits(), packed.isTagged());
    }
}

