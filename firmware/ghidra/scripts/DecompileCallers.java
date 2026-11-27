// Decompile all functions that call a function whose name matches a regex.
// Args: <outfile> <regex> [max]
// @category cx580
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;
import java.util.regex.*;

public class DecompileCallers extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        Pattern p = Pattern.compile(a[1]);
        int max = a.length > 2 ? Integer.parseInt(a[2]) : 300;
        FunctionManager fm = currentProgram.getFunctionManager();
        LinkedHashSet<Function> callers = new LinkedHashSet<>();
        List<Function> targets = new ArrayList<>();
        for (Function f : fm.getFunctions(true)) if (p.matcher(f.getName(true)).find()) targets.add(f);
        for (Function f : fm.getExternalFunctions()) if (p.matcher(f.getName(true)).find()) targets.add(f);
        for (Function t : targets) {
            for (Reference r : getReferencesTo(t.getEntryPoint())) {
                Function c = fm.getFunctionContaining(r.getFromAddress());
                if (c == null) continue;
                if (c.isThunk()) { for (Reference r2 : getReferencesTo(c.getEntryPoint())) { Function c2 = fm.getFunctionContaining(r2.getFromAddress()); if (c2 != null && !c2.isThunk()) callers.add(c2); } }
                else callers.add(c);
            }
        }
        DecompInterface dec = new DecompInterface();
        dec.openProgram(currentProgram);
        try (PrintWriter w = new PrintWriter(new FileWriter(a[0]))) {
            w.println("// targets: " + targets);
            int n = 0;
            for (Function c : callers) {
                if (n++ >= max) break;
                w.println("// ==== " + c.getName(true) + " @ " + c.getEntryPoint() + " size=" + c.getBody().getNumAddresses());
                DecompileResults r = dec.decompileFunction(c, 120, monitor);
                w.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// failed: " + r.getErrorMessage());
            }
        }
        println("callers: " + callers.size());
    }
}
