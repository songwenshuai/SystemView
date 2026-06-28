/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import java.io.IOException;
import java.io.InputStream;

public final class BinaryParseBuffer {
    int size;
    public byte[] buffer;
    public int data = 0;
    private int used = 0;
    private int cleaned = 0;
    BinaryParseBuffer inserted;
    BinaryParseBuffer parent;
    public int pos = 0;
    public static final int NONE = 0;
    public static final int OK = 1;
    public static final int NOT_ENOUGH_DATA = 2;
    public static final int ERROR = 3;
    private boolean parsing;
    private int result;
    private String errorText;

    public BinaryParseBuffer(int size) {
        this.size = size;
        this.buffer = new byte[size];
    }

    public BinaryParseBuffer(BinaryParseBuffer parent, byte[] bytes) {
        this.size = this.data = bytes.length;
        this.buffer = bytes;
        this.parent = parent;
    }

    public boolean fill(InputStream in) throws IOException {
        if (this.parsing || this.parent != null) {
            return false;
        }
        int read = in.read(this.buffer, this.data, this.size - this.data);
        if (read > 0) {
            this.data += read;
        }
        return read != -1;
    }

    public void clean() {
        if (this.parsing) {
            return;
        }
        this.data -= this.used <= this.data ? this.used : this.data;
        if (this.data > 0) {
            System.arraycopy(this.buffer, this.used, this.buffer, 0, this.data);
        }
        this.cleaned += this.used;
        this.used = 0;
    }

    public void clear() {
        if (this.parsing) {
            return;
        }
        this.data = 0;
        this.used = 0;
        this.result = 0;
        this.inserted = null;
    }

    public int used() {
        return this.used;
    }

    public void resize(int size) {
        int current = this.buffer.length;
        if (size > current) {
            byte[] buffer = new byte[size];
            System.arraycopy(this.buffer, 0, buffer, 0, this.data);
            this.buffer = buffer;
        }
    }

    public BinaryParseBuffer begin() {
        if (this.inserted != null) {
            return this.inserted.begin();
        }
        this.parsing = true;
        this.result = 0;
        this.pos = this.used;
        return this;
    }

    public BinaryParseBuffer end() {
        this.parsing = false;
        if (this.result == 1) {
            this.used = this.pos;
            if (this.parent != null && this.used >= this.data && this.inserted == null) {
                this.parent.inserted = null;
                this.parent.result = 1;
                return this.parent.end();
            }
        }
        return this;
    }

    public void setOk() {
        this.result = 1;
    }

    public void setNotEnoughData() {
        this.result = 2;
    }

    public void setError(String text) {
        this.result = 3;
        this.errorText = text;
    }

    public boolean isOk() {
        return this.result == 1;
    }

    public boolean isError() {
        return this.result == 3;
    }

    public String getErrorText() {
        return this.errorText;
    }

    public int available() {
        return this.data - this.pos;
    }

    public long parsePlus() {
        int shift = 0;
        long value = 0L;
        while (this.pos < this.data) {
            byte sn = this.buffer[this.pos++];
            value |= (long)((sn & 0x7F) << shift);
            shift += 7;
            if ((sn & 0x80) != 0) continue;
            this.result = 1;
            return value;
        }
        this.setNotEnoughData();
        return 0L;
    }

    public long parseLong() {
        int size = (int)this.parsePlus();
        if (this.result != 1) {
            return 0L;
        }
        if (this.data - this.pos >= size) {
            byte[] bytes = this.buffer;
            int pos = this.pos;
            this.pos += size;
            if (size == 0) {
                return 0L;
            }
            if (size <= 4) {
                int value = (this.buffer[pos + size - 1] & 0x80) != 0 ? -1 : 0;
                int i = pos + size - 1;
                while (i >= pos) {
                    value = value << 8 | 0xFF & bytes[i];
                    --i;
                }
                return value;
            }
            if (size <= 8) {
                long value = (bytes[pos + size - 1] & 0x80) != 0 ? -1L : 0L;
                int i = pos + size - 1;
                while (i >= pos) {
                    value = value << 8 | (long)(0xFF & bytes[i]);
                    --i;
                }
                return value;
            }
            return 0L;
        }
        this.setNotEnoughData();
        return 0L;
    }

    public String parseString() {
        int size = (int)this.parsePlus();
        if (this.result != 1) {
            return null;
        }
        if (this.data - this.pos >= size) {
            String value = new String(this.buffer, this.pos, size);
            this.pos += size;
            this.result = 1;
            return value;
        }
        this.setNotEnoughData();
        return null;
    }

    public String parseString(int size) {
        if (this.data - this.pos >= size) {
            String value = new String(this.buffer, this.pos, size);
            this.pos += size;
            this.result = 1;
            return value;
        }
        this.setNotEnoughData();
        return null;
    }

    public byte[] parseBytes() {
        int size = (int)this.parsePlus();
        if (this.result != 1) {
            return null;
        }
        if (this.data - this.pos >= size) {
            return this.getBytes(size);
        }
        this.setNotEnoughData();
        return null;
    }

    public String parseLine() {
        int start = this.pos;
        while (this.pos < this.data) {
            if (this.buffer[this.pos++] != 10) continue;
            return new String(this.buffer, start, this.pos - start);
        }
        this.setNotEnoughData();
        return null;
    }

    public byte getByte() {
        if (this.data - this.pos >= 1) {
            byte value = this.buffer[this.pos];
            ++this.pos;
            this.result = 1;
            return value;
        }
        this.setNotEnoughData();
        return 0;
    }

    public short getWord() {
        if (this.data - this.pos >= 2) {
            short value = (short)(this.buffer[this.pos] & 0xFF | (this.buffer[this.pos + 1] & 0xFF) << 8 & 0xFFFF);
            this.pos += 2;
            this.result = 1;
            return value;
        }
        this.setNotEnoughData();
        return 0;
    }

    public byte[] getBytes(int size) {
        if (this.data - this.pos >= size) {
            byte[] value = new byte[size];
            System.arraycopy(this.buffer, this.pos, value, 0, size);
            this.pos += size;
            this.result = 1;
            return value;
        }
        this.setNotEnoughData();
        return null;
    }

    public byte peekByte() {
        if (this.data - this.pos >= 1) {
            byte value = this.buffer[this.pos];
            this.result = 1;
            return value;
        }
        this.setNotEnoughData();
        return 0;
    }

    public byte peekByte(int idx) {
        if (this.data - this.pos >= idx + 1) {
            byte value = this.buffer[this.pos + idx];
            this.result = 1;
            return value;
        }
        this.setNotEnoughData();
        return 0;
    }

    public byte[] peekBytes(int size) {
        if (this.data - this.pos >= size) {
            byte[] value = new byte[size];
            System.arraycopy(this.buffer, this.pos, value, 0, size);
            this.result = 1;
            return value;
        }
        this.setNotEnoughData();
        return null;
    }

    public void skipBytes(int size) {
        if (this.data - this.pos >= size) {
            this.pos += size;
            this.result = 1;
            return;
        }
        this.setNotEnoughData();
    }

    public void insertBytes(byte[] bytes) {
        this.inserted = new BinaryParseBuffer(this, bytes);
    }
}

