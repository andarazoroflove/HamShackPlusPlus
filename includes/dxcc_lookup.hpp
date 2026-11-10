#pragma once

#include <string>
#include <cctype>
#include <algorithm>
#include <unordered_map>

namespace hamshack {

inline std::string lookup_state_or_country(const std::string& call_in, const std::string& grid_in) {
    std::string call = call_in;
    while (!call.empty() && (call.back() == ' ' || call.back() == '/' || !std::isalnum(static_cast<unsigned char>(call.back())))) {
        if (!std::isalnum(static_cast<unsigned char>(call.back()))) call.pop_back();
        else break;
    }
    // Uppercase
    for (char& c : call) c = std::toupper(static_cast<unsigned char>(c));

    std::string grid = grid_in;
    for (char& c : grid) c = std::toupper(static_cast<unsigned char>(c));
    if (grid.size() > 4) grid = grid.substr(0, 4);

    // 1. Grid-based precise US State and Canadian Province lookup
    if (grid.size() == 4) {
        static const std::unordered_map<std::string, const char*> kGridToState = {
            // Pacific NW
            {"CN87", "WA, USA"}, {"CN88", "WA, USA"}, {"CN89", "BC, Canada"}, {"CN97", "WA, USA"},
            {"CN72", "OR, USA"}, {"CN82", "OR, USA"}, {"CN83", "OR, USA"}, {"CN84", "OR, USA"}, {"CN85", "OR, USA"},
            // California
            {"CM87", "CA, USA"}, {"CM88", "CA, USA"}, {"CM97", "CA, USA"}, {"CM98", "CA, USA"},
            {"DM04", "CA, USA"}, {"DM05", "CA, USA"}, {"DM06", "CA, USA"}, {"DM07", "CA, USA"},
            {"DM12", "CA, USA"}, {"DM13", "CA, USA"}, {"DM14", "CA, USA"}, {"DM15", "CA, USA"},
            // Mountain West
            {"DN13", "ID, USA"}, {"DN14", "ID, USA"}, {"DN15", "ID, USA"}, {"DN23", "ID, USA"}, {"DN24", "ID, USA"},
            {"DN30", "UT, USA"}, {"DN31", "UT, USA"}, {"DN40", "UT, USA"}, {"DN41", "UT, USA"},
            {"DM37", "UT, USA"}, {"DM47", "UT, USA"},
            {"DM09", "NV, USA"}, {"DM19", "NV, USA"}, {"DM29", "NV, USA"}, {"DN10", "NV, USA"}, {"DN20", "NV, USA"},
            {"DM22", "AZ, USA"}, {"DM23", "AZ, USA"}, {"DM32", "AZ, USA"}, {"DM33", "AZ, USA"}, {"DM34", "AZ, USA"}, {"DM43", "AZ, USA"},
            {"DN51", "WY, USA"}, {"DN52", "WY, USA"}, {"DN53", "WY, USA"}, {"DN61", "WY, USA"}, {"DN62", "WY, USA"}, {"DN63", "WY, USA"}, {"DN71", "WY, USA"}, {"DN72", "WY, USA"},
            {"DN25", "MT, USA"}, {"DN26", "MT, USA"}, {"DN35", "MT, USA"}, {"DN36", "MT, USA"}, {"DN45", "MT, USA"}, {"DN46", "MT, USA"},
            {"DN55", "MT, USA"}, {"DN56", "MT, USA"}, {"DN65", "MT, USA"}, {"DN66", "MT, USA"}, {"DN75", "MT, USA"}, {"DN76", "MT, USA"},
            {"DN60", "CO, USA"}, {"DN70", "CO, USA"}, {"DM68", "CO, USA"}, {"DM69", "CO, USA"}, {"DM78", "CO, USA"}, {"DM79", "CO, USA"},
            {"DM52", "NM, USA"}, {"DM53", "NM, USA"}, {"DM54", "NM, USA"}, {"DM55", "NM, USA"},
            {"DM63", "NM, USA"}, {"DM64", "NM, USA"}, {"DM65", "NM, USA"}, {"DM73", "NM, USA"}, {"DM74", "NM, USA"}, {"DM75", "NM, USA"},
            // Plains
            {"DN85", "ND, USA"}, {"DN86", "ND, USA"}, {"DN87", "ND, USA"}, {"DN88", "ND, USA"}, {"DN95", "ND, USA"}, {"DN96", "ND, USA"}, {"DN97", "ND, USA"}, {"DN98", "ND, USA"},
            {"DN83", "SD, USA"}, {"DN84", "SD, USA"}, {"DN93", "SD, USA"}, {"DN94", "SD, USA"}, {"EN03", "SD, USA"}, {"EN04", "SD, USA"}, {"EN13", "SD, USA"}, {"EN14", "SD, USA"},
            {"DN80", "NE, USA"}, {"DN81", "NE, USA"}, {"DN82", "NE, USA"}, {"DN90", "NE, USA"}, {"DN91", "NE, USA"}, {"DN92", "NE, USA"}, {"EN00", "NE, USA"}, {"EN01", "NE, USA"}, {"EN02", "NE, USA"}, {"EN10", "NE, USA"}, {"EN11", "NE, USA"},
            {"DM87", "KS, USA"}, {"DM88", "KS, USA"}, {"DM89", "KS, USA"}, {"DM97", "KS, USA"}, {"DM98", "KS, USA"}, {"DM99", "KS, USA"}, {"EM07", "KS, USA"}, {"EM08", "KS, USA"}, {"EM09", "KS, USA"}, {"EM17", "KS, USA"}, {"EM18", "KS, USA"}, {"EM19", "KS, USA"},
            {"DM84", "OK, USA"}, {"DM85", "OK, USA"}, {"DM86", "OK, USA"}, {"DM94", "OK, USA"}, {"DM95", "OK, USA"}, {"DM96", "OK, USA"}, {"EM04", "OK, USA"}, {"EM05", "OK, USA"}, {"EM06", "OK, USA"}, {"EM14", "OK, USA"}, {"EM15", "OK, USA"}, {"EM16", "OK, USA"},
            {"DM81", "TX, USA"}, {"DM82", "TX, USA"}, {"DM91", "TX, USA"}, {"DM92", "TX, USA"},
            {"EM00", "TX, USA"}, {"EM01", "TX, USA"}, {"EM02", "TX, USA"}, {"EM03", "TX, USA"},
            {"EM10", "TX, USA"}, {"EM11", "TX, USA"}, {"EM12", "TX, USA"}, {"EM13", "TX, USA"},
            {"EM20", "TX, USA"}, {"EM21", "TX, USA"}, {"EM22", "TX, USA"}, {"EM23", "TX, USA"},
            {"EL09", "TX, USA"}, {"EL19", "TX, USA"}, {"EL29", "TX, USA"}, {"EL96", "FL, USA"},
            // Midwest
            {"EN15", "MN, USA"}, {"EN16", "MN, USA"}, {"EN25", "MN, USA"}, {"EN26", "MN, USA"}, {"EN35", "MN, USA"}, {"EN36", "MN, USA"},
            {"EN12", "IA, USA"}, {"EN21", "IA, USA"}, {"EN22", "IA, USA"}, {"EN31", "IA, USA"}, {"EN32", "IA, USA"},
            {"EM27", "MO, USA"}, {"EM28", "MO, USA"}, {"EM29", "MO, USA"}, {"EM37", "MO, USA"}, {"EM38", "MO, USA"}, {"EM39", "MO, USA"}, {"EM48", "MO, USA"}, {"EM49", "MO, USA"},
            {"EM24", "AR, USA"}, {"EM25", "AR, USA"}, {"EM26", "AR, USA"}, {"EM34", "AR, USA"}, {"EM35", "AR, USA"}, {"EM36", "AR, USA"}, {"EM44", "AR, USA"}, {"EM45", "AR, USA"}, {"EM46", "AR, USA"},
            {"EM30", "LA, USA"}, {"EM31", "LA, USA"}, {"EM32", "LA, USA"}, {"EM40", "LA, USA"}, {"EM41", "LA, USA"}, {"EM42", "LA, USA"},
            {"EN42", "WI, USA"}, {"EN43", "WI, USA"}, {"EN44", "WI, USA"}, {"EN45", "WI, USA"}, {"EN52", "WI, USA"}, {"EN53", "WI, USA"}, {"EN54", "WI, USA"}, {"EN55", "WI, USA"}, {"EN64", "WI, USA"}, {"EN65", "WI, USA"},
            {"EM47", "IL, USA"}, {"EM48", "IL, USA"}, {"EM57", "IL, USA"}, {"EM58", "IL, USA"}, {"EN50", "IL, USA"}, {"EN51", "IL, USA"}, {"EN60", "IL, USA"}, {"EN61", "IL, USA"},
            {"EM50", "MS, USA"}, {"EM51", "MS, USA"}, {"EM52", "MS, USA"}, {"EM53", "MS, USA"}, {"EM54", "MS, USA"},
            {"EM55", "TN, USA"}, {"EM56", "TN, USA"}, {"EM65", "TN, USA"}, {"EM66", "TN, USA"}, {"EM75", "TN, USA"}, {"EM76", "TN, USA"},
            {"EM60", "AL, USA"}, {"EM61", "AL, USA"}, {"EM62", "AL, USA"}, {"EM63", "AL, USA"}, {"EM64", "AL, USA"},
            {"EM67", "KY, USA"}, {"EM68", "KY, USA"}, {"EM77", "KY, USA"}, {"EM78", "KY, USA"}, {"EM87", "KY, USA"}, {"EM88", "KY, USA"},
            {"EM69", "IN, USA"}, {"EM79", "IN, USA"}, {"EN61", "IN, USA"}, {"EN70", "IN, USA"}, {"EN71", "IN, USA"},
            {"EN62", "MI, USA"}, {"EN63", "MI, USA"}, {"EN72", "MI, USA"}, {"EN73", "MI, USA"}, {"EN74", "MI, USA"}, {"EN82", "MI, USA"}, {"EN83", "MI, USA"}, {"EN84", "MI, USA"}, {"EN85", "MI, USA"},
            {"EM89", "OH, USA"}, {"EN80", "OH, USA"}, {"EN81", "OH, USA"}, {"EN90", "OH, USA"}, {"EN91", "OH, USA"},
            // South & East Coast
            {"EM70", "GA, USA"}, {"EM71", "GA, USA"}, {"EM72", "GA, USA"}, {"EM73", "GA, USA"}, {"EM74", "GA, USA"}, {"EM81", "GA, USA"}, {"EM82", "GA, USA"}, {"EM83", "GA, USA"}, {"EM84", "GA, USA"},
            {"EM98", "WV, USA"}, {"EM99", "WV, USA"}, {"FM08", "WV, USA"}, {"FM09", "WV, USA"},
            {"EM85", "NC, USA"}, {"EM86", "NC, USA"}, {"EM95", "NC, USA"}, {"EM96", "NC, USA"}, {"FM05", "NC, USA"}, {"FM06", "NC, USA"}, {"FM15", "NC, USA"}, {"FM16", "NC, USA"},
            {"EM93", "SC, USA"}, {"EM94", "SC, USA"},
            {"EM97", "VA, USA"}, {"FM07", "VA, USA"}, {"FM17", "VA, USA"}, {"FM18", "VA, USA"}, {"FM28", "VA, USA"},
            {"EL79", "FL, USA"}, {"EL88", "FL, USA"}, {"EL89", "FL, USA"}, {"EL97", "FL, USA"}, {"EL98", "FL, USA"}, {"EL99", "FL, USA"}, {"EM80", "FL, USA"}, {"EM90", "FL, USA"},
            {"FM19", "MD/DC, USA"},
            {"FN00", "PA, USA"}, {"FN01", "PA, USA"}, {"FN10", "PA, USA"}, {"FN11", "PA, USA"}, {"FN20", "PA, USA"}, {"FN21", "PA, USA"},
            {"FN30", "NJ, USA"},
            {"FN02", "NY, USA"}, {"FN03", "NY, USA"}, {"FN12", "NY, USA"}, {"FN13", "NY, USA"}, {"FN22", "NY, USA"}, {"FN23", "NY, USA"}, {"FN32", "NY, USA"}, {"FN33", "NY, USA"},
            {"FN31", "CT, USA"}, {"FN41", "MA/RI, USA"}, {"FN42", "MA, USA"},
            {"FN34", "VT, USA"}, {"FN35", "VT, USA"},
            {"FN43", "NH, USA"}, {"FN44", "NH, USA"},
            {"FN45", "ME, USA"}, {"FN53", "ME, USA"}, {"FN54", "ME, USA"}, {"FN55", "ME, USA"}, {"FN56", "ME, USA"},
            {"BK29", "HI, USA"}, {"BL10", "HI, USA"}, {"BL11", "HI, USA"},
            {"FK68", "Puerto Rico"}
        };
        auto it = kGridToState.find(grid);
        if (it != kGridToState.end()) return it->second;
    }

    // 2. Specialized US & Canadian Call Prefix matching
    if (call.rfind("KL7", 0) == 0 || call.rfind("AL7", 0) == 0 || call.rfind("NL7", 0) == 0 || call.rfind("WL7", 0) == 0) return "AK, USA";
    if (call.rfind("KH6", 0) == 0 || call.rfind("AH6", 0) == 0 || call.rfind("NH6", 0) == 0 || call.rfind("WH6", 0) == 0) return "HI, USA";
    if (call.rfind("KP4", 0) == 0 || call.rfind("NP4", 0) == 0 || call.rfind("WP4", 0) == 0) return "Puerto Rico";
    if (call.rfind("KP2", 0) == 0 || call.rfind("NP2", 0) == 0 || call.rfind("WP2", 0) == 0) return "US Virgin Is.";

    if (call.rfind("VE1", 0) == 0 || call.rfind("VA1", 0) == 0) return "NS, Canada";
    if (call.rfind("VE2", 0) == 0 || call.rfind("VA2", 0) == 0) return "QC, Canada";
    if (call.rfind("VE3", 0) == 0 || call.rfind("VA3", 0) == 0) return "ON, Canada";
    if (call.rfind("VE4", 0) == 0 || call.rfind("VA4", 0) == 0) return "MB, Canada";
    if (call.rfind("VE5", 0) == 0 || call.rfind("VA5", 0) == 0) return "SK, Canada";
    if (call.rfind("VE6", 0) == 0 || call.rfind("VA6", 0) == 0) return "AB, Canada";
    if (call.rfind("VE7", 0) == 0 || call.rfind("VA7", 0) == 0) return "BC, Canada";
    if (call.rfind("VE8", 0) == 0 || call.rfind("VA8", 0) == 0) return "NWT, Canada";
    if (call.rfind("VE9", 0) == 0 || call.rfind("VA9", 0) == 0) return "NB, Canada";
    if (call.rfind("VO1", 0) == 0 || call.rfind("VO2", 0) == 0) return "NL, Canada";
    if (call.rfind("VY1", 0) == 0) return "Yukon, Canada";
    if (call.rfind("VY2", 0) == 0) return "PEI, Canada";
    if (call.rfind("VE", 0) == 0 || call.rfind("VA", 0) == 0 || call.rfind("VO", 0) == 0 || call.rfind("VY", 0) == 0) return "Canada";

    // 3. International DXCC Entity Lookup by Prefix
    if (call.rfind("JA", 0) == 0 || call.rfind("JH", 0) == 0 || call.rfind("JR", 0) == 0 ||
        call.rfind("JE", 0) == 0 || call.rfind("JF", 0) == 0 || call.rfind("JG", 0) == 0 ||
        call.rfind("JI", 0) == 0 || call.rfind("JJ", 0) == 0 || call.rfind("7J", 0) == 0 ||
        call.rfind("7K", 0) == 0 || call.rfind("7L", 0) == 0 || call.rfind("7M", 0) == 0 || call.rfind("7N", 0) == 0) return "Japan";

    if (call.rfind("GM", 0) == 0 || call.rfind("MM", 0) == 0 || call.rfind("2M", 0) == 0) return "Scotland";
    if (call.rfind("GW", 0) == 0 || call.rfind("MW", 0) == 0 || call.rfind("2W", 0) == 0) return "Wales";
    if (call.rfind("GI", 0) == 0 || call.rfind("MI", 0) == 0 || call.rfind("2I", 0) == 0) return "N. Ireland";
    if (call.rfind("G", 0) == 0 || call.rfind("M", 0) == 0 || call.rfind("2E", 0) == 0) return "England";

    if (call.rfind("DL", 0) == 0 || call.rfind("DK", 0) == 0 || call.rfind("DJ", 0) == 0 ||
        call.rfind("DF", 0) == 0 || call.rfind("DH", 0) == 0 || call.rfind("DA", 0) == 0 ||
        call.rfind("DB", 0) == 0 || call.rfind("DC", 0) == 0 || call.rfind("DD", 0) == 0 ||
        call.rfind("DG", 0) == 0 || call.rfind("DM", 0) == 0 || call.rfind("DO", 0) == 0 || call.rfind("DP", 0) == 0) return "Germany";

    if (call.rfind("F", 0) == 0 || call.rfind("TM", 0) == 0) return "France";
    if (call.rfind("I", 0) == 0 || call.rfind("IK", 0) == 0 || call.rfind("IZ", 0) == 0 ||
        call.rfind("IU", 0) == 0 || call.rfind("IT", 0) == 0 || call.rfind("IS", 0) == 0) return "Italy";

    if (call.rfind("EA8", 0) == 0) return "Canary Is.";
    if (call.rfind("EA9", 0) == 0) return "Ceuta/Melilla";
    if (call.rfind("EA", 0) == 0 || call.rfind("EB", 0) == 0 || call.rfind("EC", 0) == 0 || call.rfind("ED", 0) == 0) return "Spain";

    if (call.rfind("PY", 0) == 0 || call.rfind("PP", 0) == 0 || call.rfind("PR", 0) == 0 ||
        call.rfind("PU", 0) == 0 || call.rfind("PT", 0) == 0 || call.rfind("PV", 0) == 0) return "Brazil";
    if (call.rfind("LU", 0) == 0 || call.rfind("LW", 0) == 0 || call.rfind("AY", 0) == 0 || call.rfind("AZ", 0) == 0) return "Argentina";
    if (call.rfind("VK", 0) == 0) return "Australia";
    if (call.rfind("ZL", 0) == 0) return "New Zealand";
    if (call.rfind("ZS", 0) == 0 || call.rfind("ZR", 0) == 0 || call.rfind("ZT", 0) == 0) return "South Africa";

    if (call.rfind("UA", 0) == 0 || call.rfind("UB", 0) == 0 || call.rfind("UC", 0) == 0 ||
        call.rfind("UD", 0) == 0 || call.rfind("RA", 0) == 0 || call.rfind("RU", 0) == 0 ||
        call.rfind("RV", 0) == 0 || call.rfind("RW", 0) == 0 || call.rfind("RX", 0) == 0 || call.rfind("RZ", 0) == 0) return "Russia";

    if (call.rfind("SP", 0) == 0 || call.rfind("SQ", 0) == 0 || call.rfind("SN", 0) == 0 || call.rfind("SO", 0) == 0) return "Poland";
    if (call.rfind("OK", 0) == 0 || call.rfind("OL", 0) == 0) return "Czech Rep.";
    if (call.rfind("OM", 0) == 0) return "Slovakia";
    if (call.rfind("HA", 0) == 0 || call.rfind("HG", 0) == 0) return "Hungary";
    if (call.rfind("OE", 0) == 0) return "Austria";
    if (call.rfind("HB9", 0) == 0 || call.rfind("HB0", 0) == 0) return "Switzerland";
    if (call.rfind("SM", 0) == 0 || call.rfind("SA", 0) == 0 || call.rfind("SB", 0) == 0 || call.rfind("SK", 0) == 0) return "Sweden";
    if (call.rfind("OH0", 0) == 0) return "Aland Is.";
    if (call.rfind("OH", 0) == 0 || call.rfind("OF", 0) == 0 || call.rfind("OG", 0) == 0) return "Finland";
    if (call.rfind("LA", 0) == 0 || call.rfind("LB", 0) == 0 || call.rfind("LN", 0) == 0) return "Norway";
    if (call.rfind("OZ", 0) == 0 || call.rfind("OU", 0) == 0 || call.rfind("OV", 0) == 0 || call.rfind("5Q", 0) == 0) return "Denmark";
    if (call.rfind("ON", 0) == 0 || call.rfind("OO", 0) == 0 || call.rfind("OT", 0) == 0) return "Belgium";
    if (call.rfind("PA", 0) == 0 || call.rfind("PB", 0) == 0 || call.rfind("PD", 0) == 0 || call.rfind("PE", 0) == 0 || call.rfind("PI", 0) == 0) return "Netherlands";
    if (call.rfind("CU", 0) == 0) return "Azores";
    if (call.rfind("CT", 0) == 0 || call.rfind("CS", 0) == 0 || call.rfind("CR", 0) == 0) return "Portugal";
    if (call.rfind("SV", 0) == 0 || call.rfind("SX", 0) == 0 || call.rfind("SY", 0) == 0) return "Greece";
    if (call.rfind("UR", 0) == 0 || call.rfind("US", 0) == 0 || call.rfind("UT", 0) == 0 || call.rfind("UX", 0) == 0) return "Ukraine";
    if (call.rfind("VU", 0) == 0 || call.rfind("AT", 0) == 0) return "India";
    if (call.rfind("HL", 0) == 0 || call.rfind("DS", 0) == 0 || call.rfind("DT", 0) == 0 || call.rfind("6K", 0) == 0) return "South Korea";
    if (call.rfind("BV", 0) == 0 || call.rfind("BX", 0) == 0) return "Taiwan";
    if (call.rfind("BY", 0) == 0 || call.rfind("BA", 0) == 0 || call.rfind("BD", 0) == 0 || call.rfind("BG", 0) == 0 || call.rfind("BH", 0) == 0) return "China";
    if (call.rfind("HS", 0) == 0 || call.rfind("E2", 0) == 0) return "Thailand";
    if (call.rfind("YB", 0) == 0 || call.rfind("YC", 0) == 0 || call.rfind("YD", 0) == 0) return "Indonesia";
    if (call.rfind("9V", 0) == 0) return "Singapore";
    if (call.rfind("XE", 0) == 0 || call.rfind("XF", 0) == 0 || call.rfind("4A", 0) == 0) return "Mexico";
    if (call.rfind("ZF", 0) == 0) return "Cayman Is.";
    if (call.rfind("TI", 0) == 0) return "Costa Rica";
    if (call.rfind("HP", 0) == 0) return "Panama";
    if (call.rfind("YV", 0) == 0) return "Venezuela";
    if (call.rfind("HK", 0) == 0) return "Colombia";
    if (call.rfind("HC", 0) == 0) return "Ecuador";
    if (call.rfind("OA", 0) == 0) return "Peru";
    if (call.rfind("CE", 0) == 0) return "Chile";
    if (call.rfind("EI", 0) == 0 || call.rfind("EJ", 0) == 0) return "Ireland";
    if (call.rfind("4X", 0) == 0 || call.rfind("4Z", 0) == 0) return "Israel";

    // 4. US Call District Fallback
    if (!call.empty() && (call[0] == 'K' || call[0] == 'W' || call[0] == 'N' || (call.size() >= 2 && call[0] == 'A' && call[1] >= 'A' && call[1] <= 'L'))) {
        for (char c : call) {
            if (c >= '0' && c <= '9') {
                switch (c) {
                    case '1': return "New England, USA";
                    case '2': return "NY/NJ, USA";
                    case '3': return "PA/MD/DE, USA";
                    case '4': return "SE USA";
                    case '5': return "TX/Gulf, USA";
                    case '6': return "CA, USA";
                    case '7': return "West USA";
                    case '8': return "MI/OH, USA";
                    case '9': return "Midwest, USA";
                    case '0': return "Plains, USA";
                }
            }
        }
        return "USA";
    }

    return "--";
}

} // namespace hamshack
