-- EQEmulator Inventory Conversion
-- "Swap wear slot bits for 'Ammo' and 'Power Source' slots"

-- Notes:
-- - Bit 21 = 2,097,152
-- - Bit 22 = 4,194,304
-- - Where bits 21 and 22 are (1,1) or (0,0), no processing is required
-- - Success can be verified using filters 'slots & 2097152' and 'slots & 4194304' before and after running this script


-- Swap bitmask positions 21 and 22
UPDATE items SET slots = slots ^ 6291456 WHERE (slots & 2097152) ^ (slots & 4194304);
