// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2009 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

#if XMP_MacBuild
	#include <CoreServices/CoreServices.h>
#else
	#include "XMPFiles/source/FormatSupport/MacScriptExtracts.h"
#endif

#include "XMPFiles/source/FormatSupport/QuickTime_Support.hpp"

#include "source/UnicodeConversions.hpp"
#include "source/UnicodeInlines.incl_cpp"
#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"
#include "source/XIO.hpp"
#include "source/EndianUtils.hpp"

// =================================================================================================

static const char * kMacRomanUTF8 [128] = {	// UTF-8 mappings for MacRoman 80..FF.
	"\xC3\x84", "\xC3\x85", "\xC3\x87", "\xC3\x89", "\xC3\x91", "\xC3\x96", "\xC3\x9C", "\xC3\xA1",
	"\xC3\xA0", "\xC3\xA2", "\xC3\xA4", "\xC3\xA3", "\xC3\xA5", "\xC3\xA7", "\xC3\xA9", "\xC3\xA8",
	"\xC3\xAA", "\xC3\xAB", "\xC3\xAD", "\xC3\xAC", "\xC3\xAE", "\xC3\xAF", "\xC3\xB1", "\xC3\xB3",
	"\xC3\xB2", "\xC3\xB4", "\xC3\xB6", "\xC3\xB5", "\xC3\xBA", "\xC3\xB9", "\xC3\xBB", "\xC3\xBC",
	"\xE2\x80\xA0", "\xC2\xB0", "\xC2\xA2", "\xC2\xA3", "\xC2\xA7", "\xE2\x80\xA2", "\xC2\xB6", "\xC3\x9F",
	"\xC2\xAE", "\xC2\xA9", "\xE2\x84\xA2", "\xC2\xB4", "\xC2\xA8", "\xE2\x89\xA0", "\xC3\x86", "\xC3\x98",
	"\xE2\x88\x9E", "\xC2\xB1", "\xE2\x89\xA4", "\xE2\x89\xA5", "\xC2\xA5", "\xC2\xB5", "\xE2\x88\x82", "\xE2\x88\x91",
	"\xE2\x88\x8F", "\xCF\x80", "\xE2\x88\xAB", "\xC2\xAA", "\xC2\xBA", "\xCE\xA9", "\xC3\xA6", "\xC3\xB8",
	"\xC2\xBF", "\xC2\xA1", "\xC2\xAC", "\xE2\x88\x9A", "\xC6\x92", "\xE2\x89\x88", "\xE2\x88\x86", "\xC2\xAB",
	"\xC2\xBB", "\xE2\x80\xA6", "\xC2\xA0", "\xC3\x80", "\xC3\x83", "\xC3\x95", "\xC5\x92", "\xC5\x93",
	"\xE2\x80\x93", "\xE2\x80\x94", "\xE2\x80\x9C", "\xE2\x80\x9D", "\xE2\x80\x98", "\xE2\x80\x99", "\xC3\xB7", "\xE2\x97\x8A",
	"\xC3\xBF", "\xC5\xB8", "\xE2\x81\x84", "\xE2\x82\xAC", "\xE2\x80\xB9", "\xE2\x80\xBA", "\xEF\xAC\x81", "\xEF\xAC\x82",
	"\xE2\x80\xA1", "\xC2\xB7", "\xE2\x80\x9A", "\xE2\x80\x9E", "\xE2\x80\xB0", "\xC3\x82", "\xC3\x8A", "\xC3\x81",
	"\xC3\x8B", "\xC3\x88", "\xC3\x8D", "\xC3\x8E", "\xC3\x8F", "\xC3\x8C", "\xC3\x93", "\xC3\x94",
	"\xEF\xA3\xBF", "\xC3\x92", "\xC3\x9A", "\xC3\x9B", "\xC3\x99", "\xC4\xB1", "\xCB\x86", "\xCB\x9C",
	"\xC2\xAF", "\xCB\x98", "\xCB\x99", "\xCB\x9A", "\xC2\xB8", "\xCB\x9D", "\xCB\x9B", "\xCB\x87"
};

static const XMP_Uns32 kMacRomanCP [128] = {	// Unicode codepoints for MacRoman 80..FF.
	0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1,
	0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8,
	0x00EA, 0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3,
	0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC,
	0x2020, 0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF,
	0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8,
	0x221E, 0x00B1, 0x2264, 0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211,
	0x220F, 0x03C0, 0x222B, 0x00AA, 0x00BA, 0x03A9, 0x00E6, 0x00F8,
	0x00BF, 0x00A1, 0x00AC, 0x221A, 0x0192, 0x2248, 0x2206, 0x00AB,
	0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3, 0x00D5, 0x0152, 0x0153,
	0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA,
	0x00FF, 0x0178, 0x2044, 0x20AC, 0x2039, 0x203A, 0xFB01, 0xFB02,
	0x2021, 0x00B7, 0x201A, 0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1,
	0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4,
	0xF8FF, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC,	// ! U+F8FF is private use solid Apple icon.
	0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7
};

// -------------------------------------------------------------------------------------------------

