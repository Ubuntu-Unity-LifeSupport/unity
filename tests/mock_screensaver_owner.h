// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2026 NeiroNext <mihail.rozshko@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TESTS_MOCK_SCREENSAVER_OWNER_H
#define TESTS_MOCK_SCREENSAVER_OWNER_H

#include <memory>
#include <vector>

#include <UnityCore/GLibDBusServer.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>

#include "test_utils.h"

namespace unity
{
namespace lockscreen
{

// Another screen locker: owns the name on a bus connection of its own, without
// allowing replacement, and records the calls it gets.
struct MockScreenSaverOwner
{
  typedef std::shared_ptr<MockScreenSaverOwner> Ptr;

  MockScreenSaverOwner(std::string const& name)
    : locks(0)
    , owns_name(false)
    , owner_id_(0)
  {
    static const std::string INTROSPECTION_XML =
    R"(<node>
      <interface name="org.gnome.ScreenSaver">
        <method name="Lock" />
        <method name="SetActive">
          <arg type="b" direction="in" name="value" />
        </method>
        <method name="SimulateUserActivity" />
        <signal name="ActiveChanged">
          <arg name="new_value" type="b" />
        </signal>
      </interface>
    </node>)";

    glib::String address(g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SESSION, nullptr, nullptr));
    auto flags = static_cast<GDBusConnectionFlags>(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                                   G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION);
    connection_ = g_dbus_connection_new_for_address_sync(address, flags, nullptr, nullptr, nullptr);

    object_ = std::make_shared<glib::DBusObject>(INTROSPECTION_XML, "org.gnome.ScreenSaver");
    object_->SetMethodsCallsHandler([this] (std::string const& method, GVariant* parameters) -> GVariant* {
      if (method == "Lock")
        ++locks;
      else if (method == "SetActive")
        set_active.push_back(glib::Variant(parameters).GetBool());

      return nullptr;
    });
    object_->Register(connection_, "/org/gnome/ScreenSaver");

    owner_id_ = g_bus_own_name_on_connection(connection_, name.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE,
      [] (GDBusConnection*, const gchar*, gpointer data) {
        static_cast<MockScreenSaverOwner*>(data)->owns_name = true;
      }, nullptr, this, nullptr);

    Utils::WaitUntilMSec(owns_name);
  }

  ~MockScreenSaverOwner()
  {
    Exit();
  }

  // The locker process goes away.
  void Exit()
  {
    if (!connection_)
      return;

    g_bus_unown_name(owner_id_);
    object_->UnRegister();
    g_dbus_connection_close_sync(connection_, nullptr, nullptr);
    connection_ = nullptr;
    owns_name = false;
  }

  void EmitActiveChanged(bool active)
  {
    object_->EmitSignal("ActiveChanged", g_variant_new("(b)", active ? TRUE : FALSE));
  }

  unsigned locks;
  std::vector<bool> set_active;
  bool owns_name;

private:
  glib::Object<GDBusConnection> connection_;
  glib::DBusObject::Ptr object_;
  guint owner_id_;
};

} // lockscreen
} // unity

#endif
