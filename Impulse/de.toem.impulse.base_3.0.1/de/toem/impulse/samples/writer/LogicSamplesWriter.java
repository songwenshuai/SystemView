/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.writer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ILogicSamplesWriter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.writer.SamplesWriter;
import de.toem.impulse.values.Logic;

public class LogicSamplesWriter
extends SamplesWriter
implements ILogicSamplesWriter {
    private int bitWidth = 1;

    public LogicSamplesWriter(String id, String name, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        super(id, name, processType, signalType, signalDescriptor, domainBase);
        this.bitWidth = signalDescriptor.getScale();
    }

    @Override
    public final boolean write(long units, boolean tag, byte states) {
        return this.write(units, tag, states >= 2 ? (states >= 4 ? 3 : 2) : 1, states);
    }

    @Override
    public final boolean write(long units, boolean tag, int stateLevel, byte states) {
        if (this.signalType != ISamples.SignalType.Logic) {
            return false;
        }
        int begin = this.beginWrite(units, 1);
        if (begin < 0) {
            return false;
        }
        if (stateLevel == 1) {
            this.buffer[this.buffered++] = (byte)(0x40 | (states == 0 ? 0 : 8) | (tag ? 1 : 0));
            this.buffer[this.buffered++] = 0;
        } else {
            this.buffer[this.buffered++] = (byte)(stateLevel << 6 & 0xC0 | 0x10 | (tag ? 1 : 0));
            this.buffer[this.buffered++] = 1;
            this.buffer[this.buffered++] = stateLevel == 2 ? LOGIC_L4_BYTE_FILL[states] : LOGIC_L16_BYTE_FILL[states];
        }
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean write(long units, boolean tag, int stateLevel, byte precedingStates, byte state) {
        if (this.signalType != ISamples.SignalType.Logic) {
            return false;
        }
        int begin = this.beginWrite(units, 1);
        if (begin < 0) {
            return false;
        }
        if (stateLevel == 1 && (this.bitWidth == 1 || precedingStates == state)) {
            this.buffer[this.buffered++] = (byte)(0x40 | (state == 0 ? 0 : 8) | (tag ? 1 : 0));
            this.buffer[this.buffered++] = 0;
        } else {
            this.buffer[this.buffered++] = (byte)(stateLevel << 6 & 0xC0 | 0x10 | (tag ? 1 : 0));
            this.buffer[this.buffered++] = 1;
            switch (stateLevel) {
                case 1: {
                    this.buffer[this.buffered++] = (byte)(LOGIC_L2_BYTE_FILL[precedingStates] & 0xFE | state & 1);
                    break;
                }
                case 2: {
                    this.buffer[this.buffered++] = (byte)(LOGIC_L4_BYTE_FILL[precedingStates] & 0xFC | state & 3);
                    break;
                }
                case 3: {
                    this.buffer[this.buffered++] = (byte)(LOGIC_L16_BYTE_FILL[precedingStates] & 0xF0 | state & 0xF);
                }
            }
        }
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean write(long units, boolean tag, int stateLevel, byte precedingStates, byte[] states, int start, int length) {
        if (this.signalType != ISamples.SignalType.Logic || length > this.bitWidth) {
            return false;
        }
        int statesPerByte = 8 >>> stateLevel - 1;
        int dlength = Math.min((length + statesPerByte) / statesPerByte, (this.bitWidth + statesPerByte - 1) / statesPerByte);
        int begin = this.beginWrite(units, dlength);
        if (begin < 0) {
            return false;
        }
        if (stateLevel == 1 && (this.bitWidth == 1 || length == 0)) {
            this.buffer[this.buffered++] = (byte)(0x40 | ((length == 0 ? precedingStates : states[start]) == 0 ? 0 : 8) | (tag ? 1 : 0));
            this.buffer[this.buffered++] = 0;
        } else {
            this.buffer[this.buffered++] = (byte)(stateLevel << 6 & 0xC0 | 0x10 | (tag ? 1 : 0));
            if (dlength <= 15) {
                this.buffer[this.buffered++] = (byte)dlength;
            } else {
                int s = dlength;
                this.buffer[this.buffered++] = (byte)(s & 0xF | 0x10);
                s >>>= 4;
                while (true) {
                    if (s <= 127) {
                        this.buffer[this.buffered++] = (byte)(s & 0x7F);
                        break;
                    }
                    this.buffer[this.buffered++] = (byte)(s & 0x7F | 0x80);
                    s >>>= 7;
                }
            }
            switch (stateLevel) {
                case 1: {
                    byte fill = LOGIC_L2_BYTE_FILL[precedingStates];
                    int to = length - dlength * 8;
                    int n = 0;
                    while (n < dlength) {
                        byte d = fill;
                        int from = to;
                        to += 8;
                        if (from < 0) {
                            from = 0;
                        }
                        int i = from;
                        while (i < to) {
                            d = (byte)(d << 1 | states[start + i]);
                            ++i;
                        }
                        this.buffer[this.buffered++] = d;
                        ++n;
                    }
                    break;
                }
                case 2: {
                    byte fill = LOGIC_L4_BYTE_FILL[precedingStates];
                    int to = length - dlength * 4;
                    int n = 0;
                    while (n < dlength) {
                        byte d = fill;
                        int from = to;
                        to += 4;
                        if (from < 0) {
                            from = 0;
                        }
                        int i = from;
                        while (i < to) {
                            d = (byte)(d << 2 | states[start + i]);
                            ++i;
                        }
                        this.buffer[this.buffered++] = d;
                        ++n;
                    }
                    break;
                }
                case 3: {
                    byte fill = LOGIC_L16_BYTE_FILL[precedingStates];
                    int to = length - dlength * 2;
                    int n = 0;
                    while (n < dlength) {
                        byte d = fill;
                        int from = to;
                        to += 2;
                        if (from < 0) {
                            from = 0;
                        }
                        int i = from;
                        while (i < to) {
                            d = (byte)(d << 4 | states[start + i]);
                            ++i;
                        }
                        this.buffer[this.buffered++] = d;
                        ++n;
                    }
                    break;
                }
            }
        }
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean write(long units, boolean tag, byte precedingStates, byte[] states, int start, int length) {
        if (length == this.bitWidth) {
            precedingStates = states[start];
        }
        boolean crop = true;
        int totalLen = length;
        int initialStart = start;
        byte maxState = precedingStates;
        int n = initialStart;
        while (n < initialStart + totalLen) {
            if (crop && precedingStates == states[n]) {
                ++start;
                --length;
            } else {
                crop = false;
            }
            if (states[n] > maxState) {
                maxState = states[n];
            }
            ++n;
        }
        int stateLevel = maxState >= 2 ? (maxState >= 4 ? 3 : 2) : 1;
        return this.write(units, tag, stateLevel, precedingStates, states, start, length);
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
    public boolean write(long units, Logic logic) {
        return this.write(units, false, logic);
    }

    @Override
    public boolean write(long time, boolean tag, Logic logic) {
        if (logic != null) {
            return logic.write(this, time, tag);
        }
        return false;
    }

    @Override
    public boolean writeSample(long units, boolean tag, Object value) {
        if (value == null) {
            return this.write(units, tag);
        }
        if (value instanceof Logic) {
            return this.write(units, tag, (Logic)value);
        }
        return false;
    }

    @Override
    public int getBitWidth() {
        return this.bitWidth;
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
}

