// From an RTTI type-name string, find typeinfo -> vtable(s) -> virtual methods, decompile them.
// Args: <outfile> <exact type name string, e.g. 26menuExclusiveCameraSetting> [maxSlots]
// @category cx580
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;

public class DecompileVtable extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        String name = a[1];
        int maxSlots = a.length > 2 ? Integer.parseInt(a[2]) : 40;
        Memory mem = currentProgram.getMemory();
        byte[] pat = (name + "\0").getBytes("latin1");
        Address s = mem.findBytes(currentProgram.getMinAddress(), pat, null, true, monitor);
        if (s == null) { println("string not found"); return; }
        List<Address> typeinfos = new ArrayList<>();
        // typeinfo object: [vptr][name ptr]... so references to the string come from ti+4
        for (Address p = findWord(s.getOffset(), currentProgram.getMinAddress()); p != null; p = findWord(s.getOffset(), p.add(4)))
            typeinfos.add(p.subtract(4));
        FunctionManager fm = currentProgram.getFunctionManager();
        DecompInterface dec = new DecompInterface();
        dec.openProgram(currentProgram);
        try (PrintWriter w = new PrintWriter(new FileWriter(a[0]))) {
            w.println("// type name at " + s + ", typeinfo candidates " + typeinfos);
            for (Address ti : typeinfos) {
                for (Address vt = findWord(ti.getOffset(), currentProgram.getMinAddress()); vt != null; vt = findWord(ti.getOffset(), vt.add(4))) {
                    // vtable: [offset-to-top][typeinfo ptr][slot0]...
                    w.println("// vtable typeinfo-ref at " + vt);
                    for (int i = 0; i < maxSlots; i++) {
                        Address slot = vt.add(4 + 4L * i);
                        long fp = mem.getInt(slot) & 0xffffffffL;
                        Function f = fm.getFunctionAt(toAddr(fp & ~1L));
                        if (f == null && fp > 0x10000 && mem.contains(toAddr(fp & ~1L)) && mem.getBlock(toAddr(fp & ~1L)).isExecute()) {
                            disassemble(toAddr(fp & ~1L));
                            f = createFunction(toAddr(fp & ~1L), null);
                        }
                        if (f == null) { w.println("// slot " + i + ": " + Long.toHexString(fp) + " (no function, stop)"); break; }
                        w.println("// ==== slot " + i + " " + f.getName(true) + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses());
                        DecompileResults r = dec.decompileFunction(f, 120, monitor);
                        w.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// failed: " + r.getErrorMessage());
                    }
                }
            }
        }
    }

    Address findWord(long value, Address start) throws Exception {
        byte[] b = new byte[] { (byte) value, (byte) (value >> 8), (byte) (value >> 16), (byte) (value >> 24) };
        Memory mem = currentProgram.getMemory();
        Address cur = start;
        while (true) {
            Address r = mem.findBytes(cur, b, null, true, monitor);
            if (r == null) return null;
            if (r.getOffset() % 4 == 0) return r;
            cur = r.add(1);
        }
    }
}
