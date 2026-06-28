/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.swv;

import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.cells.serializer.Serializer;
import de.toem.toolkits.pattern.element.CellAnnotation;

@CellAnnotation(type="impulse.serializer.configuration.swv", dynamicChildOf={"impulse.serializer"})
public class SwvConfiguration
extends ReaderConfiguration {
    public static final String TYPE = "impulse.serializer.configuration.swv";
    public int tsFrequency = 8000000;
    public int tsDevider = 1;
    public boolean waitForSync = false;
    public static final int ITM_MODE_NONE = 0;
    public static final int ITM_MODE_TEXT = 1;
    public static final int ITM_MODE_TEXT_CRLF = 2;
    public static final int ITM_MODE_SINT8 = 3;
    public static final int ITM_MODE_UINT8 = 4;
    public static final int ITM_MODE_SINT16 = 5;
    public static final int ITM_MODE_UINT16 = 6;
    public static final int ITM_MODE_SINT32 = 7;
    public static final int ITM_MODE_UINT32 = 8;
    public static final int ITM_MODE_SINT64 = 9;
    public static final int ITM_MODE_UINT64 = 10;
    public static final int ITM_MODE_FLOAT32 = 11;
    public static final int ITM_MODE_FLOAT64 = 21;
    public static final int ITM_MODE_LOGICSTATE = 13;
    public static final int ITM_MODE_LOG = 14;
    public static final int ITM_MODE_BINARY = 15;
    public static final String[] ITM_MODE_LABELS = new String[]{"Not used", "Text-0x0", "Text-CR/LF", "Signed 8", "Unsigned 8", "Signed 16", "Unsigned 16", "Signed 32", "Unsigned 32", "Signed 64", "Unsigned 64", "Float 32", "Float 64", "Logic State", "Log", "Binary"};
    public static final Object[] ITM_MODE_OPTIONS = new Object[]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 21, 13, 14, 15};
    public String itmLabel_0 = "0";
    public int itmMode_0;
    public String itmLabel_1 = "1";
    public int itmMode_1;
    public String itmLabel_2 = "2";
    public int itmMode_2;
    public String itmLabel_3 = "3";
    public int itmMode_3;
    public String itmLabel_4 = "4";
    public int itmMode_4;
    public String itmLabel_5 = "5";
    public int itmMode_5;
    public String itmLabel_6 = "6";
    public int itmMode_6;
    public String itmLabel_7 = "7";
    public int itmMode_7;
    public String itmLabel_8 = "8";
    public int itmMode_8;
    public String itmLabel_9 = "9";
    public int itmMode_9;
    public String itmLabel_10 = "10";
    public int itmMode_10;
    public String itmLabel_11 = "11";
    public int itmMode_11;
    public String itmLabel_12 = "12";
    public int itmMode_12;
    public String itmLabel_13 = "13";
    public int itmMode_13;
    public String itmLabel_14 = "14";
    public int itmMode_14;
    public String itmLabel_15 = "15";
    public int itmMode_15;
    public String itmLabel_16 = "16";
    public int itmMode_16;
    public String itmLabel_17 = "17";
    public int itmMode_17;
    public String itmLabel_18 = "18";
    public int itmMode_18;
    public String itmLabel_19 = "19";
    public int itmMode_19;
    public String itmLabel_20 = "20";
    public int itmMode_20;
    public String itmLabel_21 = "21";
    public int itmMode_21;
    public String itmLabel_22 = "22";
    public int itmMode_22;
    public String itmLabel_23 = "23";
    public int itmMode_23;
    public String itmLabel_24 = "24";
    public int itmMode_24;
    public String itmLabel_25 = "25";
    public int itmMode_25;
    public String itmLabel_26 = "26";
    public int itmMode_26;
    public String itmLabel_27 = "27";
    public int itmMode_27;
    public String itmLabel_28 = "28";
    public int itmMode_28;
    public String itmLabel_29 = "29";
    public int itmMode_29;
    public String itmLabel_30 = "30";
    public int itmMode_30;
    public String itmLabel_31 = "31";
    public int itmMode_31;

    @Override
    public boolean supports(Serializer serializer) {
        return "de.toem.impulse.serializer.swv".equals(serializer.id);
    }
}

