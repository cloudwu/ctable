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

----------------

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

local diff = ctd.diff(b1,b2)

t1 = ctd.undump(b1)
t2 = ctd.undump(b2)
dump(t1, "[1]")
dump(t2, "[2]")
local t = ctd.undump(diff)

assert(eq(t2,t))

dump(t, "DIFF")

local o1 = ctable.new(b1)
local b = o1.b
local a = o1.a
print(".a[1]=", a[1])
print(".b[1]=", b[1])
ctable.update(b, diff)
print("update.b[1]=", b[1])
print("update.d[1]=", o1.d[1])

local function geta1()
	return a[1]
end
assert(pcall(geta1) == false) -- a is removed