static const XMP_Uns16 kMacLangToScript_0_94 [95] = {

	/* langEnglish (0) */		smRoman,
	/* langFrench (1) */		smRoman,
	/* langGerman (2) */		smRoman,
	/* langItalian (3) */		smRoman,
	/* langDutch (4) */			smRoman,
	/* langSwedish (5) */		smRoman,
	/* langSpanish (6) */		smRoman,
	/* langDanish (7) */		smRoman,
	/* langPortuguese (8) */	smRoman,
	/* langNorwegian (9) */		smRoman,

	/* langHebrew (10) */		smHebrew,
	/* langJapanese (11) */		smJapanese,
	/* langArabic (12) */		smArabic,
	/* langFinnish (13) */		smRoman,
	/* langGreek (14) */		smRoman,
	/* langIcelandic (15) */	smRoman,
	/* langMaltese (16) */		smRoman,
	/* langTurkish (17) */		smRoman,
	/* langCroatian (18) */		smRoman,
	/* langTradChinese (19) */	smTradChinese,

	/* langUrdu (20) */			smArabic,
	/* langHindi (21) */		smDevanagari,
	/* langThai (22) */			smThai,
	/* langKorean (23) */		smKorean,
	/* langLithuanian (24) */	smCentralEuroRoman,
	/* langPolish (25) */		smCentralEuroRoman,
	/* langHungarian (26) */	smCentralEuroRoman,
	/* langEstonian (27) */		smCentralEuroRoman,
	/* langLatvian (28) */		smCentralEuroRoman,
	/* langSami (29) */			kNoMacScript,	// ! Not known, missing from Apple comments.

	/* langFaroese (30) */		smRoman,
	/* langFarsi (31) */		smArabic,
	/* langRussian (32) */		smCyrillic,
	/* langSimpChinese (33) */	smSimpChinese,
	/* langFlemish (34) */		smRoman,
	/* langIrishGaelic (35) */	smRoman,
	/* langAlbanian (36) */		smRoman,
	/* langRomanian (37) */		smRoman,
	/* langCzech (38) */		smCentralEuroRoman,
	/* langSlovak (39) */		smCentralEuroRoman,

	/* langSlovenian (40) */	smRoman,
	/* langYiddish (41) */		smHebrew,
	/* langSerbian (42) */		smCyrillic,
	/* langMacedonian (43) */	smCyrillic,
	/* langBulgarian (44) */	smCyrillic,
	/* langUkrainian (45) */	smCyrillic,
	/* langBelorussian (46) */	smCyrillic,
	/* langUzbek (47) */		smCyrillic,
	/* langKazakh (48) */		smCyrillic,
	/* langAzerbaijani (49) */	smCyrillic,

	/* langAzerbaijanAr (50) */	smArabic,
	/* langArmenian (51) */		smArmenian,
	/* langGeorgian (52) */		smGeorgian,
	/* langMoldavian (53) */	smCyrillic,
	/* langKirghiz (54) */		smCyrillic,
	/* langTajiki (55) */		smCyrillic,
	/* langTurkmen (56) */		smCyrillic,
	/* langMongolian (57) */	smMongolian,
	/* langMongolianCyr (58) */	smCyrillic,
	/* langPashto (59) */		smArabic,

	/* langKurdish (60) */		smArabic,
	/* langKashmiri (61) */		smArabic,
	/* langSindhi (62) */		smArabic,
	/* langTibetan (63) */		smTibetan,
	/* langNepali (64) */		smDevanagari,
	/* langSanskrit (65) */		smDevanagari,
	/* langMarathi (66) */		smDevanagari,
	/* langBengali (67) */		smBengali,
	/* langAssamese (68) */		smBengali,
	/* langGujarati (69) */		smGujarati,

	/* langPunjabi (70) */		smGurmukhi,
	/* langOriya (71) */		smOriya,
	/* langMalayalam (72) */	smMalayalam,
	/* langKannada (73) */		smKannada,
	/* langTamil (74) */		smTamil,
	/* langTelugu (75) */		smTelugu,
	/* langSinhalese (76) */	smSinhalese,
	/* langBurmese (77) */		smBurmese,
	/* langKhmer (78) */		smKhmer,
	/* langLao (79) */			smLao,

	/* langVietnamese (80) */	smVietnamese,
	/* langIndonesian (81) */	smRoman,
	/* langTagalog (82) */		smRoman,
	/* langMalayRoman (83) */	smRoman,
	/* langMalayArabic (84) */	smArabic,
	/* langAmharic (85) */		smEthiopic,
	/* langTigrinya (86) */		smEthiopic,
	/* langOromo (87) */		smEthiopic,
	/* langSomali (88) */		smRoman,
	/* langSwahili (89) */		smRoman,

	/* langKinyarwanda (90) */	smRoman,
	/* langRundi (91) */		smRoman,
	/* langNyanja (92) */		smRoman,
	/* langMalagasy (93) */		smRoman,
	/* langEsperanto (94) */	smRoman

};	// kMacLangToScript_0_94

static const XMP_Uns16 kMacLangToScript_128_151 [24] = {

	/* langWelsh (128) */				smRoman,
	/* langBasque (129) */				smRoman,

	/* langCatalan (130) */				smRoman,
	/* langLatin (131) */				smRoman,
	/* langQuechua (132) */				smRoman,
	/* langGuarani (133) */				smRoman,
	/* langAymara (134) */				smRoman,
	/* langTatar (135) */				smCyrillic,
	/* langUighur (136) */				smArabic,
	/* langDzongkha (137) */			smTibetan,
	/* langJavaneseRom (138) */			smRoman,
	/* langSundaneseRom (139) */		smRoman,

	/* langGalician (140) */			smRoman,
	/* langAfrikaans (141) */			smRoman,
	/* langBreton (142) */				smRoman,
	/* langInuktitut (143) */			smEthiopic,
	/* langScottishGaelic (144) */		smRoman,
	/* langManxGaelic (145) */			smRoman,
	/* langIrishGaelicScript (146) */	smRoman,
	/* langTongan (147) */				smRoman,
	/* langGreekAncient (148) */		smGreek,
	/* langGreenlandic (149) */			smRoman,

	/* langAzerbaijanRoman (150) */		smRoman,
	/* langNynorsk (151) */				smRoman
	
};	// kMacLangToScript_128_151

// -------------------------------------------------------------------------------------------------

