#include "Geode/ui/Notification.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/MenuLayer.hpp>


static bool detect_wine() {
	HMODULE hntdll = GetModuleHandle("ntdll.dll");
	
	if (GetProcAddress(hntdll, "wine_get_version")) {
		return true;
	}

	return false;
}

enum ContainerType {
	NATIVE,
	FLATPAK,
	APPIMAGE,
	SNAP
};

static ContainerType detect_container() {
    if (getenv("container")) {
        return FLATPAK;
    } else if (getenv("APPIMAGE")) {
        return APPIMAGE;
    } else if (getenv("SNAP")) {
        return SNAP;
    }
    return NATIVE;
}


class $modify(DRPCLinux, MenuLayer) {
	struct Fields {
		std::string container_name = "Native Repo";
    };

	void showWineError() {
		FLAlertLayer::create(
			"Sniff sniff", 
			"It doesn't smell like Wine / Proton...",
			"OK"
		)->show();
	}

	void showContainerWarning() {
		FLAlertLayer::create(
			"Containerized!", 
			fmt::format(
				"<co>Linux DRPC:</c> It looks like you're running the compatibility layer from <cg>{}</c>.\n\n",
				this->m_fields->container_name
			) +
			std::string("Please make sure that Steam / Wine has the <cy>appropriate permissions</c>."), 
			"OK"
		)->show();
	}

	bool init() {
		if (!MenuLayer::init()) {
			return false;
		}

		static bool shown_wine_notification = false;
		auto running_under_wine = detect_wine();

		if (!running_under_wine) {
			if (!shown_wine_notification) {
				auto wine_error_call = CCCallFunc::create(
					this, callfunc_selector(DRPCLinux::showWineError)
				);
				this->runAction(wine_error_call);
				shown_wine_notification = true;
			}
			return true;
		}

		static bool shown_container_notification = false;
		auto container_type = detect_container();

		if (container_type != NATIVE && !shown_container_notification) {
			this->m_fields->container_name = 
			container_type == FLATPAK 
					? "Flatpak" 
					: (
						container_type == APPIMAGE 
						? "AppImage" 
						: "Snap"
					);
			auto wine_error_call = CCCallFunc::create(
				this, callfunc_selector(DRPCLinux::showContainerWarning)
			);
			this->runAction(wine_error_call);
			shown_container_notification = true;
		}

		std::string path = CCFileUtils::get()->fullPathForFilename("winediscordipcbridge.exe"_spr, true);

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		if (!CreateProcess(path.c_str(), NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
			Notification::create("Failed to create Discord IPC Bridge Process", NotificationIcon::Error)->show();
			return true;
		}

		return true;
	}
};