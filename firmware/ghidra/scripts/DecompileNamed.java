// Decompile all functions whose (namespaced) name matches a regex.
// Args: <outfile> <regex>
// @category cx580
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;
import java.util.regex.*;

public class DecompileNamed extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        Pattern p = Pattern.compile(a[1]);
        DecompInterface dec = new DecompInterface();
        dec.openProgram(currentProgram);
        int n = 0;
        try (PrintWriter w = new PrintWriter(new FileWriter(a[0]))) {
            for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
                if (f.isThunk() || !p.matcher(f.getName(true)).find()) continue;
                n++;
                w.println("// ==== " + f.getName(true) + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses());
                DecompileResults r = dec.decompileFunction(f, 120, monitor);
                w.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// failed: " + r.getErrorMessage());
            }
        }
        println("functions: " + n);
    }
}
