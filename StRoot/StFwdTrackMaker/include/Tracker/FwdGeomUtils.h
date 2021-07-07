#ifndef FWD_GEOM_UTILS_H
#define FWD_GEOM_UTILS_H

#include "TGeoVolume.h"
#include "TGeoNode.h"
#include "TGeoMatrix.h"
#include "TGeoNavigator.h"

class FwdGeomUtils {
    public:

        FwdGeomUtils( TGeoManager * gMan ) {
            if ( gMan != nullptr )
                _navigator = gMan->AddNavigator();
        }

        ~FwdGeomUtils(){

        }

        bool cd( const char* path ){
            // Change to the specified path
            bool ret = _navigator -> cd(path);
            // If successful, set the node, the volume, and the GLOBAL transformation
            // for the requested node.  Otherwise, invalidate these
            if ( ret ) {
                _matrix = _navigator->GetCurrentMatrix();
                _node   = _navigator->GetCurrentNode();
                _volume = _node->GetVolume();
            } else {
                _matrix = 0; _node = 0; _volume = 0;
            }
            return ret;
        }

        double stgcZ( int index ) {

            stringstream spath;
            spath << "/HALL_1/CAVE_1/STGM_1/TGCP_" << (index + 1) * 8 << "/"; 
            // 0 -> 8
            // 1 -> 16
            // 2 -> 24
            // 3 -> 32
            bool can = cd( spath.str().c_str() );
            if ( can && _matrix != nullptr ){
                return _matrix->GetTranslation()[2];
            }
            return 0.0;
        }

        double siZ( int index ) {

            stringstream spath;
            spath << "/HALL_1/CAVE_1/FTSM_1/FTSD_" << (index + 1) << "/"; 
            bool can = cd( spath.str().c_str() );
            if ( can && _matrix != nullptr ){
                return _matrix->GetTranslation()[2];
            }
            return 0.0;
        }

    protected:
    TGeoVolume    *_volume    = nullptr;
    TGeoNode      *_node      = nullptr;
    TGeoHMatrix   *_matrix    = nullptr;
    TGeoIterator  *_iter      = nullptr;
    TGeoNavigator *_navigator = nullptr;
};

#endif