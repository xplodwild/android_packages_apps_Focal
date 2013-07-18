#ifndef __MacScriptExtracts__
#define __MacScriptExtracts__

// Extracts of script (smXyz) and language (langXyz) enums from Apple's old Script.h.
// These are used to support "traditional" QuickTime metadata processing.

/*
   Script codes:
   These specify a Mac OS encoding that is related to a FOND ID range.
   Some of the encodings have several variants (e.g. for different localized systems)
    which all share the same script code.
   Not all of these script codes are currently supported by Apple software.
   Notes:
   - Script code 0 (smRoman) is also used (instead of smGreek) for the Greek encoding
     in the Greek localized system.
   - Script code 28 (smEthiopic) is also used for the Inuit encoding in the Inuktitut
     system.
*/
enum {
  smRoman                       = 0,
  smJapanese                    = 1,
  smTradChinese                 = 2,    /* Traditional Chinese*/
  smKorean                      = 3,
  smArabic                      = 4,
  smHebrew                      = 5,
  smGreek                       = 6,
  smCyrillic                    = 7,
  smRSymbol                     = 8,    /* Right-left symbol*/
  smDevanagari                  = 9,
  smGurmukhi                    = 10,
  smGujarati                    = 11,
  smOriya                       = 12,
  smBengali                     = 13,
  smTamil                       = 14,
  smTelugu                      = 15,
  smKannada                     = 16,   /* Kannada/Kanarese*/
  smMalayalam                   = 17,
  smSinhalese                   = 18,
  smBurmese                     = 19,
  smKhmer                       = 20,   /* Khmer/Cambodian*/
  smThai                        = 21,
  smLao                         = 22,
  smGeorgian                    = 23,
  smArmenian                    = 24,
  smSimpChinese                 = 25,   /* Simplified Chinese*/
  smTibetan                     = 26,
  smMongolian                   = 27,
  smEthiopic                    = 28,
  smGeez                        = 28,   /* Synonym for smEthiopic*/
  smCentralEuroRoman            = 29,   /* For Czech, Slovak, Polish, Hungarian, Baltic langs*/
  smVietnamese                  = 30,
  smExtArabic                   = 31,   /* extended Arabic*/
  smUninterp                    = 32    /* uninterpreted symbols, e.g. palette symbols*/
};

/* Extended script code for full Unicode input*/
enum {
  smUnicodeScript               = 0x7E
};

/* Obsolete script code names (kept for backward compatibility):*/
enum {
  smChinese                     = 2,    /* (Use smTradChinese or smSimpChinese)*/
  smRussian                     = 7,    /* Use smCyrillic*/
                                        /* smMaldivian = 25: deleted, no code for Maldivian*/
  smLaotian                     = 22,   /* Use smLao                                     */
  smAmharic                     = 28,   /* Use smEthiopic or smGeez*/
  smSlavic                      = 29,   /* Use smCentralEuroRoman*/
  smEastEurRoman                = 29,   /* Use smCentralEuroRoman*/
  smSindhi                      = 31,   /* Use smExtArabic*/
  smKlingon                     = 32
};

/*
   Language codes:
   These specify a language implemented using a particular Mac OS encoding.
   Not all of these language codes are currently supported by Apple software.
*/
enum {
  langEnglish                   = 0,    /* smRoman script*/
  langFrench                    = 1,    /* smRoman script*/
  langGerman                    = 2,    /* smRoman script*/
  langItalian                   = 3,    /* smRoman script*/
  langDutch                     = 4,    /* smRoman script*/
  langSwedish                   = 5,    /* smRoman script*/
  langSpanish                   = 6,    /* smRoman script*/
  langDanish                    = 7,    /* smRoman script*/
  langPortuguese                = 8,    /* smRoman script*/
  langNorwegian                 = 9,    /* (Bokmal) smRoman script*/
  langHebrew                    = 10,   /* smHebrew script*/
  langJapanese                  = 11,   /* smJapanese script*/
  langArabic                    = 12,   /* smArabic script*/
  langFinnish                   = 13,   /* smRoman script*/
  langGreek                     = 14,   /* Greek script (monotonic) using smRoman script code*/
  langIcelandic                 = 15,   /* modified smRoman/Icelandic script*/
  langMaltese                   = 16,   /* Roman script*/
  langTurkish                   = 17,   /* modified smRoman/Turkish script*/
  langCroatian                  = 18,   /* modified smRoman/Croatian script*/
  langTradChinese               = 19,   /* Chinese (Mandarin) in traditional characters*/
  langUrdu                      = 20,   /* smArabic script*/
  langHindi                     = 21,   /* smDevanagari script*/
  langThai                      = 22,   /* smThai script*/
  langKorean                    = 23    /* smKorean script*/
};

