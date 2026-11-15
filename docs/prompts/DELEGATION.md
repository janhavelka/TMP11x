# Bounded subagents and independent review

Use actual tools and models exposed by the IDE; do not invent model IDs or
change account settings. Strong reasoning is appropriate for vendor conflicts,
read-to-clear status, failure ambiguity, EEPROM lock and operation deadlines;
simpler agents can validate links, source inventories, independent vectors
and fixed-interface tests. Each subagent brief must state objective, selected
prompt, read-only vs writable paths, relevant contracts, inputs, acceptance,
report path and no-Git rule.

Recommended splits:

- Read-only independent TMP116/TMP117/TMP119 field-table reviewer; no writing
  shared ledgers until lead reconciles differences.
- Source/library conventions audit versus exact silicon, as separate tasks.
- Focused native device-model/negative test implementation after public API
  freezes, with lead owning shared headers/state.
- Independent reviewer of `0x07` variants, ID ambiguity, DATA_READY clearing,
  EEPROM unlock and unsafe raw writes.
- Independent package/SDK verification after integration, with disjoint build
  folders and no edits to sibling repositories.

Review actual diff and test evidence before merge. Do not delegate work that
requires two agents to edit one owner state machine in parallel. One lead
controls stage status, commits, pushes and final claims.
