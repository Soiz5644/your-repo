Attribute VB_Name = "Module1"
Sub Traitement_sensor_data()
Attribute Traitement_sensor_data.VB_ProcData.VB_Invoke_Func = "Q\n14"
'
' Traitement_sensor_data Macro
'
' Touche de raccourci du clavier: Ctrl+Shift+Q
'

    Dim sourceRange As Range
    Dim fillRange As Range
    Dim ws As Worksheet
    Dim filePath As String
    Dim fileContent As String
    Dim fileNumber As Integer
    
    ' Set the path to your CSV file
    filePath = "D:\Users\gibeaux\Documents\GitHub\sensor_data.csv" ' Change this to your actual file path
    
    ' Read the file content
    fileNumber = FreeFile
    Open filePath For Input As #fileNumber
    fileContent = Input(LOF(fileNumber), fileNumber)
    Close #fileNumber
    
    ' Replace tabs with commas
    fileContent = Replace(fileContent, vbTab, ",")
    
    ' Write the updated content back to the file or to a new file
    filePath = "D:\Users\gibeaux\Documents\GitHub\sensor_data.csv" ' Change this to your desired output path
    fileNumber = FreeFile
    Open filePath For Output As #fileNumber
    Print #fileNumber, fileContent
    Close #fileNumber
    
    ' Open the updated CSV file in Excel
    Workbooks.Open filePath
    Columns("B:B").Select
    Selection.Insert Shift:=xlToRight, CopyOrigin:=xlFormatFromLeftOrAbove
    Range("B1").Select
    ActiveCell.FormulaR1C1 = "Time"
    Range("B2").Select
    ActiveCell.FormulaR1C1 = "=VALUE(RIGHT(RC[-1],8))"
    Range("B2").Select
    
    ' Define the source range (single cell or range that you want to autofill)
    Set sourceRange = Range("B2") ' source cell
    
    ' Determine the last row with data in the column to the right (column C in this case)
    Dim lastRow As Long
    lastRow = Cells(Rows.Count, sourceRange.Column + 1).End(xlUp).Row
    
    ' Define the fill range based on the last row in the column to the right
    Set fillRange = Range(sourceRange, Cells(lastRow, sourceRange.Column))
        
    Range("B2").Select

    ' Perform the autofill
    Selection.AutoFill Destination:=fillRange
    
    Columns("B:B").Select
    Selection.NumberFormat = "[$-x-systime]h:mm:ss AM/PM"
    Columns("I:I").Select
    Selection.Insert Shift:=xlToRight, CopyOrigin:=xlFormatFromLeftOrAbove
    Columns("K:K").Select
    Selection.Insert Shift:=xlToRight, CopyOrigin:=xlFormatFromLeftOrAbove
    Range("I1").Select
    ActiveCell.FormulaR1C1 = "SGP40_voc_ppb"
    Range("I1").Select
    Selection.Copy
    Range("K1").Select
    ActiveSheet.Paste
    Application.CutCopyMode = False
    ActiveCell.FormulaR1C1 = "SGP41_voc_ppb"
    Range("I2").Select

    ActiveCell.FormulaR1C1 = "=((LN(501-RC[-1]))-6.24)*(-381.97)"
    Range("I2").Select
    
    ' Define the source range (single cell or range that you want to autofill)
    Set sourceRange = Range("I2") ' source cell
    
    ' Determine the last row with data in the column to the right (column C in this case)
    Dim lastRow2 As Long
    lastRow2 = Cells(Rows.Count, sourceRange.Column + 1).End(xlUp).Row
    
    ' Define the fill range based on the last row in the column to the right
    Set fillRange = Range(sourceRange, Cells(lastRow2, sourceRange.Column))
        
    Range("I2").Select

    ' Perform the autofill
    Selection.AutoFill Destination:=fillRange
    
    
    Range("I2:I7943").Select
    Range("I2").Select
    Selection.Copy
    Range("K2").Select
    ActiveSheet.Paste
    Range("K2").Select
    Application.CutCopyMode = False
    ActiveCell.FormulaR1C1 = "=((LN(501-RC[-1]))-6.24)*(-381.97)"
    Range("K2").Select
    
    ' Define the source range (single cell or range that you want to autofill)
    Set sourceRange = Range("K2") ' source cell
    
    ' Determine the last row with data in the column to the right (column C in this case)
    Dim lastRow3 As Long
    lastRow3 = Cells(Rows.Count, sourceRange.Column + 1).End(xlUp).Row
    
    ' Define the fill range based on the last row in the column to the right
    Set fillRange = Range(sourceRange, Cells(lastRow3, sourceRange.Column))
        
    Range("K2").Select

    ' Perform the autofill
    Selection.AutoFill Destination:=fillRange
    Range("K:K,I:I,C:C,B:B").Select
    
    ActiveSheet.Shapes.AddChart2(240, xlXYScatterLinesNoMarkers).Select
     ActiveChart.SetSourceData Source:=Range( _
        "$K:$K,$I:$I,$C:$C,$B:$B" _
        )

    
    ActiveChart.FullSeriesCollection(1).Select
    
        ActiveSheet.ChartObjects("Graphique 1").Activate
    ActiveChart.FullSeriesCollection(3).Select
    ActiveChart.FullSeriesCollection(3).AxisGroup = 2
    ActiveSheet.ChartObjects("Graphique 1").Activate
    ActiveChart.FullSeriesCollection(3).Select
    ActiveChart.FullSeriesCollection(2).Select
    ActiveChart.FullSeriesCollection(2).AxisGroup = 2
    ActiveSheet.ChartObjects("Graphique 1").Activate
    ActiveChart.FullSeriesCollection(2).Select
    
End Sub
