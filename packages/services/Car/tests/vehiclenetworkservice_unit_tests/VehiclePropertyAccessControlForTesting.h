#ifndef CAR_VEHICLE_PROPERTY_ACCESS_CONTROL_MOCK_H_
#define CAR_VEHICLE_PROPERTY_ACCESS_CONTROL_MOCK_H_

#include <utils/String8.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <map>
#include <string>
#include <private/android_filesystem_config.h>
#include <vehicle-internal.h>
#include <VehiclePropertyAccessControl.h>

namespace android {
class VehiclePropertyAccessControlForTesting : public VehiclePropertyAccessControl {
public:
    bool isHexNotation(std::string const& s);
    bool accessToInt(int32_t* const value,const xmlChar* property,
                   const xmlChar* uid, const xmlChar* access);
    bool updateOrCreate(int32_t uid, int32_t property, int32_t access);
    bool populate(xmlNode* a_node);
    bool process(const char* policy);
    void emptyAccessControlMap();
    bool getAccessToProperty(int32_t property, std::map<int32_t, int32_t>** accessMap);
};
};
#endif
