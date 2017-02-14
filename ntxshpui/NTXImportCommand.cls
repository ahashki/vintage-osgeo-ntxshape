VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "NTXImportCommand"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
' NTXImportCommand.cls - ICommand for ArcMap


'' The contents of this file are subject to the Mozilla Public License
'' Version 1.1 (the "License"); you may not use this file except in
'' compliance with the License. You may obtain a copy of the License at
'' http://www.mozilla.org/MPL/
''
'' Software distributed under the License is distributed on an "AS IS"
'' basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
'' the License for the specific language governing rights and limitations
'' under the License.
''
'' The Original Code is NTXShape - NTX to Shapefile Converter.
''
'' The Initial Developer of the Original Code is ESRI Canada Limited.
'' Portions created by ESRI Canada are Copyright (C) 1997-2003
'' ESRI Canada Limited.  All Rights Reserved.
''
'' Contributor(s):
''   Bruce Dodson, ESRI Canada <bdodson@esricanada.com>


Implements ICommand
Implements IArcToolboxTool
 
Private m_toolboxModality As esriATModality
Private m_pToolboxBitmap As IPictureDisp
 
Private Property Get IArcToolboxTool_Bitmap() As Stdole.IPictureDisp

    Set IArcToolboxTool_Bitmap = m_pToolboxBitmap
  
End Property

Private Sub IArcToolboxTool_Execute(Optional InputData As Variant)
  
    If m_toolboxModality = esriATModal Then
        Load NTXImportForm
        NTXImportForm.Show vbModal
        Unload NTXImportForm
    Else
        MsgBox "Non-modal operation not supported.", vbExclamation, "NTX to Shapefile"
    End If

End Sub

Private Property Get IArcToolboxTool_HelpContextID() As Long
    
    IArcToolboxTool_HelpContextID = 0 ' no help context
    
End Property

Private Property Get IArcToolboxTool_HelpFile() As String

    IArcToolboxTool_HelpFile = "" ' no help file

End Property

Private Property Get IArcToolboxTool_Message() As String
    
    IArcToolboxTool_Message = ICommand_Message

End Property

Private Property Let IArcToolboxTool_Modality(ByVal RHS As esriCore.esriATModality)
    
    m_toolboxModality = RHS ' however, see _Execute

End Property

Private Property Get IArcToolboxTool_Modality() As esriCore.esriATModality

    IArcToolboxTool_Modality = m_toolboxModality
    
End Property

Private Property Get IArcToolboxTool_Name() As String
    
    IArcToolboxTool_Name = "NTX to Shapefile"
    
End Property

Private Sub IArcToolboxTool_OnCreate(ByVal hook As Object)

    m_toolboxModality = esriATModal
  
    Dim pGraRes As Object
    On Error Resume Next
    Set pGraRes = CreateObject("ATGraRes.GraphicRes")
    If Not pGraRes Is Nothing Then
        Set m_pToolboxBitmap = pGraRes.GetBMP(15122)
    End If
    
End Sub

Private Property Get IArcToolboxTool_TreeviewLocation() As String
    
    IArcToolboxTool_TreeviewLocation = "Conversion Tools\Import to Shapefile"

End Property

Private Property Get ICommand_Enabled() As Boolean
    
    ICommand_Enabled = True
    
End Property
 
Private Property Get ICommand_Checked() As Boolean
    
    ICommand_Checked = False
    
End Property
 
Private Property Get ICommand_Name() As String
    
    ICommand_Name = "Import NTX"
    
End Property
 
Private Property Get ICommand_Caption() As String
    
    ICommand_Caption = "Import NTX"
    
End Property
 
Private Property Get ICommand_Tooltip() As String
    
    ICommand_Tooltip = "Import NTX"
    
End Property
 
Private Property Get ICommand_Message() As String
    
    ICommand_Message = "Converts GIS data from CARIS NTX format into ESRI shapefile format."
    
End Property
 
Private Property Get ICommand_HelpFile() As String
    
    ICommand_HelpFile = "" ' no help
    
End Property
 
Private Property Get ICommand_HelpContextID() As Long
    
    ICommand_HelpContextID = 0
    
End Property
 
Private Property Get ICommand_Bitmap() As esriCore.OLE_HANDLE
    
    ICommand_Bitmap = 0 ' no bitmap
    
End Property
 
Private Property Get ICommand_Category() As String
    
    ICommand_Category = "Data Converters"
    
End Property
 
Private Sub ICommand_OnCreate(ByVal hook As Object)
    
    If TypeOf hook Is IApplication Then
        Set Application = hook
    End If
    
End Sub
 
Private Sub ICommand_OnClick()
    Load NTXImportForm
    NTXImportForm.Show vbModal
    Unload NTXImportForm
End Sub
