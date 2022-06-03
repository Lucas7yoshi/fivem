#include <StdInc.h>
#include <Hooking.h>

#include <CoreConsole.h>

#include <ScriptEngine.h>
#include <ScriptSerialization.h>

#include <Resource.h>
#include <fxScripting.h>
#include <ICoreGameInit.h>
#include <rageVectors.h>
#include <MinHook.h>

static uint32_t(*initVehicleArchetype)(const char* name, bool a2, unsigned int a3);

static void(*g_origArchetypeDtor)(fwArchetype* at);

static std::unordered_map<uint32_t, std::string> g_vehiclesHashes;
static std::vector<std::string> g_vehicles;

static uint32_t initVehicleArchetype_stub(const char* name, bool a2, unsigned int a3) 
{	
	// check if name already exists in g_vehicles, if so print a warning
	if (g_vehicles.end() != std::find(g_vehicles.begin(), g_vehicles.end(), name))
	{
		//trace("[VehicleArchetype] Warning: Vehicle archetype %s already exists!\n", name);
		console::PrintWarning("extra-natives-five", "Vehicle archetype %s already was registered!\n", name);
	}
	else
	{
		g_vehicles.emplace_back(name);
		//console::Printf("extra-natives-five", "Added vehicle archetype %s\n", name);
	}
	return initVehicleArchetype(name, a2, a3);
}

static void ArchetypeDtorHook(fwArchetype* at)
{
	//at->assetType is probably relevant here, but I don't know what it is for vehicles
	auto hash = HashString("foobar");
	for (auto& veh : g_vehicles)
	{
		if (HashString(veh.c_str()) == at->hash)
		{
			g_vehicles.erase(std::remove(g_vehicles.begin(), g_vehicles.end(), veh), g_vehicles.end());
			console::Printf("extra-natives-five", "Removed vehicle %s from g_vehicles\n", veh.c_str());
			break;
		}
	}
	g_origArchetypeDtor(at);
}

static HookFunction hookFunction([]() 
{
	//hook for vehicle registering
	{
		console::Printf("extra-natives-five", "hooking vehicle thing\n");
		auto location = hook::get_pattern<char>("E8 ? ? ? ? 48 8B 4D E0 48 8B 11");
		hook::set_call(&initVehicleArchetype, location);
		hook::call(location, initVehicleArchetype_stub);
		console::PrintWarning("extra-natives-five", fmt::sprintf("Hook location is %s\n", location));
	}
	// hook to catch registration of vehicles
	{
		auto location = hook::get_pattern("E8 ? ? ? ? 80 7B 60 01 74 39");
		hook::set_call(&g_origArchetypeDtor, location);
		hook::call(location, ArchetypeDtorHook);
	}
	{
		fx::ScriptEngine::RegisterNativeHandler("GET_ALL_VEHICLE_MODELS", [](fx::ScriptContext& context)
		{
			console::PrintWarning("extra-natives-five", "Native is running??\n");
			context.SetResult(fx::SerializeObject(g_vehicles));
		});
	}
});
