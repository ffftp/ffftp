function replace_afxres_h_text(s) {
    if (s.match(/^\s*#include\s*"afxres.h"\s*$/)) {
        var replaced = "// " + s + "\r\n";
        replaced += "#include <windows.h>" + "\r\n";
        replaced += "#define IDC_STATIC -1";
        return replaced; 
    }
    return s;
}
function main() {
    var fromFileName = "";
    var toFileName = "";
    var args = WScript.Arguments;
    if (args.length < 2) {
        WScript.StdErr.WriteLine("usage: cscript ReplaceAfxresh.js <InResFile> <OutResFile>");
        return;
    }
    fromFileName = args(0);
    toFileName = args(1);
    WScript.StdOut.WriteLine("ReplaceAfxresh.js - in: " + fromFileName + ", out: " + toFileName);
    var fso = new ActiveXObject("Scripting.FileSystemObject");
    var fi = fso.OpenTextFile(fromFileName, 1, false, -2);
    var fo = fso.CreateTextFile(toFileName, true, false);
    while (!fi.AtEndOfStream) {
        var line = fi.ReadLine();
        var lineReplaced = replace_afxres_h_text(line);
        fo.WriteLine(lineReplaced);
    }
    fo.Close();
    fi.Close();
}
main();
