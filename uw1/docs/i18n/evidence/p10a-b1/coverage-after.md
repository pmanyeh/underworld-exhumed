# UW1 zh-TW localization coverage

PAK `e8c9848a14c49620...`. Direct + Special = the production scope: player-facing,
non-empty, addressable entries. Debug (block 24 dungeon report) is inventory only.

**Production completion (draft or better): 209 / 6990 (2.99%)**; reviewed or better 0, approved 0.

All inventory: 10984 entries = 6990 production + 101 debug + 3852 empty + 41 unaddressable.

| Category | Total | Empty | Unaddr. | Debug | Direct | Special | Untransl. | Draft | Reviewed | Approved | Blocked | Runtime seen | Runtime hit | Runtime fallback |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| books | 512 | 442 | 0 | 0 | 70 | 0 | 70 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| chargen | 521 | 437 | 9 | 0 | 0 | 75 | 75 | 0 | 0 | 0 | 0 | 36 | 0 | 36 |
| conversation | 5189 | 99 | 0 | 0 | 4911 | 179 | 5011 | 79 | 0 | 0 | 0 | 26 | 25 | 1 |
| conversation_common | 512 | 403 | 0 | 0 | 2 | 107 | 106 | 3 | 0 | 0 | 0 | 5 | 1 | 4 |
| cutscene | 122 | 0 | 0 | 0 | 0 | 122 | 122 | 0 | 0 | 0 | 0 | 42 | 0 | 42 |
| dungeon_report | 512 | 411 | 0 | 101 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| game | 512 | 102 | 0 | 0 | 156 | 254 | 283 | 127 | 0 | 0 | 0 | 13 | 8 | 5 |
| object_look | 544 | 409 | 32 | 0 | 0 | 103 | 103 | 0 | 0 | 0 | 0 | 1 | 0 | 1 |
| object_names | 512 | 108 | 0 | 0 | 0 | 404 | 404 | 0 | 0 | 0 | 0 | 2 | 0 | 2 |
| spells | 512 | 299 | 0 | 0 | 0 | 213 | 213 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| textures | 512 | 248 | 0 | 0 | 0 | 264 | 264 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| trap_text | 512 | 498 | 0 | 0 | 14 | 0 | 14 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| writings | 512 | 396 | 0 | 0 | 98 | 18 | 116 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| **total** | 10984 | 3852 | 41 | 101 | 5251 | 1739 | 6781 | 209 | 0 | 0 | 0 | 125 | 34 | 91 |

| Production handling | Count |
| --- | ---: |
| bitmap-ui | 261 |
| direct | 5251 |
| grammar | 885 |
| input-token | 26 |
| object-name | 413 |
| template | 154 |

| Special list | Count |
| --- | ---: |
| untranslated | 6781 |
| template | 154 |
| grammar_sensitive | 1298 |
| bitmap_ui | 261 |
| hardcoded_related | 0 |
| runtime_untranslated | 90 |
| runtime_fallback | 91 |
| runtime_dynamic_or_invalid_keys | 1 |
| source_drift | 0 |
| invalid_target | 0 |
| hardcoded_literal_candidates | 2565 |
| hardcoded_literals_translated | 1 |
