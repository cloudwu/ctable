#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>

#define NODECACHE "_ctable"
#define TABLES "_ctables"

#define VALUE_NIL 0
#define VALUE_INTEGER 1
#define VALUE_REAL 2
#define VALUE_BOOLEAN 3
#define VALUE_TABLE 4
#define VALUE_STRING 5
#define VALUE_INVALID 6

struct proxy {
	const char * data;
	int index;
};

struct document {
	uint32_t strtbl;
	uint32_t n;
	uint32_t index[1];
	// table[n]
	// strings
};

struct table {
	uint32_t array;
	uint32_t dict;
	uint8_t type[1];
	// value[array]
	// kvpair[dict]
};

static inline const struct table *
gettable(const struct document *doc, int index) {
	return (const struct table *)((const char *)doc + sizeof(uint32_t) + sizeof(uint32_t) + doc->n * sizeof(uint32_t) + doc->index[index]);
}

static void
create_proxy(lua_State *L, const void *data, int index) {
	const struct table * t = gettable(data, index);
	lua_getfield(L, LUA_REGISTRYINDEX, NODECACHE);
	if (lua_rawgetp(L, -1, t) == LUA_TTABLE) {
		lua_replace(L, -2);
		return;
	}
	lua_pop(L, 1);
	lua_newtable(L);
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_setmetatable(L, -2);
	lua_pushvalue(L, -1);
	// NODECACHE, table, table
	lua_rawsetp(L, -3, t);
	lua_pushvalue(L, -1);
	struct proxy * p = lua_newuserdata(L, sizeof(struct proxy));
	p->data = data;
	p->index = index;
	// NODECACHE, table, table, proxy
	lua_rawset(L, -4);
	lua_replace(L, -2);
	// table
}

static inline uint32_t
getuint32(const void *v) {
	union {
		uint32_t d;
		uint8_t t[4];
	} test = { 1 };
	if (test.t[0] == 0) {
		// big endian
		test.d = *(const uint32_t *)v;
		return test.t[0] | test.t[1] << 4 | test.t[2] << 8 | test.t[3] << 12;
	} else {
		return *(const uint32_t *)v;
	}
}

static inline float
getfloat(const void *v) {
	union {
		uint32_t d;
		float f;
		uint8_t t[4];
	} test = { 1 };
	if (test.t[0] == 0) {
		// big endian
		test.d = *(const uint32_t *)v;
		test.d = test.t[0] | test.t[1] << 4 | test.t[2] << 8 | test.t[3] << 12;
		return test.f;
	} else {
		return *(const float *)v;
	}
}

static void
pushvalue(lua_State *L, const void *v, int type, const struct document * doc) {
	switch (type) {
	case VALUE_NIL:
		lua_pushnil(L);
		break;
	case VALUE_INTEGER:
		lua_pushinteger(L, (int32_t)getuint32(v));
		break;
	case VALUE_REAL:
		lua_pushnumber(L, getfloat(v));
		break;
	case VALUE_BOOLEAN:
		lua_pushboolean(L, getuint32(v));
		break;
	case VALUE_TABLE:
		create_proxy(L, doc, getuint32(v));
		break;
	case VALUE_STRING:
		lua_pushstring(L,  (const char *)doc + doc->strtbl + getuint32(v));
		break;
	default:
		luaL_error(L, "Invalid type %d at %p", type, v);
	}
}

static void
copytable(lua_State *L, int tbl, struct proxy *p) {
	const struct document * doc = (const struct document *)p->data; 
	if (p->index < 0 || p->index >= doc->n) {
		luaL_error(L, "Invalid index %d (%d)", p->index, (int)doc->n);
	}
	const struct table * t = gettable(doc, p->index);
	const uint32_t * v = (const uint32_t *)((const char *)t + sizeof(uint32_t) + sizeof(uint32_t) + ((t->array + t->dict + 3) & ~3));
	int i;
	for (i=0;i<t->array;i++) {
		pushvalue(L, v++, t->type[i], doc);
		lua_rawseti(L, tbl, i+1);
	}
	for (i=0;i<t->dict;i++) {
		pushvalue(L, v++, VALUE_STRING, doc);
		pushvalue(L, v++, t->type[t->array+i], doc);
		lua_rawset(L, tbl);
	}
}

static int
lnew(lua_State *L) {
	const char * data = luaL_checkstring(L, 1);
	lua_getfield(L, LUA_REGISTRYINDEX, TABLES);
	lua_pushvalue(L, 1);
	lua_rawsetp(L, -2, data);

	create_proxy(L, data, 0);
	return 1;
}

static void
copyfromdata(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, NODECACHE);
	lua_pushvalue(L, 1);
	// NODECACHE, table
	if (lua_rawget(L, -2) != LUA_TUSERDATA) {
		luaL_error(L, "Invalid proxy table %p", lua_topointer(L, 1));
	}
	struct proxy * p = lua_touserdata(L, -1);
	lua_pop(L, 2);
	copytable(L, 1, p);
	lua_pushnil(L);
	lua_setmetatable(L, 1);	// remove metatable
}

static int
lindex(lua_State *L) {
	copyfromdata(L);
	lua_rawget(L, 1);
	return 1;
}

static int
lnext(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 2);  /* create a 2nd argument if there isn't one */
	if (lua_next(L, 1))
		return 2;
	else {
		lua_pushnil(L);
		return 1;
	}
}

static int
lpairs(lua_State *L) {
	copyfromdata(L);
	lua_pushcfunction(L, lnext);
	lua_pushvalue(L, 1);
	lua_pushnil(L);
	return 3;
}

static int
llen(lua_State *L) {
	copyfromdata(L);
	lua_pushinteger(L, lua_rawlen(L, 1));
	return 1;
}

LUAMOD_API int
luaopen_ctable(lua_State *L) {
	luaL_checkversion(L);

	lua_newtable(L);	// cache
	lua_createtable(L, 0, 1);	// weak meta table
	lua_pushstring(L, "kv");
	lua_setfield(L, -2, "__mode");
	lua_setmetatable(L, -2);	// make NODECACGE weak
	lua_setfield(L, LUA_REGISTRYINDEX, NODECACHE);

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, TABLES);

	lua_createtable(L, 0, 1);	// mod table

	lua_createtable(L, 0, 2);	// metatable
	luaL_Reg l[] = {
		{ "__index", lindex },
		{ "__pairs", lpairs },
		{ "__len", llen },
		{ NULL, NULL },
	};
	lua_pushvalue(L, -1);
	luaL_setfuncs(L, l, 1);
	lua_pushcclosure(L, lnew, 1);
	lua_setfield(L, -2, "new");
	return 1;
}
