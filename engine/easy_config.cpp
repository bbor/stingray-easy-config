#include <engine_plugin_api/plugin_api.h>
#include <plugin_foundation/platform.h>
#include <plugin_foundation/const_config.h>

#if _DEBUG
	#include <stdlib.h>
	#include <time.h>
#endif

namespace PLUGIN_NAMESPACE {

using namespace stingray_plugin_foundation;

// Engine APIs
LoggingApi *log = nullptr;
ResourceManagerApi* resource_manager = nullptr;
LuaApi *lua = nullptr;

// C Scripting API
namespace stingray {
	struct UnitCApi* Unit = nullptr;
	struct MeshCApi* Mesh = nullptr;
	struct MaterialCApi* Material = nullptr;
	struct DynamicScriptDataCApi* Data = nullptr;
}

/**
 * Returns the plugin name.
 */
const char* get_name() { return "easy-config"; }

static int has_data(struct lua_State *L)
{
	const char *name = lua->tolstring(L, 1, NULL);
	if (resource_manager->can_get("config", name))
	{
		lua->pushboolean(L, true);
		return 1;
	}
	lua->pushboolean(L, false);
	return 1;
}

void unserialize_value(const ConstConfigItem &item, lua_State *L);
void unserialize_array(const ConstConfigItem &item, lua_State *L)
{
	lua->createtable(L, 0, 0);
	for (int i = 0; i < item.size(); ++i) {
		const ConstConfigItem subitem = item[i];
		unserialize_value(subitem, L);
		lua->rawseti(L, -2, i + 1);
	}
}
void unserialize_object(const ConstConfigItem &item, lua_State *L)
{
	lua->createtable(L, 0, 0);
	for (int i = 0; i < item.n_items(); ++i) {
		const char *name;
		const ConstConfigItem subitem = item.item(i, &name);
		unserialize_value(subitem, L);
		lua->setfield(L, -2, name);
	}
}
void unserialize_value(const ConstConfigItem &item, lua_State *L)
{
	if (item.is_array())
		unserialize_array(item, L);
	else if (item.is_bool())
		lua->pushboolean(L, item.to_bool());
	else if (item.is_float() || item.is_integer())
		lua->pushnumber(L, item.to_float());
	else if (item.is_nil())
		lua->pushnil(L);
	else if (item.is_object())
		unserialize_object(item, L);
	else if (item.is_string())
		lua->pushstring(L, item.to_string());
}

static int get_data(struct lua_State *L)
{
	const char *name = lua->tolstring(L, 1, NULL);
	if (!resource_manager->can_get("config", name))
	{
		lua->lib_argerror(L, 1, "No config resource found with the specified name.");
		return 1;
	}

	const ConstConfigRoot* root = (const ConstConfigRoot*)resource_manager->get("config", name);
	ConstConfigItem root_item(*root);
	unserialize_value(root_item, L);

	return 1;
}

/**
 * Setup plugin runtime resources.
 */
void setup_plugin(GetApiFunction get_engine_api)
{
	resource_manager = (ResourceManagerApi*)get_engine_api(RESOURCE_MANAGER_API_ID);
	log = (LoggingApi*)get_engine_api(LOGGING_API_ID);
	lua = (LuaApi*)get_engine_api(LUA_API_ID);

	lua->add_module_function("EasyConfig", "has_data", has_data);
	lua->add_module_function("EasyConfig", "get_data", get_data);
	log->info("EasyConfig","EasyConfig is ready for business.");
}

}

extern "C" {

	/**
	 * Load and define plugin APIs.
	 */
	PLUGIN_DLLEXPORT void *get_plugin_api(unsigned api)
	{
		using namespace PLUGIN_NAMESPACE;

		if (api == PLUGIN_API_ID) {
			static PluginApi plugin_api = { nullptr };
			plugin_api.get_name = get_name;
			plugin_api.setup_game = setup_plugin;
			return &plugin_api;
		}
		return nullptr;
	}

}
