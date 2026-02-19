-- Foundation sample for Lua filesystem binding checks.
basalt.system.log("lua_fs_demo: init")

local path = "/data/lua_fs_demo.txt"
local wrote = basalt.fs.write_text(path, "basalt lua fs demo\n")
if wrote ~= 0 then
  basalt.system.log("lua_fs_demo: write failed")
else
  local content = basalt.fs.read_text(path)
  basalt.system.log("lua_fs_demo: read=" .. tostring(content))
end
