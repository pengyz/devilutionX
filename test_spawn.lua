-- ============================================================
-- Test Item Spawner — Batch spawn all enhanced uniques + set pieces
-- Usage: Open console (Enter), run: dofile("test_spawn.lua")
-- Then pick up items, stash them, save game = test save ready.
-- ============================================================

local items = {
    -- === Set Pieces ===
    -- Butcher's Legacy (Warrior set)
    "The Butcher's Cleaver",
    "The Undead Crown",
    "Arkaine's Valor",
    
    -- === Enhanced Uniques (proc flags) ===
    "Griswold's Edge",       -- FIREBALL_ONHIT 10%
    "Shadowhawk",            -- LIFESTEAL_ONHIT 8%
    "The Grandfather",       -- CRITNEXT_ONKILL 100%
    "Veil of Steel",         -- THORNS_ONDAM 15% + set piece
    "Inferno",               -- FIREBALL_ONHIT 12%
    "Lightsabre",            -- HOLYDAM
    
    -- === Elemental Damage Uniques ===
    "Ice Shank",             -- COLDDAM
    "The Bonesaw",           -- POISONDAM
    
    -- === Ring/Amulet for set testing ===
    "Windforce",             -- Windforce's Gift set
    "Harlequin Crest",       -- Windforce's Gift set
    "Ring of Truth",         -- Windforce's Gift set
    "Naj's Puzzler",         -- Deathspeaker set
    "Optic Amulet",          -- Deathspeaker set
    "Torn Flesh of Souls",   -- Archmage's Regalia set
}

print("Spawning " .. #items .. " test items...")

for _, name in ipairs(items) do
    local result = dev.items.spawnUnique(name)
    print("  " .. name .. ": " .. result)
end

print("Done! Pick up items, stash them, save game.")
