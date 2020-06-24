#include <iostream>
#include <vector>
#include <exception>
#include <windows.h>
#include <wlanapi.h>

#ifndef UNICODE
#define UNICODE
#endif


#define HANDLE_ERROR(fn) \
  switch (dwResult) { \
    case ERROR_SUCCESS: \
      break;\
    \
    case ERROR_INVALID_PARAMETER:\
      throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) +\
      "\tERROR_INVALID_PARAMETER!");\
    \
    case ERROR_NOT_ENOUGH_MEMORY:\
      throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) +\
      "\tERROR_NOT_ENOUGH_MEMORY!");\
    \
    case ERROR_REMOTE_SESSION_LIMIT_EXCEEDED:\
      throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) +\
      "\tERROR_REMOTE_SESSION_LIMIT_EXCEEDED!");\
    \
    default:\
      throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) +\
      "\tERR CODE: " + std::to_string(dwResult) + '!');\
  }

std::pair<HANDLE, GUID> select_interface();

void scan(HANDLE, GUID);

int main() {
  std::ios::sync_with_stdio(false);

  auto[hClientHandle, guid] = select_interface();
  for (;;) {
    scan(hClientHandle, guid);
    Sleep(1000);
  }

  return 0;
}

std::pair<HANDLE, GUID> select_interface() {
  DWORD dwResult;

// Initialise the Handle
  HANDLE hClientHandle = NULL;
  {
    DWORD dwClientVersion      = 2;
    PVOID pReserved            = NULL;
    DWORD pdwNegotiatedVersion = 0;

    dwResult = WlanOpenHandle(
            dwClientVersion, pReserved, &pdwNegotiatedVersion, &hClientHandle);

    HANDLE_ERROR("WlanOpenHandle")
  }

// Get the Interface List
  PWLAN_INTERFACE_INFO_LIST pInterfaceList = NULL;
  GUID                      chosen_guid;
  {
    PVOID pReserved = NULL;

    dwResult = WlanEnumInterfaces(hClientHandle, pReserved, &pInterfaceList);

    HANDLE_ERROR("WlanEnumInterfaces")

    WCHAR GuidString[40] = {0};
    auto  interface_num  = pInterfaceList->dwNumberOfItems;

    for (DWORD i = 0; i < interface_num; i++) {
      auto pIfInfo = &pInterfaceList->InterfaceInfo[i];

      // GuidString
      {
        auto iRet = StringFromGUID2(
                pIfInfo->InterfaceGuid,
                reinterpret_cast<LPOLESTR>( &GuidString),
                sizeof(GuidString) / sizeof(WCHAR) - 1);

        if (iRet == 0)
          std::cout << "StringFromGUID2 failed\n";
        else
          std::wcout << "  Interface GUID[" << i << "]: " << GuidString << '\n';
      }

      std::wcout << "  Interface Description[" << i << "]: "
                 << pIfInfo->strInterfaceDescription << '\n' << std::flush;
      std::cout << "  Interface State[" << i << "]:\t ";

      switch (pIfInfo->isState) {
        case wlan_interface_state_not_ready:
          std::cout << "Not ready\n";
          break;
        case wlan_interface_state_connected:
          std::cout << "Connected\n";
          break;
        case wlan_interface_state_ad_hoc_network_formed:
          std::cout << "First node in a ad hoc network\n";
          break;
        case wlan_interface_state_disconnecting:
          std::cout << "Disconnecting\n";
          break;
        case wlan_interface_state_disconnected:
          std::cout << "Not connected\n";
          break;
        case wlan_interface_state_associating:
          std::cout << "Attempting to associate with a network\n";
          break;
        case wlan_interface_state_discovering:
          std::cout << "Auto configuration is discovering settings for the network\n";
          break;
        case wlan_interface_state_authenticating:
          std::cout << "In process of authenticating\n";
          break;
        default:
          std::wcout << "Unknown state: " << pIfInfo->isState << '\n';
          break;
      }
      std::cout << '\n';
    }

    if (interface_num == 1)
      chosen_guid = (&pInterfaceList->InterfaceInfo[0])->InterfaceGuid;
    else {
      std::cout << "Select an interface (0~" << interface_num - 1 << "):";
      DWORD i;
      std::cin >> i;
      chosen_guid = (&pInterfaceList->InterfaceInfo[i])->InterfaceGuid;
    }
  }

// Free up Memory
  if (pInterfaceList) {
    WlanFreeMemory(pInterfaceList);
    pInterfaceList = NULL;
  }

  return std::make_pair(hClientHandle, chosen_guid);
}

void scan(HANDLE hClientHandle, GUID chosen_guid) {
  DWORD dwResult;

// Scan
  {
    GUID           *pInterfaceGuid = &chosen_guid;
    PDOT11_SSID    pDot11Ssid      = NULL;
    PWLAN_RAW_DATA pIeData         = NULL;
    PVOID          pReserved       = NULL;

    dwResult = WlanScan(hClientHandle, pInterfaceGuid, pDot11Ssid, pIeData, pReserved);

    HANDLE_ERROR("WlanScan")
  }

// Get list
  PWLAN_AVAILABLE_NETWORK_LIST pAvailableNetworkList = NULL;
  {
    DWORD WLAN_AVAILABLE_NETWORK_INCLUDE_ALL_MANUAL_HIDDEN_PROFILES = 0x00000002;

    GUID                         *pInterfaceGuid         = &chosen_guid;
    DWORD                        dwFlags                 = WLAN_AVAILABLE_NETWORK_INCLUDE_ALL_MANUAL_HIDDEN_PROFILES;
    PVOID                        pReserved               = NULL;
    PWLAN_AVAILABLE_NETWORK_LIST *ppAvailableNetworkList = &pAvailableNetworkList;

    dwResult = WlanGetAvailableNetworkList(hClientHandle, pInterfaceGuid, dwFlags,
                                           pReserved, ppAvailableNetworkList);

    HANDLE_ERROR("WlanGetAvailableNetworkList")

    using WlanVecType = std::pair<std::string, uint8_t>;
    std::vector<WlanVecType> wlans;

    for (DWORD i = 0; i < pAvailableNetworkList->dwNumberOfItems; i++) {
      auto pBssEntry = &pAvailableNetworkList->Network[i];

      std::string ssid;
      {
        for (DWORD j = 0; j < pBssEntry->dot11Ssid.uSSIDLength
                && pBssEntry->dot11Ssid.ucSSID[j]; j++)
          ssid += static_cast<char> ( pBssEntry->dot11Ssid.ucSSID[j]);

        if (ssid.empty())
          ssid = "<hidden-ssid>";
      }

      wlans.emplace_back(std::move(ssid), pBssEntry->wlanSignalQuality);
    }

    std::sort(wlans.begin(), wlans.end(),
              [](const WlanVecType &a, const WlanVecType &b) {
                return a.second < b.second;
              }
    );

    for (const auto &x: wlans)
      std::cout << +x.second << "%\t| " << x.first << '\n';

    std::cout << "--------" << std::endl;
  }

// Free up Memory
  if (pAvailableNetworkList) {
    WlanFreeMemory(pAvailableNetworkList);
    pAvailableNetworkList = NULL;
  }
}
