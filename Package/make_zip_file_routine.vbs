If WScript.Arguments.Count > 0 Then
	Set fso = CreateObject("Scripting.FileSystemObject")
	Set sh = CreateObject("Shell.Application")
	src = WScript.Arguments(0)
	If WScript.Arguments.Count > 1 Then
		zip = WScript.Arguments(1)
	Else
		zip = fso.BuildPath(fso.GetParentFolderName(src), fso.GetBaseName(src)) & ".zip"
	End If
	tmp = zip & ".temp"
	fso.CreateTextFile(zip, True).Write Chr(&H50) & Chr(&H4b) & Chr(&H05) & Chr(&H06) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00) & Chr(&H00)
	If fso.FolderExists(tmp) Then
		fso.DeleteFolder tmp, True
	End If
	fso.CreateFolder tmp
	If fso.FolderExists(src) Then
		fso.CopyFolder src, fso.BuildPath(tmp, fso.GetFileName(src))
	Else
		fso.CopyFile src, fso.BuildPath(tmp, fso.GetFileName(src))
	End If
	sh.NameSpace(zip).MoveHere sh.NameSpace(tmp).Items
	Do While sh.NameSpace(tmp).Items.Count > 0
		WScript.Sleep(1000)
	Loop
	fso.DeleteFolder tmp, True
End if
