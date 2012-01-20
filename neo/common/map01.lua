-- This script runs before the player is spawned, and then throughout the 
-- rest of the level. Use it to adjust player settings or create events!

if neo.characterMaxWalkSpeed < 400.0 then
	neo.characterMaxWalkSpeed = neo.characterMaxWalkSpeed+math.random(0.0, 4.0)
end