static const char * kMacToXMPLang_0_94 [95] = {

	/* langEnglish (0) */		"en",
	/* langFrench (1) */		"fr",
	/* langGerman (2) */		"de",
	/* langItalian (3) */		"it",
	/* langDutch (4) */			"nl",
	/* langSwedish (5) */		"sv",
	/* langSpanish (6) */		"es",
	/* langDanish (7) */		"da",
	/* langPortuguese (8) */	"pt",
	/* langNorwegian (9) */		"no",

	/* langHebrew (10) */		"he",
	/* langJapanese (11) */		"ja",
	/* langArabic (12) */		"ar",
	/* langFinnish (13) */		"fi",
	/* langGreek (14) */		"el",
	/* langIcelandic (15) */	"is",
	/* langMaltese (16) */		"mt",
	/* langTurkish (17) */		"tr",
	/* langCroatian (18) */		"hr",
	/* langTradChinese (19) */	"zh",

	/* langUrdu (20) */			"ur",
	/* langHindi (21) */		"hi",
	/* langThai (22) */			"th",
	/* langKorean (23) */		"ko",
	/* langLithuanian (24) */	"lt",
	/* langPolish (25) */		"pl",
	/* langHungarian (26) */	"hu",
	/* langEstonian (27) */		"et",
	/* langLatvian (28) */		"lv",
	/* langSami (29) */			"se",

	/* langFaroese (30) */		"fo",
	/* langFarsi (31) */		"fa",
	/* langRussian (32) */		"ru",
	/* langSimpChinese (33) */	"zh",
	/* langFlemish (34) */		"nl",
	/* langIrishGaelic (35) */	"ga",
	/* langAlbanian (36) */		"sq",
	/* langRomanian (37) */		"ro",
	/* langCzech (38) */		"cs",
	/* langSlovak (39) */		"sk",

	/* langSlovenian (40) */	"sl",
	/* langYiddish (41) */		"yi",
	/* langSerbian (42) */		"sr",
	/* langMacedonian (43) */	"mk",
	/* langBulgarian (44) */	"bg",
	/* langUkrainian (45) */	"uk",
	/* langBelorussian (46) */	"be",
	/* langUzbek (47) */		"uz",
	/* langKazakh (48) */		"kk",
	/* langAzerbaijani (49) */	"az",

	/* langAzerbaijanAr (50) */	"az",
	/* langArmenian (51) */		"hy",
	/* langGeorgian (52) */		"ka",
	/* langMoldavian (53) */	"ro",
	/* langKirghiz (54) */		"ky",
	/* langTajiki (55) */		"tg",
	/* langTurkmen (56) */		"tk",
	/* langMongolian (57) */	"mn",
	/* langMongolianCyr (58) */	"mn",
	/* langPashto (59) */		"ps",

	/* langKurdish (60) */		"ku",
	/* langKashmiri (61) */		"ks",
	/* langSindhi (62) */		"sd",
	/* langTibetan (63) */		"bo",
	/* langNepali (64) */		"ne",
	/* langSanskrit (65) */		"sa",
	/* langMarathi (66) */		"mr",
	/* langBengali (67) */		"bn",
	/* langAssamese (68) */		"as",
	/* langGujarati (69) */		"gu",

	/* langPunjabi (70) */		"pa",
	/* langOriya (71) */		"or",
	/* langMalayalam (72) */	"ml",
	/* langKannada (73) */		"kn",
	/* langTamil (74) */		"ta",
	/* langTelugu (75) */		"te",
	/* langSinhalese (76) */	"si",
	/* langBurmese (77) */		"my",
	/* langKhmer (78) */		"km",
	/* langLao (79) */			"lo",

	/* langVietnamese (80) */	"vi",
	/* langIndonesian (81) */	"id",
	/* langTagalog (82) */		"tl",
	/* langMalayRoman (83) */	"ms",
	/* langMalayArabic (84) */	"ms",
	/* langAmharic (85) */		"am",
	/* langTigrinya (86) */		"ti",
	/* langOromo (87) */		"om",
	/* langSomali (88) */		"so",
	/* langSwahili (89) */		"sw",

	/* langKinyarwanda (90) */	"rw",
	/* langRundi (91) */		"rn",
	/* langNyanja (92) */		"ny",
	/* langMalagasy (93) */		"mg",
	/* langEsperanto (94) */	"eo"

};	// kMacToXMPLang_0_94

static const char * kMacToXMPLang_128_151 [24] = {

	/* langWelsh (128) */				"cy",
	/* langBasque (129) */				"eu",

	/* langCatalan (130) */				"ca",
	/* langLatin (131) */				"la",
	/* langQuechua (132) */				"qu",
	/* langGuarani (133) */				"gn",
	/* langAymara (134) */				"ay",
	/* langTatar (135) */				"tt",
	/* langUighur (136) */				"ug",
	/* langDzongkha (137) */			"dz",
	/* langJavaneseRom (138) */			"jv",
	/* langSundaneseRom (139) */		"su",

	/* langGalician (140) */			"gl",
	/* langAfrikaans (141) */			"af",
	/* langBreton (142) */				"br",
	/* langInuktitut (143) */			"iu",
	/* langScottishGaelic (144) */		"gd",
	/* langManxGaelic (145) */			"gv",
	/* langIrishGaelicScript (146) */	"ga",
	/* langTongan (147) */				"to",
	/* langGreekAncient (148) */		"",		// ! Has no ISO 639-1 2 letter code.
	/* langGreenlandic (149) */			"kl",

	/* langAzerbaijanRoman (150) */		"az",
	/* langNynorsk (151) */				"nn"
	
};	// kMacToXMPLang_128_151

// -------------------------------------------------------------------------------------------------

#if XMP_WinBuild

static UINT kMacScriptToWinCP[34] = {
	/* smRoman (0) */				10000,	// There don't seem to be symbolic constants.
	/* smJapanese (1) */			10001,	// From http://msdn.microsoft.com/en-us/library/dd317756(VS.85).aspx
	/* smTradChinese (2) */			10002,
	/* smKorean (3) */				10003,
	/* smArabic (4) */				10004,
	/* smHebrew (5) */				10005,
	/* smGreek (6) */				10006,
	/* smCyrillic (7) */			10007,
	/* smRSymbol (8) */				0,
	/* smDevanagari (9) */			0,
	/* smGurmukhi (10) */			0,
	/* smGujarati (11) */			0,
	/* smOriya (12) */				0,
	/* smBengali (13) */			0,
	/* smTamil (14) */				0,
	/* smTelugu (15) */				0,
	/* smKannada (16) */			0,
	/* smMalayalam (17) */			0,
	/* smSinhalese (18) */			0,
	/* smBurmese (19) */			0,
	/* smKhmer (20) */				0,
	/* smThai (21) */				10021,
	/* smLao (22) */				0,
	/* smGeorgian (23) */			0,
	/* smArmenian (24) */			0,
	/* smSimpChinese (25) */		10008,
	/* smTibetan (26) */			0,
	/* smMongolian (27) */			0,
	/* smEthiopic (28) */			0,
	/* smGeez (28) */				0,
	/* smCentralEuroRoman (29) */	10029,
	/* smVietnamese (30) */			0,
	/* smExtArabic (31) */			0,
	/* smUninterp (32) */			0
};	// kMacScriptToWinCP

