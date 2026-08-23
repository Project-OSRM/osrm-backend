-- Foot profile with walking paths across open areas enabled.
--
-- Everything else comes from foot.lua. This file only flips the opt-in switch,
-- so the two profiles cannot drift apart the way they did while this was a
-- separate copy. See docs/areas.md.

enable_area_meshing = true

return require('foot')
