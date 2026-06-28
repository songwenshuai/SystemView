/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.swv;

public abstract class SwvParser {
    int tsFrequency = 1;
    int tsDevider = 1;
    boolean closed;
    int changed;
    boolean synced = false;

    private synchronized int parse(byte[] bytes, int len) {
        if (len <= 0) {
            return 0;
        }
        int pos = 0;
        byte header = bytes[pos];
        if (!this.synced || header == 0) {
            this.synced = false;
            int zeros = 0;
            while (pos < len) {
                byte b = bytes[pos];
                if (b == 0) {
                    ++zeros;
                    continue;
                }
                pos += zeros + 1;
                if ((b & 0xFF) == 128 && zeros >= 5) {
                    this.synced = true;
                    zeros = 0;
                    header = bytes[pos];
                    break;
                }
                zeros = 0;
            }
        }
        if (this.synced) {
            while (pos < len) {
                if ((header & 3) == 0) {
                    boolean parsed = false;
                    int continuated = 0;
                    continuated = 0;
                    while (continuated < 5 && pos + continuated < len) {
                        if ((bytes[pos + continuated] & 0x80) == 0) {
                            if ((header & 0xFF) == 112) {
                                this.handleOverflow();
                            } else if ((header & 0xC) == 0) {
                                this.parseLocalTimeStamp(bytes, pos, continuated + 1);
                            } else if ((header & 0xC) == 4) {
                                this.parseGlobalTimeStamp(bytes, pos, continuated + 1);
                            } else if ((header & 8) == 8) {
                                this.parseExtension(bytes, pos, continuated + 1);
                            } else {
                                this.parseReserved(bytes, pos, continuated + 1);
                            }
                            pos += continuated + 1;
                            parsed = true;
                        }
                        ++continuated;
                    }
                    if (!parsed) {
                        if (continuated >= 5) {
                            this.handleError();
                            this.synced = false;
                            pos += continuated + 1;
                        }
                        return pos;
                    }
                } else if ((header & 3) != 0) {
                    int payload;
                    int n = payload = (header & 3) == 3 ? 4 : header & 3;
                    if (pos + payload < len) {
                        if ((header & 0xFF) == 5) {
                            this.parseEventCounter(bytes, pos, payload + 1);
                        } else if ((header & 0xCF) == 69) {
                            this.parseDataTraceMatch(bytes, pos, payload + 1);
                        } else if ((header & 0xFF) == 14) {
                            this.parseExceptionTrace(bytes, pos, payload + 1);
                        } else if ((header & 0xCC) == 68) {
                            this.parseDataTracePcValue(bytes, pos, payload + 1);
                        } else if ((header & 0xCC) == 76) {
                            this.parseDataTraceDataAddress(bytes, pos, payload + 1);
                        } else if ((header & 0xC4) == 132) {
                            this.parseDataTraceDataValue(bytes, pos, payload + 1);
                        } else if ((header & 4) == 0) {
                            this.parseInstrumentation(bytes, pos, payload + 1);
                        } else if ((header & 0xFD) != 21) {
                            this.parsePeriodicPcSample(bytes, pos, payload + 1);
                        } else {
                            this.parseReserved(bytes, pos, payload + 1);
                        }
                        pos += payload + 1;
                    }
                    return pos;
                }
                header = bytes[pos];
            }
        }
        return pos;
    }

    private void parseLocalTimeStamp(byte[] bytes, int pos, int len) {
        long ts = 0L;
        int mode = 0;
        if ((bytes[pos] & 0x80) != 0) {
            int shift = 0;
            mode = bytes[pos] >> 4 & 3;
            int n = 1;
            while (n < len) {
                ts |= (long)((bytes[pos + n] & 0x7F) << shift);
                shift += 7;
                ++n;
            }
        } else {
            ts = (bytes[pos + 0] & 0x70) >> 4;
        }
        this.handleLocalTimeStamp(ts, mode);
    }

