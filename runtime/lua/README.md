# Lua Runtime (Planned)

This folder hosts the BasaltOS Lua runtime integration path.

Current state:
- `lua_embed` component skeleton is present for build-graph integration.
- No Lua VM is embedded yet in this slice.
- Runtime dispatch reports Lua as not integrated until follow-on tasks land.

Build gating:
- Default build: Lua component disabled.
- Enable skeleton component with CMake option:
  - `idf.py -DBASALT_ENABLE_LUA_RUNTIME=ON build`
