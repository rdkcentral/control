/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2017-2020 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes made by Comcast
 * Copyright 2024 Comcast Cable Communications Management, LLC
 * Licensed under the Apache License, Version 2.0
 */

//
//  dbusutils.cpp
//

#include "dbusutils.h"
#include <string>

#include "ctrlm_log_ble.h"


using namespace std;

void parsePropertiesList(GVariant *variant, DBusPropertiesMap &propertiesList)
{
    GVariantIter interface_dict_entry_iter;
    GVariant *property_value;
    gchar *property_name;

    g_variant_iter_init (&interface_dict_entry_iter, variant);
    while (g_variant_iter_next (&interface_dict_entry_iter, "{sv}", &property_name, &property_value)) {
        // gchar *result_str = g_variant_print(property_value, false);
        // XLOGD_DEBUG("property_name = <%s>, property_value =  <%s>", property_name, result_str);
        // g_free(result_str);

        // Don't unref property_value after using it here, because its managed in DBusVariant class, which will ref
        // and unref properly in constructors/destructors
        propertiesList[string(property_name)] = std::move(DBusVariant(string(property_name), property_value));
        g_free(property_name);
    }
}