enum {
  langLithuanian                = 24,   /* smCentralEuroRoman script*/
  langPolish                    = 25,   /* smCentralEuroRoman script*/
  langHungarian                 = 26,   /* smCentralEuroRoman script*/
  langEstonian                  = 27,   /* smCentralEuroRoman script*/
  langLatvian                   = 28,   /* smCentralEuroRoman script*/
  langSami                      = 29,   /* language of the Sami people of N. Scandinavia             */
  langFaroese                   = 30,   /* modified smRoman/Icelandic script                      */
  langFarsi                     = 31,   /* modified smArabic/Farsi script*/
  langPersian                   = 31,   /* Synonym for langFarsi*/
  langRussian                   = 32,   /* smCyrillic script*/
  langSimpChinese               = 33,   /* Chinese (Mandarin) in simplified characters*/
  langFlemish                   = 34,   /* smRoman script*/
  langIrishGaelic               = 35,   /* smRoman or modified smRoman/Celtic script (without dot above)   */
  langAlbanian                  = 36,   /* smRoman script*/
  langRomanian                  = 37,   /* modified smRoman/Romanian script*/
  langCzech                     = 38,   /* smCentralEuroRoman script*/
  langSlovak                    = 39,   /* smCentralEuroRoman script*/
  langSlovenian                 = 40,   /* modified smRoman/Croatian script*/
  langYiddish                   = 41,   /* smHebrew script*/
  langSerbian                   = 42,   /* smCyrillic script*/
  langMacedonian                = 43,   /* smCyrillic script*/
  langBulgarian                 = 44,   /* smCyrillic script*/
  langUkrainian                 = 45,   /* modified smCyrillic/Ukrainian script*/
  langByelorussian              = 46,   /* smCyrillic script*/
  langBelorussian               = 46    /* Synonym for langByelorussian                          */
};

enum {
  langUzbek                     = 47,   /* Cyrillic script*/
  langKazakh                    = 48,   /* Cyrillic script*/
  langAzerbaijani               = 49,   /* Azerbaijani in Cyrillic script*/
  langAzerbaijanAr              = 50,   /* Azerbaijani in Arabic script*/
  langArmenian                  = 51,   /* smArmenian script*/
  langGeorgian                  = 52,   /* smGeorgian script*/
  langMoldavian                 = 53,   /* smCyrillic script*/
  langKirghiz                   = 54,   /* Cyrillic script*/
  langTajiki                    = 55,   /* Cyrillic script*/
  langTurkmen                   = 56,   /* Cyrillic script*/
  langMongolian                 = 57,   /* Mongolian in smMongolian script*/
  langMongolianCyr              = 58,   /* Mongolian in Cyrillic script*/
  langPashto                    = 59,   /* Arabic script*/
  langKurdish                   = 60,   /* smArabic script*/
  langKashmiri                  = 61,   /* Arabic script*/
  langSindhi                    = 62,   /* Arabic script*/
  langTibetan                   = 63,   /* smTibetan script*/
  langNepali                    = 64,   /* smDevanagari script*/
  langSanskrit                  = 65,   /* smDevanagari script*/
  langMarathi                   = 66,   /* smDevanagari script*/
  langBengali                   = 67,   /* smBengali script*/
  langAssamese                  = 68,   /* smBengali script*/
  langGujarati                  = 69,   /* smGujarati script*/
  langPunjabi                   = 70    /* smGurmukhi script*/
};