    private void parseGlobalTimeStamp(byte[] bytes, int pos, int len) {
        long ts = 0L;
        int shift = 0;
        int n = 1;
        while (n < len) {
            ts |= (long)((bytes[pos + n] & 0x7F) << shift);
            shift += 7;
            ++n;
        }
        if ((bytes[pos] & 0xFF) == 148) {
            this.handleGlobalTimeStamp(0, ts & 0x3FFFFFFL, (ts & 0x8000000L) != 0L, (ts & 0x4000000L) != 0L);
        } else {
            this.handleGlobalTimeStamp(1, ts, false, false);
        }
    }

    private void parseExtension(byte[] bytes, int pos, int len) {
        long ex = (bytes[pos + 0] & 0x70) >> 4;
        int shift = 3;
        int n = 1;
        while (n < len) {
            ex |= (long)((bytes[pos + n] & 0x7F) << shift);
            shift += 7;
            ++n;
        }
        this.handleExtension(ex);
    }

    private void parseEventCounter(byte[] bytes, int pos, int len) {
        if (len >= 2) {
            this.handleEventCounter(bytes[pos + 1] & 0xFF);
        }
    }

    private void parseDataTraceMatch(byte[] bytes, int pos, int len) {
        if (len >= 2) {
            this.handleDataTraceMatch(bytes[pos + 0] >> 4 & 3);
        }
    }

    private void parseExceptionTrace(byte[] bytes, int pos, int len) {
        if (len >= 3) {
            this.handleExceptionTrace(bytes[pos + 1] & 0xFF | (bytes[pos + 2] & 1) << 8, bytes[pos + 2] >> 4 & 3);
        }
    }

    private void parseDataTracePcValue(byte[] bytes, int pos, int len) {
        long pc = 0L;
        int n = 0;
        while (n < len - 1) {
            pc |= (long)((bytes[pos + 1 + n] & 0xFF) << n * 8);
            ++n;
        }
        this.handleDataTracePcValue(bytes[pos + 0] >> 4 & 3, pc);
    }

    private void parseDataTraceDataAddress(byte[] bytes, int pos, int len) {
        long addr = 0L;
        int n = 0;
        while (n < len - 1) {
            addr |= (long)((bytes[pos + 1 + n] & 0xFF) << n * 8);
            ++n;
        }
        this.handleDataTraceDataAddress(bytes[pos + 0] >> 4 & 3, addr);
    }

    private void parseDataTraceDataValue(byte[] bytes, int pos, int len) {
        long value = 0L;
        int n = 0;
        while (n < len - 1) {
            value |= (long)((bytes[pos + 1 + n] & 0xFF) << n * 8);
            ++n;
        }
        this.handleDataTraceDataValue(bytes[pos + 0] >> 4 & 3, value);
    }

    private void parseInstrumentation(byte[] bytes, int pos, int len) {
        long payload = 0L;
        int n = 0;
        while (n < len - 1) {
            payload |= (long)((bytes[pos + 1 + n] & 0xFF) << n * 8);
            ++n;
        }
        this.handleInstrumentation(bytes[pos + 0] >> 3 & 0x1F, payload);
    }

    private void parsePeriodicPcSample(byte[] bytes, int pos, int len) {
        long pc = 0L;
        int n = 0;
        while (n < len - 1) {
            pc |= (long)((bytes[pos + 1 + n] & 0xFF) << n * 8);
            ++n;
        }
        this.handlePeriodicPcSample(pc, len == 5);
    }

    private void parseReserved(byte[] bytes, int pos, int len) {
    }

    protected abstract void handleError();

    protected abstract void handleOverflow();

    protected abstract void handleLocalTimeStamp(long var1, int var3);

    protected abstract void handleGlobalTimeStamp(int var1, long var2, boolean var4, boolean var5);

    protected abstract void handleExtension(long var1);

    protected abstract void handleEventCounter(int var1);

    protected abstract void handleDataTraceMatch(int var1);

    protected abstract void handleExceptionTrace(int var1, int var2);

    protected abstract void handleDataTracePcValue(int var1, long var2);

    protected abstract void handleDataTraceDataAddress(int var1, long var2);

    protected abstract void handleDataTraceDataValue(int var1, long var2);

    protected abstract void handleInstrumentation(int var1, long var2);

    protected abstract void handlePeriodicPcSample(long var1, boolean var3);
}

