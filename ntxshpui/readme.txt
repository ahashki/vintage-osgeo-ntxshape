This graphical user interface for NTXShape is based on a demo that 
was developed for a presentation to a client in the fall of 2002.
It will serve as a sample app showing the NTXShape Visual Basic 
bindings, and also as a useful graphical interface for NTXShape.

The package includes the complete source code (VB6 required) as 
well as the compiled binary.  The source code is licensed under
Mozilla Public License 1.1, except where noted otherwise.

To compile it, you need Visual Basic 6.  If you don't have VB6
you can use the precompiled binaries.  Then you would need the
Visual Basic run-time files, which are available from here:
  
  http://support.microsoft.com/default.aspx?scid=kb;en-us;Q290887

You also need to have NTXShape 1.4 or higher installed for this 
sample to work, and you must have installed the VB bindings.  Of 
course, since this sample plugs into ArcMap, you also need 
ArcGIS 8.x installed in order to compile or use this sample.

Installation steps:
1) Ensure the required software is installed (NTXShape with the VB 
bindings, ArcGIS, and either VB6 or the VB6 run-time).

2) If you are going to compile from source, I assume you know how
to do that.

3) Copy NTXSHPUI.dll to some location on your system, e.g. the
directory where NTXShape was installed.

4) Start ArcMap, and go to the Customize dialog.

5) On the customize dialog, choose "Add from File..." and browse to 
wherever you put NTXSHPUI.dll.  I dialog should pop up saying that 
NTXImportCommand was loaded.  You only need to do this step once.

6) In the Commands tab of the Customize dialog, look under Data
Converters.  You should see a new command called "Import NTX".
Drag this onto the user interface, e.g. next to the "+" add-data
button on the toolbar, or into the File menu.  If you choose
"Save in normal.mxt" before doing this, you will only have to do
it once.

7) Close the customize dialog.

8) Try it out.


Further documentation, and possibly a setup wizard, will be added as
this tool matures.  Contributions / improvements are welcome, and 
will be gladly accepted.



Change Log:
May 6, 2003 - Added support for 3D line files.  As a side-effect, this 
              implies NTXSHPUI now requires 1.4a, not just 1.4.  This
              suggests that NTXSHPUI should have a setup wizard, and
              should include NTXAPI14 as a shared component to ensure
              that a compatible version is installed.  (Or, just throw
              it into the default install wizard for 1.4a.)

May 8, 2003 - Added ArcToolbox support.  The Add to Map checkbox is
              not available in ArcToolbox, but otherwise the tool
              works the same.