static UINT kMacToWinCP_0_94 [95] = {

	/* langEnglish (0) */		0,
	/* langFrench (1) */		0,
	/* langGerman (2) */		0,
	/* langItalian (3) */		0,
	/* langDutch (4) */			0,
	/* langSwedish (5) */		0,
	/* langSpanish (6) */		0,
	/* langDanish (7) */		0,
	/* langPortuguese (8) */	0,
	/* langNorwegian (9) */		0,

	/* langHebrew (10) */		10005,
	/* langJapanese (11) */		10001,
	/* langArabic (12) */		10004,
	/* langFinnish (13) */		0,
	/* langGreek (14) */		10006,
	/* langIcelandic (15) */	10079,
	/* langMaltese (16) */		0,
	/* langTurkish (17) */		10081,
	/* langCroatian (18) */		10082,
	/* langTradChinese (19) */	10002,

	/* langUrdu (20) */			0,
	/* langHindi (21) */		0,
	/* langThai (22) */			10021,
	/* langKorean (23) */		10003,
	/* langLithuanian (24) */	0,
	/* langPolish (25) */		0,
	/* langHungarian (26) */	0,
	/* langEstonian (27) */		0,
	/* langLatvian (28) */		0,
	/* langSami (29) */			0,

	/* langFaroese (30) */		0,
	/* langFarsi (31) */		0,
	/* langRussian (32) */		0,
	/* langSimpChinese (33) */	10008,
	/* langFlemish (34) */		0,
	/* langIrishGaelic (35) */	0,
	/* langAlbanian (36) */		0,
	/* langRomanian (37) */		10010,
	/* langCzech (38) */		0,
	/* langSlovak (39) */		0,

	/* langSlovenian (40) */	0,
	/* langYiddish (41) */		0,
	/* langSerbian (42) */		0,
	/* langMacedonian (43) */	0,
	/* langBulgarian (44) */	0,
	/* langUkrainian (45) */	10017,
	/* langBelorussian (46) */	0,
	/* langUzbek (47) */		0,
	/* langKazakh (48) */		0,
	/* langAzerbaijani (49) */	0,

	/* langAzerbaijanAr (50) */	0,
	/* langArmenian (51) */		0,
	/* langGeorgian (52) */		0,
	/* langMoldavian (53) */	0,
	/* langKirghiz (54) */		0,
	/* langTajiki (55) */		0,
	/* langTurkmen (56) */		0,
	/* langMongolian (57) */	0,
	/* langMongolianCyr (58) */	0,
	/* langPashto (59) */		0,

	/* langKurdish (60) */		0,
	/* langKashmiri (61) */		0,
	/* langSindhi (62) */		0,
	/* langTibetan (63) */		0,
	/* langNepali (64) */		0,
	/* langSanskrit (65) */		0,
	/* langMarathi (66) */		0,
	/* langBengali (67) */		0,
	/* langAssamese (68) */		0,
	/* langGujarati (69) */		0,

	/* langPunjabi (70) */		0,
	/* langOriya (71) */		0,
	/* langMalayalam (72) */	0,
	/* langKannada (73) */		0,
	/* langTamil (74) */		0,
	/* langTelugu (75) */		0,
	/* langSinhalese (76) */	0,
	/* langBurmese (77) */		0,
	/* langKhmer (78) */		0,
	/* langLao (79) */			0,

	/* langVietnamese (80) */	0,
	/* langIndonesian (81) */	0,
	/* langTagalog (82) */		0,
	/* langMalayRoman (83) */	0,
	/* langMalayArabic (84) */	0,
	/* langAmharic (85) */		0,
	/* langTigrinya (86) */		0,
	/* langOromo (87) */		0,
	/* langSomali (88) */		0,
	/* langSwahili (89) */		0,

	/* langKinyarwanda (90) */	0,
	/* langRundi (91) */		0,
	/* langNyanja (92) */		0,
	/* langMalagasy (93) */		0,
	/* langEsperanto (94) */	0

};	// kMacToWinCP_0_94

#endif

// =================================================================================================
// GetMacScript
// ============

static XMP_Uns16 GetMacScript ( XMP_Uns16 macLang )
{
	XMP_Uns16 macScript = kNoMacScript;
	
	if ( macLang <= 94 ) {
		macScript = kMacLangToScript_0_94[macLang];
	} else if ( (128 <= macLang) && (macLang <= 151) ) {
		macScript = kMacLangToScript_0_94[macLang-128];
	}
	
	return macScript;
	
}	// GetMacScript

// =================================================================================================
// GetWinCP
// ========

#if XMP_WinBuild

static UINT GetWinCP ( XMP_Uns16 macLang )
{
	UINT winCP = 0;
	
	if ( macLang <= 94 ) winCP = kMacToWinCP_0_94[macLang];
	
	if ( winCP == 0 ) {
		XMP_Uns16 macScript = GetMacScript ( macLang );
		if ( macScript != kNoMacScript ) winCP = kMacScriptToWinCP[macScript];
	}
	
	return winCP;
	
}	// GetWinCP

#endif

// =================================================================================================
// GetXMPLang
// ==========

static XMP_StringPtr GetXMPLang ( XMP_Uns16 macLang )
{
	XMP_StringPtr xmpLang = "";
	
	if ( macLang <= 94 ) {
		xmpLang = kMacToXMPLang_0_94[macLang];
	} else if ( (128 <= macLang) && (macLang <= 151) ) {
		xmpLang = kMacToXMPLang_128_151[macLang-128];
	}
	
	return xmpLang;
	
}	// GetXMPLang

// =================================================================================================
// GetMacLang
// ==========

