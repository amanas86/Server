-- EQEmulator Inventory Conversion
-- "Player inventory remap from linear to array format"

-- Needs major review...

-- The format may not be correct, but this is the gist of what needs to be done
-- Added fields use int16-based values (signed)

-- Modify 'inventory' table
ALTER TABLE `inventory` ADD COLUMN `slottypeid` SMALLINT(6) NOT NULL DEFAULT '-1' AFTER `charid`;
ALTER TABLE `inventory` ADD COLUMN `mainslotid` SMALLINT(6) NOT NULL DEFAULT '-1' AFTER `slottypeid`;
ALTER TABLE `inventory` ADD COLUMN `subslotid` SMALLINT(6) NOT NULL DEFAULT '-1' AFTER `mainslotid`;
ALTER TABLE `inventory` ADD COLUMN `augslotid` SMALLINT(6) NOT NULL DEFAULT '-1' AFTER `subslotid`;
-- update primary key..unknown what's needed at the moment...


-- Remap worn
-- -- Charm through Waist
UPDATE inventory SET slottypeid = 0, mainslotid = slotid, subslotid = -1, augslotid = -1 WHERE slotid >= 0 and slotid <= 20;
-- -- Power Source
UPDATE inventory SET slottypeid = 0, mainslotid = 21, subslotid = -1, augslotid = -1 WHERE slotid = 9999;
-- -- Ammo
UPDATE inventory SET slottypeid = 0, mainslotid = 22, subslotid = -1, augslotid = -1 WHERE slotid = 21;


-- Remap personal (including visible cursor)
-- -- Personal slots
UPDATE inventory SET slottypeid = 0, mainslotid = (slotid + 1), subslotid = -1, augslotid = -1 WHERE slotid >= 22 && slotid <= 29;
-- -- Personal bag slots
UPDATE inventory SET slottypeid = 0, mainslotid = (23 + ((slotid - 251) / 10)), subslotid = ((slotid - 1) % 10), augslotid = -1 WHERE slotid >= 251 && slotid <= 330;
-- -- Cursor slot
UPDATE inventory SET slottypeid = 0, mainslotid = 33, subslotid = -1, augslotid = -1 WHERE slotid = 30;
-- -- Cursor bag slots
UPDATE inventory SET slottypeid = 0, mainslotid = 33, subslotid = (slotid - 331), augslotid = -1 WHERE slotid >= 331 && slotid <= 340;


-- Remap bank
-- -- Bank slots
UPDATE inventory SET slottypeid = 1, mainslotid = (slotid - 2000), subslotid = -1, augslotid = -1 WHERE slotid >= 2000 && slotid <= 2023;
-- -- Bank bag slots
UPDATE inventory SET slottypeid = 1, mainslotid = (0 + ((slotid - 2031) / 10)), subslotid = ((slotid - 1) % 10), augslotid = -1 WHERE slotid >= 2031 && slotid <= 2270;


-- Remap shared bank
-- -- Shared Bank slots
UPDATE inventory SET slottypeid = 2, mainslotid = (slotid - 2500), subslotid = -1, augslotid = -1 WHERE slotid >= 2500 && slotid <= 2501;
-- -- Shared Bank bag slots
UPDATE inventory SET slottypeid = 2, mainslotid = (0 + ((slotid - 2531) / 10)), subslotid = ((slotid - 1) % 10), augslotid = -1 WHERE slotid >= 2531 && slotid <= 2550;


-- Remap trade
-- -- Trade slots
UPDATE inventory SET slottypeid = 3, mainslotid = (slotid - 3000), subslotid = -1, augslotid = -1 WHERE slotid >= 3000 && slotid <= 3007;
-- -- Trade bag slots
UPDATE inventory SET slottypeid = 3, mainslotid = (0 + ((slotid - 3100) / 10)), subslotid = (slotid % 10), augslotid = -1 WHERE slotid >= 3100 && slotid <= 3179;


