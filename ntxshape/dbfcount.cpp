/**
 * The contents of this file are subject to the Mozilla Public License 
 * Version 1.1 (the "License"); you may not use this file except in 
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" 
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See 
 * the License for the specific language governing rights and limitations 
 * under the License.
 *
 * The Original Code is NTXShape - NTX to Shapefile Converter.
 *
 * The Initial Developer of the Original Code is ESRI Canada Limited.
 * Portions created by ESRI Canada are Copyright (C) 1997-2003 
 * ESRI Canada Limited.  All Rights Reserved.
 *
 * Contributor(s):
 *  Bruce Dodson <bdodson@esricanada.com>
**/

#include <iostream.h>
#include "dbfout.h"

int main(int argc, char**argv) {
    if (argc > 1) {
        DBaseTable dbf;
        if (dbf.Open(argv[1],om_read)) {
            cout << dbf.NumRecords();
            for (int i=2;i<argc;++i) {
                cout << ' ' << argv[i];
            }
            cout << endl;
            return 0;
        } else {
            cerr << "DBF not found: " << argv[1] << endl;
            return 1;
        }
    }

    cerr << "Usage: dbfcount <filename> { comments... }" << endl;
    return 2;
}