static XMP_Uns16 GetMacLang ( std::string * xmpLang )
{
	if ( *xmpLang == "" ) return kNoMacLang;
	
	size_t hyphenPos = xmpLang->find ( '-' );	// Make sure the XMP language is "generic".
	if ( hyphenPos != std::string::npos ) xmpLang->erase ( hyphenPos );
	
	for ( XMP_Uns16 i = 0; i <= 94; ++i ) {	// Using std::map would be faster.
		if ( *xmpLang == kMacToXMPLang_0_94[i] ) return i;
	}
	
	for ( XMP_Uns16 i = 128; i <= 151; ++i ) {	// Using std::map would be faster.
		if ( *xmpLang == kMacToXMPLang_128_151[i-128] ) return i;
	}
	
	return kNoMacLang;
	
}	// GetMacLang

// =================================================================================================
// MacRomanToUTF8
// ==============

static void MacRomanToUTF8 ( const std::string & macRoman, std::string * utf8 )
{
	utf8->erase();
	
	for ( XMP_Uns8* chPtr = (XMP_Uns8*)macRoman.c_str(); *chPtr != 0; ++chPtr ) {	// ! Don't trust that char is unsigned.
		if ( *chPtr < 0x80 ) {
			(*utf8) += (char)*chPtr;
		} else {
			(*utf8) += kMacRomanUTF8[(*chPtr)-0x80];
		}
	}

}	// MacRomanToUTF8

// =================================================================================================
// UTF8ToMacRoman
// ==============

static void UTF8ToMacRoman ( const std::string & utf8, std::string * macRoman )
{
	macRoman->erase();
	bool inNonMRSpan = false;
	
	for ( const XMP_Uns8 * chPtr = (XMP_Uns8*)utf8.c_str(); *chPtr != 0; ++chPtr ) {	// ! Don't trust that char is unsigned.
		if ( *chPtr < 0x80 ) {
			(*macRoman) += (char)*chPtr;
			inNonMRSpan = false;
		} else {
			XMP_Uns32 cp = GetCodePoint ( &chPtr );
			--chPtr;	// Make room for the loop increment.
			XMP_Uns8  mr;
			for ( mr = 0; (mr < 0x80) && (cp != kMacRomanCP[mr]); ++mr ) {};	// Using std::map would be faster.
			if ( mr < 0x80 ) {
				(*macRoman) += (char)(mr+0x80);
				inNonMRSpan = false;
			} else if ( ! inNonMRSpan ) {
				(*macRoman) += '?';
				inNonMRSpan = true;
			}
		}
	}

}	// UTF8ToMacRoman

// =================================================================================================
// IsMacLangKnown
// ==============

static inline bool IsMacLangKnown ( XMP_Uns16 macLang )
{
	XMP_Uns16 macScript = GetMacScript ( macLang );
	if ( macScript == kNoMacScript ) return false;

	#if XMP_UNIXBuild
		if ( macScript != smRoman ) return false;
	#elif XMP_WinBuild
		if ( GetWinCP(macLang) == 0 ) return false;
	#endif
	
	return true;

}	// IsMacLangKnown

// =================================================================================================
// ConvertToMacLang
// ================

bool ConvertToMacLang ( const std::string & utf8Value, XMP_Uns16 macLang, std::string * macValue )
{
	macValue->erase();
	if ( macLang == kNoMacLang ) macLang = 0;	// *** Zero is English, ought to use the "active" OS lang.
	if ( ! IsMacLangKnown ( macLang ) ) return false;

	#if XMP_MacBuild
		XMP_Uns16 macScript = GetMacScript ( macLang );
		ReconcileUtils::UTF8ToMacEncoding ( macScript, macLang, (XMP_Uns8*)utf8Value.c_str(), utf8Value.size(), macValue );
	#elif XMP_UNIXBuild
		UTF8ToMacRoman ( utf8Value, macValue );
	#elif XMP_WinBuild
		UINT winCP = GetWinCP ( macLang );
		ReconcileUtils::UTF8ToWinEncoding ( winCP, (XMP_Uns8*)utf8Value.c_str(), utf8Value.size(), macValue );
	#endif
	
	return true;

}	// ConvertToMacLang

// =================================================================================================
// ConvertFromMacLang
// ==================

bool ConvertFromMacLang ( const std::string & macValue, XMP_Uns16 macLang, std::string * utf8Value )
{
	utf8Value->erase();
	if ( ! IsMacLangKnown ( macLang ) ) return false;
	
	#if XMP_MacBuild
		XMP_Uns16 macScript = GetMacScript ( macLang );
		ReconcileUtils::MacEncodingToUTF8 ( macScript, macLang, (XMP_Uns8*)macValue.c_str(), macValue.size(), utf8Value );
	#elif XMP_UNIXBuild
		MacRomanToUTF8 ( macValue, utf8Value );
	#elif XMP_WinBuild
		UINT winCP = GetWinCP ( macLang );
		ReconcileUtils::WinEncodingToUTF8 ( winCP, (XMP_Uns8*)macValue.c_str(), macValue.size(), utf8Value );
	#endif
	
	return true;

}	// ConvertFromMacLang

// =================================================================================================
// =================================================================================================
// TradQT_Manager
// =================================================================================================
// =================================================================================================

// =================================================================================================
// TradQT_Manager::ParseCachedBoxes
// ================================
//
// Parse the cached '©...' children of the 'moov'/'udta' box. The contents of each cached box are
// a sequence of "mini boxes" analogous to XMP AltText arrays. Each mini box has a 16-bit size,
// 16-bit language code, and text. The size is only the text size. The language codes are Macintosh
// Script Manager langXyz codes. The text encoding is implicit in the language, see comments in
// Apple's Script.h header.

