/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse;

import de.toem.impulse.IKeyListener;
import de.toem.impulse.IKeyPropertyIdProvider;
import de.toem.impulse.IKeyProvider;
import de.toem.impulse.ImpulseBase;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.keys.KeyLoader;
import de.toem.toolkits.keys.LoadedKey;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.information.IInformation;
import de.toem.toolkits.pattern.registry.RegisteredObjects;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class ImpulseLicense {
    private static RegisteredObjects<IKeyProvider> keyProviders = new RegisteredObjects("de.toem.impulse.base.keyProvider", true);
    static byte[] symetricKeyData = new byte[]{28, 69, -101, 42, -56, 88, 25, 87, 22, 98, 109, 26, 69, 74, -85, -125, -113, -71, -74, 73, 112, -89, 87, -60};
    static byte[] publicKeyData;
    private static Map<IKeyProvider, LoadedKey> loadedKeys;
    private static Set<String> activeProperties;
    public static final String FEATURE_DOMAIN_SERIALIZER = "de.toem.impulse.featureDomain.serializer";
    public static final String FEATURE_DOMAIN_DIAGRAMS = "de.toem.impulse.featureDomain.diagrams";
    public static final String FEATURE_DOMAIN_PRODUCTIONS = "de.toem.impulse.featureDomain.productions";
    public static final String FEATURE_DOMAIN_DISCLOSURES = "de.toem.impulse.featureDomain.disclosures";
    public static final String FEATURE_DOMAIN_PORTS = "de.toem.impulse.featureDomain.ports";
    public static final String FEATURE_DOMAIN_GLOBAL = "de.toem.impulse.featureDomain.global";
    public static final String FEATURE_DEFAULT = "de.toem.impulse.feature.default";
    public static final String FEATURE_DIAGRAM_RELATION = "de.toem.impulse.feature.diagram.relation";
    public static final String FEATURE_DIAGRAM_LABEL = "de.toem.impulse.feature.diagram.label";
    public static final String FEATURE_PORT_SYNC = "de.toem.impulse.feature.diagram.label";
    public static final String FEATURE_GLOBAL_DIFF = "de.toem.impulse.feature.global.diff";
    public static final String FEATURE_GLOBAL_PRINT = "de.toem.impulse.feature.global.print";
    public static final String PROPERTY_ESSENTIALS = "Essentials";
    public static final String PROPERTY_BASE = "Base";
    public static final String PROPERTY_ANALYZE = "Analyze";
    public static final String PROPERTY_ANALYSE = "Analyse";
    public static final String PROPERTY_DESIGN = "Design";
    public static final String PROPERTY_ESSENTIAL_SERIALIZER = "essentialSerializer";
    public static final String PROPERTY_BASE_SERIALIZER = "baseSerializer";
    public static final String PROPERTY_ESSETIAL_DIAGRAMS = "essentialDiagrams";
    public static final String PROPERTY_BASE_DIAGRAMS = "baseDiagrams";
    public static final String PROPERTY_ANAYLSE_DIAGRAMS = "analyseDiagrams";
    public static final String PROPERTY_BASE_PRODUCTIONS = "baseProductions";
    public static final String PROPERTY_ANALYSE_PRODUCTIONS = "analyseProductions";
    public static final String PROPERTY_ESSENTIAL_DISCLOSURES = "essentialDisclosures";
    public static final String PROPERTY_ANALYSE_DISCLOSURES = "analyseDisclosures";
    public static final String PROPERTY_BASE_PORTS = "basePorts";
    public static final String PROPERTY_ANALYSE_PORTS = "analysePorts";
    public static final String PROPERTY_DIFF = "diff";
    public static final String PROPERTY_PRINT = "print";
    public static final String PROPERTY_PORT_SYNC = "portSync";

    static {
        byte[] byArray = new byte[444];
        byArray[0] = 48;
        byArray[1] = -126;
        byArray[2] = 1;
        byArray[3] = -72;
        byArray[4] = 48;
        byArray[5] = -126;
        byArray[6] = 1;
        byArray[7] = 44;
        byArray[8] = 6;
        byArray[9] = 7;
        byArray[10] = 42;
        byArray[11] = -122;
        byArray[12] = 72;
        byArray[13] = -50;
        byArray[14] = 56;
        byArray[15] = 4;
        byArray[16] = 1;
        byArray[17] = 48;
        byArray[18] = -126;
        byArray[19] = 1;
        byArray[20] = 31;
        byArray[21] = 2;
        byArray[22] = -127;
        byArray[23] = -127;
        byArray[25] = -3;
        byArray[26] = 127;
        byArray[27] = 83;
        byArray[28] = -127;
        byArray[29] = 29;
        byArray[30] = 117;
        byArray[31] = 18;
        byArray[32] = 41;
        byArray[33] = 82;
        byArray[34] = -33;
        byArray[35] = 74;
        byArray[36] = -100;
        byArray[37] = 46;
        byArray[38] = -20;
        byArray[39] = -28;
        byArray[40] = -25;
        byArray[41] = -10;
        byArray[42] = 17;
        byArray[43] = -73;
        byArray[44] = 82;
        byArray[45] = 60;
        byArray[46] = -17;
        byArray[47] = 68;
        byArray[49] = -61;
        byArray[50] = 30;
        byArray[51] = 63;
        byArray[52] = -128;
        byArray[53] = -74;
        byArray[54] = 81;
        byArray[55] = 38;
        byArray[56] = 105;
        byArray[57] = 69;
        byArray[58] = 93;
        byArray[59] = 64;
        byArray[60] = 34;
        byArray[61] = 81;
        byArray[62] = -5;
        byArray[63] = 89;
        byArray[64] = 61;
        byArray[65] = -115;
        byArray[66] = 88;
        byArray[67] = -6;
        byArray[68] = -65;
        byArray[69] = -59;
        byArray[70] = -11;
        byArray[71] = -70;
        byArray[72] = 48;
        byArray[73] = -10;
        byArray[74] = -53;
        byArray[75] = -101;
        byArray[76] = 85;
        byArray[77] = 108;
        byArray[78] = -41;
        byArray[79] = -127;
        byArray[80] = 59;
        byArray[81] = -128;
        byArray[82] = 29;
        byArray[83] = 52;
        byArray[84] = 111;
        byArray[85] = -14;
        byArray[86] = 102;
        byArray[87] = 96;
        byArray[88] = -73;
        byArray[89] = 107;
        byArray[90] = -103;
        byArray[91] = 80;
        byArray[92] = -91;
        byArray[93] = -92;
        byArray[94] = -97;
        byArray[95] = -97;
        byArray[96] = -24;
        byArray[97] = 4;
        byArray[98] = 123;
        byArray[99] = 16;
        byArray[100] = 34;
        byArray[101] = -62;
        byArray[102] = 79;
        byArray[103] = -69;
        byArray[104] = -87;
        byArray[105] = -41;
        byArray[106] = -2;
        byArray[107] = -73;
        byArray[108] = -58;
        byArray[109] = 27;
        byArray[110] = -8;
        byArray[111] = 59;
        byArray[112] = 87;
        byArray[113] = -25;
        byArray[114] = -58;
        byArray[115] = -88;
        byArray[116] = -90;
        byArray[117] = 21;
        byArray[118] = 15;
        byArray[119] = 4;
        byArray[120] = -5;
        byArray[121] = -125;
        byArray[122] = -10;
        byArray[123] = -45;
        byArray[124] = -59;
        byArray[125] = 30;
        byArray[126] = -61;
        byArray[127] = 2;
        byArray[128] = 53;
        byArray[129] = 84;
        byArray[130] = 19;
        byArray[131] = 90;
        byArray[132] = 22;
        byArray[133] = -111;
        byArray[134] = 50;
        byArray[135] = -10;
        byArray[136] = 117;
        byArray[137] = -13;
        byArray[138] = -82;
        byArray[139] = 43;
        byArray[140] = 97;
        byArray[141] = -41;
        byArray[142] = 42;
        byArray[143] = -17;
        byArray[144] = -14;
        byArray[145] = 34;
        byArray[146] = 3;
        byArray[147] = 25;
        byArray[148] = -99;
        byArray[149] = -47;
        byArray[150] = 72;
        byArray[151] = 1;
        byArray[152] = -57;
        byArray[153] = 2;
        byArray[154] = 21;
        byArray[156] = -105;
        byArray[157] = 96;
        byArray[158] = 80;
        byArray[159] = -113;
        byArray[160] = 21;
        byArray[161] = 35;
        byArray[162] = 11;
        byArray[163] = -52;
        byArray[164] = -78;
        byArray[165] = -110;
        byArray[166] = -71;
        byArray[167] = -126;
        byArray[168] = -94;
        byArray[169] = -21;
        byArray[170] = -124;
        byArray[171] = 11;
        byArray[172] = -16;
        byArray[173] = 88;
        byArray[174] = 28;
        byArray[175] = -11;
        byArray[176] = 2;
        byArray[177] = -127;
        byArray[178] = -127;
        byArray[180] = -9;
        byArray[181] = -31;
        byArray[182] = -96;
        byArray[183] = -123;
        byArray[184] = -42;
        byArray[185] = -101;
        byArray[186] = 61;
        byArray[187] = -34;
        byArray[188] = -53;
        byArray[189] = -68;
        byArray[190] = -85;
        byArray[191] = 92;
        byArray[192] = 54;
        byArray[193] = -72;
        byArray[194] = 87;
        byArray[195] = -71;
        byArray[196] = 121;
        byArray[197] = -108;
        byArray[198] = -81;
        byArray[199] = -69;
        byArray[200] = -6;
        byArray[201] = 58;
        byArray[202] = -22;
        byArray[203] = -126;
        byArray[204] = -7;
        byArray[205] = 87;
        byArray[206] = 76;
        byArray[207] = 11;
        byArray[208] = 61;
        byArray[209] = 7;
        byArray[210] = -126;
        byArray[211] = 103;
        byArray[212] = 81;
        byArray[213] = 89;
        byArray[214] = 87;
        byArray[215] = -114;
        byArray[216] = -70;
        byArray[217] = -44;
        byArray[218] = 89;
        byArray[219] = 79;
        byArray[220] = -26;
        byArray[221] = 113;
        byArray[222] = 7;
        byArray[223] = 16;
        byArray[224] = -127;
        byArray[225] = -128;
        byArray[226] = -76;
        byArray[227] = 73;
        byArray[228] = 22;
        byArray[229] = 113;
        byArray[230] = 35;
        byArray[231] = -24;
        byArray[232] = 76;
        byArray[233] = 40;
        byArray[234] = 22;
        byArray[235] = 19;
        byArray[236] = -73;
        byArray[237] = -49;
        byArray[238] = 9;
        byArray[239] = 50;
        byArray[240] = -116;
        byArray[241] = -56;
        byArray[242] = -90;
        byArray[243] = -31;
        byArray[244] = 60;
        byArray[245] = 22;
        byArray[246] = 122;
        byArray[247] = -117;
        byArray[248] = 84;
        byArray[249] = 124;
        byArray[250] = -115;
        byArray[251] = 40;
        byArray[252] = -32;
        byArray[253] = -93;
        byArray[254] = -82;
        byArray[255] = 30;
        byArray[256] = 43;
        byArray[257] = -77;
        byArray[258] = -90;
        byArray[259] = 117;
        byArray[260] = -111;
        byArray[261] = 110;
        byArray[262] = -93;
        byArray[263] = 127;
        byArray[264] = 11;
        byArray[265] = -6;
        byArray[266] = 33;
        byArray[267] = 53;
        byArray[268] = 98;
        byArray[269] = -15;
        byArray[270] = -5;
        byArray[271] = 98;
        byArray[272] = 122;
        byArray[273] = 1;
        byArray[274] = 36;
        byArray[275] = 59;
        byArray[276] = -52;
        byArray[277] = -92;
        byArray[278] = -15;
        byArray[279] = -66;
        byArray[280] = -88;
        byArray[281] = 81;
        byArray[282] = -112;
        byArray[283] = -119;
        byArray[284] = -88;
        byArray[285] = -125;
        byArray[286] = -33;
        byArray[287] = -31;
        byArray[288] = 90;
        byArray[289] = -27;
        byArray[290] = -97;
        byArray[291] = 6;
        byArray[292] = -110;
        byArray[293] = -117;
        byArray[294] = 102;
        byArray[295] = 94;
        byArray[296] = -128;
        byArray[297] = 123;
        byArray[298] = 85;
        byArray[299] = 37;
        byArray[300] = 100;
        byArray[301] = 1;
        byArray[302] = 76;
        byArray[303] = 59;
        byArray[304] = -2;
        byArray[305] = -49;
        byArray[306] = 73;
        byArray[307] = 42;
        byArray[308] = 3;
        byArray[309] = -127;
        byArray[310] = -123;
        byArray[312] = 2;
        byArray[313] = -127;
        byArray[314] = -127;
        byArray[316] = -96;
        byArray[317] = -36;
        byArray[318] = -78;
        byArray[319] = -107;
        byArray[320] = 22;
        byArray[321] = -8;
        byArray[322] = 120;
        byArray[323] = 8;
        byArray[324] = -57;
        byArray[325] = -37;
        byArray[326] = -96;
        byArray[327] = -68;
        byArray[328] = -101;
        byArray[329] = 108;
        byArray[330] = 88;
        byArray[331] = 121;
        byArray[332] = -2;
        byArray[333] = -119;
        byArray[334] = -125;
        byArray[335] = 118;
        byArray[336] = 99;
        byArray[337] = -9;
        byArray[338] = 5;
        byArray[339] = -47;
        byArray[340] = 89;
        byArray[341] = 88;
        byArray[342] = -43;
        byArray[343] = -120;
        byArray[344] = -22;
        byArray[345] = 42;
        byArray[346] = 84;
        byArray[347] = -37;
        byArray[348] = 4;
        byArray[349] = 16;
        byArray[350] = -17;
        byArray[351] = 60;
        byArray[352] = 74;
        byArray[353] = 122;
        byArray[354] = 82;
        byArray[355] = 122;
        byArray[356] = 55;
        byArray[357] = 16;
        byArray[358] = 82;
        byArray[359] = -58;
        byArray[360] = 56;
        byArray[361] = 96;
        byArray[362] = 116;
        byArray[363] = 111;
        byArray[364] = 7;
        byArray[365] = -112;
        byArray[366] = 49;
        byArray[367] = 71;
        byArray[368] = -8;
        byArray[369] = -110;
        byArray[370] = -126;
        byArray[371] = -71;
        byArray[372] = 111;
        byArray[373] = 34;
        byArray[374] = 38;
        byArray[375] = 14;
        byArray[376] = 108;
        byArray[377] = -112;
        byArray[378] = -46;
        byArray[379] = 10;
        byArray[380] = -111;
        byArray[381] = -110;
        byArray[382] = 61;
        byArray[383] = 84;
        byArray[384] = -126;
        byArray[385] = 52;
        byArray[386] = -16;
        byArray[387] = -128;
        byArray[388] = -80;
        byArray[389] = 18;
        byArray[390] = -42;
        byArray[392] = -8;
        byArray[393] = 48;
        byArray[394] = -31;
        byArray[395] = 48;
        byArray[396] = -120;
        byArray[397] = 9;
        byArray[398] = 11;
        byArray[399] = 50;
        byArray[400] = -89;
        byArray[401] = 93;
        byArray[402] = 123;
        byArray[403] = -30;
        byArray[404] = 39;
        byArray[405] = -62;
        byArray[406] = -60;
        byArray[407] = 98;
        byArray[408] = 122;
        byArray[409] = 48;
        byArray[410] = 63;
        byArray[411] = -100;
        byArray[412] = 42;
        byArray[413] = -94;
        byArray[414] = -53;
        byArray[415] = -92;
        byArray[416] = 54;
        byArray[417] = 13;
        byArray[418] = 55;
        byArray[419] = -51;
        byArray[420] = 1;
        byArray[421] = -75;
        byArray[422] = -128;
        byArray[423] = -119;
        byArray[424] = -57;
        byArray[425] = -62;
        byArray[426] = 101;
        byArray[427] = -39;
        byArray[428] = 13;
        byArray[429] = -90;
        byArray[430] = -105;
        byArray[431] = 101;
        byArray[432] = -82;
        byArray[433] = -58;
        byArray[434] = -12;
        byArray[435] = -85;
        byArray[436] = -80;
        byArray[437] = -122;
        byArray[438] = 36;
        byArray[439] = -71;
        byArray[440] = 16;
        byArray[441] = 55;
        byArray[442] = -110;
        byArray[443] = 90;
        publicKeyData = byArray;
        loadedKeys = new HashMap<IKeyProvider, LoadedKey>();
        ImpulseLicense.loadKeys(loadedKeys);
    }

    private static void loadKeys(final Map<IKeyProvider, LoadedKey> map) {
        for (IKeyProvider provider : keyProviders.getAll()) {
            provider.addListener(new IKeyListener(){

                /*
                 * WARNING - Removed try catching itself - possible behaviour change.
                 */
                @Override
                public void keyChanged(IKeyProvider provider) {
                    LoadedKey loaded = KeyLoader.decrypt(provider.getLabel(), provider.getKey(), provider.getPreset(), provider.getPriority(), symetricKeyData, publicKeyData);
                    if (loaded == null) {
                        if (map.containsKey(provider)) {
                            map.remove(provider);
                        }
                    } else {
                        map.put(provider, loaded);
                        if (loaded.isValid()) {
                            ImpulseLicense.checkKey(loaded);
                        }
                    }
                    RegisteredObjects registeredObjects = keyProviders;
                    synchronized (registeredObjects) {
                        activeProperties = null;
                    }
                }
            });
            LoadedKey loaded = KeyLoader.decrypt(provider.getLabel(), provider.getKey(), provider.getPreset(), provider.getPriority(), symetricKeyData, publicKeyData);
            if (loaded == null) continue;
            map.put(provider, loaded);
            if (!loaded.isValid()) continue;
            ImpulseLicense.checkKey(loaded);
        }
    }

    private static void checkKey(LoadedKey loaded) {
        if (loaded.isValid()) {
            String versions;
            String version = ImpulseBase.getBundle().getVersion().toString().substring(0, 3);
            if (loaded.contains("VERSION") && !(versions = loaded.get("VERSION")).contains(version)) {
                loaded.setInValid();
            }
        }
    }

    public static List<LoadedKey> getAllKeys() {
        ArrayList<LoadedKey> keys = new ArrayList<LoadedKey>();
        keys.addAll(loadedKeys.values());
        return keys;
    }

    public static LoadedKey getActiveKey() {
        LoadedKey active = null;
        List<LoadedKey> keys = ImpulseLicense.getAllKeys();
        for (LoadedKey key : keys) {
            if (!key.isValid() || active != null && key.getPriority() <= active.getPriority()) continue;
            active = key;
        }
        return active;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public static Set<String> getActiveProperties() {
        RegisteredObjects<IKeyProvider> registeredObjects = keyProviders;
        synchronized (registeredObjects) {
            String properties;
            Set<String> active = activeProperties;
            if (active != null) {
                return active;
            }
            active = new HashSet<String>();
            LoadedKey key = ImpulseLicense.getActiveKey();
            if (key != null && (properties = key.get("PROPERTIES")) != null) {
                String[] splitted;
                String[] stringArray = splitted = properties.split(" ");
                int n = splitted.length;
                int n2 = 0;
                while (n2 < n) {
                    String p = stringArray[n2];
                    if (PROPERTY_ESSENTIALS.equalsIgnoreCase(p)) {
                        active.add(PROPERTY_ESSENTIAL_SERIALIZER);
                        active.add(PROPERTY_ESSETIAL_DIAGRAMS);
                        active.add(PROPERTY_ESSENTIAL_DISCLOSURES);
                    } else if (PROPERTY_BASE.equalsIgnoreCase(p)) {
                        active.add(PROPERTY_ESSENTIAL_SERIALIZER);
                        active.add(PROPERTY_ESSETIAL_DIAGRAMS);
                        active.add(PROPERTY_ESSENTIAL_DISCLOSURES);
                        active.add(PROPERTY_BASE_SERIALIZER);
                        active.add(PROPERTY_BASE_DIAGRAMS);
                        active.add(PROPERTY_BASE_PRODUCTIONS);
                        active.add(PROPERTY_BASE_PORTS);
                    } else if (PROPERTY_ANALYZE.equalsIgnoreCase(p) || PROPERTY_ANALYSE.equalsIgnoreCase(p)) {
                        active.add(PROPERTY_ESSENTIAL_SERIALIZER);
                        active.add(PROPERTY_ESSETIAL_DIAGRAMS);
                        active.add(PROPERTY_ESSENTIAL_DISCLOSURES);
                        active.add(PROPERTY_BASE_SERIALIZER);
                        active.add(PROPERTY_BASE_DIAGRAMS);
                        active.add(PROPERTY_BASE_PRODUCTIONS);
                        active.add(PROPERTY_BASE_PORTS);
                        active.add(PROPERTY_ANAYLSE_DIAGRAMS);
                        active.add(PROPERTY_ANALYSE_PRODUCTIONS);
                        active.add(PROPERTY_ANALYSE_DISCLOSURES);
                        active.add(PROPERTY_ANALYSE_PORTS);
                        active.add(PROPERTY_DIFF);
                        active.add(PROPERTY_PRINT);
                        active.add(PROPERTY_PORT_SYNC);
                    } else if (!PROPERTY_DESIGN.equalsIgnoreCase(p)) {
                        if (p.startsWith("-") && p.length() > 1) {
                            active.remove(p.substring(1));
                        } else if (p.startsWith("+") && p.length() > 1) {
                            active.add(p.substring(1));
                        }
                    }
                    ++n2;
                }
            }
            activeProperties = active;
            return active;
        }
    }

    public static boolean isFeatureLocked(String featureDomain, String feature, Object idprovider) {
        String id = null;
        if (idprovider instanceof ICell) {
            id = ((ICell)idprovider).getType();
        } else if (idprovider instanceof IKeyPropertyIdProvider) {
            id = ((IKeyPropertyIdProvider)idprovider).getKeyPropertyId(featureDomain);
        } else if (idprovider instanceof IInformation) {
            id = ((IInformation)idprovider).getId();
        }
        if (id != null) {
            return ImpulseLicense.isFeatureLocked(featureDomain, feature, id);
        }
        return true;
    }

    public static boolean isFeatureLocked(String featureDomain, String feature, String id) {
        Set<String> properties = ImpulseLicense.getActiveProperties();
        if (FEATURE_DOMAIN_SERIALIZER.equals(featureDomain)) {
            if (FEATURE_DEFAULT.equals(feature)) {
                if (properties.contains(PROPERTY_BASE_SERIALIZER)) {
                    return false;
                }
                if (("de.toem.pattern.xmlSerializer".equals(id) || "de.toem.pattern.zmlSerializer".equals(id) || "de.toem.impulse.serializer.vcd".equals(id) || "de.toem.impulse.serializer.tabular".equals(id) || "de.toem.impulse.serializer.trace".equals(id) || "de.toem.impulse.serializer.flux".equals(id)) && properties.contains(PROPERTY_ESSENTIAL_SERIALIZER)) {
                    return false;
                }
            }
        } else if (FEATURE_DOMAIN_DIAGRAMS.equals(featureDomain)) {
            if (FEATURE_DEFAULT.equals(feature)) {
                int diagram = Utils.parseInt(id, -1);
                if (diagram == 0) {
                    return false;
                }
                if ((1 == diagram || 2 == diagram || 4 == diagram) && properties.contains(PROPERTY_ESSETIAL_DIAGRAMS)) {
                    return false;
                }
                if ((6 == diagram || 5 == diagram || 3 == diagram || 7 == diagram || 10 == diagram) && properties.contains(PROPERTY_BASE_DIAGRAMS)) {
                    return false;
                }
                if ((8 == diagram || 9 == diagram) && properties.contains(PROPERTY_ANAYLSE_DIAGRAMS)) {
                    return false;
                }
            } else if ("de.toem.impulse.feature.diagram.label".equals(feature) ? properties.contains(PROPERTY_ANAYLSE_DIAGRAMS) : FEATURE_DIAGRAM_RELATION.equals(feature) && properties.contains(PROPERTY_BASE_DIAGRAMS)) {
                return false;
            }
        } else if (FEATURE_DOMAIN_PRODUCTIONS.equals(featureDomain)) {
            if (FEATURE_DEFAULT.equals(feature)) {
                if (Utils.isEmpty(id)) {
                    return false;
                }
                if (properties.contains(PROPERTY_ANALYSE_PRODUCTIONS)) {
                    return false;
                }
                if (("de.toem.impulse.production.script.js".equals(id) || "de.toem.impulse.production.logic".equals(id) || "de.toem.impulse.production.logicExtract".equals(id) || "de.toem.impulse.production.memberExtract".equals(id) || "de.toem.impulse.production.textFilter".equals(id) || "de.toem.impulse.production.textExtract".equals(id) || "de.toem.impulse.production.delta".equals(id) || "de.toem.impulse.production.array".equals(id)) && properties.contains(PROPERTY_BASE_PRODUCTIONS)) {
                    return false;
                }
            }
        } else if (FEATURE_DOMAIN_DISCLOSURES.equals(featureDomain)) {
            if (FEATURE_DEFAULT.equals(feature)) {
                if (("de.toem.impulse.disclosures.noOfEvents".equals(id) || "de.toem.impulse.disclosures.firstEvent".equals(id)) && properties.contains(PROPERTY_ESSENTIAL_DISCLOSURES)) {
                    return false;
                }
                if (properties.contains(PROPERTY_ANALYSE_DISCLOSURES)) {
                    return false;
                }
            }
        } else if (FEATURE_DOMAIN_PORTS.equals(featureDomain)) {
            if (FEATURE_DEFAULT.equals(feature)) {
                if (properties.contains(PROPERTY_BASE_PORTS)) {
                    return false;
                }
            } else if ("de.toem.impulse.feature.diagram.label".equals(feature)) {
                if (("port.record.pipe".equals(id) || "port.record.socket".equals(id) || "port.record.process".equals(id) || "port.record.serial".equals(id) || "port.record.sigrok".equals(id)) && properties.contains(PROPERTY_BASE_PORTS)) {
                    return false;
                }
                if (properties.contains(PROPERTY_ANALYSE_PORTS)) {
                    return false;
                }
            }
        } else if (FEATURE_DOMAIN_GLOBAL.equals(featureDomain)) {
            if (FEATURE_GLOBAL_DIFF.equals(feature) && properties.contains(PROPERTY_DIFF)) {
                return false;
            }
            if (FEATURE_GLOBAL_PRINT.equals(feature) && properties.contains(PROPERTY_PRINT)) {
                return false;
            }
        }
        return id == null || !ImpulseLicense.isValidKeyPropertyId(featureDomain, id);
    }

    private static boolean isValidKeyPropertyId(String feature, String pid) {
        if (pid.length() > 9) {
            String idh1 = pid.substring(pid.length() - 8);
            String id = pid.substring(0, pid.length() - 9);
            String idh2 = Integer.toHexString(-Math.abs((String.valueOf(feature) + id).hashCode()));
            return idh1.equals(idh2);
        }
        return false;
    }
}

