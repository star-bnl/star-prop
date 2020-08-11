#ifndef ConfigUtil_h
#define ConfigUtil_h

#include "TString.h"
#include "XmlConfig.h"

namespace jdb {
template <>
TString XmlConfig::get<TString>(string path) const {
    TString r(getString(path));
    return r;
}

template <>
TString XmlConfig::get<TString>(string path, TString dv) const {
    if (!exists(path))
        return dv;

    TString r(getString(path));
    return r;
}
} // namespace jdb
#endif