bool TradQT_Manager::ParseCachedBoxes ( const MOOV_Manager & moovMgr )
{
	MOOV_Manager::BoxInfo udtaInfo;
	MOOV_Manager::BoxRef  udtaRef = moovMgr.GetBox ( "moov/udta", &udtaInfo );
	if ( udtaRef == 0 ) return false;

	for ( XMP_Uns32 i = 0; i < udtaInfo.childCount; ++i ) {

		MOOV_Manager::BoxInfo currInfo;
		MOOV_Manager::BoxRef  currRef = moovMgr.GetNthChild ( udtaRef, i, &currInfo );
		if ( currRef == 0 ) break;	// Sanity check, should not happen.
		if ( (currInfo.boxType >> 24) != 0xA9 ) continue;
		if ( currInfo.contentSize < 2+2+1 ) continue;	// Want enough for a non-empty value.
		
		InfoMapPos newInfo = this->parsedBoxes.insert ( this->parsedBoxes.end(),
														InfoMap::value_type ( currInfo.boxType, ParsedBoxInfo ( currInfo.boxType ) ) );
		std::vector<ValueInfo> * newValues = &newInfo->second.values;
		
		XMP_Uns8 * boxPtr = (XMP_Uns8*) currInfo.content;
		XMP_Uns8 * boxEnd = boxPtr + currInfo.contentSize;
		XMP_Uns16 miniLen, macLang;

		for ( ; boxPtr < boxEnd-4; boxPtr += miniLen ) {

			miniLen = 4 + GetUns16BE ( boxPtr );	// ! Include header in local miniLen.
			macLang  = GetUns16BE ( boxPtr+2);
			if ( (miniLen <= 4) || (miniLen > (boxEnd - boxPtr)) ) continue;	// Ignore bad or empty values.
			
			XMP_StringPtr valuePtr = (char*)(boxPtr+4);
			size_t valueLen = miniLen - 4;

			newValues->push_back ( ValueInfo() );
			ValueInfo * newValue = &newValues->back();
			
			// Only set the XMP language if the Mac script is known, i.e. the value can be converted.
			
			newValue->macLang = macLang;
			if ( IsMacLangKnown ( macLang ) ) newValue->xmpLang = GetXMPLang ( macLang );
			newValue->macValue.assign ( valuePtr, valueLen );

		}

	}
	
	return (! this->parsedBoxes.empty());

}	// TradQT_Manager::ParseCachedBoxes

// =================================================================================================
// TradQT_Manager::ImportSimpleXMP
// ===============================
//
// Update a simple XMP property if the QT value looks newer.

bool TradQT_Manager::ImportSimpleXMP ( XMP_Uns32 id, SXMPMeta * xmp, XMP_StringPtr ns, XMP_StringPtr prop ) const
{

	try {

		InfoMapCPos infoPos = this->parsedBoxes.find ( id );
		if ( infoPos == this->parsedBoxes.end() ) return false;
		if ( infoPos->second.values.empty() ) return false;
		
		std::string xmpValue, tempValue;
		XMP_OptionBits flags;
		bool xmpExists = xmp->GetProperty ( ns, prop, &xmpValue, &flags );
		if ( xmpExists && (! XMP_PropIsSimple ( flags )) ) {
			XMP_Throw ( "TradQT_Manager::ImportSimpleXMP - XMP property must be simple", kXMPErr_BadParam );
		}
		
		bool convertOK;
		const ValueInfo & qtItem = infoPos->second.values[0];	// ! Use the first QT entry.
	
		if ( xmpExists ) {
			convertOK = ConvertToMacLang ( xmpValue, qtItem.macLang, &tempValue );
			if ( ! convertOK ) return false;	// throw?
			if ( tempValue == qtItem.macValue ) return false;	// QT value matches back converted XMP value.
		}
	
		convertOK = ConvertFromMacLang ( qtItem.macValue, qtItem.macLang, &tempValue );
		if ( ! convertOK ) return false;	// throw?
		xmp->SetProperty ( ns, prop, tempValue.c_str() );
		return true;
	
	} catch ( ... ) {

		return false;	// Don't let one failure abort other imports.	
	
	}
		
}	// TradQT_Manager::ImportSimpleXMP

// =================================================================================================
// TradQT_Manager::ImportLangItem
// ==============================
//
// Update a specific XMP AltText item if the QuickTime value looks newer.

bool TradQT_Manager::ImportLangItem ( const ValueInfo & qtItem, SXMPMeta * xmp,
									  XMP_StringPtr ns, XMP_StringPtr langArray ) const
{

	try {

		XMP_StringPtr genericLang, specificLang;
		if ( qtItem.xmpLang[0] != 0 ) {
			genericLang  = qtItem.xmpLang;
			specificLang = qtItem.xmpLang;
		} else {
			genericLang  = "";
			specificLang = "x-default";
		}
	
		bool convertOK;
		std::string xmpValue, tempValue, actualLang;
		bool xmpExists = xmp->GetLocalizedText ( ns, langArray, genericLang, specificLang, &actualLang, &xmpValue, 0 );
		if ( xmpExists ) {
			convertOK = ConvertToMacLang ( xmpValue, qtItem.macLang, &tempValue );
			if ( ! convertOK ) return false;	// throw?
			if ( tempValue == qtItem.macValue ) return true;	// QT value matches back converted XMP value.
			specificLang = actualLang.c_str();
		}
	
		convertOK = ConvertFromMacLang ( qtItem.macValue, qtItem.macLang, &tempValue );
		if ( ! convertOK ) return false;	// throw?
		xmp->SetLocalizedText ( ns, langArray, "", specificLang, tempValue.c_str() );
		return true;
	
	} catch ( ... ) {

		return false;	// Don't let one failure abort other imports.	
	
	}

}	// TradQT_Manager::ImportLangItem

// =================================================================================================
// TradQT_Manager::ImportLangAltXMP
// ================================
//
// Update items in the XMP array if the QT value looks newer.

bool TradQT_Manager::ImportLangAltXMP ( XMP_Uns32 id, SXMPMeta * xmp, XMP_StringPtr ns, XMP_StringPtr langArray ) const
{

	try {

		InfoMapCPos infoPos = this->parsedBoxes.find ( id );
		if ( infoPos == this->parsedBoxes.end() ) return false;
		if ( infoPos->second.values.empty() ) return false;	// Quit now if there are no values.
		
		XMP_OptionBits flags;
		bool xmpExists = xmp->GetProperty ( ns, langArray, 0, &flags );
		if ( ! xmpExists ) {
			xmp->SetProperty ( ns, langArray, 0, kXMP_PropArrayIsAltText );
		} else if ( ! XMP_ArrayIsAltText ( flags ) ) {
			XMP_Throw ( "TradQT_Manager::ImportLangAltXMP - XMP array must be AltText", kXMPErr_BadParam );
		}
	
		// Process all of the QT values, looking up the appropriate XMP language for each.
		
		bool haveMappings = false;
		const ValueVector & qtValues = infoPos->second.values;
		
		for ( size_t i = 0, limit = qtValues.size(); i < limit; ++i ) {
			const ValueInfo & qtItem = qtValues[i];
			if ( *qtItem.xmpLang == 0 ) continue;	// Only do known mappings in the loop.
			haveMappings |= this->ImportLangItem ( qtItem, xmp, ns, langArray );
		}
		
		if ( ! haveMappings ) {
			// If nothing mapped, process the first QT item to XMP's "x-default".
			haveMappings = this->ImportLangItem ( qtValues[0], xmp, ns, langArray );	// ! No xmpLang implies "x-default".
		}
	
		return haveMappings;
	
	} catch ( ... ) {

		return false;	// Don't let one failure abort other imports.	
	
	}

}	// TradQT_Manager::ImportLangAltXMP

