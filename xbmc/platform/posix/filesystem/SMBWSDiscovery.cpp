/*
 *  Copyright (C) 2021- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SMBWSDiscovery.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "network/IWSDiscovery.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include "platform/posix/filesystem/SMBWSDiscoveryListener.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace std::chrono;
using namespace WSDiscovery;

namespace WSDiscovery
{
std::unique_ptr<IWSDiscovery> IWSDiscovery::GetInstance()
{
  return std::make_unique<WSDiscovery::CWSDiscoveryPosix>();
}

CWSDiscoveryPosix::CWSDiscoveryPosix()
{
  // Set our wsd_instance ID to seconds since epoch
  auto epochduration = system_clock::now().time_since_epoch();
  wsd_instance_id = epochduration.count() * system_clock::period::num / system_clock::period::den;

  m_WSDListenerUDP = std::make_unique<CWSDiscoveryListenerUDP>();
}

CWSDiscoveryPosix::~CWSDiscoveryPosix()
{
  StopServices();
}

bool CWSDiscoveryPosix::StopServices()
{
  m_WSDListenerUDP->Stop();

  return true;
}

bool CWSDiscoveryPosix::StartServices()
{
  m_WSDListenerUDP->Start();

  return true;
}

bool CWSDiscoveryPosix::IsRunning()
{
  return m_WSDListenerUDP->IsRunning();
}

bool CWSDiscoveryPosix::GetServerList(CFileItemList& items)
{
  {
    CSingleLock lock(m_critWSD);

    for (const auto& item : m_vecWSDInfo)
    {
      auto found = item.computer.find('/');
      std::string host;
      if (found != std::string::npos)
        host = item.computer.substr(0, found);
      else
      {
        if (item.xaddrs_host.empty())
          continue;
        host = item.xaddrs_host;
      }

      CFileItemPtr pItem = std::make_shared<CFileItem>(host);
      pItem->SetPath("smb://" + host + '/');
      pItem->m_bIsFolder = true;
      items.Add(pItem);
    }
  }
  return true;
}

void CWSDiscoveryPosix::SetItems(std::vector<wsd_req_info> entries)
{
  {
    CSingleLock lock(m_critWSD);
    m_vecWSDInfo = std::move(entries);
  }
}
} // namespace WSDiscovery
