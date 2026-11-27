// Decompile every function that references a string matching a regex.
// Args: <outfile> <regex> [maxFunctions]
// @category cx580
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.data.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.*;
import java.io.*;
import java.util.*;
import java.util.regex.*;

public class DecompileRefs extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        Pattern p = Pattern.compile(a[1]);
        int max = a.length > 2 ? Integer.parseInt(a[2]) : 200;
        Listing listing = currentProgram.getListing();
        ReferenceManager rm = currentProgram.getReferenceManager();
        FunctionManager fm = currentProgram.getFunctionManager();
        Map<Function, Set<String>> funcs = new LinkedHashMap<>();
        DataIterator it = listing.getDefinedData(true);
        while (it.hasNext() && !monitor.isCancelled()) {
            Data d = it.next();
            if (!(d.getValue() instanceof String)) continue;
            String s = (String) d.getValue();
            if (!p.matcher(s).find()) continue;
            for (Reference r : rm.getReferencesTo(d.getAddress())) {
                Address from = r.getFromAddress();
                Function f = fm.getFunctionContaining(from);
                if (f == null) {
                    // literal pool entry: follow refs to the pool word
                    for (Reference r2 : rm.getReferencesTo(from)) {
                        Function f2 = fm.getFunctionContaining(r2.getFromAddress());
                        if (f2 != null) funcs.computeIfAbsent(f2, k -> new TreeSet<>()).add(s);
                    }
                } else funcs.computeIfAbsent(f, k -> new TreeSet<>()).add(s);
            }
        }
        DecompInterface dec = new DecompInterface();
        dec.openProgram(currentProgram);
        try (PrintWriter w = new PrintWriter(new FileWriter(a[0]))) {
            int n = 0;
            for (Map.Entry<Function, Set<String>> e : funcs.entrySet()) {
                if (n++ >= max) break;
                Function f = e.getKey();
                w.println("// ==== " + f.getName() + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses());
                for (String s : e.getValue()) w.println("//   ref: " + s.replace("\n", "\\n"));
                DecompileResults r = dec.decompileFunction(f, 120, monitor);
                w.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// failed: " + r.getErrorMessage());
            }
            println("functions: " + funcs.size());
        }
    }
}
