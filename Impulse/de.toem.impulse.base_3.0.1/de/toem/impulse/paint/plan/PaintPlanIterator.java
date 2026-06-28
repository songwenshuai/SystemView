/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.PaintPlan;
import de.toem.toolkits.core.Assert;
import de.toem.toolkits.core.Base64;
import java.util.List;
import java.util.function.Consumer;
import org.eclipse.swt.graphics.Rectangle;

public class PaintPlanIterator
implements IPlan.IPaintPlanIterator {
    private PaintPlan plan;
    byte[] bytes;
    int current;
    int pos;
    int len;
    boolean dir;
    List<String> values;
    double[] scale = new double[]{0.0, 1.0};
    static final int MIN_PACKET_SIZE = 4;
    static final int PREV = 0;
    static final int CURRENT = 1;
    static final int NEXT = 2;
    static final int PAINT = 0;
    static final int IDX = 1;
    static final int FLAG = 2;
    static final int LOADED = 3;
    static final int X = 4;
    static final int X2T = 5;
    static final int X3D = 6;
    static final int X4D = 7;
    static final int X5D = 8;
    static final int X6D = 9;
    static final int P1A = 10;
    static final int P2 = 11;
    static final int P3B = 12;
    static final int P4 = 13;
    static final int P5C = 14;
    static final int P6 = 15;
    static final int P7D = 16;
    static final int P8 = 17;
    static final int NVALS = 18;
    static final int VAL = 19;
    static final int SIZE = 275;
    static final int[][] data = new int[3][275];

    public PaintPlanIterator(IPlan.IPaintPlan plan, boolean interPlot) {
        this.plan = (PaintPlan)plan;
        if (interPlot) {
            this.bytes = ((PaintPlan)plan).iPaints;
            this.len = ((PaintPlan)plan).iLen;
        } else {
            this.bytes = ((PaintPlan)plan).pPaints;
            this.len = ((PaintPlan)plan).pLen;
        }
        this.values = ((PaintPlan)plan).values;
        if (((PaintPlan)plan).scale != null) {
            this.scale[0] = ((PaintPlan)plan).scale[0];
            this.scale[1] = ((PaintPlan)plan).scale[1];
        }
        this.pos = 0;
        this.current = -1;
        this.dir = true;
        this.parse(true, 2);
    }

    @Override
    public ITheme getTheme() {
        return this.plan.getTheme();
    }

    @Override
    public IDomainAxis getAxis() {
        return this.plan.getAxis();
    }

    @Override
    public Rectangle getArea() {
        return this.plan.getArea();
    }

    @Override
    public int getStyle() {
        return this.plan.getStyle();
    }

    @Override
    public ITreeItem getItem() {
        return this.plan.getItem();
    }

    @Override
    public int getScheme() {
        return this.plan.getScheme();
    }

    public boolean parse(boolean forward, int set) {
        int flag = 0;
        int len = 4;
        int nvals = 0;
        PaintPlanIterator.data[set][3] = 0;
        if (!forward) {
            if (this.pos < 4) {
                return false;
            }
            flag = this.bytes[this.pos - 1] & 0xFF;
            if ((flag & 3) == 3) {
                nvals = this.bytes[this.pos - 2] & 0xFF;
            }
        } else {
            if (this.pos + 4 > this.len) {
                return false;
            }
            flag = this.bytes[this.pos] & 0xFF;
            if ((flag & 3) == 3) {
                nvals = this.bytes[this.pos + 1] & 0xFF;
            }
        }
        switch (flag & 0xC0) {
            case 192: {
                len += 8;
            }
            case 128: {
                len += 2;
            }
            case 64: {
                len += 2;
            }
        }
        switch (flag & 0x30) {
            case 48: {
                len += 8;
            }
            case 32: {
                len += 4;
            }
            case 16: {
                len += 4;
            }
        }
        switch (flag & 0xC) {
            case 12: {
                len += 2;
            }
            case 8: {
                ++len;
            }
            case 4: {
                ++len;
            }
        }
        switch (flag & 3) {
            case 3: {
                len += 2 + nvals * 2;
                break;
            }
            case 2: {
                len += 2;
            }
            case 1: {
                len += 2;
            }
        }
        if (!forward) {
            if (this.pos < len) {
                return false;
            }
            this.pos -= len;
            Assert.isTrue((this.bytes[this.pos] & 0xFF) == flag);
        } else {
            if (this.pos + len > this.len) {
                return false;
            }
            Assert.isTrue((this.bytes[this.pos + len - 1] & 0xFF) == flag);
        }
        this.pos += (flag & 3) == 3 ? 2 : 1;
        PaintPlanIterator.data[set][2] = flag;
        PaintPlanIterator.data[set][0] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
        if ((flag & 0xC0) >= 64) {
            PaintPlanIterator.data[set][4] = this.bytes[this.pos++] & 0xFF | this.bytes[this.pos++] << 8;
            if ((flag & 0xC0) >= 128) {
                PaintPlanIterator.data[set][5] = this.bytes[this.pos++] & 0xFF | this.bytes[this.pos++] << 8;
                if ((flag & 0xC0) >= 192) {
                    PaintPlanIterator.data[set][6] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                    PaintPlanIterator.data[set][7] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                    PaintPlanIterator.data[set][8] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                    PaintPlanIterator.data[set][9] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                }
            }
        }
        if ((flag & 0x30) >= 16) {
            PaintPlanIterator.data[set][10] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
            PaintPlanIterator.data[set][11] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
            if ((flag & 0x30) >= 32) {
                PaintPlanIterator.data[set][12] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                PaintPlanIterator.data[set][13] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                if ((flag & 0x30) >= 48) {
                    PaintPlanIterator.data[set][14] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                    PaintPlanIterator.data[set][15] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                    PaintPlanIterator.data[set][16] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                    PaintPlanIterator.data[set][17] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                }
            }
        }
        if ((flag & 0xC) >= 4) {
            PaintPlanIterator.data[set][1] = this.bytes[this.pos++] & 0xFF;
            if ((flag & 0xC) >= 8) {
                int[] nArray = data[set];
                nArray[1] = nArray[1] | (this.bytes[this.pos++] & 0xFF) << 8;
                if ((flag & 0xC) >= 12) {
                    int[] nArray2 = data[set];
                    nArray2[1] = nArray2[1] | (this.bytes[this.pos++] & 0xFF) << 16;
                    int[] nArray3 = data[set];
                    nArray3[1] = nArray3[1] | (this.bytes[this.pos++] & 0xFF) << 24;
                }
            }
        }
        if ((flag & 3) >= 1) {
            if ((flag & 3) == 3) {
                PaintPlanIterator.data[set][18] = nvals;
                int n = 0;
                while (n < nvals) {
                    PaintPlanIterator.data[set][19 + n] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                    ++n;
                }
            } else {
                PaintPlanIterator.data[set][18] = 1;
                PaintPlanIterator.data[set][19] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                if ((flag & 3) == 2) {
                    int[] nArray = data[set];
                    nArray[18] = nArray[18] + 1;
                    PaintPlanIterator.data[set][20] = this.bytes[this.pos++] & 0xFF | (this.bytes[this.pos++] & 0xFF) << 8;
                }
            }
        }
        this.pos = !forward ? (this.pos -= len - 1) : (this.pos += (flag & 3) == 3 ? 2 : 1);
        PaintPlanIterator.data[set][3] = 1;
        return true;
    }

    @Override
    public boolean hasNext() {
        return data[2][3] == 1;
    }

    @Override
    public Integer next() {
        if (!this.hasNext()) {
            return 0;
        }
        if (!this.dir) {
            this.parse(true, 0);
            this.parse(true, 1);
            this.parse(true, 2);
            this.dir = true;
        } else {
            int[] b = data[0];
            PaintPlanIterator.data[0] = data[1];
            PaintPlanIterator.data[1] = data[2];
            PaintPlanIterator.data[2] = b;
            this.parse(true, 2);
        }
        ++this.current;
        return this.paint();
    }

    @Override
    public boolean hasPrev() {
        return data[0][3] == 1;
    }

    @Override
    public Integer prev() {
        if (!this.hasPrev()) {
            return 0;
        }
        if (this.dir) {
            this.parse(false, 2);
            this.parse(false, 1);
            this.parse(false, 0);
            this.dir = false;
        } else {
            int[] b = data[2];
            PaintPlanIterator.data[2] = data[1];
            PaintPlanIterator.data[1] = data[0];
            PaintPlanIterator.data[0] = b;
            this.parse(false, 0);
        }
        --this.current;
        return this.paint();
    }

    @Override
    public int index() {
        return data[1][1];
    }

    @Override
    public int paint() {
        return data[1][0];
    }

    @Override
    public int nextPaint() {
        return data[2][0];
    }

    @Override
    public int prevPaint() {
        return data[0][0];
    }

    @Override
    public boolean hasX() {
        return (data[1][2] & 0x40) != 0;
    }

    @Override
    public int x() {
        return (int)((double)data[1][4] * this.scale[1] + this.scale[0]);
    }

    @Override
    public boolean hasX2() {
        return (data[1][2] & 0x80) != 0;
    }

    @Override
    public int x2() {
        return (int)((double)data[1][5] * this.scale[1] + this.scale[0]);
    }

    @Override
    public boolean hasPosition() {
        return (data[1][2] & 0xC0) != 0;
    }

    @Override
    public long position() {
        return (long)data[1][6] & 0xFFFFL | ((long)data[1][7] & 0xFFFFL) << 16 | ((long)data[1][8] & 0xFFFFL) << 32 | ((long)data[1][9] & 0xFFFFL) << 48;
    }

    @Override
    public boolean hasXn() {
        return (data[1][2] & 0xC0) != 0;
    }

    @Override
    public int xn(int n) {
        if (n >= 0 && n < 4) {
            return (int)((double)data[1][6 + n] * this.scale[1] + this.scale[0]);
        }
        return 0;
    }

    @Override
    public boolean hasType() {
        return (data[1][2] & 0x80) != 0;
    }

    @Override
    public int type() {
        return data[1][5];
    }

    @Override
    public int nextX() {
        return (int)((double)data[2][4] * this.scale[1] + this.scale[0]);
    }

    @Override
    public int prevX() {
        return (int)((double)data[0][4] * this.scale[1] + this.scale[0]);
    }

    @Override
    public boolean hasP() {
        return (data[1][2] & 0x30) >= 16;
    }

    @Override
    public boolean has2P() {
        return (data[1][2] & 0x30) >= 16;
    }

    @Override
    public boolean has4P() {
        return (data[1][2] & 0x30) >= 32;
    }

    @Override
    public boolean has8P() {
        return (data[1][2] & 0x30) >= 48;
    }

    @Override
    public short p() {
        return (short)data[1][10];
    }

    @Override
    public short p2() {
        return (short)data[1][11];
    }

    @Override
    public short p3() {
        return (short)data[1][12];
    }

    @Override
    public short p4() {
        return (short)data[1][13];
    }

    @Override
    public short p5() {
        return (short)data[1][14];
    }

    @Override
    public short p6() {
        return (short)data[1][15];
    }

    @Override
    public short p7() {
        return (short)data[1][16];
    }

    @Override
    public short p8() {
        return (short)data[1][17];
    }

    @Override
    public boolean hasA() {
        return (data[1][2] & 0x30) >= 16;
    }

    @Override
    public boolean hasAb() {
        return (data[1][2] & 0x30) >= 32;
    }

    @Override
    public boolean hasAbcd() {
        return (data[1][2] & 0x30) == 48;
    }

    @Override
    public boolean hasNextAb() {
        return (data[2][2] & 0x30) >= 32;
    }

    @Override
    public double a() {
        int p12 = data[1][10] & 0xFFFF | (data[1][11] & 0xFFFF) << 16;
        return Float.intBitsToFloat(p12);
    }

    @Override
    public double b() {
        int p34 = data[1][12] & 0xFFFF | (data[1][13] & 0xFFFF) << 16;
        return Float.intBitsToFloat(p34);
    }

    @Override
    public double c() {
        int p34 = data[1][14] & 0xFFFF | (data[1][15] & 0xFFFF) << 16;
        return Float.intBitsToFloat(p34);
    }

    @Override
    public double d() {
        int p34 = data[1][16] & 0xFFFF | (data[1][17] & 0xFFFF) << 16;
        return Float.intBitsToFloat(p34);
    }

    @Override
    public double nextA() {
        int p12 = data[2][10] & 0xFFFF | (data[2][11] & 0xFFFF) << 16;
        return Float.intBitsToFloat(p12);
    }

    @Override
    public double nextB() {
        int p34 = data[2][12] & 0xFFFF | (data[2][13] & 0xFFFF) << 16;
        return Float.intBitsToFloat(p34);
    }

    @Override
    public boolean hasVal() {
        return (data[1][2] & 3) != 0;
    }

    @Override
    public int noOfVals() {
        return (data[1][2] & 3) != 0 ? data[1][18] : 0;
    }

    @Override
    public String val() {
        int idx;
        if ((data[1][2] & 3) != 0 && (idx = data[1][19]) >= 0 && idx < this.values.size()) {
            return this.values.get(idx);
        }
        return null;
    }

    @Override
    public byte[] bval() {
        String val = this.val();
        if (val != null) {
            return Base64.decode(val);
        }
        return null;
    }

    @Override
    public String valAt(int n) {
        int idx;
        if ((data[1][2] & 3) != 0 && (idx = data[1][19 + n]) >= 0 && idx < this.values.size()) {
            return this.values.get(idx);
        }
        return null;
    }

    @Override
    public int current() {
        return this.current;
    }

    @Override
    public void remove() {
    }

    @Override
    public void forEachRemaining(Consumer<? super Integer> action) {
    }
}

