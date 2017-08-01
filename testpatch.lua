local ctd = require "ctabledump"
local ctable = require "ctable"
local tree = require "tree"

local t1 = {
	a = {1},
	b = {2},
	c = {3},
}

local t2 = {
	-- remove a
	b = {1},
	c = {2},
	d = {3},
}

local b1 = ctd.dump(t1)
local b2 = ctd.dump(t2)


local function make_patch(b1, b2)
	local t1 = ctd.undump(b1)
	local t2 = ctd.undump(b2)
	local p = tree.diff(t1,t2)
	return ctd.dump(p)
end

local function apply_patch(b1, patch)
	local t1 = ctd.undump(b1)
	local p = ctd.undump(patch)
	local t2 = tree.patch(t1, p)
	return ctd.dump(t2)
end


local patch = make_patch(b1,b2)

print("PATCH size", #patch)

local b2 = apply_patch(b1, patch)

local t2 = ctd.undump(b2)

local function dump(t, prefix)
	for k,v in pairs(t) do
		print(prefix, k, v)
		if type(v) == "table" then
			dump(v, prefix .. "." .. k)
		end
	end
end

dump(t2, "[2]")





