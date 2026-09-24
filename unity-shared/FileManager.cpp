// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2022 Canonical Ltd
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
 *
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "FileManager.h"

#include <UnityCore/GLibWrapper.h>

#include "GnomeFileManager.h"
#include "NemoFileManager.h"

#include <gio/gio.h>

namespace unity
{

FileManager::Ptr FileManager::GetDefault()
{
  /* Some logic to choose the default */
  static FileManager::Ptr fm;

  if (!fm) {
    glib::Object<GAppInfo> directory_app(g_app_info_get_default_for_type("inode/directory", TRUE));

    if (directory_app)
    {
      auto app_id = glib::gchar_to_string(g_app_info_get_id (directory_app));

      if (app_id == "org.gnome.Nautilus.desktop")
        fm = GnomeFileManager::Get();
    }

    // Nemo when there is no default, or one Unity has no support for:
    // callers take the result as never empty.
    if (!fm)
      fm = NemoFileManager::Get();
  }

  return fm;
}

} // namespace unity
