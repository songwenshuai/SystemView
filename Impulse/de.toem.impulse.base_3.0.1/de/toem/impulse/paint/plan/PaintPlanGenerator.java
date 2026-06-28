/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.axis.IValueAxis;
import de.toem.impulse.axis.ValueAxis;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.SinglePaintPlanProvision;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.core.Assert;
import de.toem.toolkits.core.Base64;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import org.eclipse.swt.graphics.Rectangle;

public final class PaintPlanGenerator
extends SinglePaintPlanProvision
implements IPlan.IPaintPlanGenerator {
    private static final int BLOCK_SIZE = 16384;
    List<Number> unscaled;
    List<Attachment> attachments;
    Map<String, Integer> textMap;

    protected PaintPlanGenerator(IPlotItem item, IDomainAxis axis, Rectangle area, int style, int scheme) {
        super(item, axis, area, style, scheme);
        this.pPaints = new byte[16384];
        this.values = new ArrayList();
        this.scheme = scheme;
    }

    @Override
    public void extend(int delta) {
        super.extend(delta);
    }

    @Override
    public void add(int paint, int x, int idx) {
        byte[] paints = this.pPaints;
        int pos = this.pLen;
        int len = 4;
        int flag = 0;
        if (x > Short.MAX_VALUE) {
            x = Short.MAX_VALUE;
        } else if (x < Short.MIN_VALUE) {
            x = Short.MIN_VALUE;
        }
        len += 2;
        flag = (byte)(flag | 0x40);
        if (idx >= 0) {
            int aflag = 4;
            ++len;
            if (idx >= 256) {
                aflag = 8;
                ++len;
                if (idx >= 65536) {
                    aflag = 12;
                    len += 2;
                }
            }
            flag = (byte)(flag | aflag);
        }
        if (paints == null || paints.length - pos < len) {
            if (paints == null) {
                paints = new byte[16384];
            } else {
                byte[] newBuffer = new byte[paints.length + 16384];
                System.arraycopy(paints, 0, newBuffer, 0, pos);
                paints = newBuffer;
            }
            this.pPaints = paints;
        }
        paints[pos++] = flag;
        paints[pos++] = (byte)(paint & 0xFF);
        paints[pos++] = (byte)(paint >> 8 & 0xFF);
        paints[pos++] = (byte)(x & 0xFF);
        paints[pos++] = (byte)(x >> 8 & 0xFF);
        if (idx >= 0) {
            paints[pos++] = (byte)(idx & 0xFF);
            if (idx >= 256) {
                paints[pos++] = (byte)(idx >> 8 & 0xFF);
                if (idx >= 65536) {
                    paints[pos++] = (byte)(idx >> 16 & 0xFF);
                    paints[pos++] = (byte)(idx >> 24 & 0xFF);
                }
            }
        }
        paints[pos++] = flag;
        this.pLen = pos;
    }

    @Override
    public void add(int paint, int x, int idx, String value) {
        this._add(paint, x, idx, this.value(value));
    }

    private int value(String value) {
        int val = -1;
        if (value != null && this.vLen < 65536) {
            if (this.textMap == null) {
                this.textMap = new HashMap<String, Integer>();
            }
            if (this.textMap.containsKey(value)) {
                val = this.textMap.get(value);
            } else {
                val = this.values.size();
                this.textMap.put(value, val);
                this.values.add(value);
                ++this.vLen;
            }
        }
        return val;
    }

    private int[] values(List<String> value) {
        int[] val = null;
        if (value != null) {
            val = new int[Math.min(value.size(), 255)];
            int n = 0;
            while (n < val.length) {
                val[n] = this.value(value.get(n));
                ++n;
            }
        }
        return val;
    }

    @Override
    public void add(int paint, int x, int idx, byte[] value) {
        this._add(paint, x, idx, this.value(String.valueOf(Base64.encode(value))));
    }

    @Override
    public void add(int paint, int x, int idx, List<String> value) {
        this._add(paint, x, idx, this.values(value));
    }

    private void _add(int paint, int x, int idx, int val) {
        byte[] paints = this.pPaints;
        int pos = this.pLen;
        int len = 4;
        int flag = 0;
        if (x > Short.MAX_VALUE) {
            x = Short.MAX_VALUE;
        } else if (x < Short.MIN_VALUE) {
            x = Short.MIN_VALUE;
        }
        len += 2;
        flag = (byte)(flag | 0x40);
        if (idx >= 0) {
            int aflag = 4;
            ++len;
            if (idx >= 256) {
                aflag = 8;
                ++len;
                if (idx >= 65536) {
                    aflag = 12;
                    len += 2;
                }
            }
            flag = (byte)(flag | aflag);
        }
        if (val >= 0) {
            len += 2;
            flag = (byte)(flag | 1);
        }
        if (paints == null || paints.length - pos < len) {
            if (paints == null) {
                paints = new byte[16384];
            } else {
                byte[] newBuffer = new byte[paints.length + 16384];
                System.arraycopy(paints, 0, newBuffer, 0, pos);
                paints = newBuffer;
            }
            this.pPaints = paints;
        }
        paints[pos++] = flag;
        paints[pos++] = (byte)(paint & 0xFF);
        paints[pos++] = (byte)(paint >> 8 & 0xFF);
        paints[pos++] = (byte)(x & 0xFF);
        paints[pos++] = (byte)(x >> 8 & 0xFF);
        if (idx >= 0) {
            paints[pos++] = (byte)(idx & 0xFF);
            if (idx >= 256) {
                paints[pos++] = (byte)(idx >> 8 & 0xFF);
                if (idx >= 65536) {
                    paints[pos++] = (byte)(idx >> 16 & 0xFF);
                    paints[pos++] = (byte)(idx >> 24 & 0xFF);
                }
            }
        }
        if (val >= 0) {
            paints[pos++] = (byte)(val & 0xFF);
            paints[pos++] = (byte)(val >> 8 & 0xFF);
        }
        paints[pos++] = flag;
        this.pLen = pos;
    }

    private void _add(int paint, int x, int idx, int[] val) {
        byte[] paints = this.pPaints;
        int pos = this.pLen;
        int len = 4;
        int flag = 0;
        int nvals = 0;
        if (x > Short.MAX_VALUE) {
            x = Short.MAX_VALUE;
        } else if (x < Short.MIN_VALUE) {
            x = Short.MIN_VALUE;
        }
        len += 2;
        flag = (byte)(flag | 0x40);
        if (idx >= 0) {
            int aflag = 4;
            ++len;
            if (idx >= 256) {
                aflag = 8;
                ++len;
                if (idx >= 65536) {
                    aflag = 12;
                    len += 2;
                }
            }
            flag = (byte)(flag | aflag);
        }
        if (val != null) {
            nvals = val.length;
            if (nvals > 2) {
                len += 2 + nvals * 2;
                flag = (byte)(flag | 3);
            } else if (nvals > 0) {
                len += nvals * 2;
                flag = (byte)(flag | (nvals == 2 ? 2 : 1));
            }
        }
        if (paints == null || paints.length - pos < len) {
            if (paints == null) {
                paints = new byte[16384];
            } else {
                byte[] newBuffer = new byte[paints.length + 16384];
                System.arraycopy(paints, 0, newBuffer, 0, pos);
                paints = newBuffer;
            }
            this.pPaints = paints;
        }
        paints[pos++] = flag;
        if (nvals > 2) {
            paints[pos++] = (byte)(nvals & 0xFF);
        }
        paints[pos++] = (byte)(paint & 0xFF);
        paints[pos++] = (byte)(paint >> 8 & 0xFF);
        paints[pos++] = (byte)(x & 0xFF);
        paints[pos++] = (byte)(x >> 8 & 0xFF);
        if (idx >= 0) {
            paints[pos++] = (byte)(idx & 0xFF);
            if (idx >= 256) {
                paints[pos++] = (byte)(idx >> 8 & 0xFF);
                if (idx >= 65536) {
                    paints[pos++] = (byte)(idx >> 16 & 0xFF);
                    paints[pos++] = (byte)(idx >> 24 & 0xFF);
                }
            }
        }
        if (val != null) {
            int n = 0;
            while (n < nvals) {
                int v = val[n];
                paints[pos++] = (byte)(v & 0xFF);
                paints[pos++] = (byte)(v >> 8 & 0xFF);
                ++n;
            }
        }
        if (nvals > 2) {
            paints[pos++] = (byte)(nvals & 0xFF);
        }
        paints[pos++] = flag;
        this.pLen = pos;
    }

    @Override
    public void add(int paint, int x, short p0, short p1, int idx, String value) {
        this._add(paint, x, p0, p1, idx, this.value(value));
    }

    private void _add(int paint, int x, short p0, short p1, int idx, int val) {
        byte[] paints = this.pPaints;
        int pos = this.pLen;
        int len = 4;
        int flag = 0;
        if (x > Short.MAX_VALUE) {
            x = Short.MAX_VALUE;
        } else if (x < Short.MIN_VALUE) {
            x = Short.MIN_VALUE;
        }
        len += 6;
        flag = (byte)(flag | 0x50);
        if (idx >= 0) {
            int aflag = 4;
            ++len;
            if (idx >= 256) {
                aflag = 8;
                ++len;
                if (idx >= 65536) {
                    aflag = 12;
                    len += 2;
                }
            }
            flag = (byte)(flag | aflag);
        }
        if (val >= 0) {
            len += 2;
            flag = (byte)(flag | 1);
        }
        if (paints == null || paints.length - pos < len) {
            if (paints == null) {
                paints = new byte[16384];
            } else {
                byte[] newBuffer = new byte[paints.length + 16384];
                System.arraycopy(paints, 0, newBuffer, 0, pos);
                paints = newBuffer;
            }
            this.pPaints = paints;
        }
        paints[pos++] = flag;
        paints[pos++] = (byte)(paint & 0xFF);
        paints[pos++] = (byte)(paint >> 8 & 0xFF);
        paints[pos++] = (byte)(x & 0xFF);
        paints[pos++] = (byte)(x >> 8 & 0xFF);
        paints[pos++] = (byte)(p0 & 0xFF);
        paints[pos++] = (byte)(p0 >> 8 & 0xFF);
        paints[pos++] = (byte)(p1 & 0xFF);
        paints[pos++] = (byte)(p1 >> 8 & 0xFF);
        if (idx >= 0) {
            paints[pos++] = (byte)(idx & 0xFF);
            if (idx >= 256) {
                paints[pos++] = (byte)(idx >> 8 & 0xFF);
                if (idx >= 65536) {
                    paints[pos++] = (byte)(idx >> 16 & 0xFF);
                    paints[pos++] = (byte)(idx >> 24 & 0xFF);
                }
            }
        }
        if (val >= 0) {
            paints[pos++] = (byte)(val & 0xFF);
            paints[pos++] = (byte)(val >> 8 & 0xFF);
        }
        paints[pos++] = flag;
        this.pLen = pos;
    }

    @Override
    public void add(int paint, int x, double a, int idx) {
        byte[] paints = this.pPaints;
        int pos = this.pLen;
        int len = 4;
        int flag = 0;
        if (this.unscaled == null) {
            this.unscaled = new ArrayList<Number>(1024);
        }
        if (x > Short.MAX_VALUE) {
            x = Short.MAX_VALUE;
        } else if (x < Short.MIN_VALUE) {
            x = Short.MIN_VALUE;
        }
        len += 2;
        flag = (byte)(flag | 0x40);
        len += 4;
        flag = (byte)(flag | 0x10);
        if (idx >= 0) {
            int aflag = 4;
            ++len;
            if (idx >= 256) {
                aflag = 8;
                ++len;
                if (idx >= 65536) {
                    aflag = 12;
                    len += 2;
                }
            }
            flag = (byte)(flag | aflag);
        }
        if (paints == null || paints.length - pos < len) {
            if (paints == null) {
                paints = new byte[16384];
            } else {
                byte[] newBuffer = new byte[paints.length + 16384];
                System.arraycopy(paints, 0, newBuffer, 0, pos);
                paints = newBuffer;
            }
            this.pPaints = paints;
        }
        paints[pos++] = flag;
        paints[pos++] = (byte)(paint & 0xFF);
        paints[pos++] = (byte)(paint >> 8 & 0xFF);
        paints[pos++] = (byte)(x & 0xFF);
        paints[pos++] = (byte)(x >> 8 & 0xFF);
        this.unscaled.add(pos);
        this.unscaled.add(a);
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        if (idx >= 0) {
            paints[pos++] = (byte)(idx & 0xFF);
            if (idx >= 256) {
                paints[pos++] = (byte)(idx >> 8 & 0xFF);
                if (idx >= 65536) {
                    paints[pos++] = (byte)(idx >> 16 & 0xFF);
                    paints[pos++] = (byte)(idx >> 24 & 0xFF);
                }
            }
        }
        paints[pos++] = flag;
        this.pLen = pos;
    }

    @Override
    public void add(int paint, int x, double a, double b, int idx) {
        byte[] paints = this.pPaints;
        int pos = this.pLen;
        int len = 4;
        int flag = 0;
        if (this.unscaled == null) {
            this.unscaled = new ArrayList<Number>(1024);
        }
        if (x > Short.MAX_VALUE) {
            x = Short.MAX_VALUE;
        } else if (x < Short.MIN_VALUE) {
            x = Short.MIN_VALUE;
        }
        len += 2;
        flag = (byte)(flag | 0x40);
        len += 8;
        flag = (byte)(flag | 0x20);
        if (idx >= 0) {
            int aflag = 4;
            ++len;
            if (idx >= 256) {
                aflag = 8;
                ++len;
                if (idx >= 65536) {
                    aflag = 12;
                    len += 2;
                }
            }
            flag = (byte)(flag | aflag);
        }
        if (paints == null || paints.length - pos < len) {
            if (paints == null) {
                paints = new byte[16384];
            } else {
                byte[] newBuffer = new byte[paints.length + 16384];
                System.arraycopy(paints, 0, newBuffer, 0, pos);
                paints = newBuffer;
            }
            this.pPaints = paints;
        }
        paints[pos++] = flag;
        paints[pos++] = (byte)(paint & 0xFF);
        paints[pos++] = (byte)(paint >> 8 & 0xFF);
        paints[pos++] = (byte)(x & 0xFF);
        paints[pos++] = (byte)(x >> 8 & 0xFF);
        this.unscaled.add(pos);
        this.unscaled.add(a);
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        this.unscaled.add(pos);
        this.unscaled.add(b);
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        if (idx >= 0) {
            paints[pos++] = (byte)(idx & 0xFF);
            if (idx >= 256) {
                paints[pos++] = (byte)(idx >> 8 & 0xFF);
                if (idx >= 65536) {
                    paints[pos++] = (byte)(idx >> 16 & 0xFF);
                    paints[pos++] = (byte)(idx >> 24 & 0xFF);
                }
            }
        }
        paints[pos++] = flag;
        this.pLen = pos;
    }

    @Override
    public void add(int paint, int x, short p0, short p1, double b, int idx) {
        byte[] paints = this.pPaints;
        int pos = this.pLen;
        int len = 4;
        int flag = 0;
        if (this.unscaled == null) {
            this.unscaled = new ArrayList<Number>(1024);
        }
        if (x > Short.MAX_VALUE) {
            x = Short.MAX_VALUE;
        } else if (x < Short.MIN_VALUE) {
            x = Short.MIN_VALUE;
        }
        len += 2;
        flag = (byte)(flag | 0x40);
        len += 8;
        flag = (byte)(flag | 0x20);
        if (idx >= 0) {
            int aflag = 4;
            ++len;
            if (idx >= 256) {
                aflag = 8;
                ++len;
                if (idx >= 65536) {
                    aflag = 12;
                    len += 2;
                }
            }
            flag = (byte)(flag | aflag);
        }
        if (paints == null || paints.length - pos < len) {
            if (paints == null) {
                paints = new byte[16384];
            } else {
                byte[] newBuffer = new byte[paints.length + 16384];
                System.arraycopy(paints, 0, newBuffer, 0, pos);
                paints = newBuffer;
            }
            this.pPaints = paints;
        }
        paints[pos++] = flag;
        paints[pos++] = (byte)(paint & 0xFF);
        paints[pos++] = (byte)(paint >> 8 & 0xFF);
        paints[pos++] = (byte)(x & 0xFF);
        paints[pos++] = (byte)(x >> 8 & 0xFF);
        paints[pos++] = (byte)(p0 & 0xFF);
        paints[pos++] = (byte)(p0 >> 8 & 0xFF);
        paints[pos++] = (byte)(p1 & 0xFF);
        paints[pos++] = (byte)(p1 >> 8 & 0xFF);
        this.unscaled.add(pos);
        this.unscaled.add(b);
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        if (idx >= 0) {
            paints[pos++] = (byte)(idx & 0xFF);
            if (idx >= 256) {
                paints[pos++] = (byte)(idx >> 8 & 0xFF);
                if (idx >= 65536) {
                    paints[pos++] = (byte)(idx >> 16 & 0xFF);
                    paints[pos++] = (byte)(idx >> 24 & 0xFF);
                }
            }
        }
        paints[pos++] = flag;
        this.pLen = pos;
    }

    @Override
    public void add(int paint, int x, double a, double b, double c, double d, int idx) {
        byte[] paints = this.pPaints;
        int pos = this.pLen;
        int len = 4;
        int flag = 0;
        if (this.unscaled == null) {
            this.unscaled = new ArrayList<Number>(1024);
        }
        if (x > Short.MAX_VALUE) {
            x = Short.MAX_VALUE;
        } else if (x < Short.MIN_VALUE) {
            x = Short.MIN_VALUE;
        }
        len += 2;
        flag = (byte)(flag | 0x40);
        len += 16;
        flag = (byte)(flag | 0x30);
        if (idx >= 0) {
            int aflag = 4;
            ++len;
            if (idx >= 256) {
                aflag = 8;
                ++len;
                if (idx >= 65536) {
                    aflag = 12;
                    len += 2;
                }
            }
            flag = (byte)(flag | aflag);
        }
        if (paints == null || paints.length - pos < len) {
            if (paints == null) {
                paints = new byte[16384];
            } else {
                byte[] newBuffer = new byte[paints.length + 16384];
                System.arraycopy(paints, 0, newBuffer, 0, pos);
                paints = newBuffer;
            }
            this.pPaints = paints;
        }
        paints[pos++] = flag;
        paints[pos++] = (byte)(paint & 0xFF);
        paints[pos++] = (byte)(paint >> 8 & 0xFF);
        paints[pos++] = (byte)(x & 0xFF);
        paints[pos++] = (byte)(x >> 8 & 0xFF);
        this.unscaled.add(pos);
        this.unscaled.add(a);
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        this.unscaled.add(pos);
        this.unscaled.add(b);
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        this.unscaled.add(pos);
        this.unscaled.add(c);
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        this.unscaled.add(pos);
        this.unscaled.add(d);
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        paints[pos++] = 0;
        if (idx >= 0) {
            paints[pos++] = (byte)(idx & 0xFF);
            if (idx >= 256) {
                paints[pos++] = (byte)(idx >> 8 & 0xFF);
                if (idx >= 65536) {
                    paints[pos++] = (byte)(idx >> 16 & 0xFF);
                    paints[pos++] = (byte)(idx >> 24 & 0xFF);
                }
            }
        }
        paints[pos++] = flag;
        this.pLen = pos;
    }

    @Override
    public void add(int paint, int[] x, short p0, short p1, int idx, List<String> value) {
        this._add(paint, x, p0, p1, idx, this.values(value));
    }

    private void _add(int paint, int[] x, short p0, short p1, int idx, int[] val) {
        byte[] paints = this.pPaints;
        int pos = this.pLen;
        int len = 4;
        int flag = 0;
        int nvals = 0;
        if (x != null) {
            int n = 0;
            while (n < x.length) {
                if (x[n] > Short.MAX_VALUE) {
                    x[n] = Short.MAX_VALUE;
                } else if (x[n] < Short.MIN_VALUE) {
                    x[n] = Short.MIN_VALUE;
                }
                ++n;
            }
            switch (x.length) {
                case 6: {
                    flag = (byte)(flag | 0xC0);
                    len += 12;
                    break;
                }
                case 2: {
                    flag = (byte)(flag | 0x80);
                    len += 4;
                    break;
                }
                case 1: {
                    flag = (byte)(flag | 0x40);
                    len += 2;
                    break;
                }
                default: {
                    Assert.notGettingHere();
                }
            }
        }
        flag = (byte)(flag | 0x10);
        len += 4;
        if (idx >= 0) {
            int aflag = 4;
            ++len;
            if (idx >= 256) {
                aflag = 8;
                ++len;
                if (idx >= 65536) {
                    aflag = 12;
                    len += 2;
                }
            }
            flag = (byte)(flag | aflag);
        }
        if (val != null) {
            nvals = val.length;
            if (nvals > 2) {
                len += 2 + nvals * 2;
                flag = (byte)(flag | 3);
            } else if (nvals > 0) {
                len += nvals * 2;
                flag = (byte)(flag | (nvals == 2 ? 2 : 1));
            }
        }
        if (paints == null || paints.length - pos < len) {
            if (paints == null) {
                paints = new byte[16384];
            } else {
                byte[] newBuffer = new byte[paints.length + 16384];
                System.arraycopy(paints, 0, newBuffer, 0, pos);
                paints = newBuffer;
            }
            this.pPaints = paints;
        }
        paints[pos++] = flag;
        if (nvals > 2) {
            paints[pos++] = (byte)(nvals & 0xFF);
        }
        paints[pos++] = (byte)(paint & 0xFF);
        paints[pos++] = (byte)(paint >> 8 & 0xFF);
        if (x != null) {
            int n = 0;
            while (n < x.length) {
                paints[pos++] = (byte)(x[n] & 0xFF);
                paints[pos++] = (byte)(x[n] >> 8 & 0xFF);
                ++n;
            }
        }
        paints[pos++] = (byte)(p0 & 0xFF);
        paints[pos++] = (byte)(p0 >> 8 & 0xFF);
        paints[pos++] = (byte)(p1 & 0xFF);
        paints[pos++] = (byte)(p1 >> 8 & 0xFF);
        if (idx >= 0) {
            paints[pos++] = (byte)(idx & 0xFF);
            if (idx >= 256) {
                paints[pos++] = (byte)(idx >> 8 & 0xFF);
                if (idx >= 65536) {
                    paints[pos++] = (byte)(idx >> 16 & 0xFF);
                    paints[pos++] = (byte)(idx >> 24 & 0xFF);
                }
            }
        }
        if (val != null) {
            int n = 0;
            while (n < nvals) {
                int v = val[n];
                paints[pos++] = (byte)(v & 0xFF);
                paints[pos++] = (byte)(v >> 8 & 0xFF);
                ++n;
            }
        }
        if (nvals > 2) {
            paints[pos++] = (byte)(nvals & 0xFF);
        }
        paints[pos++] = flag;
        this.pLen = pos;
    }

    @Override
    public void attach(int paint, int x, int index, IAttachment value) {
        if (this.attachments == null) {
            this.attachments = new ArrayList<Attachment>();
        }
        this.attachments.add(new Attachment(paint, x, null, index, value));
    }

    @Override
    public void attach(int paint, int x, Double a, int index, IAttachment value) {
        if (this.attachments == null) {
            this.attachments = new ArrayList<Attachment>();
        }
        this.attachments.add(new Attachment(paint, x, a, index, value));
    }

    private void _attach(int paint, int x, Double a, int idx, int val) {
        byte[] paints = this.iPaints;
        int pos = this.iLen;
        int len = 4;
        int flag = 0;
        if (x > Short.MAX_VALUE) {
            x = Short.MAX_VALUE;
        } else if (x < Short.MIN_VALUE) {
            x = Short.MIN_VALUE;
        }
        len += 2;
        flag = (byte)(flag | 0x40);
        if (a != null) {
            len += 4;
            flag = (byte)(flag | 0x10);
        }
        if (idx >= 0) {
            int aflag = 4;
            ++len;
            if (idx >= 256) {
                aflag = 8;
                ++len;
                if (idx >= 65536) {
                    aflag = 12;
                    len += 2;
                }
            }
            flag = (byte)(flag | aflag);
        }
        if (val >= 0) {
            len += 2;
            flag = (byte)(flag | 1);
        }
        if (paints == null || paints.length - pos < len) {
            if (paints == null) {
                paints = new byte[16384];
            } else {
                byte[] newBuffer = new byte[paints.length + 16384];
                System.arraycopy(paints, 0, newBuffer, 0, pos);
                paints = newBuffer;
            }
            this.iPaints = paints;
        }
        paints[pos++] = flag;
        paints[pos++] = (byte)(paint & 0xFF);
        paints[pos++] = (byte)(paint >> 8 & 0xFF);
        paints[pos++] = (byte)(x & 0xFF);
        paints[pos++] = (byte)(x >> 8 & 0xFF);
        if (a != null) {
            int intpar = Float.floatToIntBits(a.floatValue());
            paints[pos++] = (byte)(intpar & 0xFF);
            paints[pos++] = (byte)(intpar >> 8 & 0xFF);
            paints[pos++] = (byte)(intpar >> 16 & 0xFF);
            paints[pos++] = (byte)(intpar >> 24 & 0xFF);
        }
        if (idx >= 0) {
            paints[pos++] = (byte)(idx & 0xFF);
            if (idx >= 256) {
                paints[pos++] = (byte)(idx >> 8 & 0xFF);
                if (idx >= 65536) {
                    paints[pos++] = (byte)(idx >> 16 & 0xFF);
                    paints[pos++] = (byte)(idx >> 24 & 0xFF);
                }
            }
        }
        if (val >= 0) {
            paints[pos++] = (byte)(val & 0xFF);
            paints[pos++] = (byte)(val >> 8 & 0xFF);
        }
        paints[pos++] = flag;
        this.iLen = pos;
    }

    private void _attach(int paint, int x, int type, long position, int p0, int p1, int idx, int val0, int val1, int val2) {
        byte[] paints = this.iPaints;
        int pos = this.iLen;
        int len = 4;
        int flag = 0;
        if (x > Short.MAX_VALUE) {
            x = Short.MAX_VALUE;
        } else if (x < Short.MIN_VALUE) {
            x = Short.MIN_VALUE;
        }
        len += 12;
        flag = (byte)(flag | 0xC0);
        if (p0 != 0 || p1 != 0) {
            len += 4;
            flag = (byte)(flag | 0x10);
        }
        if (idx >= 0) {
            int aflag = 4;
            ++len;
            if (idx >= 256) {
                aflag = 8;
                ++len;
                if (idx >= 65536) {
                    aflag = 12;
                    len += 2;
                }
            }
            flag = (byte)(flag | aflag);
        }
        if (val2 >= 0) {
            len += 8;
            flag = (byte)(flag | 3);
        } else {
            len += 4;
            flag = (byte)(flag | 2);
        }
        if (paints == null || paints.length - pos < len) {
            if (paints == null) {
                paints = new byte[16384];
            } else {
                byte[] newBuffer = new byte[paints.length + 16384];
                System.arraycopy(paints, 0, newBuffer, 0, pos);
                paints = newBuffer;
            }
            this.iPaints = paints;
        }
        paints[pos++] = flag;
        if (val2 >= 0) {
            paints[pos++] = 3;
        }
        paints[pos++] = (byte)(paint & 0xFF);
        paints[pos++] = (byte)(paint >> 8 & 0xFF);
        paints[pos++] = (byte)(x & 0xFF);
        paints[pos++] = (byte)(x >> 8 & 0xFF);
        paints[pos++] = (byte)(type & 0xFF);
        paints[pos++] = (byte)(type >> 8 & 0xFF);
        paints[pos++] = (byte)(position & 0xFFL);
        paints[pos++] = (byte)(position >> 8 & 0xFFL);
        paints[pos++] = (byte)(position >> 16 & 0xFFL);
        paints[pos++] = (byte)(position >> 24 & 0xFFL);
        paints[pos++] = (byte)(position >> 32 & 0xFFL);
        paints[pos++] = (byte)(position >> 40 & 0xFFL);
        paints[pos++] = (byte)(position >> 48 & 0xFFL);
        paints[pos++] = (byte)(position >> 56 & 0xFFL);
        if (p0 != 0 || p1 != 0) {
            paints[pos++] = (byte)(p0 & 0xFF);
            paints[pos++] = (byte)(p0 >> 8 & 0xFF);
            paints[pos++] = (byte)(p1 & 0xFF);
            paints[pos++] = (byte)(p1 >> 8 & 0xFF);
        }
        if (idx >= 0) {
            paints[pos++] = (byte)(idx & 0xFF);
            if (idx >= 256) {
                paints[pos++] = (byte)(idx >> 8 & 0xFF);
                if (idx >= 65536) {
                    paints[pos++] = (byte)(idx >> 16 & 0xFF);
                    paints[pos++] = (byte)(idx >> 24 & 0xFF);
                }
            }
        }
        paints[pos++] = (byte)(val0 & 0xFF);
        paints[pos++] = (byte)(val0 >> 8 & 0xFF);
        paints[pos++] = (byte)(val1 & 0xFF);
        paints[pos++] = (byte)(val1 >> 8 & 0xFF);
        if (val2 >= 0) {
            paints[pos++] = (byte)(val2 & 0xFF);
            paints[pos++] = (byte)(val2 >> 8 & 0xFF);
        }
        if (val2 >= 0) {
            paints[pos++] = 3;
        }
        paints[pos++] = flag;
        this.iLen = pos;
    }

    @Override
    public void setEndX(int end) {
        if (end > Short.MAX_VALUE) {
            end = Short.MAX_VALUE;
        } else if (end < Short.MIN_VALUE) {
            end = Short.MIN_VALUE;
        }
        this.xEnd = end;
    }

    @Override
    public void setTagDomain(ISamples.TagDomain tagDomain) {
        this.tagDomain = tagDomain;
    }

    @Override
    public void setNoOfLayers(int layers) {
        this.layers = layers;
    }

    @Override
    public void setVaxis(IValueAxis vaxis) {
        this.vaxis = vaxis;
    }

    @Override
    public void setScheme(int scheme) {
        this.scheme = scheme;
    }

    @Override
    public void addScheme(int scheme) {
        this.scheme |= scheme;
    }

    @Override
    public void removeScheme(int scheme) {
        this.scheme &= ~scheme;
    }

    @Override
    public void setScript(String script) {
        this.script = script;
    }

    @Override
    public void setStatus(String status) {
        this.status = status;
    }

    @Override
    public void extendToInfinity(boolean left, boolean right) {
    }

    @Override
    public boolean isEmpty() {
        return this.pLen == 0 && this.iLen == 0 && (this.plan == null || this.plan.isEmpty());
    }

    @Override
    public double[] getScaleBorder() {
        if (this.unscaled == null || this.unscaled.isEmpty()) {
            return null;
        }
        double max = -1.7976931348623157E308;
        double min = Double.MAX_VALUE;
        for (Number num : this.unscaled) {
            if (!(num instanceof Double)) continue;
            if ((Double)num > max) {
                max = (Double)num;
            }
            if (!((Double)num < min)) continue;
            min = (Double)num;
        }
        return new double[]{min, max};
    }

    @Override
    public void applyScale(int type, String unit, Double imin, Double imax) {
        double min;
        double max;
        block13: {
            block11: {
                block12: {
                    if (this.unscaled == null || this.unscaled.isEmpty()) {
                        return;
                    }
                    max = -1.7976931348623157E308;
                    min = Double.MAX_VALUE;
                    if (imin == null || imax == null) break block11;
                    if (!(imin <= imax)) break block12;
                    min = imin;
                    max = imax;
                    break block13;
                }
                if (!(imin > imax)) break block13;
                min = imax;
                max = imin;
                break block13;
            }
            for (Number num : this.unscaled) {
                if (!(num instanceof Double)) continue;
                if ((Double)num > max) {
                    max = (Double)num;
                }
                if (!((Double)num < min)) continue;
                min = (Double)num;
            }
        }
        if (max == min) {
            max += max != 0.0 ? Math.abs(max / 10.0) : 0.1;
            min -= min != 0.0 ? Math.abs(min / 10.0) : 0.1;
        }
        if (type == 1) {
            if (min <= 0.0) {
                min = 0.001f;
            }
            if (max < (double)0.001f) {
                max = 1.0;
            }
            double lmin = Math.log(min);
            double lmax = Math.log(max);
            double ldiff = lmax - lmin;
            Iterator<Number> iter = this.unscaled.iterator();
            while (iter.hasNext()) {
                int pos = iter.next().intValue();
                double val = iter.next().doubleValue();
                float par = (float)(val > 0.0 ? (Math.log(val) - lmin) / ldiff : -1.0);
                int intpar = Float.floatToIntBits(par);
                this.pPaints[pos++] = (byte)(intpar & 0xFF);
                this.pPaints[pos++] = (byte)(intpar >> 8 & 0xFF);
                this.pPaints[pos++] = (byte)(intpar >> 16 & 0xFF);
                this.pPaints[pos++] = (byte)(intpar >> 24 & 0xFF);
            }
        } else {
            double diff = max - min;
            Iterator<Number> iter = this.unscaled.iterator();
            while (iter.hasNext()) {
                int pos = iter.next().intValue();
                double val = iter.next().doubleValue();
                float par = (float)((val - min) / diff);
                int intpar = Float.floatToIntBits(par);
                this.pPaints[pos++] = (byte)(intpar & 0xFF);
                this.pPaints[pos++] = (byte)(intpar >> 8 & 0xFF);
                this.pPaints[pos++] = (byte)(intpar >> 16 & 0xFF);
                this.pPaints[pos++] = (byte)(intpar >> 24 & 0xFF);
            }
        }
        if (unit != null) {
            this.setVaxis(new ValueAxis(type, unit, min, max));
        }
    }

    @Override
    public void applyAttachments() {
        int added = 0;
        int ignored = 0;
        if (this.attachments != null) {
            double rate = 100.0 / (double)this.attachments.size();
            for (Attachment a : this.attachments) {
                if (added == 0 || 1.0 * (double)added / (double)(added + ignored) < rate) {
                    ++added;
                    if (a.value instanceof IAttachment.IAttachedRelation) {
                        int type = ((IAttachment.IAttachedRelation)a.value).getType() | 2;
                        long units = ((IAttachment.IAttachedRelation)a.value).getAbsoluteTargetUnits(null);
                        String domainBase = null;
                        String style = ((IAttachment.IAttachedRelation)a.value).getStyle();
                        String target = ((IAttachment.IAttachedRelation)a.value).getTargetId();
                        int sourceLayer = ((IAttachment.IAttachedRelation)a.value).getSourceLayer();
                        int targetLayer = ((IAttachment.IAttachedRelation)a.value).getTargetLayer();
                        this._attach(a.paint, a.x, type, units, sourceLayer, targetLayer, a.index, this.value(style), this.value(target), this.value(domainBase));
                        continue;
                    }
                    if (!(a.value instanceof IAttachment.IAttachedLabel)) continue;
                    String style = ((IAttachment.IAttachedLabel)a.value).getStyle();
                    this._attach(a.paint, a.x, a.a, a.index, this.value(style));
                    continue;
                }
                ++ignored;
            }
        }
    }

    class Attachment {
        int paint;
        int x;
        Double a;
        int index;
        IAttachment value;

        public Attachment(int paint, int x, Double a, int index, IAttachment value) {
            this.paint = paint;
            this.x = x;
            this.a = a;
            this.index = index;
            this.value = value;
        }
    }
}

