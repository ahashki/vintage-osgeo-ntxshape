@REM * The contents of this file are subject to the Mozilla Public License 
@REM * Version 1.1 (the "License"); you may not use this file except in 
@REM * compliance with the License. You may obtain a copy of the License at
@REM * http://www.mozilla.org/MPL/
@REM * 
@REM * Software distributed under the License is distributed on an "AS IS" 
@REM * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See 
@REM * the License for the specific language governing rights and limitations 
@REM * under the License.
@REM * 
@REM * The Original Code is NTXShape - NTX to Shapefile Converter.
@REM * 
@REM * The Initial Developer of the Original Code is ESRI Canada Limited.
@REM * Portions created by ESRI Canada are Copyright (C) 1997-2003 
@REM * ESRI Canada Limited.  All Rights Reserved.
@REM * 
@REM * Contributor(s):
@REM *  Bruce Dodson <bdodson@esricanada.com>

@echo off
gcc -MM *.cpp *.c > dependencies
gcc -MM -xc++ *.rc | sed -e "s/\.rc\.o/.ro/" >> dependencies
gcc -MM -xc++ *.idl | sed -e "s/\.idl\.o/.tlb/" >> dependencies
