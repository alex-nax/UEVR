// dllmain
#include <windows.h>
#include <cstdint>
#include <ShlObj.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <utility/Thread.hpp>

#include "Framework.hpp"
#include "obse64_common/obse64_version.h"
#include "obse64_common/BranchTrampoline.h"
#include "obse64_common/Log.h"
#include "obse64/PluginAPI.h"

PluginHandle g_pluginHandle;
OBSEMessagingInterface* g_messaging = nullptr;

void startup_thread(HMODULE poc_module) {
    g_framework = std::make_unique<Framework>(poc_module);
	_MESSAGE("UEVR: framework initialized");
}

void OnOBSEMessage(OBSEMessagingInterface::Message* msg) {
    if (msg->type == OBSEMessagingInterface::kMessage_PostLoad) {
        _MESSAGE("UEVR: PostLoad");
    } else if (msg->type == OBSEMessagingInterface::kMessage_PostPostLoad) {
        _MESSAGE("UEVR: PostPostLoad");
    } else {
        _MESSAGE("UEVR: Unknown message type %u", msg->type);
    }

    DebugLog::flush();
}

extern "C" {
	__declspec(dllexport) OBSEPluginVersionData OBSEPlugin_Version =
	{
		OBSEPluginVersionData::kVersion,
		
		1,
		"UEVR Plugin",
		"Alex D",

		0,	// not address independent
		0,	// not structure independent
		{ RUNTIME_VERSION_0_411_140, 0 },	// compatible with 0.411.140 and that's it

		0,	// works with any version of the script extender. you probably do not need to put anything here
		0, 0, 0	// set these reserved fields to 0
	};

	 __declspec(dllexport) bool OBSEPlugin_Preload(const OBSEInterface* obse) {
		DebugLog::openRelative(CSIDL_MYDOCUMENTS, "\\My Games\\" SAVE_FOLDER_NAME "\\OBSE\\Logs\\obse64-uevr.txt");
		_MESSAGE("UEVR: preload");
		return true;
	 }

	 __declspec(dllexport) bool OBSEPlugin_Load(const OBSEInterface* obse) {
		 _MESSAGE("UEVR: loading plugin");

		 g_pluginHandle = obse->GetPluginHandle();

		 if (g_pluginHandle == kPluginHandle_Invalid) {
			 _MESSAGE("UEVR: Invalid plugin handle");
			 return false;
		 }

		 g_messaging = (OBSEMessagingInterface*)obse->QueryInterface(kInterface_Messaging);
		 g_messaging->RegisterListener(g_pluginHandle, "OBSE", OnOBSEMessage);

		if (!g_branchTrampoline.create(1024 * 128)) {
			return false;
		}

		auto moduleHandle = reinterpret_cast<void*>(GetModuleHandleA("UEVRBackend.dll"));

		if (!g_localTrampoline.create(1024 * 128, moduleHandle)) {
			_MESSAGE("UEVR: Failed to create local trampoline");
			return false;
		}

		_MESSAGE("UEVR: initializing startup thread");
		DebugLog::flush();
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)startup_thread, moduleHandle, 0, nullptr);
		return true;
     }
};
