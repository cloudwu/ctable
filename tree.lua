local tree = {}

local NIL = "__NIL"

local function copy(t)
	if type(t) == "table" then
		local r = {}
		for k,v in pairs(t) do
			r[k] = copy(v)
		end
		return r
	elseif t == NIL then
		return nil
	else
		return t
	end
end

local function diff(a,b,result)
	for k,v2 in pairs(b) do
		local v1 = a[k]
		if v1 ~= v2 then
			if type(v1) == "table" then
				if type(v2) == "table" then
					result[k] = diff(v1, v2, {})
				else
					result[k] = v2
				end
			else
				result[k] = copy(v2)
			end
		end
	end
	for k,v in pairs(a) do
		if b[k] == nil then
			result[k] = NIL
		end
	end
	return result
end

function tree.diff(a,b)
	return diff(a,b,{})
end

local function patch(base, p)
	for k,v in pairs(p) do
		if v == NIL then
			base[k] = nil
		elseif type(v) == "table" then
			local ov = base[k]
			if type(ov) == "table" then
				patch(ov, v)
			else
				base[k] = copy(v)
			end
		else
			base[k] = v
		end
	end
end

function tree.patch(a, p)
	patch(a,p)
	return a
end

return tree
