// Emulates libObj's InhFeatureCheck (FUN_00aeb48c) for a range of message ids
// and records which feature ids it asks the gate (FUN_00af489c) about.
// Args: <outfile> <firstMsgId> <lastMsgId>
// For each message: category 0x3100; GetParam returns 1; every Param value
// read returns the next value from a list so value-dependent branches show up.
// The gate returns 0 (not inhibited) so the function walks its whole branch.
// @category cx580
import ghidra.app.script.GhidraScript;
import ghidra.app.emulator.EmulatorHelper;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.math.BigInteger;
import java.util.*;

public class EmuInhCheck extends GhidraScript {
    Address addr(long a) { return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(a); }

    Set<Address> thunksNamed(String regex) {
        Set<Address> s = new HashSet<>();
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            if (f.isThunk() && f.getThunkedFunction(true).getName(true).matches(regex)) s.add(f.getEntryPoint());
        }
        return s;
    }

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        int first = Integer.decode(a[1]), last = Integer.decode(a[2]);
        Address target = addr(0x00aeb48cL), gate = addr(0x00af489cL), sentinel = addr(0x00000100L);
        Set<Address> cat = thunksNamed(".*ObjMsg::GetCategoryId");
        Set<Address> mid = thunksNamed(".*ObjMsg::GetMessageId");
        Set<Address> getParam = thunksNamed(".*ObjMsg::GetParam");
        Set<Address> getValue = thunksNamed(".*Param(Std|Array)_Template.*::GetValue");
        println("thunks: cat=" + cat.size() + " mid=" + mid.size() + " getParam=" + getParam.size() + " getValue=" + getValue.size());
        Listing listing = currentProgram.getListing();

        try (PrintWriter w = new PrintWriter(new FileWriter(a[0]))) {
            for (int msg = first; msg <= last && !monitor.isCancelled(); msg++) {
                for (int vsel = 0; vsel < 1; vsel++) {
                    EmulatorHelper emu = new EmulatorHelper(currentProgram);
                    try {
                        emu.writeRegister("sp", 0x7fff0000L);
                        emu.writeRegister("lr", sentinel.getOffset());
                        emu.writeRegister("r0", 0x7ff00000L);   // fake ObjMsg*
                        emu.writeRegister(emu.getPCRegister(), target.getOffset());
                        List<String> feats = new ArrayList<>();
                        int steps = 0;
                        int valueCalls = 0;
                        boolean done = false;
                        while (!done && steps++ < 200000) {
                            Address pc = emu.getExecutionAddress();
                            long ret = -1;
                            boolean stub = false;
                            if (pc.equals(sentinel)) break;
                            if (cat.contains(pc)) { ret = 0x3100; stub = true; }
                            else if (mid.contains(pc)) { ret = msg; stub = true; }
                            else if (getParam.contains(pc)) { ret = 0x7ff10000L; stub = true; }
                            else if (getValue.contains(pc)) { ret = 0; valueCalls++; stub = true; }
                            else if (pc.equals(gate)) {
                                feats.add(String.format("%#x", emu.readRegister("r0").longValue()));
                                ret = 0; stub = true;
                            } else {
                                Function f = currentProgram.getFunctionManager().getFunctionAt(pc);
                                if (f != null && (f.isThunk() || f.isExternal()) && !pc.equals(target)) { ret = 0; stub = true; }
                            }
                            if (stub) {
                                emu.writeRegister("r0", ret);
                                emu.writeRegister(emu.getPCRegister(), emu.readRegister("lr").longValue() & ~1L);
                                continue;
                            }
                            if (!emu.step(monitor)) { feats.add("emu-error:" + emu.getLastError()); break; }
                        }
                        long r0 = emu.readRegister("r0").longValue();
                        if (!feats.isEmpty()) w.println(String.format("%#06x\t%s\tret=%d", msg, String.join(",", feats), r0));
                    } finally {
                        emu.dispose();
                    }
                }
            }
        }
    }
}
