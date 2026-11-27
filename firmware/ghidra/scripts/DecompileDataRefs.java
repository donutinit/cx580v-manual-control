// Decompile functions that reference the given data addresses (e.g. literal pool words).
// Args: <outfile> <addr>...
// @category cx580
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;

public class DecompileDataRefs extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        FunctionManager fm = currentProgram.getFunctionManager();
        Map<Function, List<String>> fs = new LinkedHashMap<>();
        for (int i = 1; i < a.length; i++) {
            for (Reference r : getReferencesTo(toAddr(a[i]))) {
                Function f = fm.getFunctionContaining(r.getFromAddress());
                if (f != null) fs.computeIfAbsent(f, k -> new ArrayList<>()).add(a[i] + "<-" + r.getFromAddress());
            }
        }
        DecompInterface dec = new DecompInterface();
        dec.openProgram(currentProgram);
        try (PrintWriter w = new PrintWriter(new FileWriter(a[0]))) {
            for (Map.Entry<Function, List<String>> e : fs.entrySet()) {
                Function f = e.getKey();
                w.println("// ==== " + f.getName(true) + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses() + " refs=" + e.getValue());
                DecompileResults r = dec.decompileFunction(f, 120, monitor);
                w.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// failed: " + r.getErrorMessage());
            }
        }
        println("functions: " + fs.size());
    }
}
