// Decompile functions at given addresses, plus their direct callees (depth 1).
// Args: <outfile> <addr> [addr...]   prefix addr with '+' to include callees
// @category cx580
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import java.io.*;
import java.util.*;

public class DecompileAt extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        DecompInterface dec = new DecompInterface();
        dec.openProgram(currentProgram);
        FunctionManager fm = currentProgram.getFunctionManager();
        LinkedHashSet<Function> todo = new LinkedHashSet<>();
        for (int i = 1; i < a.length; i++) {
            boolean deep = a[i].startsWith("+");
            Address ad = toAddr(deep ? a[i].substring(1) : a[i]);
            Function f = fm.getFunctionContaining(ad);
            if (f == null) { println("no function at " + ad); continue; }
            todo.add(f);
            if (deep) for (Function c : f.getCalledFunctions(monitor)) if (!c.isExternal() && !c.isThunk()) todo.add(c);
        }
        try (PrintWriter w = new PrintWriter(new FileWriter(a[0]))) {
            for (Function f : todo) {
                w.println("// ==== " + f.getName() + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses());
                DecompileResults r = dec.decompileFunction(f, 120, monitor);
                w.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// failed: " + r.getErrorMessage());
            }
        }
    }
}
