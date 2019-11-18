#ifndef CONFIG_UTIL_H
#define CONFIG_UTIL_H

#include "XmlConfig.h"
#include "TString.h"

namespace jdb
{
template <>
TString XmlConfig::get<TString>( string path ) const
{
   TString r( getString( path ) );
   return r;
}

template <>
TString XmlConfig::get<TString>( string path, TString dv ) const
{
   if ( !exists( path ) )
      return dv;

   TString r( getString( path ) );
   return r;
}
}
#endif
