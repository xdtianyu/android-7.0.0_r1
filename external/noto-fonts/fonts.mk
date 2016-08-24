# Copyright (C) 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# We have to use PRODUCT_PACKAGES (together with BUILD_PREBUILT) instead of
# PRODUCT_COPY_FILES to install the font files, so that the NOTICE file can
# get installed too.

PRODUCT_PACKAGES := \
    NotoColorEmoji.ttf \
    NotoNaskhArabic-Regular.ttf \
    NotoNaskhArabic-Bold.ttf \
    NotoNaskhArabicUI-Regular.ttf \
    NotoNaskhArabicUI-Bold.ttf \
    NotoSansArmenian-Regular.ttf \
    NotoSansArmenian-Bold.ttf \
    NotoSansBalinese-Regular.ttf \
    NotoSansBamum-Regular.ttf \
    NotoSansBatak-Regular.ttf \
    NotoSansBengali-Regular.ttf \
    NotoSansBengali-Bold.ttf \
    NotoSansBengaliUI-Regular.ttf \
    NotoSansBengaliUI-Bold.ttf \
    NotoSansBuginese-Regular.ttf \
    NotoSansBuhid-Regular.ttf \
    NotoSansCJK-Regular.ttc \
    NotoSansCanadianAboriginal-Regular.ttf \
    NotoSansCham-Regular.ttf \
    NotoSansCham-Bold.ttf \
    NotoSansCherokee-Regular.ttf \
    NotoSansCoptic-Regular.ttf \
    NotoSansDevanagari-Regular.ttf \
    NotoSansDevanagari-Bold.ttf \
    NotoSansDevanagariUI-Regular.ttf \
    NotoSansDevanagariUI-Bold.ttf \
    NotoSansEthiopic-Regular.ttf \
    NotoSansEthiopic-Bold.ttf \
    NotoSansGeorgian-Regular.ttf \
    NotoSansGeorgian-Bold.ttf \
    NotoSansGlagolitic-Regular.ttf \
    NotoSansGujarati-Regular.ttf \
    NotoSansGujarati-Bold.ttf \
    NotoSansGujaratiUI-Regular.ttf \
    NotoSansGujaratiUI-Bold.ttf \
    NotoSansGurmukhi-Regular.ttf \
    NotoSansGurmukhi-Bold.ttf \
    NotoSansGurmukhiUI-Regular.ttf \
    NotoSansGurmukhiUI-Bold.ttf \
    NotoSansHanunoo-Regular.ttf \
    NotoSansHebrew-Regular.ttf \
    NotoSansHebrew-Bold.ttf \
    NotoSansJavanese-Regular.ttf \
    NotoSansKannada-Regular.ttf \
    NotoSansKannada-Bold.ttf \
    NotoSansKannadaUI-Regular.ttf \
    NotoSansKannadaUI-Bold.ttf \
    NotoSansKayahLi-Regular.ttf \
    NotoSansKhmer-Regular.ttf \
    NotoSansKhmer-Bold.ttf \
    NotoSansKhmerUI-Regular.ttf \
    NotoSansKhmerUI-Bold.ttf \
    NotoSansLao-Regular.ttf \
    NotoSansLao-Bold.ttf \
    NotoSansLaoUI-Regular.ttf \
    NotoSansLaoUI-Bold.ttf \
    NotoSansLepcha-Regular.ttf \
    NotoSansLimbu-Regular.ttf \
    NotoSansLisu-Regular.ttf \
    NotoSansMalayalam-Regular.ttf \
    NotoSansMalayalam-Bold.ttf \
    NotoSansMalayalamUI-Regular.ttf \
    NotoSansMalayalamUI-Bold.ttf \
    NotoSansMandaic-Regular.ttf \
    NotoSansMeeteiMayek-Regular.ttf \
    NotoSansMongolian-Regular.ttf \
    NotoSansMyanmar-Regular.ttf \
    NotoSansMyanmar-Bold.ttf \
    NotoSansMyanmarUI-Regular.ttf \
    NotoSansMyanmarUI-Bold.ttf \
    NotoSansNewTaiLue-Regular.ttf \
    NotoSansNKo-Regular.ttf \
    NotoSansOlChiki-Regular.ttf \
    NotoSansOriya-Regular.ttf \
    NotoSansOriya-Bold.ttf \
    NotoSansOriyaUI-Regular.ttf \
    NotoSansOriyaUI-Bold.ttf \
    NotoSansRejang-Regular.ttf \
    NotoSansSaurashtra-Regular.ttf \
    NotoSansSinhala-Regular.ttf \
    NotoSansSinhala-Bold.ttf \
    NotoSansSundanese-Regular.ttf \
    NotoSansSylotiNagri-Regular.ttf \
    NotoSansSymbols-Regular-Subsetted.ttf \
    NotoSansSymbols-Regular-Subsetted2.ttf \
    NotoSansSyriacEstrangela-Regular.ttf \
    NotoSansTagbanwa-Regular.ttf \
    NotoSansTaiLe-Regular.ttf \
    NotoSansTaiTham-Regular.ttf \
    NotoSansTaiViet-Regular.ttf \
    NotoSansTamil-Regular.ttf \
    NotoSansTamil-Bold.ttf \
    NotoSansTamilUI-Regular.ttf \
    NotoSansTamilUI-Bold.ttf \
    NotoSansTelugu-Regular.ttf \
    NotoSansTelugu-Bold.ttf \
    NotoSansTeluguUI-Regular.ttf \
    NotoSansTeluguUI-Bold.ttf \
    NotoSansThaana-Regular.ttf \
    NotoSansThaana-Bold.ttf \
    NotoSansThai-Regular.ttf \
    NotoSansThai-Bold.ttf \
    NotoSansThaiUI-Regular.ttf \
    NotoSansThaiUI-Bold.ttf \
    NotoSansTibetan-Regular.ttf \
    NotoSansTibetan-Bold.ttf \
    NotoSansTifinagh-Regular.ttf \
    NotoSansVai-Regular.ttf \
    NotoSansYi-Regular.ttf \
    NotoSerif-Regular.ttf \
    NotoSerif-Bold.ttf \
    NotoSerif-Italic.ttf \
    NotoSerif-BoldItalic.ttf
