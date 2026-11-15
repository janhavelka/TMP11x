# Exact-part evidence and provenance

For each documented claim and implemented bitfield:

1. Identify **part/model/package/accuracy grade**, datasheet document ID,
   revision, PDF page and printed table/figure. Source data include MIN/TYP/MAX,
   legal encodings, reset domain, access (R/W/RC), read side effects and
   timing start/end conditions. When PDF extraction is ambiguous, view the
   rendered original table, not just parsed text.
2. Compare independently with relevant TI examples, Linux/Zephyr drivers,
   Adafruit or other libraries where helpful. Record exact source revisions
   and licenses. GPL or incompatible reference code is factual evidence only;
   do not copy it into production MIT sources.
3. Record in `docs/reference/manifest.json` source URLs, local paths, SHA-256,
   title/revision/license and acquisition date. Save immutable vendor PDFs
   under `docs/reference/source/` in the development repository; exclude
   bulky binaries from PlatformIO package/installed SDK where unnecessary.
4. Maintain `docs/registers/tmp11x.json` or an equally narrow ledger with
   per-part register addresses, width, bit masks, source pages, reset/EEPROM
   domains, side effects, valid values, capabilities and test owner. No invented
   reserved-bit meaning. Record known discrepancies rather than hiding them.
5. Derive numeric test vectors from the datasheet before looking at production
   codec code. Keep documented vs implemented vs host-tested vs firmware-built
   vs physically-qualified fields separate for each exact model.
6. For identity, retain raw 16-bit word, interpreted family ID and revision;
   distinguish verified address/family from selected BOM model and physically
   confirmed part. Do not equate default revision nibble with stable identity.
7. For write operations, record whether a partial or ambiguous callback can
   leave hardware changed, whether EEPROM is unlocked/busy, and how the caller
   detects/restores trust. A successful readback only proves fields not cleared
   by the read and only at that instant.
8. For freshness, distinguish latest register observation, explicitly completed
   one-shot, and DATA_READY that can be cleared by another read. Preserve
   ambiguous samples instead of promoting them to fresh.

Each stage report cites actual paths/line/symbols and source-page records.
Unresolved essential silicon behavior gates that one implementation; it does
not block independent capabilities. Mock/host tests, firmware builds, CI and
real HIL have different evidence levels and cannot be substituted.