// =================================================================================================
// TradQT_Manager::ExportSimpleXMP
// ===============================
//
// Export a simple XMP value to the first existing QuickTime item. Delete all of the QT values if the
// XMP value is empty or the XMP does not exist.

// ! We don't create new QuickTime items since we don't know the language.

void TradQT_Manager::ExportSimpleXMP ( XMP_Uns32 id, const SXMPMeta & xmp, XMP_StringPtr ns, XMP_StringPtr prop,
									   bool createWithZeroLang /* = false */  )
{
	std::string xmpValue, macValue;

	InfoMapPos infoPos = this->parsedBoxes.find ( id );
	bool qtFound = (infoPos != this->parsedBoxes.end()) && (! infoPos->second.values.empty());

	bool xmpFound = xmp.GetProperty ( ns, prop, &xmpValue, 0 );
	if ( (! xmpFound) || (xmpValue.empty()) ) {
		if ( qtFound ) {
			this->parsedBoxes.erase ( infoPos );
			this->changed = true;
		}
		return;
	}
	
	XMP_Assert ( xmpFound );
	if ( ! qtFound ) {
		if ( ! createWithZeroLang ) return;
		infoPos = this->parsedBoxes.insert ( this->parsedBoxes.end(),
											 InfoMap::value_type ( id, ParsedBoxInfo ( id ) ) );
		ValueVector * newValues = &infoPos->second.values;
		newValues->push_back ( ValueInfo() );
		ValueInfo * newValue = &newValues->back();
		newValue->macLang = 0;	// Happens to be langEnglish.
		newValue->xmpLang = kMacToXMPLang_0_94[0];
		this->changed = infoPos->second.changed = true;
	}
		
	ValueInfo * qtItem = &infoPos->second.values[0];	// ! Use the first QT entry.
	if ( ! IsMacLangKnown ( qtItem->macLang ) ) return;
	
	bool convertOK = ConvertToMacLang ( xmpValue, qtItem->macLang, &macValue );
	if ( convertOK && (macValue != qtItem->macValue) ) {
		qtItem->macValue = macValue;
		this->changed = infoPos->second.changed = true;
	}
	
}	// TradQT_Manager::ExportSimpleXMP

// =================================================================================================
// TradQT_Manager::ExportLangAltXMP
// ================================
//
// Export XMP LangAlt array items to QuickTime, where the language and encoding mappings are known.
// If there are no known language and encoding mappings, map the XMP default item to the first
// existing QuickTime item.

void TradQT_Manager::ExportLangAltXMP ( XMP_Uns32 id, const SXMPMeta & xmp, XMP_StringPtr ns, XMP_StringPtr langArray )
{
	bool haveMappings = false;
	std::string xmpPath, xmpValue, xmpLang, macValue;

	InfoMapPos infoPos = this->parsedBoxes.find ( id );
	if ( infoPos == this->parsedBoxes.end() ) {
		infoPos = this->parsedBoxes.insert ( this->parsedBoxes.end(), 
											 InfoMap::value_type ( id, ParsedBoxInfo ( id ) ) );
	}
	
	ValueVector * qtValues = &infoPos->second.values;
	XMP_Index xmpCount = xmp.CountArrayItems ( ns, langArray );
	bool convertOK;

	if ( xmpCount == 0 ) {
		// Delete the "mappable" QuickTime items if there are no XMP values. Leave the others alone.
		for ( int i = (int)qtValues->size()-1; i > 0; --i ) {	// ! Need a signed index.
			if ( (*qtValues)[i].xmpLang[0] != 0 ) {
				qtValues->erase ( qtValues->begin() + i );
				this->changed = infoPos->second.changed = true;
			}
		}
		return;
	}
	
	// Go through the XMP and look for a related macLang QuickTime item to update or create.
	
	for ( XMP_Index xmpIndex = 1; xmpIndex <= xmpCount; ++xmpIndex ) {	// ! XMP index starts at 1!

		SXMPUtils::ComposeArrayItemPath ( ns, langArray, xmpIndex, &xmpPath );
		xmp.GetProperty ( ns, xmpPath.c_str(), &xmpValue, 0 );
		xmp.GetQualifier ( ns, xmpPath.c_str(), kXMP_NS_XML, "lang", &xmpLang, 0 );
		if ( xmpLang == "x-default" ) continue;
		
		XMP_Uns16 macLang = GetMacLang ( &xmpLang );
		if ( macLang == kNoMacLang ) continue;
		
		size_t qtIndex, qtLimit;
		for ( qtIndex = 0, qtLimit = qtValues->size(); qtIndex < qtLimit; ++qtIndex ) {
			if ( (*qtValues)[qtIndex].macLang == macLang ) break;
		}

		if ( qtIndex == qtLimit ) {
			// No existing QuickTime item, try to create one.
			if ( ! IsMacLangKnown ( macLang ) ) continue;
			qtValues->push_back ( ValueInfo() );
			qtIndex = qtValues->size() - 1;
			ValueInfo * newItem = &((*qtValues)[qtIndex]);
			newItem->macLang = macLang;
			newItem->xmpLang = GetXMPLang ( macLang );	// ! Use the 2 character root language.
		}
		
		ValueInfo * qtItem = &((*qtValues)[qtIndex]);
		qtItem->marked = true;	// Mark it whether updated or not, don't delete it in the next pass.

		convertOK = ConvertToMacLang ( xmpValue, qtItem->macLang, &macValue );
		if ( convertOK && (macValue != qtItem->macValue) ) {
			qtItem->macValue.swap ( macValue );	// No need to make a copy.
			haveMappings = true;
		}

	}
	this->changed |= haveMappings;
	infoPos->second.changed |= haveMappings;
	
	// Go through the QuickTime items that are unmarked and delete those that have an xmpLang
	// and known macScript. Clear all marks.

	for ( int i = (int)qtValues->size()-1; i > 0; --i ) {	// ! Need a signed index.
		ValueInfo * qtItem = &((*qtValues)[i]);
		if ( qtItem->marked ) {
			qtItem->marked = false;
		} else if ( (qtItem->xmpLang[0] != 0) && IsMacLangKnown ( qtItem->macLang ) ) {
			qtValues->erase ( qtValues->begin() + i );
			this->changed = infoPos->second.changed = true;
		}
	}
	
	// If there were no mappings, export the XMP default item to the first QT item.

	if ( (! haveMappings) && (! qtValues->empty()) ) {
	
		bool ok = xmp.GetLocalizedText ( ns, langArray, "", "x-default", 0, &xmpValue, 0 );
		if ( ! ok ) return;
		
		ValueInfo * qtItem = &((*qtValues)[0]);
		if ( ! IsMacLangKnown ( qtItem->macLang ) ) return;
		
		convertOK = ConvertToMacLang ( xmpValue, qtItem->macLang, &macValue );
		if ( convertOK && (macValue != qtItem->macValue) ) {
			qtItem->macValue.swap ( macValue );	// No need to make a copy.
			this->changed = infoPos->second.changed = true;
		}

	}

}	// TradQT_Manager::ExportLangAltXMP

