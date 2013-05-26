-- EQEmulator Inventory Conversion
-- "Player sharedbank remap from linear to array format"

-- Notes:
-- - Needs review...
-- - Added fields use int16-based values
-- - Unique index updates appear to process for working database..unsure of result for corrupted/mis-managed one
-- - May need to add charges to augment insertions
-- - Augments update may need to have custom_data field updated so they don't return a 'null'
-- - May need to move field drop to a separate file..along with a check to determine missed items before deleting unneeded fields
-- - (Check for missed items -> 'WHERE slottypeid = -1 && mainslotid = -1 && subslotid = -1 && augslotid = -1')
-- - Unneeded field DROP is disabled until certified by management


-- Drop old indicies
ALTER TABLE sharedbank DROP INDEX account;


-- Modify 'sharedbank' table
ALTER TABLE sharedbank ADD COLUMN slottypeid SMALLINT(6) NOT NULL DEFAULT '-1' AFTER acctid;
ALTER TABLE sharedbank ADD COLUMN mainslotid SMALLINT(6) NOT NULL DEFAULT '-1' AFTER slottypeid;
ALTER TABLE sharedbank ADD COLUMN subslotid SMALLINT(6) NOT NULL DEFAULT '-1' AFTER mainslotid;
ALTER TABLE sharedbank ADD COLUMN augslotid SMALLINT(6) NOT NULL DEFAULT '-1' AFTER subslotid;


-- Remap shared bank [Shared Bank (2500 - 2501) and Shared Bank bag (2531 - 2550)]
UPDATE sharedbank SET slottypeid = 2, mainslotid = (slotid - 2500), subslotid = -1, augslotid = -1 WHERE slotid >= 2500 && slotid <= 2501;
UPDATE sharedbank SET slottypeid = 2, mainslotid = (0 + ((slotid - 2531) / 10)), subslotid = ((slotid - 1) % 10), augslotid = -1 WHERE slotid >= 2531 && slotid <= 2550;


-- Remap augment 1
INSERT INTO sharedbank (acctid, slottypeid, mainslotid, subslotid, augslot1) SELECT acctid, slottypeid, mainslotid, subslotid, augslot1 FROM sharedbank WHERE augslot1 != 0 && itemid != augslot1;
UPDATE sharedbank SET itemid = augslot1, augslotid = 0 WHERE itemid = 0 && augslot1 > 0;


-- Remap augment 2
INSERT INTO sharedbank (acctid, slottypeid, mainslotid, subslotid, augslot2) SELECT acctid, slottypeid, mainslotid, subslotid, augslot2 FROM sharedbank WHERE augslot2 != 0 && itemid != augslot2;
UPDATE sharedbank SET itemid = augslot2, augslotid = 1 WHERE itemid = 0 && augslot2 > 0;


-- Remap augment 3
INSERT INTO sharedbank (acctid, slottypeid, mainslotid, subslotid, augslot3) SELECT acctid, slottypeid, mainslotid, subslotid, augslot3 FROM sharedbank WHERE augslot3 != 0 && itemid != augslot3;
UPDATE sharedbank SET itemid = augslot3, augslotid = 2 WHERE itemid = 0 && augslot3 > 0;


-- Remap augment 4
INSERT INTO sharedbank (acctid, slottypeid, mainslotid, subslotid, augslot4) SELECT acctid, slottypeid, mainslotid, subslotid, augslot4 FROM sharedbank WHERE augslot4 != 0 && itemid != augslot4;
UPDATE sharedbank SET itemid = augslot4, augslotid = 3 WHERE itemid = 0 && augslot4 > 0;


-- Remap augment 5
INSERT INTO sharedbank (acctid, slottypeid, mainslotid, subslotid, augslot5) SELECT acctid, slottypeid, mainslotid, subslotid, augslot5 FROM sharedbank WHERE augslot5 != 0 && itemid != augslot5;
UPDATE sharedbank SET itemid = augslot5, augslotid = 4 WHERE itemid = 0 && augslot5 > 0;


-- Drop old slot and augment fields
-- ALTER TABLE sharedbank DROP COLUMN slotid;
-- ALTER TABLE sharedbank DROP COLUMN augslot1;
-- ALTER TABLE sharedbank DROP COLUMN augslot2;
-- ALTER TABLE sharedbank DROP COLUMN augslot3;
-- ALTER TABLE sharedbank DROP COLUMN augslot4;
-- ALTER TABLE sharedbank DROP COLUMN augslot5;


-- Add new unique indicies
ALTER TABLE sharedbank ADD UNIQUE INDEX account (acctid, slottypeid, mainslotid, subslotid, augslotid);