enum {
  langOriya                     = 71,   /* smOriya script*/
  langMalayalam                 = 72,   /* smMalayalam script*/
  langKannada                   = 73,   /* smKannada script*/
  langTamil                     = 74,   /* smTamil script*/
  langTelugu                    = 75,   /* smTelugu script*/
  langSinhalese                 = 76,   /* smSinhalese script*/
  langBurmese                   = 77,   /* smBurmese script*/
  langKhmer                     = 78,   /* smKhmer script*/
  langLao                       = 79,   /* smLao script*/
  langVietnamese                = 80,   /* smVietnamese script*/
  langIndonesian                = 81,   /* smRoman script*/
  langTagalog                   = 82,   /* Roman script*/
  langMalayRoman                = 83,   /* Malay in smRoman script*/
  langMalayArabic               = 84,   /* Malay in Arabic script*/
  langAmharic                   = 85,   /* smEthiopic script*/
  langTigrinya                  = 86,   /* smEthiopic script*/
  langOromo                     = 87,   /* smEthiopic script*/
  langSomali                    = 88,   /* smRoman script*/
  langSwahili                   = 89,   /* smRoman script*/
  langKinyarwanda               = 90,   /* smRoman script*/
  langRuanda                    = 90,   /* synonym for langKinyarwanda*/
  langRundi                     = 91,   /* smRoman script*/
  langNyanja                    = 92,   /* smRoman script*/
  langChewa                     = 92,   /* synonym for langNyanja*/
  langMalagasy                  = 93,   /* smRoman script*/
  langEsperanto                 = 94    /* Roman script*/
};

enum {
  langWelsh                     = 128,  /* modified smRoman/Celtic script*/
  langBasque                    = 129,  /* smRoman script*/
  langCatalan                   = 130,  /* smRoman script*/
  langLatin                     = 131,  /* smRoman script*/
  langQuechua                   = 132,  /* smRoman script*/
  langGuarani                   = 133,  /* smRoman script*/
  langAymara                    = 134,  /* smRoman script*/
  langTatar                     = 135,  /* Cyrillic script*/
  langUighur                    = 136,  /* Arabic script*/
  langDzongkha                  = 137,  /* (lang of Bhutan) smTibetan script*/
  langJavaneseRom               = 138,  /* Javanese in smRoman script*/
  langSundaneseRom              = 139,  /* Sundanese in smRoman script*/
  langGalician                  = 140,  /* smRoman script*/
  langAfrikaans                 = 141   /* smRoman script                                   */
};

enum {
  langBreton                    = 142,  /* smRoman or modified smRoman/Celtic script                 */
  langInuktitut                 = 143,  /* Inuit script using smEthiopic script code                 */
  langScottishGaelic            = 144,  /* smRoman or modified smRoman/Celtic script                 */
  langManxGaelic                = 145,  /* smRoman or modified smRoman/Celtic script                 */
  langIrishGaelicScript         = 146,  /* modified smRoman/Gaelic script (using dot above)               */
  langTongan                    = 147,  /* smRoman script                                   */
  langGreekAncient              = 148,  /* Classical Greek, polytonic orthography                    */
  langGreenlandic               = 149,  /* smRoman script                                   */
  langAzerbaijanRoman           = 150,  /* Azerbaijani in Roman script                             */
  langNynorsk                   = 151   /* Norwegian Nyorsk in smRoman*/
};

enum {
  langUnspecified               = 32767 /* Special code for use in resources (such as 'itlm')           */
};

/*
   Obsolete language code names (kept for backward compatibility):
   Misspelled, ambiguous, misleading, considered pejorative, archaic, etc.
*/
enum {
  langPortugese                 = 8,    /* Use langPortuguese*/
  langMalta                     = 16,   /* Use langMaltese*/
  langYugoslavian               = 18,   /* (use langCroatian, langSerbian, etc.)*/
  langChinese                   = 19,   /* (use langTradChinese or langSimpChinese)*/
  langLettish                   = 28,   /* Use langLatvian                                     */
  langLapponian                 = 29,   /* Use langSami*/
  langLappish                   = 29,   /* Use langSami*/
  langSaamisk                   = 29,   /* Use langSami                                    */
  langFaeroese                  = 30,   /* Use langFaroese                                     */
  langIrish                     = 35,   /* Use langIrishGaelic                                  */
  langGalla                     = 87,   /* Use langOromo                                 */
  langAfricaans                 = 141,  /* Use langAfrikaans                                */
  langGreekPoly                 = 148   /* Use langGreekAncient*/
};

#endif /* __MacScriptExtracts__ */
