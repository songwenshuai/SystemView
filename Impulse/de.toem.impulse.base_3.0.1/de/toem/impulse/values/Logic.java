/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.samples.ILogicSamplesWriter;
import de.toem.impulse.values.IArrayValue;
import de.toem.impulse.values.ILogicOperation;
import java.math.BigInteger;
import java.util.Arrays;

public class Logic
extends Number
implements IArrayValue<byte[]>,
ILogicOperation {
    private static final long serialVersionUID = 1L;
    byte[] states;
    public static byte[] char2State = new byte[256];

    static {
        int n = 0;
        while (n < 256) {
            Logic.char2State[n] = 0;
            ++n;
        }
        n = 0;
        while (n < 16) {
            Logic.char2State[Logic.STATE_LC_DIGITS[n] & 0xFF] = (byte)n;
            Logic.char2State[Logic.STATE_UC_DIGITS[n] & 0xFF] = (byte)n;
            ++n;
        }
    }

    public Logic() {
        this.states = new byte[0];
    }

    public Logic(int bitWidth) {
        this.states = new byte[bitWidth];
    }

    public Logic(int bitWidth, byte fill) {
        this.states = new byte[bitWidth];
        Arrays.fill(this.states, fill);
    }

    public Logic(byte[] states) {
        this.states = new byte[states.length];
        System.arraycopy(states, 0, this.states, 0, states.length);
    }

    public Logic(Logic logic) {
        this.states = new byte[logic.states.length];
        System.arraycopy(logic.states, 0, this.states, 0, logic.states.length);
    }

    public Logic(int bitWidth, byte precedingStates, byte[] states) {
        this.states = new byte[bitWidth];
        Arrays.fill(this.states, precedingStates);
        int preceiding = bitWidth - states.length;
        System.arraycopy(states, preceiding >= 0 ? 0 : -preceiding, this.states, preceiding >= 0 ? preceiding : 0, preceiding >= 0 ? states.length : states.length + preceiding);
    }

    public Logic(String states) {
        char[] chars = new char[states.length()];
        states.getChars(0, states.length(), chars, 0);
        this.states = new byte[states.length()];
        int n = 0;
        while (n < chars.length) {
            this.states[n] = char2State[chars[n] & 0xFF];
            ++n;
        }
    }

    public Logic(int bitWidth, byte precedingStates, String states) {
        char[] chars = new char[states.length()];
        states.getChars(0, states.length(), chars, 0);
        byte[] bstates = new byte[states.length()];
        int n = 0;
        while (n < chars.length) {
            bstates[n] = char2State[chars[n] & 0xFF];
            ++n;
        }
        this.states = new byte[bitWidth];
        Arrays.fill(this.states, precedingStates);
        int preceiding = bitWidth - bstates.length;
        System.arraycopy(bstates, preceiding >= 0 ? 0 : -preceiding, this.states, preceiding >= 0 ? preceiding : 0, preceiding >= 0 ? bstates.length : bstates.length + preceiding);
    }

    public static Logic valueOf(boolean bool) {
        Logic logic = new Logic(1);
        logic.states[0] = (byte)(bool ? 1 : 0);
        return logic;
    }

    public static Logic valueOfBoolean(boolean bool) {
        Logic logic = new Logic(1);
        logic.states[0] = (byte)(bool ? 1 : 0);
        return logic;
    }

    public static Logic valueOf(byte state) {
        return new Logic(1, state);
    }

    public static Logic valueOfState(byte state) {
        return new Logic(1, state);
    }

    public static Logic valueOf(char state) {
        return new Logic(String.valueOf(state));
    }

    public static Logic valueOfChar(char state) {
        return new Logic(String.valueOf(state));
    }

    public static Logic valueOf(String states) {
        return new Logic(states);
    }

    public static Logic valueOf(long states) {
        return new Logic(String.valueOf(states >= 0L ? "0" : "") + Long.toBinaryString(states));
    }

    public static Logic valueOfLong(long states) {
        return new Logic(String.valueOf(states >= 0L ? "0" : "") + Long.toBinaryString(states));
    }

    public static Logic valueOf(int states) {
        return new Logic(String.valueOf(states >= 0 ? "0" : "") + Integer.toBinaryString(states));
    }

    public static Logic valueOfInt(int states) {
        return new Logic(String.valueOf(states >= 0 ? "0" : "") + Integer.toBinaryString(states));
    }

    public static Logic valueOfFloat(float states) {
        return Logic.valueOf(Float.floatToRawIntBits(states));
    }

    public static Logic valueOf(float states) {
        return Logic.valueOf(Float.floatToRawIntBits(states));
    }

    public static Logic valueOf(double states) {
        return Logic.valueOf(Double.doubleToRawLongBits(states));
    }

    public static Logic valueOfDoublr(double states) {
        return Logic.valueOf(Double.doubleToRawLongBits(states));
    }

    public static Logic valueOf(Number states) {
        if (states instanceof Byte) {
            return Logic.valueOf(states.byteValue());
        }
        if (states instanceof Short) {
            return Logic.valueOf(states.shortValue());
        }
        if (states instanceof Integer) {
            return Logic.valueOf(states.intValue());
        }
        if (states instanceof Long) {
            return Logic.valueOf(states.longValue());
        }
        if (states instanceof Float) {
            return Logic.valueOf(states.floatValue());
        }
        if (states instanceof Double) {
            return Logic.valueOf(states.doubleValue());
        }
        if (states instanceof BigInteger) {
            return Logic.valueOf(((BigInteger)states).toString(1));
        }
        return null;
    }

    public static Logic expand(int bitWidth, int format0, byte[] packed, int pos, int length) {
        Logic logic;
        block15: {
            byte[] states;
            int xdf;
            block14: {
                logic = new Logic(new byte[bitWidth]);
                int df = format0 & 0xC0;
                xdf = format0 & 0x18;
                states = logic.states;
                if (length <= 0 || xdf != 16) break block14;
                byte preceidingStates = (byte)(df == 64 ? packed[pos] >>> 7 & 1 : (df == 128 ? packed[pos] >>> 6 & 3 : packed[pos] >>> 4) & 0xF);
                Arrays.fill(states, preceidingStates);
                int index = bitWidth - 1;
                switch (df) {
                    case 64: {
                        int n = length - 1;
                        while (n >= 0) {
                            byte pack = packed[pos + n];
                            int m = 0;
                            while (m < 8) {
                                byte state = (byte)(pack & 1);
                                pack = (byte)(pack >>> 1);
                                if (index < 0) break;
                                states[index--] = state;
                                ++m;
                            }
                            --n;
                        }
                        break block15;
                    }
                    case 128: {
                        int n = length - 1;
                        while (n >= 0) {
                            byte pack = packed[pos + n];
                            int m = 0;
                            while (m < 4) {
                                byte state = (byte)(pack & 3);
                                pack = (byte)(pack >>> 2);
                                if (index < 0) break;
                                states[index--] = state;
                                ++m;
                            }
                            --n;
                        }
                        break block15;
                    }
                    case 192: {
                        int n = length - 1;
                        while (n >= 0) {
                            byte pack = packed[pos + n];
                            int m = 0;
                            while (m < 2) {
                                byte state = (byte)(pack & 0xF);
                                pack = (byte)(pack >>> 4);
                                if (index < 0) break;
                                states[index--] = state;
                                ++m;
                            }
                            --n;
                        }
                        break block0;
                    }
                }
                break block15;
            }
            if (xdf == 0) {
                Arrays.fill(states, (byte)0);
            } else if (xdf == 8) {
                Arrays.fill(states, (byte)1);
            }
        }
        return logic;
    }

    @Override
    public byte[] getArray() {
        if (this.states == null) {
            return null;
        }
        byte[] array = new byte[this.states.length];
        int n = 0;
        while (n < this.states.length) {
            array[n] = this.states[this.states.length - 1 - n];
            ++n;
        }
        return array;
    }

    @Override
    public int length() {
        return this.getBitWidth();
    }

    public byte[] getStates() {
        return this.states;
    }

    public int getBitWidth() {
        return this.states.length;
    }

    public byte getState(int index) {
        if (index >= 0 && index < this.states.length) {
            return this.states[this.states.length - 1 - index];
        }
        return 15;
    }

    public void getStates(int index, byte[] states) {
        int n = 0;
        while (n < states.length) {
            states[states.length - 1 - n] = index + n >= 0 && index + n < this.states.length ? this.states[this.states.length - 1 - (index + n)] : 15;
            ++n;
        }
    }

    public byte getState() {
        if (this.states.length == 1) {
            return this.states[0];
        }
        if (this.states.length > 1) {
            byte state = this.states[0];
            int n = 1;
            while (n < this.states.length) {
                if (this.states[n] != state) {
                    state = 15;
                }
                ++n;
            }
            return (byte)state;
        }
        return 15;
    }

    public int getLevel() {
        int level = 1;
        int index = this.states.length - 1;
        while (index >= 0) {
            if ((this.states[index] & 0xFE) != 0) {
                level = 2;
                break;
            }
            --index;
        }
        while (index >= 0) {
            if ((this.states[index] & 0xFC) != 0) {
                level = 3;
                break;
            }
            --index;
        }
        return level;
    }

    public boolean write(ILogicSamplesWriter writer, long units) {
        if (this.states != null && this.states.length > 0) {
            int start = 0;
            if (this.states.length > writer.getBitWidth()) {
                start = this.states.length - writer.getBitWidth();
            }
            int length = this.states.length - start;
            byte preceiding = this.states[start];
            return writer.write(units, false, preceiding, this.states, start, length);
        }
        return false;
    }

    public boolean write(ILogicSamplesWriter writer, long units, boolean tag) {
        if (this.states != null && this.states.length > 0) {
            int start = 0;
            if (this.states.length > writer.getBitWidth()) {
                start = this.states.length - writer.getBitWidth();
            }
            int length = this.states.length - start;
            byte preceiding = this.states[start];
            return writer.write(units, tag, preceiding, this.states, start, length);
        }
        return false;
    }

    @Override
    public int intValue() {
        Number number = this.toNumber(false);
        if (number != null) {
            return number.intValue();
        }
        return 0;
    }

    @Override
    public long longValue() {
        Number number = this.toNumber(false);
        if (number != null) {
            return number.longValue();
        }
        return 0L;
    }

    @Override
    public float floatValue() {
        Number number = this.toNumber(false);
        if (number != null) {
            return number.floatValue();
        }
        return 0.0f;
    }

    @Override
    public double doubleValue() {
        Number number = this.toNumber(false);
        if (number != null) {
            return number.doubleValue();
        }
        return 0.0;
    }

    public boolean booleanValue() {
        byte state = this.getState();
        return state == 1 || state == 5;
    }

    public String toString(int format) {
        switch (format & 0xFFFF) {
            case 1: {
                return String.valueOf('b') + this.toBinaryString();
            }
            case 2: {
                return String.valueOf('o') + this.toMergedString(3);
            }
            case 3: {
                return String.valueOf('x') + this.toMergedString(4);
            }
            case 5: {
                Number d = this.toNumber(false);
                return d != null ? d.toString() : null;
            }
            case 4: {
                Number n = this.toNumber(false);
                if (n == null) break;
                if (n.intValue() > 0 && Character.isDefined((char)n.intValue())) {
                    return String.valueOf((char)n.intValue());
                }
                return "";
            }
        }
        return null;
    }

    private String toMergedString(int bits) {
        StringBuilder builder = new StringBuilder();
        int index = this.states.length;
        int value = 0;
        char digit = '\u0000';
        byte[] byArray = this.states;
        int n = this.states.length;
        int n2 = 0;
        while (n2 < n) {
            byte state = byArray[n2];
            if (state != 0 && state != 1 && state != 4 && state != 5) {
                if (digit == '\u0000') {
                    digit = STATE_UC_DIGITS[state & 0xF];
                } else if (digit != STATE_UC_DIGITS[state & 0xF]) {
                    digit = '#';
                }
            } else {
                value = value << 1 | state & 1;
                if (digit == '\u0000') {
                    digit = '0';
                } else if (digit != '0') {
                    digit = '#';
                }
            }
            if (--index % bits == 0) {
                if (digit != '0') {
                    builder.append(digit);
                } else if (value <= 9) {
                    builder.append((char)(48 + value));
                } else {
                    builder.append((char)(65 + value - 10));
                }
                value = 0;
                digit = '\u0000';
            }
            ++n2;
        }
        return builder.toString();
    }

    private String toBinaryString() {
        StringBuilder builder = new StringBuilder();
        byte[] byArray = this.states;
        int n = this.states.length;
        int n2 = 0;
        while (n2 < n) {
            byte state = byArray[n2];
            builder.append(STATE_LC_DIGITS[state & 0xF]);
            ++n2;
        }
        return builder.toString();
    }

    public Number toNumber(boolean signed) {
        int bitWidth = this.states.length;
        if (signed && bitWidth > 64 || !signed && bitWidth > 63) {
            return this.toBigInteger(signed);
        }
        long value = 0L;
        byte[] byArray = this.states;
        int n = this.states.length;
        int n2 = 0;
        while (n2 < n) {
            byte state = byArray[n2];
            if (state == 0 || state == 4) {
                value <<= 1;
            } else if (state == 1 || state == 5) {
                value = value << 1 | 1L;
            } else {
                return null;
            }
            ++n2;
        }
        if (signed && signed && (value & 1L << bitWidth - 1) != 0L) {
            value |= -1L << bitWidth;
        }
        return value;
    }

    private Number toBigInteger(boolean signed) {
        int bitWidth = this.states.length;
        byte[] value = new byte[(bitWidth + 7) / 8];
        int index = this.states.length;
        byte[] byArray = this.states;
        int n = this.states.length;
        int n2 = 0;
        while (n2 < n) {
            byte state = byArray[n2];
            int n3 = value.length - --index / 8 - 1;
            if (state == 0 || state == 4) {
                int n4 = n3;
                value[n4] = (byte)(value[n4] << 1);
            } else if (state == 1 || state == 5) {
                value[n3] = (byte)(value[n3] << 1 | 1);
            } else {
                return null;
            }
            ++n2;
        }
        if (signed && signed && (value[0] & 1 << bitWidth % 8 - 1) != 0) {
            value[0] = (byte)((long)value[0] | -1L << bitWidth % 8);
        }
        return new BigInteger(value);
    }

    public Logic extract(int xbit, int xlength) {
        if (xbit < 0 || xbit >= this.states.length) {
            return null;
        }
        if (xbit + xlength > this.states.length) {
            xlength = this.states.length - xbit;
        }
        Logic logic = new Logic(xlength);
        int n = 0;
        while (n < xlength) {
            logic.states[xlength - 1 - n] = this.states[this.states.length - 1 - n - xbit];
            ++n;
        }
        return logic;
    }

    public Logic invert() {
        Logic logic = new Logic(this);
        int n = 0;
        while (n < this.states.length) {
            logic.states[n] = (byte)OPERATOR_NOT[this.states[n]];
            ++n;
        }
        return logic;
    }

    public Logic swap() {
        Logic logic = new Logic(this);
        int n = 0;
        while (n < this.states.length) {
            logic.states[n] = this.states[this.states.length - 1 - n];
            ++n;
        }
        return logic;
    }

    public String toString() {
        return this.toString(1);
    }

    public boolean equals(Object obj) {
        if (obj instanceof Logic) {
            return Arrays.equals(this.states, ((Logic)obj).states);
        }
        return false;
    }

    public static void main(String[] args) {
    }
}

