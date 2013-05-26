-- EQEmulator Inventory Conversion
-- "Player inventory remap from linear to array format"

-- Notes:
-- - Needs review...
-- - Added fields use int16-based values
-- - Primary key updates appear to process for working database..unsure of result for corrupted/mis-managed one
-- - Double-check existence of world bag database entries
-- - May need to add charges, color and instnodrop to augment insertions
-- - Augments update may need to have custom_data field updated so they don't return a 'null'
-- - Add any other slot range conversions that aren't currently accounted for
-- - Shared Bank is not stored in this table, but is checked to avoid possible 'lost item' entries
-- - May need some sort of 'lost item' script to assign slot struct ids to items that may have fallen in-between the cracks
-- - May need to move field drop to a separate file..along with a check to determine missed items before deleting unneeded fields
-- - (Check for missed items -> 'WHERE slottypeid = -1 && mainslotid = -1 && subslotid = -1 && augslotid = -1')
-- - Unneeded field DROP is disabled until certified by management
-- - Special thanks to KLS and Tabasco for helping with a few issues


-- Drop old primary keys
ALTER TABLE inventory DROP PRIMARY KEY;


-- Modify 'inventory' table
ALTER TABLE inventory ADD COLUMN slottypeid SMALLINT(6) NOT NULL DEFAULT '-1' AFTER charid;
ALTER TABLE inventory ADD COLUMN mainslotid SMALLINT(6) NOT NULL DEFAULT '-1' AFTER slottypeid;
ALTER TABLE inventory ADD COLUMN subslotid SMALLINT(6) NOT NULL DEFAULT '-1' AFTER mainslotid;
ALTER TABLE inventory ADD COLUMN augslotid SMALLINT(6) NOT NULL DEFAULT '-1' AFTER subslotid;


-- Remap worn [Charm through Waist (0 - 20), Power Source (9999) and Ammo (21)]
UPDATE inventory SET slottypeid = 0, mainslotid = slotid, subslotid = -1, augslotid = -1 WHERE slotid >= 0 and slotid <= 20;
UPDATE inventory SET slottypeid = 0, mainslotid = 21, subslotid = -1, augslotid = -1 WHERE slotid = 9999;
UPDATE inventory SET slottypeid = 0, mainslotid = 22, subslotid = -1, augslotid = -1 WHERE slotid = 21;


-- Remap personal [Personal (22 - 29), Personal bag (251 - 330), Cursor (30) and Cursor bag (331 - 340)]
UPDATE inventory SET slottypeid = 0, mainslotid = (slotid + 1), subslotid = -1, augslotid = -1 WHERE slotid >= 22 && slotid <= 29;
UPDATE inventory SET slottypeid = 0, mainslotid = (23 + ((slotid - 251) / 10)), subslotid = ((slotid - 1) % 10), augslotid = -1 WHERE slotid >= 251 && slotid <= 330;
UPDATE inventory SET slottypeid = 0, mainslotid = 33, subslotid = -1, augslotid = -1 WHERE slotid = 30;
UPDATE inventory SET slottypeid = 0, mainslotid = 33, subslotid = (slotid - 331), augslotid = -1 WHERE slotid >= 331 && slotid <= 340;


-- Remap bank [Bank (2000 - 2023) and Bank bag (2031 - 2270)]
UPDATE inventory SET slottypeid = 1, mainslotid = (slotid - 2000), subslotid = -1, augslotid = -1 WHERE slotid >= 2000 && slotid <= 2023;
UPDATE inventory SET slottypeid = 1, mainslotid = (0 + ((slotid - 2031) / 10)), subslotid = ((slotid - 1) % 10), augslotid = -1 WHERE slotid >= 2031 && slotid <= 2270;


-- Remap shared bank [Shared Bank (2500 - 2501) and Shared Bank bag (2531 - 2550)]
UPDATE inventory SET slottypeid = 2, mainslotid = (slotid - 2500), subslotid = -1, augslotid = -1 WHERE slotid >= 2500 && slotid <= 2501;
UPDATE inventory SET slottypeid = 2, mainslotid = (0 + ((slotid - 2531) / 10)), subslotid = ((slotid - 1) % 10), augslotid = -1 WHERE slotid >= 2531 && slotid <= 2550;


