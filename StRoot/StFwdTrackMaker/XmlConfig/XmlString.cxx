#include "XmlString.h"
#include "XmlConfig.h"


namespace jdb
{

XmlString::XmlString() {}
XmlString::~XmlString() {}

string XmlString::TOKEN_START = "{";
string XmlString::TOKEN_STOP = "}";


bool XmlString::replace_token( const XmlConfig &cfg, string &_s, string &_key, int index, int len, bool _clobber )
{
   // LOG_DEBUG << classname() << cfg.getFilename() << ", _s=" << _s << ", _key=" << _key << ", index=" << index << ", len=" << len << ", _clobber=" << bts( _clobber ) << endm;
   if ( kv.count( _key ) >= 1 ) {
      // LOG_DEBUG << classname() << "map" << endm;
      string rv = kv[ _key ];
      _s.replace( index, len, rv );
      return true;
   }
   else if ( getenv( _key.c_str() ) ) {
      // LOG_DEBUG << classname() << "ENV" << endm;
      string env = getenv( _key.c_str() );
      _s.replace( index, len, env );
      return true;
   }
   else if ( cfg.exists( _key ) ) {
      // LOG_DEBUG << classname() << "CFG" << endm;
      string rv = cfg.getXString( _key ); // careful this could cause infinite recursion
      _s.replace( index, len, rv );
      return true;
   }
   else if ( _clobber ) {
      // LOG_DEBUG << classname() << "CLOBBER" << endm;
      _s.replace( index, len, "" );
      return true;
   }

   return false;
}

bool XmlString::replace_token( string &_s, string &_key, int index, int len, bool _clobber )
{

   if ( kv.count( _key ) >= 1 ) {
      string rv = kv[ _key ];
      _s.replace( index, len, rv );
      return true;
   }
   else if ( getenv( _key.c_str() ) ) {
      string env = getenv( _key.c_str() );
      _s.replace( index, len, env );
      return true;
   }
   else if ( _clobber ) {
      _s.replace( index, len, "" );
      return true;
   }

   return false;
}
}
