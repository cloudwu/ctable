local ctd = require "ctabledump"
local ctable = require "ctable"

local function eq(t1,t2)
	local n = 0
	for k1,v1 in pairs(t1) do
		local v2 = t2[k1]
		if v1 ~= v2 then
			local t1 = type(v1)
			local t2 = type(v2)
			if t1 ~= t2 then
				return false
			end
			if t1 == "table" then
				if not eq(v1,v2) then
					return false
				end
			else
				return false
			end
		end
		n = n + 1
	end
	for k2, v2 in pairs(t2) do
		n = n - 1
	end
	return n == 0
end

local function dump(t, prefix)
	for k,v in pairs(t) do
		print(prefix, k, v)
		if type(v) == "table" then
			dump(v, prefix .. "." .. k)
		end
	end
end

local t = {
	a = {1,2,3},
	b = 4,
	c = { x = 5, y = 6 },
	d = true,
}

local d = ctd.dump(t)
local t2 = ctd.undump(d)

assert(eq(t,t2))

local t3 = ctable.new(d)

print("#.a", #t3.a)

dump(t3,"")