-- Remap trade [Trade (3000 - 3007) and Trade bag (3100 - 3179)]
UPDATE inventory SET slottypeid = 3, mainslotid = (slotid - 3000), subslotid = -1, augslotid = -1 WHERE slotid >= 3000 && slotid <= 3007;
UPDATE inventory SET slottypeid = 3, mainslotid = (0 + ((slotid - 3100) / 10)), subslotid = (slotid % 10), augslotid = -1 WHERE slotid >= 3100 && slotid <= 3179;


-- Remap world [World (4000 - 4009)]
UPDATE inventory SET slottypeid = 4, mainslotid = (slotid - 4000), subslotid = -1, augslotid = -1 WHERE slotid >= 4000 && slotid <= 4009;


-- Remap limbo [Limbo (8000 - 8999)]
UPDATE inventory SET slottypeid = 5, mainslotid = (slotid - 8000), subslotid = -1, augslotid = -1 WHERE slotid >= 8000 && slotid <= 8999;


-- Remap tribute [Tribute (400 - 404)]
UPDATE inventory SET slottypeid = 6, mainslotid = (slotid - 400), subslotid = -1, augslotid = -1 WHERE slotid >= 400 && slotid <= 404;


-- Remap augment 1
INSERT INTO inventory (charid, slottypeid, mainslotid, subslotid, augslot1) SELECT charid, slottypeid, mainslotid, subslotid, augslot1 FROM inventory WHERE augslot1 != 0 && itemid != augslot1;
UPDATE inventory SET itemid = augslot1, augslotid = 0 WHERE itemid = 0 && augslot1 > 0;


-- Remap augment 2
INSERT INTO inventory (charid, slottypeid, mainslotid, subslotid, augslot2) SELECT charid, slottypeid, mainslotid, subslotid, augslot2 FROM inventory WHERE augslot2 != 0 && itemid != augslot2;
UPDATE inventory SET itemid = augslot2, augslotid = 1 WHERE itemid = 0 && augslot2 > 0;


-- Remap augment 3
INSERT INTO inventory (charid, slottypeid, mainslotid, subslotid, augslot3) SELECT charid, slottypeid, mainslotid, subslotid, augslot3 FROM inventory WHERE augslot3 != 0 && itemid != augslot3;
UPDATE inventory SET itemid = augslot3, augslotid = 2 WHERE itemid = 0 && augslot3 > 0;


-- Remap augment 4
INSERT INTO inventory (charid, slottypeid, mainslotid, subslotid, augslot4) SELECT charid, slottypeid, mainslotid, subslotid, augslot4 FROM inventory WHERE augslot4 != 0 && itemid != augslot4;
UPDATE inventory SET itemid = augslot4, augslotid = 3 WHERE itemid = 0 && augslot4 > 0;


-- Remap augment 5
INSERT INTO inventory (charid, slottypeid, mainslotid, subslotid, augslot5) SELECT charid, slottypeid, mainslotid, subslotid, augslot5 FROM inventory WHERE augslot5 != 0 && itemid != augslot5;
UPDATE inventory SET itemid = augslot5, augslotid = 4 WHERE itemid = 0 && augslot5 > 0;


-- Drop old slot and augment fields
-- ALTER TABLE inventory DROP COLUMN slotid;
-- ALTER TABLE inventory DROP COLUMN augslot1;
-- ALTER TABLE inventory DROP COLUMN augslot2;
-- ALTER TABLE inventory DROP COLUMN augslot3;
-- ALTER TABLE inventory DROP COLUMN augslot4;
-- ALTER TABLE inventory DROP COLUMN augslot5;


-- Add new primary keys
ALTER TABLE inventory ADD PRIMARY KEY (charid, slottypeid, mainslotid, subslotid, augslotid);