-- Remap world
-- -- World slots
UPDATE inventory SET slottypeid = 4, mainslotid = (slotid - 4000), subslotid = -1, augslotid = -1 WHERE slotid >= 4000 && slotid <= 4009;
-- -- World bag slots
-- -- (Unknown existence..needs verification)
-- UPDATE inventory SET slottypeid = 4, mainslotid = (0 + ((slotid - 2531) / 10)), subslotid = ((slotid - 1) % 10), augslotid = -1 WHERE slotid >= 2531 && slotid <= 2550;


-- Remap limbo
-- -- Limbo slots
UPDATE inventory SET slottypeid = 5, mainslotid = (slotid - 8000), subslotid = -1, augslotid = -1 WHERE slotid >= 8000 && slotid <= 8999;
-- -- Limbo bag slots
-- -- (Does not exist in old format...)


-- Remap tribute
-- -- Tribute slots
UPDATE inventory SET slottypeid = 6, mainslotid = (slotid - 400), subslotid = -1, augslotid = -1 WHERE slotid >= 400 && slotid <= 404;


-- Remap <unknown> (Any other slot ranges that aren't currently accounted for)


-- Remap augments (charges? color? instnodrop?) (Currently, does not work...)
-- -- Augment1
-- INSERT INTO `inventory` (`slottypeid`, `mainslotid`, `subslotid`, `augslot1`) SELECT (`slottypeid`, `mainslotid`, `subslotid`, `augslot1`) FROM `inventory` WHERE `itemid` != `augslot1`;
-- UPDATE inventory SET itemid = augslot1, augslotid = 0 WHERE itemid = 0 && augslot1 > 0;
-- -- Augment2
-- INSERT INTO `inventory` (`slottypeid`, `mainslotid`, `subslotid`, `augslot2`) SELECT (`slottypeid`, `mainslotid`, `subslotid`, `augslot2`) FROM `inventory` WHERE `itemid` != `augslot2`;
-- UPDATE inventory SET itemid = augslot2, augslotid = 1 WHERE itemid = 0 && augslot2 > 0;
-- -- Augment3
-- INSERT INTO `inventory` (`slottypeid`, `mainslotid`, `subslotid`, `augslot3`) SELECT (`slottypeid`, `mainslotid`, `subslotid`, `augslot3`) FROM `inventory` WHERE `itemid` != `augslot3`;
-- UPDATE inventory SET itemid = augslot3, augslotid = 2 WHERE itemid = 0 && augslot3 > 0;
-- -- Augment4
-- INSERT INTO `inventory` (`slottypeid`, `mainslotid`, `subslotid`, `augslot4`) SELECT (`slottypeid`, `mainslotid`, `subslotid`, `augslot4`) FROM `inventory` WHERE `itemid` != `augslot4`;
-- UPDATE inventory SET itemid = augslot4, augslotid = 3 WHERE itemid = 0 && augslot4 > 0;
-- -- Augment5
-- INSERT INTO `inventory` (`slottypeid`, `mainslotid`, `subslotid`, `augslot5`) SELECT (`slottypeid`, `mainslotid`, `subslotid`, `augslot5`) FROM `inventory` WHERE `itemid` != `augslot5`;
-- UPDATE inventory SET itemid = augslot5, augslotid = 4 WHERE itemid = 0 && augslot5 > 0;


-- May need to move this to a separate file..along with a check to determine missed items before deleting these fields
-- (Add code to delete SlotID and all 5 Augment fields)
-- ALTER TABLE `inventory` DROP COLUMN `slotid`;
-- ALTER TABLE `inventory` DROP COLUMN `augslot1`;
-- ALTER TABLE `inventory` DROP COLUMN `augslot2`;
-- ALTER TABLE `inventory` DROP COLUMN `augslot3`;
-- ALTER TABLE `inventory` DROP COLUMN `augslot4`;
-- ALTER TABLE `inventory` DROP COLUMN `augslot5`;