// =================================================================================================
// TradQT_Manager::UpdateChangedBoxes
// ==================================

void TradQT_Manager::UpdateChangedBoxes ( MOOV_Manager * moovMgr )
{
	MOOV_Manager::BoxInfo udtaInfo;
	MOOV_Manager::BoxRef  udtaRef = moovMgr->GetBox ( "moov/udta", &udtaInfo );
	XMP_Assert ( (udtaRef != 0) || (udtaInfo.childCount == 0) );
	
	if ( udtaRef != 0 ) {	// Might not have been a moov/udta box in the parse.

		// First go through the moov/udta/©... children and delete those that are not in the map.
	
		for ( XMP_Uns32 ordinal = udtaInfo.childCount; ordinal > 0; --ordinal ) {	// ! Go backwards because of deletions.
	
			MOOV_Manager::BoxInfo currInfo;
			MOOV_Manager::BoxRef  currRef = moovMgr->GetNthChild ( udtaRef, (ordinal-1), &currInfo );
			if ( currRef == 0 ) break;	// Sanity check, should not happen.
			if ( (currInfo.boxType >> 24) != 0xA9 ) continue;
			if ( currInfo.contentSize < 2+2+1 ) continue;	// These were skipped by ParseCachedBoxes.
			
			InfoMapPos infoPos = this->parsedBoxes.find ( currInfo.boxType );
			if ( infoPos == this->parsedBoxes.end() ) moovMgr->DeleteNthChild ( udtaRef, (ordinal-1) );
		
		}
	
	}
	
	// Now go through the changed items in the map and update them in the moov/udta subtree.
	
	InfoMapCPos infoPos = this->parsedBoxes.begin();
	InfoMapCPos infoEnd = this->parsedBoxes.end();
	
	for ( ; infoPos != infoEnd; ++infoPos ) {

		ParsedBoxInfo * qtItem = (ParsedBoxInfo*) &infoPos->second;
		if ( ! qtItem->changed ) continue;
		qtItem->changed = false;
		
		XMP_Uns32 qtTotalSize = 0;	// Total size of the QT values, ignoring empty values.
		for ( size_t i = 0, limit = qtItem->values.size(); i < limit; ++i ) {
			if ( ! qtItem->values[i].macValue.empty() ) {
				if ( qtItem->values[i].macValue.size() > 0xFFFF ) qtItem->values[i].macValue.erase ( 0xFFFF );
				qtTotalSize += (XMP_Uns32)(2+2 + qtItem->values[i].macValue.size());
			}
		}
		
		if ( udtaRef == 0 ) {	// Might not have been a moov/udta box in the parse.
			moovMgr->SetBox ( "moov/udta", 0, 0 );
			udtaRef = moovMgr->GetBox ( "moov/udta", &udtaInfo );
			XMP_Assert ( udtaRef != 0 );
		}
		
		if ( qtTotalSize == 0 ) {
		
			moovMgr->DeleteTypeChild ( udtaRef, qtItem->id );
		
		} else {
		
			// Compose the complete box content.
			
			RawDataBlock fullValue;
			fullValue.assign ( qtTotalSize, 0 );
			XMP_Uns8 * valuePtr = &fullValue[0];
		
			for ( size_t i = 0, limit = qtItem->values.size(); i < limit; ++i ) {
				XMP_Assert ( qtItem->values[i].macValue.size() <= 0xFFFF );
				XMP_Uns16 textSize = (XMP_Uns16)qtItem->values[i].macValue.size();
				if ( textSize == 0 ) continue;
				PutUns16BE ( textSize, valuePtr );									valuePtr += 2;
				PutUns16BE ( qtItem->values[i].macLang, valuePtr );					valuePtr += 2;
				memcpy ( valuePtr, qtItem->values[i].macValue.c_str(), textSize );	valuePtr += textSize;
			}
			
			// Look for an existing box to update, else add a new one.

			MOOV_Manager::BoxInfo itemInfo;
			MOOV_Manager::BoxRef  itemRef = moovMgr->GetTypeChild ( udtaRef, qtItem->id, &itemInfo );
			
			if ( itemRef != 0 ) {
				moovMgr->SetBox ( itemRef, &fullValue[0], qtTotalSize );
			} else {
				moovMgr->AddChildBox ( udtaRef, qtItem->id, &fullValue[0], qtTotalSize );
			}
			
		}

	}

}	// TradQT_Manager::UpdateChangedBoxes

// =================================================================================================
