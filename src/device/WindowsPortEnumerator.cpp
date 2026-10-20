#include "device/WindowsPortEnumerator.h"

#include "device/UsbHardwareId.h"

#include <windows.h>
#include <initguid.h>
#include <devguid.h>
#include <setupapi.h>

#include <optional>
#include <string>

namespace coral {
namespace {

constexpr DWORD kMaxPropertyChars = 1024;
constexpr DWORD kPropertyBufferBytes = (kMaxPropertyChars - 1) * sizeof(wchar_t);
constexpr std::string_view kComPrefix = "COM";

std::string narrowAscii(const wchar_t* text) {
    std::string result;
    for (; *text != L'\0'; ++text) result.push_back(static_cast<char>(*text));
    return result;
}

// First entry of the REG_MULTI_SZ hardware-id list is the most specific id.
std::optional<std::string> readHardwareId(HDEVINFO deviceSet, SP_DEVINFO_DATA& device) {
    wchar_t buffer[kMaxPropertyChars] = {};
    if (!SetupDiGetDeviceRegistryPropertyW(deviceSet, &device, SPDRP_HARDWAREID, nullptr,
                                           reinterpret_cast<PBYTE>(buffer), kPropertyBufferBytes,
                                           nullptr)) {
        return std::nullopt;
    }
    return narrowAscii(buffer);
}

std::optional<std::string> readPortName(HDEVINFO deviceSet, SP_DEVINFO_DATA& device) {
    HKEY key = SetupDiOpenDevRegKey(deviceSet, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if (key == INVALID_HANDLE_VALUE) return std::nullopt;
    wchar_t buffer[kMaxPropertyChars] = {};
    DWORD size = kPropertyBufferBytes;
    DWORD type = 0;
    const LONG status = RegQueryValueExW(key, L"PortName", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || type != REG_SZ) return std::nullopt;
    std::string name = narrowAscii(buffer);
    if (!name.starts_with(kComPrefix)) return std::nullopt;
    return name;
}

class DeviceInfoSet {
public:
    DeviceInfoSet() : handle_(SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT)) {}
    ~DeviceInfoSet() {
        if (isValid()) SetupDiDestroyDeviceInfoList(handle_);
    }
    DeviceInfoSet(const DeviceInfoSet&) = delete;
    DeviceInfoSet& operator=(const DeviceInfoSet&) = delete;
    bool isValid() const { return handle_ != INVALID_HANDLE_VALUE; }
    HDEVINFO handle() const { return handle_; }

private:
    HDEVINFO handle_;
};

}  // namespace

Result<std::vector<SerialPortInfo>> WindowsPortEnumerator::listPorts() {
    using PortsResult = Result<std::vector<SerialPortInfo>>;
    DeviceInfoSet deviceSet;
    if (!deviceSet.isValid()) {
        return PortsResult::failure("SetupDiGetClassDevs failed, Windows error " + std::to_string(GetLastError()));
    }
    std::vector<SerialPortInfo> ports;
    SP_DEVINFO_DATA device{};
    device.cbSize = sizeof(device);
    for (DWORD index = 0; SetupDiEnumDeviceInfo(deviceSet.handle(), index, &device); ++index) {
        const auto portName = readPortName(deviceSet.handle(), device);
        if (!portName) continue;  // not a COM port (e.g. LPT): nothing to report
        SerialPortInfo info{*portName, 0, 0};
        if (const auto hardwareId = readHardwareId(deviceSet.handle(), device)) {
            if (const auto usb = parseUsbHardwareId(*hardwareId)) {
                info.vendorId = usb->vendorId;
                info.productId = usb->productId;
            }
        }
        ports.push_back(info);
    }
    return PortsResult::success(std::move(ports));
}

}  // namespace coral